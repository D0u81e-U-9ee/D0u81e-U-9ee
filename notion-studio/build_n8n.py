#!/usr/bin/env python3
"""Erzeugt die n8n-Workflows der ALEX-Agentur als Import-Dateien (n8n/workflows/*.json).

Aufruf:  python3 build_n8n.py                  → Produktiv-Workflows
         python3 build_n8n.py --test URL       → Test-Variante, Claude-Aufrufe gehen an URL (Mock)
"""
import argparse
import json
import os
import uuid

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, "n8n", "src")
IDS = json.load(open(os.path.join(HERE, "notion_ids.json")))
NOTION = "https://api.notion.com/v1"
NOTION_CRED = {"httpHeaderAuth": {"id": "alexNotionCred01", "name": "Notion · ALEX Agentur"}}
CLAUDE_CRED = {"httpHeaderAuth": {"id": "alexClaudeCred01", "name": "Anthropic · ALEX Agentur"}}

CFG = {
    "db": {k: IDS[k] for k in ("jobs", "anfragen", "agenten", "protokoll", "briefings", "aufgaben", "content", "kunden", "projekte")},
    "defaultModel": "claude-opus-5-5",
    "fallbacks": True,
}


def code(*files):
    lib = open(os.path.join(SRC, "_lib.js"), encoding="utf-8").read()
    body = "\n".join(open(os.path.join(SRC, f), encoding="utf-8").read() for f in files)
    return f"const CFG = {json.dumps(CFG, ensure_ascii=False)};\n{lib}\n{body}"


class WF:
    def __init__(self, name, claude_url):
        self.name, self.nodes, self.conn, self.x = name, [], {}, 0
        self.claude_url = claude_url

    def add(self, name, type_, version, params, creds=None, on_error=None, y=300):
        node = {"id": str(uuid.uuid5(uuid.NAMESPACE_URL, self.name + name)), "name": name, "type": type_,
                "typeVersion": version, "position": [self.x, y], "parameters": params}
        if creds:
            node["credentials"] = creds
        if on_error:
            node["onError"] = on_error
        self.nodes.append(node)
        self.x += 240
        return name

    def link(self, a, b):
        self.conn.setdefault(a, {"main": [[]]})["main"][0].append({"node": b, "type": "main", "index": 0})

    def chain(self, *names):
        for a, b in zip(names, names[1:]):
            self.link(a, b)

    # --- Bausteine
    def schedule(self, name, rule):
        return self.add(name, "n8n-nodes-base.scheduleTrigger", 1.2, {"rule": {"interval": [rule]}}, y=200)

    def manual(self):
        self.x -= 240
        return self.add("Manuell starten", "n8n-nodes-base.manualTrigger", 1, {}, y=420)

    def js(self, name, *files):
        return self.add(name, "n8n-nodes-base.code", 2, {"jsCode": code(*files), "mode": "runOnceForAllItems"})

    def notion(self, name, method, path, body=None, on_error=None):
        url = NOTION + path
        if "{{" in url:
            url = "=" + url
        p = {"method": method, "url": url, "authentication": "genericCredentialType",
             "genericAuthType": "httpHeaderAuth", "sendHeaders": True,
             "headerParameters": {"parameters": [{"name": "Notion-Version", "value": "2022-06-28"}]},
             "options": {"timeout": 60000}}
        if isinstance(body, str) and body.startswith("={{") and body.endswith("}}"):
            inner = body[3:-2]
            while "}}" in inner:  # n8n beendet Ausdrücke bei "}}" – innere Klammern trennen
                inner = inner.replace("}}", "} }")
            body = "={{" + inner + "}}"
        if body is not None:
            p.update({"sendBody": True, "specifyBody": "json",
                      "jsonBody": body if isinstance(body, str) else json.dumps(body, ensure_ascii=False)})
        return self.add(name, "n8n-nodes-base.httpRequest", 4.2, p, NOTION_CRED, on_error)

    def claude(self, name="Claude"):
        p = {"method": "POST", "url": self.claude_url, "authentication": "genericCredentialType",
             "genericAuthType": "httpHeaderAuth", "sendHeaders": True,
             "headerParameters": {"parameters": [
                 {"name": "anthropic-version", "value": "2026-10-21"},
                 {"name": "anthropic-beta", "value": "server-side-fallback-2026-07-01"}]},
             "sendBody": True, "specifyBody": "json", "jsonBody": "={{ JSON.stringify($json.claude_body) }}",
             "options": {"timeout": 600000}}
        return self.add(name, "n8n-nodes-base.httpRequest", 4.2, p, CLAUDE_CRED, "continueRegularOutput")

    def export(self):
        return {"name": self.name, "nodes": self.nodes, "connections": self.conn, "active": False,
                "settings": {"executionOrder": "v1", "timezone": "Europe/Berlin", "saveManualExecutions": True},
                "pinData": {}, "meta": {"templateCredsSetupCompleted": False},
                "tags": []}


def status_filter(*names):
    f = [{"property": "Status", "select": {"equals": n}} for n in names]
    return f[0] if len(f) == 1 else {"or": f}


def worker(cu):
    w = WF("ALEX Agentur · 2 Worker", cu)
    t = w.schedule("Alle 5 Minuten", {"field": "minutes", "minutesInterval": 5})
    m = w.manual()
    q = w.notion("Jobs abfragen", "POST", f"/databases/{IDS['jobs']}/query",
                 {"filter": status_filter("Bereit", "Überarbeiten"),
                  "sorts": [{"timestamp": "created_time", "direction": "ascending"}], "page_size": 25})
    w.link(t, q); w.link(m, q)
    w.chain(q,
            w.js("Job auswählen", "worker_1_job_auswaehlen.js"),
            w.notion("Job sperren", "PATCH", "/pages/{{ $json.job_id }}",
                     '={{ JSON.stringify({properties: {Status: {select: {name: "In Bearbeitung"}}, '
                     '"Gestartet": {date: {start: $json.start}}, "Lauf-ID": {rich_text: [{text: {content: $json.lauf_id}}]}}}) }}'),
            w.notion("Agent laden", "GET", "/pages/{{ $('Job auswählen').first().json.agent_id }}"),
            w.js("Seitenliste", "worker_2_seitenliste.js"),
            w.notion("Seiten laden", "GET", "/pages/{{ $json.id }}"),
            w.js("Prompt bauen", "worker_3_prompt_bauen.js"),
            w.claude(),
            w.js("Ergebnis verarbeiten", "worker_4_ergebnis.js"),
            w.notion("Ergebnis anhängen", "PATCH", "/blocks/{{ $json.job_id }}/children",
                     "={{ JSON.stringify({children: $json.blocks}) }}", on_error="continueRegularOutput"),
            w.notion("Job aktualisieren", "PATCH", "/pages/{{ $('Ergebnis verarbeiten').first().json.job_id }}",
                     "={{ JSON.stringify({properties: $('Ergebnis verarbeiten').first().json.props}) }}"),
            w.notion("Protokoll schreiben", "POST", "/pages",
                     "={{ JSON.stringify($('Ergebnis verarbeiten').first().json.log) }}"))
    return w


def review(cu):
    w = WF("ALEX Agentur · 3 Review (CreativeDirector)", cu)
    t = w.schedule("Alle 5 Minuten", {"field": "minutes", "minutesInterval": 5})
    m = w.manual()
    q = w.notion("Review-Jobs abfragen", "POST", f"/databases/{IDS['jobs']}/query",
                 {"filter": status_filter("Review"), "sorts": [{"timestamp": "created_time", "direction": "ascending"}], "page_size": 1})
    w.link(t, q); w.link(m, q)
    w.chain(q,
            w.js("Review-Job auswählen", "review_1_auswaehlen.js"),
            w.notion("Agenten laden", "POST", f"/databases/{IDS['agenten']}/query", {"page_size": 100}),
            w.js("Review-Prompt bauen", "review_2_prompt.js"),
            w.claude(),
            w.js("Review auswerten", "review_3_auswerten.js"),
            w.notion("Review anhängen", "PATCH", "/blocks/{{ $json.job_id }}/children",
                     "={{ JSON.stringify({children: $json.blocks}) }}", on_error="continueRegularOutput"),
            w.notion("Job aktualisieren", "PATCH", "/pages/{{ $('Review auswerten').first().json.job_id }}",
                     "={{ JSON.stringify({properties: $('Review auswerten').first().json.props}) }}"),
            w.notion("Protokoll schreiben", "POST", "/pages", "={{ JSON.stringify($('Review auswerten').first().json.log) }}"))
    return w


def dispatcher(cu):
    w = WF("ALEX Agentur · 1 Dispatcher (ALEX plant)", cu)
    t = w.schedule("Alle 5 Minuten", {"field": "minutes", "minutesInterval": 5})
    m = w.manual()
    q = w.notion("Neue Anfragen", "POST", f"/databases/{IDS['anfragen']}/query",
                 {"filter": status_filter("Neu"), "sorts": [{"timestamp": "created_time", "direction": "ascending"}], "page_size": 1})
    w.link(t, q); w.link(m, q)
    w.chain(q,
            w.js("Anfrage auswählen", "disp_1_auswaehlen.js"),
            w.notion("Anfrage sperren", "PATCH", "/pages/{{ $json.anfrage_id }}",
                     '{"properties": {"Status": {"select": {"name": "In Planung"}}}}'),
            w.notion("Seiteninhalt laden", "GET", "/blocks/{{ $('Anfrage auswählen').first().json.anfrage_id }}/children?page_size=100"),
            w.notion("Aktive Agenten", "POST", f"/databases/{IDS['agenten']}/query",
                     {"filter": {"property": "Status", "select": {"equals": "Aktiv"}}, "page_size": 100}),
            w.js("Plan-Prompt bauen", "disp_2_prompt.js"),
            w.claude(),
            w.js("Plan auswerten", "disp_3_auswerten.js"),
            w.notion("Anfrage aktualisieren", "PATCH", "/pages/{{ $json.anfrage_id }}",
                     "={{ JSON.stringify({properties: $json.anfrage_props}) }}"),
            w.notion("Protokoll schreiben", "POST", "/pages", "={{ JSON.stringify($('Plan auswerten').first().json.log) }}"),
            w.js("Jobs aufteilen", "disp_4_jobs_aufteilen.js"),
            w.notion("Job anlegen", "POST", "/pages", "={{ JSON.stringify($json.body) }}"),
            w.js("Abhängigkeiten", "disp_5_abhaengigkeiten.js"),
            w.notion("Abhängigkeiten setzen", "PATCH", "/pages/{{ $json.id }}", "={{ JSON.stringify({properties: $json.props}) }}"))
    return w


def briefing(cu):
    w = WF("ALEX Agentur · 4 Morgen-Briefing", cu)
    t = w.schedule("Mo–Fr 07:00", {"field": "cronExpression", "expression": "0 7 * * 1-5"})
    m = w.manual()
    D = IDS
    nodes = [
        w.notion("Meine Aufgaben", "POST", f"/databases/{D['aufgaben']}/query",
                 {"filter": {"property": "Status", "select": {"does_not_equal": "Erledigt"}}, "page_size": 100}),
        w.notion("Offene Jobs", "POST", f"/databases/{D['jobs']}/query",
                 {"filter": {"property": "Status", "select": {"does_not_equal": "Erledigt"}}, "page_size": 100}),
        w.notion("Content diese Woche", "POST", f"/databases/{D['content']}/query",
                 "={{ JSON.stringify({filter: {and: [{property: 'Veröffentlichung', date: {on_or_after: $now.toISODate()}}, "
                 "{property: 'Veröffentlichung', date: {on_or_before: $now.plus({days: 7}).toISODate()}}]}, page_size: 100}) }}"),
        w.notion("Follow-ups", "POST", f"/databases/{D['kunden']}/query",
                 "={{ JSON.stringify({filter: {and: [{property: 'Nächster Follow-up', date: {on_or_before: $now.toISODate()}}, "
                 "{property: 'Status', select: {does_not_equal: 'Archiv'}}]}, page_size: 100}) }}"),
        w.notion("Projekte mit Deadline", "POST", f"/databases/{D['projekte']}/query",
                 "={{ JSON.stringify({filter: {and: [{property: 'Deadline', date: {on_or_before: $now.plus({days: 14}).toISODate()}}, "
                 "{property: 'Status', select: {does_not_equal: 'Abgeschlossen'}}]}, page_size: 100}) }}"),
        w.notion("Anfragen für dich", "POST", f"/databases/{D['anfragen']}/query",
                 {"filter": status_filter("Rückfrage", "Wartet auf dich"), "page_size": 100}),
        w.notion("ALEX laden", "POST", f"/databases/{D['agenten']}/query",
                 {"filter": {"property": "Agent", "title": {"equals": "ALEX"}}, "page_size": 1}),
        w.js("Briefing-Prompt bauen", "brief_1_prompt.js"),
        w.claude(),
        w.js("Briefing speichern", "brief_2_speichern.js"),
        w.notion("Seite anlegen", "POST", "/pages", "={{ JSON.stringify($json.page) }}"),
        w.notion("Protokoll schreiben", "POST", "/pages", "={{ JSON.stringify($('Briefing speichern').first().json.log) }}"),
    ]
    w.link(t, nodes[0]); w.link(m, nodes[0])
    w.chain(*nodes)
    return w


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--test", help="Mock-URL für Claude (Test)")
    a = ap.parse_args()
    cu = a.test or "https://api.anthropic.com/v1/messages"
    out = os.path.join(HERE, "n8n", "workflows-test" if a.test else "workflows")
    os.makedirs(out, exist_ok=True)
    for fn in (dispatcher, worker, review, briefing):
        w = fn(cu)
        path = os.path.join(out, w.name.split("·")[1].strip().split(" ")[0] + "_" + fn.__name__ + ".json")
        json.dump(w.export(), open(path, "w"), indent=2, ensure_ascii=False)
        print("  ✓", os.path.relpath(path, HERE), f"({len(w.nodes)} Nodes)")


if __name__ == "__main__":
    main()

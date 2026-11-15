#!/usr/bin/env python3
"""Seite „System & n8n“: Einrichtung, Workflows, Job-Protokoll, IDs, Workflow-JSON zum Kopieren."""
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import notion_lib as b  # noqa: E402
from notion_lib import md  # noqa: E402

IDS = json.load(open(os.path.join(HERE, "notion_ids.json")))
PAGE = IDS["page_system"]
WF_DIR = os.path.join(HERE, "n8n", "workflows")

TEXT = f"""
!⚙️ Bauplan der Agentur. ALEX und das Team laufen als **4 n8n-Workflows**, die über Notion zusammenarbeiten. Neuer Agent = neue Zeile im Agenten-Team (kein neuer Workflow).
## So arbeitet das System
```
📨 Anfragen an ALEX ──(1) Dispatcher──► ⚙️ Jobs ──(2) Worker──► Ergebnis im Job
                                                         │
                                         kreativ? ──(3) Review (CreativeDirector)
                                                         ▼
                                  ✋ Freigabe durch Achim ──► ✅ Erledigt
                     (4) Morgen-Briefing Mo–Fr 07:00 ──► ☀️ Briefings
Jeder Lauf ──► 📜 Agenten-Protokoll
```
## Die 4 Workflows
1. **Dispatcher (ALEX plant)** – alle 5 Min.: nimmt die älteste Anfrage mit Status „Neu“, ALEX plant 1–5 Jobs (strukturierte JSON-Antwort), legt sie an, setzt Abhängigkeiten. Bei Unklarheit: Status „Rückfrage“.
2. **Worker** – alle 5 Min.: nimmt den wichtigsten Job mit „Bereit“/„Überarbeiten“ ohne offene Vorgänger, lädt den System Prompt des Agenten, gibt Ergebnisse der Vorgänger-Jobs und ggf. Feedback mit, schreibt das Ergebnis in den Job. Kreative Agenten → „Review“, sonst → „Freigabe“.
3. **Review (CreativeDirector)** – alle 5 Min.: bewertet Jobs in „Review“ nach 10 Kriterien. Score ≥ 7 → „Freigabe“, sonst „Überarbeiten“ mit Feedback (max. 3 Versuche).
4. **Morgen-Briefing** – Mo–Fr 07:00: Aufgaben, Freigaben, Fehler, hängende Jobs, Rückfragen, Deadlines, Content, Follow-ups → Briefing-Seite.
## Einrichtung in n8n (ca. 15 Minuten)
### 1 · Notion-Zugang
- [ ] notion.so/profile/integrations → **Neue Integration** (intern), z. B. „ALEX Agentur“ → Secret kopieren
- [ ] In Notion die Seite **ALEX · Media Creative Studio** öffnen → **… → Verbindungen** → Integration hinzufügen (gilt für alle Unterseiten)
### 2 · Zugangsdaten in n8n anlegen (Namen exakt so – dann verbindet n8n sie beim Import automatisch)
- [ ] Credentials → **Header Auth** · Name: `Notion · ALEX Agentur` · Header-Name: `Authorization` · Wert: `Bearer ntn_…` (dein Notion-Secret)
- [ ] Credentials → **Header Auth** · Name: `Anthropic · ALEX Agentur` · Header-Name: `x-api-key` · Wert: dein Anthropic-API-Key (console.anthropic.com)
### 3 · Workflows importieren
- [ ] Die 4 Dateien unten öffnen → Inhalt kopieren → in n8n neuen Workflow öffnen → **Strg+V** (oder „Import from File“ mit den Dateien aus dem Repository `notion-studio/n8n/workflows/`)
- [ ] Reihenfolge: 1 Dispatcher · 2 Worker · 3 Review · 4 Morgen-Briefing
### 4 · Testen, dann aktivieren
- [ ] In „Anfragen an ALEX“ eine Test-Anfrage anlegen (Status „Neu“)
- [ ] Dispatcher öffnen → **Test workflow** (Manuell starten) → es entstehen Jobs
- [ ] Worker manuell starten (je Job einmal) → Ergebnis erscheint im Job
- [ ] Review manuell starten → Score und Feedback erscheinen
- [ ] Läuft alles: in allen 4 Workflows oben rechts **Active** einschalten
## Das Job-Protokoll (Vertrag zwischen Notion und n8n)
- **Entwurf** → noch nicht startklar · **Bereit** → Worker startet · **In Bearbeitung** → gesperrt
- **Review** → CreativeDirector prüft · **Freigabe** → wartet auf Achim · **Überarbeiten** → zurück an Agent
- **Erledigt** → freigegeben · **Fehler** → Details im Protokoll; Status auf „Bereit“ setzen = neuer Versuch
- Abhängigkeiten: Feld „Wartet auf“. Ein Job startet erst, wenn alle Vorgänger „Freigabe“ oder „Erledigt“ sind (Rollup „Offene Vorgänger“ = 0).
- Achtung: Status- und Feldnamen nicht umbenennen – die Workflows lesen sie. Neue Felder hinzufügen ist unproblematisch.
## Agenten steuern
- **Aktiv / Im Aufbau / Pausiert** – nur aktive Agenten bekommen Jobs von ALEX (manuell zugewiesene Jobs laufen bei jedem Agenten).
- **System Prompt** – das Gehirn des Agenten. Hier anpassen, wirkt beim nächsten Lauf.
- **Modell / Effort / Max Tokens** – Standard `claude-opus-5-5`; `claude-sonnet-5-5` ist günstiger, `claude-haiku-5-5` am günstigsten. Effort: low · medium · high · xhigh.
- **Websuche** – Agent darf im Web recherchieren (z. B. ResearchAnalyst).
- **Review durch CD** – Ergebnisse gehen vor der Freigabe zum CreativeDirector.
- **Neuen Agenten anlegen:** Zeile im Agenten-Team anlegen, Rolle + „Kann“ + System Prompt (Vorlage: Struktur der bestehenden Agenten inkl. Agentur-Regeln), Status „Aktiv“. Fertig.
## Technik
- Claude Messages API · `anthropic-version: 2026-10-21` · Fallback bei Ablehnung: `fallbacks: "default"` (Beta `server-side-fallback-2026-07-01`)
- Strukturierte Antworten (JSON-Schema) für Planung und Review · Websuche `web_search_20260209`
- Notion API `2022-06-28` · Zeitzone Europe/Berlin
- Getestet mit n8n 1.123 (CLI-Ausführung gegen dieses Notion, Claude als Mock): Dispatcher → Worker → Review (Überarbeiten) → Worker → Review (Freigabe) → Briefing sowie Fehlerfall.
## Fehlerbehebung
- **Job steht lange auf „In Bearbeitung“** – Workflow ist abgebrochen. Status auf „Bereit“ setzen. Das Briefing meldet solche Jobs.
- **Status „Fehler“** – Ursache im Feld „Feedback“ und im Agenten-Protokoll (Ansicht „Fehler“). Häufig: API-Key falsch, Guthaben leer, Notion-Seite nicht mit der Integration verbunden.
- **401 von Notion** – Integration nicht mit der Seite verbunden oder Secret falsch.
- **401 von Anthropic** – API-Key falsch; Header-Name muss `x-api-key` sein.
- **Ergebnis abgeschnitten** – „Max Tokens“ des Agenten erhöhen.
## Kosten im Blick
- Jeder Lauf schreibt Tokens Input/Output ins Agenten-Protokoll. Leere Läufe (nichts zu tun) kosten nichts – Claude wird nur aufgerufen, wenn Arbeit da ist.
## IDs (für Anpassungen)
"""


def main():
    blocks = md(TEXT)
    for k in ("anfragen", "jobs", "agenten", "protokoll", "briefings", "aufgaben", "projekte", "kunden", "content", "ideen", "finanzen", "wissen"):
        blocks.append(b.bullet([b.mention_db(IDS[k]), b.rt("  "), b.rt(IDS[k], code=True)]))
    blocks += md("## Workflow-Dateien (zum Kopieren in n8n)")
    b.append(PAGE, blocks)
    for f in sorted(os.listdir(WF_DIR)):
        raw = open(os.path.join(WF_DIR, f), encoding="utf-8").read()
        name = json.loads(raw)["name"]
        tog = b.call("PATCH", f"/blocks/{PAGE}/children", {"children": [
            {"type": "toggle", "toggle": {"rich_text": [b.rt(f"📦 {name}  ({f})", bold=True)]}}]})["results"][0]["id"]
        chunks = b.chunks(raw, 1990)
        for i in range(0, len(chunks), 100):
            b.call("PATCH", f"/blocks/{tog}/children", {"children": [
                {"type": "code", "code": {"rich_text": chunks[i:i + 100], "language": "json"}}]})
        print("  ✓", f, len(raw), "Zeichen")
    print("  ✓ System & n8n")


if __name__ == "__main__":
    main()

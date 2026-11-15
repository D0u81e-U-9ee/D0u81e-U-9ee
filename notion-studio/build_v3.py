#!/usr/bin/env python3
"""ALEX · Media Creative Studio – Neuaufbau v3.

Baut unter der Root-Seite: Agentur (Anfragen, Jobs, Agenten-Team, Protokoll, Briefings),
Studio (Projekte, Meine Aufgaben, Kunden, Content, Ideen, Finanzen), Wissen & Standards.
Schreibt alle IDs nach notion_ids.json (für n8n).
"""
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import notion_lib as b  # noqa: E402
from notion_lib import sel, msel, title, text, date, number, url, email, phone, check, created, formula, md  # noqa: E402

ROOT = "2a6ed125-9f02-80ab-b4d8-c14332d2960a"
IDS_FILE = os.path.join(HERE, "notion_ids.json")
PRIO = sel(("Hoch", "red"), ("Normal", "yellow"), ("Niedrig", "gray"))
AGENTS_DIR = os.path.join(HERE, "agents")


def rel(target, back):
    return {"rel": target, "back": back}


# ------------------------------------------------------------------ Datenbanken
DBS = [
    # ---------------- Studio
    ("studio", "kunden", "Kunden & Kontakte", "👥", "Kunden, Leads, Partner und Freelancer – mit Follow-up-Erinnerung.", {
        "Name": title(), "Firma": text(),
        "Typ": sel(("Kunde", "green"), ("Lead", "yellow"), ("Partner", "blue"), ("Freelancer", "purple")),
        "Status": sel(("Aktiv", "green"), ("Warm", "orange"), ("Kalt", "gray"), ("Archiv", "default")),
        "E-Mail": email(), "Telefon": phone(), "Website": url(),
        "Letzter Kontakt": date(), "Nächster Follow-up": date(), "Notizen": text(),
    }, {"⏰ Follow-up": 'if(!empty(prop("Nächster Follow-up")) and prop("Nächster Follow-up") <= today(), "⏰ Jetzt melden", "")'}),
    ("studio", "projekte", "Projekte", "🎞", "Alle Produktionen und Vorhaben – von der Idee bis zur Auslieferung.", {
        "Projekt": title(),
        "Status": sel(("Idee", "gray"), ("Angebot", "brown"), ("Planung", "yellow"), ("Produktion", "orange"),
                      ("Postproduktion", "purple"), ("Review", "pink"), ("Abgeschlossen", "green"), ("Pausiert", "default")),
        "Typ": sel("Video", "Foto", "Content", "Brand", "Website", "Podcast", "Animation"),
        "Priorität": PRIO, "Start": date(), "Deadline": date(), "Budget": number("euro"),
        "Ordner / Drive": url(), "Briefing": text(),
        "Kunde": rel("kunden", "Projekte"),
    }, {"⏳ Deadline": 'if(empty(prop("Deadline")) or prop("Status") == "Abgeschlossen", "", '
                      'if(prop("Deadline") < today(), "⚠️ überfällig", format(dateBetween(prop("Deadline"), today(), "days")) + " Tage"))'}),
    ("studio", "aufgaben", "Meine Aufgaben", "✅", "Deine eigenen To-dos. ALEX legt hier Aufgaben an, die nur du erledigen kannst.", {
        "Aufgabe": title(),
        "Status": sel(("Offen", "gray"), ("In Arbeit", "blue"), ("Warten auf", "yellow"), ("Erledigt", "green")),
        "Priorität": PRIO, "Fällig": date(), "Heute": check(),
        "Bereich": sel("Produktion", "Kunde", "Content", "Admin", "Finanzen", "Lernen", "Privat"),
        "Aufwand": sel(("15 Min", "green"), ("1 Std", "yellow"), ("Halber Tag", "orange"), ("Ganzer Tag", "red")),
        "Quelle": sel(("Ich", "blue"), ("ALEX", "purple")),
        "Notiz": text(),
        "Projekt": rel("projekte", "Aufgaben"),
    }, {"Erledigt-Wert": 'if(prop("Status") == "Erledigt", 1, 0)',
        "⚠️": 'if(prop("Status") != "Erledigt" and !empty(prop("Fällig")) and prop("Fällig") < today(), "⚠️ überfällig", "")'}),
    ("studio", "content", "Content-Kalender", "📅", "Planung und Veröffentlichung für alle Kanäle.", {
        "Titel": title(),
        "Status": sel(("Idee", "gray"), ("Skript", "yellow"), ("Dreh", "orange"), ("Schnitt", "purple"),
                      ("Geplant", "blue"), ("Veröffentlicht", "green")),
        "Plattform": msel(("YouTube", "red"), ("Instagram", "pink"), ("TikTok", "default"), ("LinkedIn", "blue"),
                          ("Website/Blog", "brown"), ("Newsletter", "purple")),
        "Format": sel("Reel / Short", "Long-Form", "Post", "Karussell", "Story", "Artikel", "Newsletter"),
        "Veröffentlichung": date(), "Hook": text(), "Call-to-Action": text(), "Link": url(), "Views": number(),
        "Projekt": rel("projekte", "Content"),
    }, {}),
    ("studio", "ideen", "Ideen", "💡", "Ideen sammeln, bewerten und an ALEX übergeben.", {
        "Idee": title(),
        "Kategorie": sel("Video", "Foto", "Content", "Business", "Kunde", "Experiment"),
        "Bewertung": sel(("⭐", "gray"), ("⭐⭐", "yellow"), ("⭐⭐⭐", "orange"), ("⭐⭐⭐⭐", "red"), ("⭐⭐⭐⭐⭐", "purple")),
        "Status": sel(("Neu", "blue"), ("Reift", "yellow"), ("An ALEX übergeben", "purple"), ("Umgesetzt", "green"), ("Verworfen", "gray")),
        "Link": url(), "Beschreibung": text(), "Erstellt": created(),
    }, {}),
    ("studio", "finanzen", "Finanzen", "💶", "Einnahmen, Ausgaben, Rechnungsstatus.", {
        "Posten": title(),
        "Typ": sel(("Einnahme", "green"), ("Ausgabe", "red")),
        "Betrag": number("euro"), "Datum": date(),
        "Kategorie": sel("Honorar", "Lizenz", "Equipment", "Software / Abo", "Reise", "Freelancer", "Marketing", "Sonstiges"),
        "Status": sel(("Entwurf", "gray"), ("Offen", "yellow"), ("Bezahlt", "green"), ("Überfällig", "red")),
        "Rechnungsnr.": text(),
        "Projekt": rel("projekte", "Finanzen"), "Kunde": rel("kunden", "Finanzen"),
    }, {"Betrag (±)": 'if(prop("Typ") == "Ausgabe", -prop("Betrag"), prop("Betrag"))'}),
    # ---------------- Wissen
    ("wissen", "wissen", "Wissensbasis", "📚", "Tutorials, Vorlagen, Referenzen und Recherche-Ergebnisse.", {
        "Titel": title(),
        "Kategorie": sel("SOP", "Brand", "Vorlage", "Tutorial", "Referenz", "Prompt", "Recherche"),
        "Tags": msel("Video", "Foto", "Social", "Kunde", "Technik", "Business"),
        "Link": url(), "Notiz": text(), "Erstellt": created(),
    }, {}),
    # ---------------- Agentur
    ("agentur", "agenten", "Agenten-Team", "🧑‍🚀", "Das Team von ALEX. Nur Agenten mit Status „Aktiv“ bekommen Jobs. Der System Prompt wird vom n8n-Worker gelesen.", {
        "Agent": title(), "Rolle": text(),
        "Abteilung": sel(("Leitung", "purple"), ("Kreation", "pink"), ("Produktion", "orange"), ("Kommunikation", "blue"),
                         ("Recherche", "green"), ("Technik", "brown"), ("Betrieb", "gray")),
        "Status": sel(("Aktiv", "green"), ("Im Aufbau", "yellow"), ("Pausiert", "gray")),
        "Kann": text(),
        "Modell": sel(("claude-opus-5-5", "purple"), ("claude-sonnet-5-5", "blue"), ("claude-haiku-5-5", "green")),
        "Effort": sel(("low", "gray"), ("medium", "yellow"), ("high", "orange"), ("xhigh", "red")),
        "Websuche": check(), "Review durch CD": check(), "Max Tokens": number(),
        "System Prompt": text(),
    }, {}),
    ("agentur", "anfragen", "Anfragen an ALEX", "📨", "Hier gibst du ALEX Aufträge. Status „Neu“ = ALEX plant beim nächsten Lauf. Bei „Rückfrage“: Antwort eintragen und Status wieder auf „Neu“.", {
        "Anfrage": title(),
        "Status": sel(("Neu", "blue"), ("In Planung", "yellow"), ("Rückfrage", "red"), ("In Arbeit", "orange"),
                      ("Wartet auf dich", "pink"), ("Erledigt", "green"), ("Abgebrochen", "gray")),
        "Art": sel("Auftrag", "Content", "Video", "Recherche", "E-Mail", "Idee", "Frage"),
        "Priorität": PRIO, "Gewünscht bis": date(),
        "Beschreibung": text(), "Rückfrage von ALEX": text(), "Antwort an ALEX": text(), "ALEX-Plan": text(),
        "Erstellt": created(),
        "Projekt": rel("projekte", "Anfragen"), "Kunde": rel("kunden", "Anfragen"),
    }, {}),
    ("agentur", "jobs", "Jobs", "⚙️", "Die Warteschlange der Agentur. Der n8n-Worker holt Jobs mit Status „Bereit“ oder „Überarbeiten“.", {
        "Job": title(),
        "Status": sel(("Entwurf", "gray"), ("Bereit", "blue"), ("In Bearbeitung", "yellow"), ("Review", "purple"),
                      ("Freigabe", "pink"), ("Überarbeiten", "orange"), ("Erledigt", "green"), ("Fehler", "red")),
        "Priorität": PRIO, "Fällig": date(), "Reihenfolge": number(),
        "Auftrag": text(), "Zusammenfassung": text(), "Feedback": text(),
        "Score": number(), "Versuch": number(),
        "Lauf-ID": text(), "Gestartet": date(), "Beendet": date(), "Erstellt": created(),
        "Agent": rel("agenten", "Jobs"), "Anfrage": rel("anfragen", "Jobs"), "Projekt": rel("projekte", "Jobs"),
    }, {"Offen-Wert": 'if(prop("Status") == "Erledigt" or prop("Status") == "Freigabe", 0, 1)',
        "Erledigt-Wert": 'if(prop("Status") == "Erledigt", 1, 0)',
        "⚠️": 'if(prop("Status") != "Erledigt" and !empty(prop("Fällig")) and prop("Fällig") < today(), "⚠️ überfällig", "")'}),
    ("agentur", "protokoll", "Agenten-Protokoll", "📜", "Jeder Lauf der n8n-Workflows: Dauer, Tokens, Status, Fehler.", {
        "Lauf": title(),
        "Workflow": sel(("Dispatcher", "purple"), ("Worker", "blue"), ("Review", "pink"), ("Briefing", "yellow")),
        "Status": sel(("Erfolg", "green"), ("Teilweise", "yellow"), ("Fehler", "red")),
        "Dauer (Sek)": number(), "Tokens Input": number(), "Tokens Output": number(),
        "Modell": text(), "Fehler": text(), "n8n-Ausführung": text(), "Zeit": created(),
        "Agent": rel("agenten", "Läufe"), "Job": rel("jobs", "Läufe"),
    }, {}),
    ("agentur", "briefings", "Briefings", "☀️", "Morgen-Briefings und Wochenrückblicke von ALEX.", {
        "Briefing": title(), "Datum": date(),
        "Typ": sel(("Morgen-Briefing", "yellow"), ("Wochenrückblick", "purple")),
        "Fokus": text(), "Erstellt": created(),
    }, {}),
]

# Agent, Rolle, Abteilung, Status, Kann, Modell, Effort, Websuche, Review, MaxTokens, Emoji
AGENTS = [
    ("ALEX", "Persönlicher Assistent & Agenturleiter", "Leitung", "Aktiv",
     "Plant Anfragen, verteilt Jobs, schreibt Briefings, erledigt Planungs- und Zusammenfassungsjobs", "claude-opus-5-5", "high", False, False, 16000, "🎯"),
    ("CreativeDirector", "Creative Director & Brand Experience Lead", "Leitung", "Aktiv",
     "Prüft kreative Ergebnisse (10 Kriterien, Freigabe ab 7/10), Creative Briefs, kreative Richtung", "claude-opus-5-5", "high", False, False, 16000, "🎨"),
    ("ContentWriter", "Content Writer & Copywriting Specialist", "Kreation", "Aktiv",
     "Social Posts, Captions, Video-Skripte, Website-/Blogtexte, Newsletter", "claude-opus-5-5", "medium", False, True, 16000, "✍️"),
    ("VideoProducer", "Video Production & Planning Specialist", "Produktion", "Aktiv",
     "Video-Konzepte, Shotlists, Drehpläne, Packlisten, Schnittpläne, Kalkulationsgerüste", "claude-opus-5-5", "medium", False, True, 16000, "🎬"),
    ("ResearchAnalyst", "Research & Data Analysis Specialist", "Recherche", "Aktiv",
     "Markt-, Wettbewerbs-, Trend- und Tool-Recherche mit Quellen (Websuche)", "claude-opus-5-5", "medium", True, False, 16000, "🔎"),
    ("EmailManager", "Email Management & Communication Coordinator", "Kommunikation", "Aktiv",
     "E-Mail-Entwürfe: Antworten, Angebote, Follow-ups, Erinnerungen – sendet nie selbst", "claude-opus-5-5", "medium", False, False, 8000, "✉️"),
    ("BrandStrategist", "Brand Strategy & Positioning Specialist", "Kreation", "Im Aufbau",
     "Positionierung, Messaging-Framework, Claims, Tonalität", "claude-opus-5-5", "high", False, True, 16000, "🧭"),
    ("DesignConsultant", "Visual Design & UX Consultant", "Kreation", "Im Aufbau",
     "Moodboards, Layouts, Thumbnail-Konzepte, Bild-Prompts", "claude-opus-5-5", "medium", False, True, 16000, "🖌️"),
    ("WebDeveloper", "Web Development & Implementation Specialist", "Technik", "Im Aufbau",
     "Seitenstruktur, Webflow/HTML/CSS/JS, SEO- und DSGVO-Checklisten", "claude-opus-5-5", "high", False, False, 16000, "💻"),
    ("LaunchCoordinator", "Launch & Go-Live Manager", "Technik", "Im Aufbau",
     "Launch-Pläne, Go-Live-Checklisten, Post-Launch-Monitoring", "claude-opus-5-5", "medium", False, False, 16000, "🚀"),
    ("SystemAdmin", "System Administrator & Infrastructure Manager", "Betrieb", "Im Aufbau",
     "Protokoll-Auswertung, Fix-Vorschläge für Workflows, Systempflege", "claude-opus-5-5", "medium", False, False, 8000, "🛠️"),
]


def prompt(name):
    own = open(os.path.join(AGENTS_DIR, f"{name}.md"), encoding="utf-8").read().strip()
    rules = open(os.path.join(AGENTS_DIR, "_Agentur_Regeln.md"), encoding="utf-8").read().strip()
    return own + "\n\n" + rules


# ------------------------------------------------------------------ Bau
def create_db(parent, key, name, emoji, desc, props, ids):
    plain = {k: v for k, v in props.items() if "rel" not in v}
    db = b.call("POST", "/databases", {
        "parent": {"type": "page_id", "page_id": parent}, "is_inline": False,
        "icon": {"type": "emoji", "emoji": emoji}, "title": [b.rt(name)],
        "description": [b.rt(desc)], "properties": plain})
    ids[key] = db["id"]
    for pname, r in props.items():
        if "rel" not in r:
            continue
        target = ids[r["rel"]]
        res = b.call("PATCH", f"/databases/{db['id']}", {"properties": {pname: {"relation": {
            "database_id": target, "type": "dual_property", "dual_property": {}}}}})
        synced = res["properties"][pname]["relation"]["dual_property"]["synced_property_name"]
        if synced != r["back"]:
            b.call("PATCH", f"/databases/{target}", {"properties": {synced: {"name": r["back"]}}})
    print(f"  ✓ {emoji} {name}")


def patch_props(db, props):
    b.call("PATCH", f"/databases/{db}", {"properties": props})


def build():
    ids = {"root": ROOT}
    # Bereichsseiten
    ids["page_agentur"] = b.create_page(ROOT, "Agentur", "🤖", md("""
!🤖 Das Büro von ALEX und seinem Team. Du gibst Aufträge in **Anfragen an ALEX** – ALEX plant sie als **Jobs**, das Team arbeitet sie in n8n ab, der CreativeDirector prüft, du gibst frei.
## So läuft ein Auftrag
```
Du ──Anfrage──► ALEX plant (Dispatcher) ──Jobs──► Agent arbeitet (Worker)
 ▲                                                   │
 │                                   kreativ? ──► CreativeDirector prüft (Review)
 └──────────── ✋ Freigabe ◄──────────────────────────┘
```
## Job-Status
- **Entwurf** – noch nicht startklar (z. B. von dir angelegt, Auftrag fehlt)
- **Bereit** – der Agent startet beim nächsten Worker-Lauf
- **In Bearbeitung** – läuft gerade (gesperrt)
- **Review** – kreatives Ergebnis wartet auf den CreativeDirector
- **Freigabe** – wartet auf dich ✋
- **Überarbeiten** – zurück an den Agenten, mit Feedback (max. 3 Versuche)
- **Erledigt** – von dir freigegeben
- **Fehler** – Lauf fehlgeschlagen, Details im Agenten-Protokoll
## Deine Handgriffe
- [ ] Ergebnis lesen (steht im Job unter „Ergebnis“)
- [ ] Passt → Status **Erledigt** · Passt nicht → Text in **Feedback** schreiben und Status **Überarbeiten**
- [ ] Rückfrage von ALEX in einer Anfrage → **Antwort an ALEX** ausfüllen, Status wieder **Neu**
- [ ] Eigene Jobs direkt anlegen: Agent wählen, Auftrag schreiben, Status **Bereit**
## Datenbanken
"""))
    ids["page_studio"] = b.create_page(ROOT, "Studio", "🎬", md("""
!🎬 Dein Business: Projekte, eigene Aufgaben, Kunden, Content, Ideen und Finanzen – alles miteinander verknüpft.
## Datenbanken
"""))
    ids["page_wissen"] = b.create_page(ROOT, "Wissen & Standards", "📚", md("""
!📚 Das Gedächtnis von dir und dem Team: Marke, Qualitätsstandards, Abläufe, Vorlagen.
"""))
    print("  ✓ Bereichsseiten")

    parents = {"studio": ids["page_studio"], "wissen": ids["page_wissen"], "agentur": ids["page_agentur"]}
    for area, key, name, emoji, desc, props, _ in DBS:
        create_db(parents[area], key, name, emoji, desc, props, ids)

    # Formeln
    for area, key, name, emoji, desc, props, forms in DBS:
        for fname, expr in forms.items():
            patch_props(ids[key], {fname: formula(expr)})
    # Selbst-Relation „Wartet auf“ + Rollups
    patch_props(ids["jobs"], {"Wartet auf": {"relation": {"database_id": ids["jobs"], "type": "single_property", "single_property": {}}}})
    patch_props(ids["jobs"], {"Offene Vorgänger": {"rollup": {"relation_property_name": "Wartet auf",
                                                              "rollup_property_name": "Offen-Wert", "function": "sum"}}})
    patch_props(ids["anfragen"], {"Fortschritt": {"rollup": {"relation_property_name": "Jobs",
                                                             "rollup_property_name": "Erledigt-Wert", "function": "average"}}})
    patch_props(ids["anfragen"], {"📊 Fortschritt": formula(
        'if(empty(prop("Jobs")), "", format(round(prop("Fortschritt") * 100)) + " %")')})
    patch_props(ids["anfragen"], {"Offene Jobs": {"rollup": {"relation_property_name": "Jobs",
                                                             "rollup_property_name": "Offen-Wert", "function": "sum"}}})
    patch_props(ids["anfragen"], {"🔔": formula(
        'if(prop("Status") == "In Arbeit" and !empty(prop("Jobs")) and prop("Offene Jobs") == 0, "✋ Ergebnisse da", "")')})
    patch_props(ids["projekte"], {"Fortschritt": {"rollup": {"relation_property_name": "Aufgaben",
                                                             "rollup_property_name": "Erledigt-Wert", "function": "average"}}})
    patch_props(ids["projekte"], {"📊 Fortschritt": formula(
        'if(empty(prop("Aufgaben")), "", format(round(prop("Fortschritt") * 100)) + " %")')})
    patch_props(ids["kunden"], {"Umsatz gesamt": {"rollup": {"relation_property_name": "Finanzen",
                                                             "rollup_property_name": "Betrag (±)", "function": "sum"}}})
    print("  ✓ Formeln, Rollups, Selbst-Relation")

    # Agenten
    ids["agents"] = {}
    for name, role, dept, status, kann, model, effort, web, review, maxt, emoji in AGENTS:
        pr = prompt(name)
        page = b.call("POST", "/pages", {"parent": {"database_id": ids["agenten"]},
                                         "icon": {"type": "emoji", "emoji": emoji}, "properties": {
            "Agent": {"title": [b.rt(name)]}, "Rolle": {"rich_text": [b.rt(role)]},
            "Abteilung": {"select": {"name": dept}}, "Status": {"select": {"name": status}},
            "Kann": {"rich_text": [b.rt(kann)]}, "Modell": {"select": {"name": model}},
            "Effort": {"select": {"name": effort}}, "Websuche": {"checkbox": web},
            "Review durch CD": {"checkbox": review}, "Max Tokens": {"number": maxt},
            "System Prompt": {"rich_text": b.chunks(pr)},
        }})
        ids["agents"][name] = page["id"]
        b.append(page["id"], [b.callout("Dieser Text ist eine lesbare Kopie. Der n8n-Worker liest das Feld „System Prompt“ – "
                                        "Änderungen bitte dort vornehmen (oder im Repository unter agents/).", "ℹ️")] + md(pr))
        print(f"  ✓ Agent {emoji} {name} ({status})")

    json.dump(ids, open(IDS_FILE, "w"), indent=2)
    return ids


if __name__ == "__main__":
    ids = build()
    print(json.dumps({k: v for k, v in ids.items() if k != "agents"}, indent=2))

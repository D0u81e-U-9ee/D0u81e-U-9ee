#!/usr/bin/env python3
"""Baut Alex 2.0 fertig (nach ALEX_Optimierte_Agenten_Architektur.md + ALEX_Master_Dokumentation_Final.md).

* aktualisiert alle Agenten (Name, Full Name, Role, Specialization, Tools, System Prompt)
* legt ResearchAnalyst neu an
* ergänzt Status „Vorschlag“ in Alex 2.0 Aufgaben (für den TaskCreator)
* gibt der Alex-2.0-Seite einen Titel
* erstellt die Seite „ALEX 2.0 System-Dokumentation“ (Architektur, Agenten, Workflow, n8n-Code v2)
"""
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import build_workspace as b  # noqa: E402
from build_creative_director import md, chunks  # noqa: E402

IDS = json.load(open(os.path.join(HERE, "workspace_ids.json")))
ALEX_PAGE = IDS["alex"]["page"]
AGENTS_DB = "2a7ed125-9f02-8138-aac6-f2c7b47e1ac9"
TASKS_DB = "2a7ed125-9f02-8099-a2eb-d289e0e7d5a4"
LOGS_DB = "2a7ed125-9f02-801a-b585-fc5b2593085c"
COMMON = open(os.path.join(HERE, "agents", "_Gemeinsame_Regeln.md"), encoding="utf-8").read().strip()

# (alter Titel, neuer Titel, Emoji, Tier, Full Name, Role, Specialization, Tools)
AGENTS = [
    ("Alex-A", "ALEX", "🎯", "1 · Master & Coordination", "AI Launch Executive & Master Coordinator",
     "Orchestriert alle Agenten, erstellt tägliche Briefings, koordiniert Workflows und priorisiert Tasks nach 80/20-Prinzip",
     "Master Orchestration", ["Anthropic Claude", "Notion API", "n8n Workflows", "Agent Communication Bus"]),
    ("Task-T", "TaskManager", "⏰", "1 · Master & Coordination", "Task & Deadline Manager",
     "Überwacht Deadlines, priorisiert Aufgaben, warnt bei Problemen, tracked Progress",
     "Task Management & Monitoring", ["Anthropic Claude", "Notion API", "Calendar Integration", "Deadline Alerts"]),
    ("TaskCreator-TC", "TaskCreator", "💡", "1 · Master & Coordination", "Autonomous Task Generator",
     "Analysiert Lücken im System, erstellt automatisch notwendige Aufgaben, schlägt Optimierungen vor",
     "Intelligent Task Generation", ["Anthropic Claude", "Notion API", "Pattern Recognition"]),
    ("CreativeDirector", "CreativeDirector", "🎨", "2 · Content & Creative", "Creative Director & Brand Experience Lead",
     "Definiert kreative Vision, orchestriert Brand/Design/Content-Agenten, setzt kreative Standards, entscheidet über "
     "Look & Feel, quality-checkt alle kreativen Outputs, entwickelt Brand Experience",
     "Creative Direction & Brand Experience",
     ["Anthropic Claude", "Notion API", "Design Systems", "Brand Guidelines", "Creative Frameworks", "Moodboards", "Figma"]),
    ("Brand-E", "BrandStrategist", "🧭", "2 · Content & Creative", "Brand Strategy & Positioning Specialist",
     "Entwickelt Brand-Strategien, definiert Positioning, erstellt Messaging-Frameworks, analysiert Markt",
     "Brand Strategy & Market Positioning",
     ["Anthropic Claude", "Notion API", "Web Search", "Competitive Analysis Tools", "Brand Framework Templates"]),
    ("Design-A", "DesignConsultant", "🖌️", "2 · Content & Creative", "Visual Design & UX Consultant",
     "Erstellt Design-Konzepte, berät zu UX/UI, entwickelt visuelle Identität, optimiert User Experience",
     "Visual Design & User Experience", ["Anthropic Claude", "Notion API", "Figma", "Design Systems", "Image Generation"]),
    ("Copy-C", "ContentWriter", "✍️", "2 · Content & Creative", "Content Writer & Copywriting Specialist",
     "Erstellt Marketing-Copy, schreibt Blog-Posts, entwickelt Content-Strategien, optimiert für SEO",
     "Content Creation & Copywriting", ["Anthropic Claude", "Notion API", "Web Search", "SEO Tools", "Content Templates"]),
    ("Web-W", "WebDeveloper", "💻", "3 · Technical & Implementation", "Web Development & Implementation Specialist",
     "Entwickelt Websites, implementiert Features, optimiert Performance, maintained Code",
     "Web Development & Technical Implementation",
     ["Anthropic Claude", "Notion API", "Webflow", "Code Generation", "Git", "DevTools"]),
    ("Launch-L", "LaunchCoordinator", "🚀", "3 · Technical & Implementation", "Launch & Go-Live Manager",
     "Plant Launches, koordiniert Go-Live, erstellt Checklisten, monitored Post-Launch, managed Rollouts",
     "Launch Management & Project Rollout",
     ["Anthropic Claude", "Notion API", "Analytics", "Project Management Tools", "Monitoring Tools"]),
    (None, "ResearchAnalyst", "🔎", "4 · Research & Communication", "Research & Data Analysis Specialist",
     "Recherchiert Informationen, analysiert Daten, erstellt Reports, sammelt Competitive Intelligence",
     "Research & Data Analysis", ["Anthropic Claude", "Notion API", "Web Search", "Data Analysis Tools", "Web Scraping"]),
    ("E-Mail-M", "EmailManager", "✉️", "4 · Research & Communication", "Email Management & Communication Coordinator",
     "Verwaltet Emails, priorisiert Inbox, drafted Responses, koordiniert Kommunikation, tracked Follow-ups",
     "Email Management & Communication",
     ["Anthropic Claude", "Notion API", "Gmail API", "Email Templates", "Calendar Integration"]),
    ("System-S", "SystemAdmin", "🛠️", "4 · Research & Communication", "System Administrator & Infrastructure Manager",
     "Verwaltet System-Infrastruktur, monitored Performance, maintained Datenbanken, optimiert Workflows",
     "System Administration & Infrastructure", ["n8n Workflows", "Notion API", "Monitoring Tools", "Backup Systems"]),
]


def prompt_for(name):
    own = open(os.path.join(HERE, "agents", f"{name}_SystemPrompt.md"), encoding="utf-8").read().strip()
    return own if name == "CreativeDirector" else own + "\n\n" + COMMON


def find_agent(*titles):
    for t in titles:
        if not t:
            continue
        r = b.call("POST", f"/databases/{AGENTS_DB}/query", {"filter": {"property": "Agent", "title": {"equals": t}}})
        if r["results"]:
            return r["results"][0]["id"]
    return None


def update_agents():
    ids = {}
    for old, new, emoji, tier, full, role, spec, tools in AGENTS:
        props = {
            "Agent": {"title": [b.rt(new)]},
            "Full Name": {"rich_text": [b.rt(full)]},
            "Role": {"rich_text": [b.rt(role)]},
            "Specialization": {"multi_select": [{"name": spec}]},
            "Tools": {"multi_select": [{"name": t} for t in tools]},
            "Status": {"select": {"name": "Active"}},
            "System Prompt": {"rich_text": chunks(prompt_for(new))},
        }
        pid = find_agent(new, old)
        icon = {"type": "emoji", "emoji": emoji}
        if pid:
            b.call("PATCH", f"/pages/{pid}", {"properties": props, "icon": icon})
        else:
            props["Datum"] = {"date": {"start": b.TODAY.isoformat()}}
            pid = b.call("POST", "/pages", {"parent": {"database_id": AGENTS_DB}, "icon": icon, "properties": props})["id"]
        ids[new] = pid
        print(f"  ✓ {emoji} {new}" + (f"  (vorher {old})" if old and old != new else "  (neu)" if not old else ""))
    return ids


def add_status_option():
    props = b.call("GET", f"/databases/{TASKS_DB}")["properties"]
    options = props["Status"]["select"]["options"]
    if any(o["name"] == "Vorschlag" for o in options):
        return
    new = [{"name": o["name"], "color": o["color"]} for o in options] + [{"name": "Vorschlag", "color": "purple"}]
    b.call("PATCH", f"/databases/{TASKS_DB}", {"properties": {"Status": {"select": {"options": new}}}})
    print("  ✓ Status „Vorschlag“ in Alex 2.0 Aufgaben")


def code_block(text, lang="javascript"):
    return {"type": "code", "code": {"rich_text": chunks(text), "language": lang}}


def doc_page(agent_ids):
    cd = IDS.get("creative_director", {})
    blocks = md(f"""
!🧠 Zentrale Dokumentation von ALEX 2.0 · Stand {b.TODAY.strftime('%d.%m.%Y')} · gepflegt vom **SystemAdmin**
## 1. Architektur
```
ALEX (Master Coordinator)
├─ Tier 1 · Master & Coordination:   TaskManager · TaskCreator
├─ Tier 2 · Content & Creative:      CreativeDirector
│                                     ├─ BrandStrategist
│                                     ├─ DesignConsultant
│                                     └─ ContentWriter
├─ Tier 3 · Technical:               WebDeveloper · LaunchCoordinator
└─ Tier 4 · Research & Communication: ResearchAnalyst · EmailManager · SystemAdmin
```
**Namensregeln:** Agent = CamelCase ohne Bindestriche · Full Name = Funktion + Verantwortung + Titel · Role = 3–4 Verben + Ziel · Specialization ohne Nummerierung · 3–5 konkrete Tools. Basis-Tools für alle: Anthropic Claude + Notion API.
**Wichtig:** Nur Agenten mit Status „Active“ werden vom Tagesplaner erkannt.
## 2. Agenten-Verzeichnis
""")
    for old, new, emoji, tier, full, role, spec, tools in AGENTS:
        blocks.append(b.bullet([
            {"type": "mention", "mention": {"type": "page", "page": {"id": agent_ids[new]}}},
            b.rt(f"  ·  {full}  ·  Tier {tier}  ·  {spec}" + (f"  ·  vorher: {old}" if old and old != new else ""))]))
    blocks += md("""
Jeder Agent hat einen vollständigen **System Prompt** (Feld „System Prompt“ in der Agents Database). Alle Prompts enthalten zusätzlich die gemeinsamen Regeln: Kontext, Datenbanken, Freigabepflicht für externe Aktionen, Review-Pflicht für kreative Outputs, Logging, Eskalation.
## 3. Notion-Datenbanken
- **Alex 2.0 Agents Database** `2a7ed125-9f02-8138-aac6-f2c7b47e1ac9`
- **Alex 2.0 Aufgaben** `2a7ed125-9f02-8099-a2eb-d289e0e7d5a4` – Status: Offen · In Arbeit · Warte auf · Wiederkehrend · Erledigt · **Vorschlag** (TaskCreator) · neue Felder: Studio-Projekt, Heute, ⚠️ Warnung, Erledigt-Wert, Creative Brief, Creative Review, Asset Type
- **Alex 2.0 Tagespläne** `2a7ed125-9f02-807d-a1c1-ced7f999ba1c`
- **Alex Agent Logs** `2a7ed125-9f02-801a-b585-fc5b2593085c`
""")
    if cd:
        blocks += md(f"""
- **Creative Briefs** `{cd['briefs']}`
- **Creative Review Queue** `{cd['queue']}`
""")
    blocks += md("""
- **Studio-Datenbanken:** siehe „🎬 Media Creative Studio HQ“ → System (Datenbanken)
## 4. n8n-Workflow „ALEX – Tagesplaner“
```
[Schedule Trigger 07:00 Europe/Berlin]
        ↓
   ├─→ Aufgaben laden (Notion)
   ├─→ alleAgenten (Notion)
   └─→ Creative Reviews laden (Notion)   ← NEU
        ↓
     Merge (3 Eingänge)
        ↓
  Aufgaben vorbereiten (Code v2) → Message a model (Claude) → Antwort verarbeiten
        ↓
  Tagesplan erstellen (Notion) → Tagesplan erstellt (Code v2) → Log Eintrag erstellt (Notion)
```
## 5. To-do in n8n (Phase 1: System stabilisieren)
- [ ] Manual Trigger durch **Schedule Trigger** ersetzen: täglich, 07:00, Timezone Europe/Berlin
- [ ] Workflow **aktivieren** (Toggle oben rechts auf „Active“)
- [ ] Node **„Creative Reviews laden“** hinzufügen (Kopie von „Aufgaben laden“, Datenbank: Creative Review Queue) und als 3. Eingang an Merge
- [ ] Code von **„Aufgaben vorbereiten“** durch v2 ersetzen (unten)
- [ ] Code von **„Tagesplan erstellt“** durch v2 ersetzen und im Log-Node „Verarbeitete Items“ + „Ausführungszeit (Sek)“ mappen
- [ ] **Titel-Bug beheben:** In „Tagesplan erstellen“ und „Log Eintrag erstellt“ steht im Titelfeld ein führendes „=“ als Text → Feld auf *Expression* umstellen bzw. „=“ entfernen
- [ ] Claude-Modell im Node „Message a model“ prüfen und ggf. auf ein aktuelles Modell umstellen
- [ ] 1 Woche Monitoring (SystemAdmin)
## 6. Was v2 behebt und ergänzt
- **Bugfix:** Specialization ist in Notion ein *Multi-Select* – v1 las `select.name` und bekam nie einen Wert
- **Bugfix:** Agenten werden über „Full Name“ erkannt (Aufgaben haben ebenfalls ein Feld „Agent“)
- Aufgaben ohne Titel und mit Status „Vorschlag“ werden nicht eingeplant; Vorschläge werden gezählt
- Sortierung: Überfällig → ☀️ Heute → Priorität → Deadline; Überfällig wird markiert
- Creative Review Queue im Briefing (Priorität bei mehr als 5 offenen Reviews)
- Logging: Verarbeitete Items, Ausführungszeit, aussagekräftige Details
## 7. Code „Aufgaben vorbereiten“ (v2)
""")
    blocks.append(code_block(open(os.path.join(HERE, "n8n", "Aufgaben_vorbereiten_v2.js"), encoding="utf-8").read()))
    blocks += md("## 8. Code „Tagesplan erstellt“ (v2)")
    blocks.append(code_block(open(os.path.join(HERE, "n8n", "Tagesplan_erstellt_v2.js"), encoding="utf-8").read()))
    blocks += md("""
## 9. Roadmap
### Phase 2 · Multi-Agent-Zuordnung
- [ ] Tagespläne eine Woche prüfen: Werden die neuen Agentennamen korrekt zugeordnet?
- [ ] Prompt in „Aufgaben vorbereiten“ bei Bedarf schärfen
### Phase 3 · TaskCreator-Workflow
- [ ] Neuer Workflow „ALEX – Task Creator“ (z. B. wöchentlich Montag 08:00)
- [ ] Lädt Projekte, Creative Briefs, Meetings, Aufgaben → Claude mit System Prompt des TaskCreator
- [ ] Schreibt max. 5 Aufgaben mit Status **„Vorschlag“**
### Phase 4 · Orchestrierung
- [ ] ALEX triggert Agent-Workflows (n8n „Execute Workflow“) anhand des Felds „Agent“
- [ ] Jeder Agent-Workflow lädt seinen System Prompt aus der Agents Database (kein Prompt im n8n-Code duplizieren)
- [ ] Shared Context: Ergebnisse als Seiteninhalt der Aufgabe + Log-Eintrag
## 10. Sicherheit
- API-Schlüssel gehören in n8n-Credentials, **nicht** in Notion-Seiten. Bitte die Seite „API Keys“ prüfen und dort gespeicherte Schlüssel in einen Passwort-Manager bzw. n8n-Credentials verschieben (und ggf. rotieren).
""")
    title = "ALEX 2.0 System-Dokumentation"
    existing = {}
    r = b.call("GET", f"/blocks/{ALEX_PAGE}/children?page_size=100")
    for bl in r["results"]:
        if bl["type"] == "child_page":
            existing[bl["child_page"]["title"]] = bl["id"]
    if title in existing:
        b.call("PATCH", f"/pages/{existing[title]}", {"archived": True})  # neu aufbauen
    pid = b.create_page(ALEX_PAGE, title, "🧠", blocks)
    print("  ✓ ALEX 2.0 System-Dokumentation")
    return pid


def main():
    agent_ids = update_agents()
    add_status_option()
    page = b.call("GET", f"/pages/{ALEX_PAGE}")
    if not "".join(x["plain_text"] for x in page["properties"]["title"]["title"]).strip():
        b.call("PATCH", f"/pages/{ALEX_PAGE}", {"properties": {"title": {"title": [b.rt("Alex 2.0")]}}})
        print("  ✓ Seitentitel „Alex 2.0“ gesetzt")
    doc = doc_page(agent_ids)
    b.call("POST", "/pages", {"parent": {"database_id": LOGS_DB}, "properties": {
        "Name": {"title": [b.rt(f"Agenten-Architektur v2 ausgerollt – {b.TODAY.strftime('%d.%m.%Y')}")]},
        "Status": {"select": {"name": "Erfolg"}},
        "Agent": {"rich_text": [b.rt("SystemAdmin")]},
        "Verarbeitete Items": {"number": len(AGENTS)},
        "Details": {"rich_text": [b.rt("12 Agenten nach neuem Namensschema aktualisiert, ResearchAnalyst neu, System Prompts für alle "
                                       "Agenten, Status „Vorschlag“ ergänzt, System-Dokumentation inkl. n8n-Code v2 erstellt.")]},
    }})
    IDS["alex"]["agents"] = agent_ids
    IDS["alex"]["doc"] = doc
    json.dump(IDS, open(os.path.join(HERE, "workspace_ids.json"), "w"), indent=2)
    print("\nDoku: https://www.notion.so/" + doc.replace("-", ""))


if __name__ == "__main__":
    main()

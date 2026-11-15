# 🎬 Media Creative Studio – Notion-Arbeitsumgebung

Ein persönliches Studio-Betriebssystem in Notion: Projekte, Kunden, Content, Assets,
Equipment, Finanzen und Wissen – verbunden über Relationen, mit Tages- und
Wochenroutinen, SOPs und Assistenten-Prompts.

## Aufbau

```
ACHIM GUTMANN – COMMAND CENTER        ← bestehend, oben verlinkt auf das HQ
└── 🎬 Media Creative Studio HQ        ← Dashboard / Startseite
    ├── 🧭 Start hier                      ← Anleitung & Bedienkonzept
    ├── ☀️ Tagesplanung                    ← tägliche Assistenten-Routine
    ├── 🔁 Wochenrückblick                 ← Weekly Review (Freitag)
    ├── 🤖 Assistent-Prompts               ← fertige Prompts für Notion AI / Claude
    ├── 📋 SOPs & Workflows
    │   ├── 🎥 Video-Produktion (Pre → Post → Delivery)
    │   ├── 🤝 Kunden-Onboarding
    │   ├── ♻️ Content-Repurposing
    │   └── 🧾 Angebot & Rechnung
    └── 🗄 System (Datenbanken)
        ├── 📥 Inbox            – Schnellerfassung für alles
        ├── ✅ Aufgaben         = „Alex 2.0 Aufgaben“ (bestehend, um Studio-Projekt, Heute, ⚠️ Warnung ergänzt)
        ├── 🎞 Projekte         – Pipeline Idee → Abgeschlossen, Fortschritt automatisch
        ├── 🎯 Ziele            – Quartalsziele, mit Projekten verknüpft
        ├── 👥 Kunden & Kontakte – Mini-CRM mit Follow-up-Erinnerung
        ├── 📅 Content-Kalender – YouTube, Instagram, TikTok, LinkedIn, Podcast …
        ├── 💡 Ideen-Pool       – Ideen sammeln & bewerten
        ├── 🗂 Asset-Bibliothek – Footage, Musik, LUTs, Templates, Lizenzen
        ├── 🧰 Equipment        – Kamera, Licht, Audio, Status & Wert
        ├── 💶 Finanzen         – Einnahmen/Ausgaben, Rechnungsstatus
        ├── 📓 Meetings & Journal – Calls, Brainstorms, Reviews
        └── 📚 Wissen & Vorlagen – Tutorials, Checklisten, Prompts
```

## Verknüpfungen (Relationen)

- Projekt ⇄ Kunde, Ziel, Aufgaben, Content, Assets, Finanzen, Meetings
- Meeting ⇄ Kontakte
- Finanzen ⇄ Kunde
- **Projekt-Fortschritt** wird automatisch aus erledigten Aufgaben berechnet
- **Überfällig-Warnungen** bei Aufgaben, **Follow-up-Warnungen** im CRM

## So hilft es dir wie ein Assistent

1. **Erfassen**: alles landet zuerst in der 📥 Inbox (Idee, Aufgabe, Link, Notiz).
2. **Täglich (5 Min.)**: ☀️ Tagesplanung → Inbox leeren, 3 Top-Aufgaben setzen (`Heute` ✔).
3. **Wöchentlich (20 Min.)**: 🔁 Wochenrückblick → Projekte, Follow-ups, Finanzen, Content.
4. **Wiederholbare Abläufe**: SOPs als Checklisten.
5. **KI-Unterstützung**: 🤖 Assistent-Prompts für Notion AI bzw. Claude.

## Einbindung von Alex 2.0 und Command Center

- **Alex 2.0 Aufgaben** ist die gemeinsame Aufgaben-Datenbank. Es wurden nur neue Felder
  ergänzt (`Studio-Projekt`, `Heute`, `Erledigt-Wert`, `⚠️ Warnung`), bestehende Felder sind
  unverändert, damit die n8n-Workflows weiterlaufen.
- Das HQ verlinkt Alex 2.0 (Seite, Tagespläne, Agent Logs, Agents).
- Im Command Center steht oben ein Hinweis mit Link zum HQ.
- Die alten englischen Datenbanken (Projects, Clients, All Tasks …) bleiben unverändert erhalten.

## Offene Punkte / Hinweise

- **Daily Briefings**: Die 370 leeren Einträge wurden am 15.11.2026 gelöscht (Papierkorb, 30 Tage wiederherstellbar). Ursache ist kein n8n-Workflow, sondern die Notion-Datenbankvorlage „Daily Briefing Template“ mit täglicher Wiederholung. Abschalten: Daily Briefings → Pfeil neben „Neu“ → „…“ an der Vorlage → Wiederholen → Aus.

## Creative Director (Alex 2.0)

Umgesetzt nach `docs/ALEX_CreativeDirector_Komplett.md` mit `build_creative_director.py`:
Agent-Eintrag inkl. System Prompt (`agents/CreativeDirector_SystemPrompt.md`), Datenbanken
Creative Briefs und Creative Review Queue, neue Felder in Alex 2.0 Aufgaben, Seiten Brand
Guidelines, Review Criteria, Playbook (inkl. n8n-Code) und Brief-Vorlage.

## Ansichten (über den Notion-Connector angelegt)

| Datenbank | Ansichten |
|---|---|
| Alex 2.0 Aufgaben | ☀️ Heute · Board · Kalender |
| Creative Review Queue | Review-Board · Wartet auf Review |
| Creative Briefs | Brief-Pipeline · Deadlines |
| Projekte | Pipeline · Timeline |
| Content-Kalender | Kalender · Produktion |
| Finanzen | Offene Rechnungen · Ergebnis-Verlauf |
| Kunden | Follow-ups |
| Inbox | Zu verarbeiten |
| Ideen / Assets / Equipment / Meetings / Ziele | Board bzw. Kalender |

## Technik

Gebaut mit `build_workspace.py` über die offizielle Notion-API
(Version 2022-06-28). Das Skript legt alle Seiten, Datenbanken, Relationen,
Formeln (Beispieldaten optional mit `--samples`) an.

```bash
python3 build_workspace.py --parent <seiten-id> [--tasks-db <id>] [--alex-page <id>] [--link-from <id>] [--samples]
```

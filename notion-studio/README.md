# ALEX · Media Creative Studio (v3)

Persönlicher Assistent **ALEX** mit einem Agenten-Team in **n8n**, das wie eine Agentur arbeitet – organisiert in **Notion**.
Konzept: [`KONZEPT.md`](KONZEPT.md) · Erkenntnisse aus Alex 2.0: [`docs/ERKENNTNISSE.md`](docs/ERKENNTNISSE.md)

## Inhalt
| Pfad | Zweck |
|---|---|
| `build_v3.py` | Baut Bereichsseiten, 12 Datenbanken (Relationen, Formeln, Rollups) und das Agenten-Team |
| `build_v3_wissen.py` | Wissen & Standards: Brand Guidelines, Review-Kriterien, Routinen, SOPs, Vorlagen, Prompts |
| `build_v3_system.py` | Seite „System & n8n“ inkl. Einrichtungsanleitung und Workflow-JSON |
| `build_n8n.py` | Erzeugt die n8n-Workflows aus `n8n/src/*.js` → `n8n/workflows/*.json` |
| `agents/` | System Prompts der Agenten + gemeinsame Agentur-Regeln |
| `n8n/src/` | Code der n8n-Code-Nodes (`_lib.js` = gemeinsame Hilfsfunktionen) |
| `n8n/workflows/` | **Import-Dateien für n8n** (1 Dispatcher · 2 Worker · 3 Review · 4 Morgen-Briefing) |
| `n8n/test/mock_claude.js` | Claude-Mock für Ende-zu-Ende-Tests ohne API-Kosten |
| `notion_ids.json` | IDs der Notion-Seiten/-Datenbanken (vom Generator genutzt) |
| `notion_lib.py` | Notion-API-Helfer (Blöcke, Markdown → Notion) |
| `cleanup_all.py` | Leert den Workspace (wurde vor dem Neuaufbau verwendet) |
| `archiv-v2/` | Vorheriger Stand (Studio v2 + Alex 2.0 Ausbau) |

## Einrichtung n8n (Kurzfassung – ausführlich auf der Notion-Seite „System & n8n“)
1. Notion-Integration anlegen und mit der Seite **ALEX · Media Creative Studio** verbinden.
2. In n8n zwei **Header Auth**-Credentials anlegen – Namen exakt:
   - `Notion · ALEX Agentur` → `Authorization: Bearer ntn_…`
   - `Anthropic · ALEX Agentur` → `x-api-key: sk-ant-…`
3. `n8n/workflows/*.json` importieren (n8n verbindet die Credentials über den Namen).
4. Mit einer Test-Anfrage manuell testen, dann alle 4 Workflows aktivieren.

## Getestet
n8n 1.123.96 (CLI) gegen das echte Notion, Claude als Mock (`n8n/test/mock_claude.js`):
Dispatcher → Worker (Recherche) → Worker (Text mit Recherche-Kontext) → Review (6/10 → Überarbeiten) →
Worker (Überarbeitung mit Feedback) → Review (8/10 → Freigabe) → Morgen-Briefing; Fehlerfall (Claude nicht erreichbar → Status „Fehler“ + Protokoll).
Alle Claude-Anfragen wurden vom Mock auf gültige Felder geprüft. Echte Claude-Antworten sind noch ungetestet (benötigt API-Key).

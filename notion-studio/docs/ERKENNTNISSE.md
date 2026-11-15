# Erkenntnisse aus Alex 2.0 und dem ersten Studio-Aufbau

Stand: 15.11.2026. Vor dem kompletten Neuaufbau festgehalten: Was war gut, was hat funktioniert, was nicht.

## Grundgedanke (bleibt)
- **ALEX** ist der persönliche Assistent von Achim und gleichzeitig Agenturleiter.
- ALEX beschäftigt ein **Team von n8n-Agenten**, die wie in einer Agentur Aufgaben übernehmen
  (Strategie, Text, Design, Recherche, Technik, Kommunikation).
- Notion ist das **Büro** der Agentur: Hier kommen Aufträge an, hier liegen Ergebnisse, hier gibt Achim frei.

## Was funktioniert hat
- n8n-Workflow „ALEX – Tagesplaner“: Aufgaben + Agenten laden → Merge → Prompt bauen (Code) →
  Claude → Tagesplan in Notion → Log. Lief nachweislich (Tagespläne/Logs vom 11.11.2025 bis 02.10.2026).
- Agenten als **Datensätze** in Notion (Name, Rolle, Status, System Prompt) – ALEX liest die Liste
  dynamisch, Umbenennen bricht nichts.
- Nur Agenten mit Status „Active“ werden genutzt → einfacher Schalter zum Ein-/Ausschalten.
- Robuster n8n-Code: Optional Chaining `?.[0]`, Unterscheidung der Datenbanken über eindeutige Felder.
- Logging pro Lauf (Agent, Aktion, Status, Details, Dauer, verarbeitete Einträge).
- Creative-Director-Konzept: Quality Gate mit 10 Kriterien, Mindestscore 7/10, Feedback-Schleife
  (Approved / Needs Work / Rejected), Creative Briefs.
- Namensschema der Agenten: CamelCase, Funktion + Typ (ContentWriter, BrandStrategist …),
  Full Name = Funktion + Verantwortung, Role = 3–4 Verben + Ziel, 4 Tiers.
- Im Studio-Aufbau sinnvoll: Inbox als einziger Eingang, Tages- und Wochenroutine, SOPs als
  Checklisten, Projekte ⇄ Kunden ⇄ Content ⇄ Finanzen verknüpft, Ansichten (Board, Kalender, Timeline),
  automatische Warnungen (überfällig, Follow-up), Projektfortschritt per Rollup.

## Warum es nicht das erwartete Ergebnis geliefert hat
1. **Die Agenten haben nie gearbeitet.** Sie standen als Einträge in einer Liste, aber es gab keinen
   Workflow, der ihnen Aufgaben übergibt, ihren Prompt ausführt und das Ergebnis zurückschreibt.
   Keiner hatte einen System Prompt oder eine n8n-URL.
2. **Kein Auftragsprotokoll.** Es fehlte ein klarer Weg: Achim → ALEX → Agent → Ergebnis → Freigabe.
   ALEX hat nur einen Tagesplan geschrieben („Agent: X“), danach passierte nichts.
3. **Zu viele Agenten auf einmal** (10–12), alle „Active“, keiner getestet.
4. **Zwei vermischte Systeme**: Command Center (Englisch, Website-Launch) und Alex 2.0 (Deutsch) mit
   doppelten Datenbanken (Projects/Tasks doppelt).
5. **Fehler, die niemand bemerkt hat**:
   - Titel mit führendem „=“ im Notion-Node (Expression-Modus falsch)
   - `Specialization` als Multi-Select gelesen wie Select → Wert kam nie im Prompt an
   - Wiederkehrende Notion-Vorlage hat 370 leere „Daily Briefing“-Seiten erzeugt
6. Manueller Trigger statt Zeitplan, Workflow nicht aktiv.

## Konsequenzen für den Neuaufbau
- **Ein generischer Agenten-Worker** in n8n statt eines Workflows pro Agent: Der Worker holt den
  nächsten Job, lädt den System Prompt des zuständigen Agenten aus Notion, ruft Claude auf und schreibt
  das Ergebnis in den Job. Neuer Agent = neuer Datensatz, kein neuer Workflow.
- **Job-Warteschlange mit festen Status** als Vertrag zwischen Notion und n8n.
- **Klein starten**: wenige aktive Agenten, die wirklich arbeiten. Weitere stehen auf „Im Aufbau“.
- **Ein System, eine Sprache (Deutsch), keine Dubletten.**
- **Mensch bleibt in der Schleife**: Alles mit Außenwirkung wartet auf Achims Freigabe.
- Fehler sichtbar machen: Status „Fehler“ + Protokoll + Hinweis im Morgen-Briefing.
- Keine wiederkehrenden Notion-Vorlagen für automatisch erzeugte Inhalte.
- API-Schlüssel nur in n8n-Credentials, nie in Notion.

## Namen und Ideen, die wir weiterverwenden
ALEX · CreativeDirector · BrandStrategist · DesignConsultant · ContentWriter · WebDeveloper ·
LaunchCoordinator · ResearchAnalyst · EmailManager · SystemAdmin · (TaskManager/TaskCreator gehen als
Funktionen in ALEX auf) · Creative Briefs · Review Queue · Brand Guidelines · Tagesplan/Briefing ·
Agent Logs · 80/20-Priorisierung · Tiers.

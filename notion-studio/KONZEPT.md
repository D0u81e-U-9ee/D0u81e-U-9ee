# ALEX · Media Creative Studio – Konzept v3 (Neuaufbau)

## Die Idee in einem Satz
Achim gibt Aufträge an **ALEX**, seinen persönlichen Assistenten. ALEX plant sie, verteilt sie als
**Jobs** an sein **Agenten-Team** in n8n, prüft die Ergebnisse und legt sie Achim zur **Freigabe** vor,
wie eine Agentur mit Achim als Chef.

```
 Achim ──Anfrage──► ALEX (Dispatcher) ──Jobs──► Agenten-Team (n8n-Worker)
   ▲                    │                              │
   │                    │◄──── Ergebnis + Review ──────┘
   └──Freigabe/Briefing─┘        (CreativeDirector prüft kreative Jobs)
```

## Notion-Struktur (alles neu, Deutsch, keine Dubletten)
```
🏠 ALEX · Media Creative Studio          ← Startseite / Cockpit
├── 🤖 Agentur                            ← das Büro von ALEX und seinem Team
│   ├── 📨 Anfragen an ALEX               (deine Aufträge, auch per Formular vom Handy)
│   ├── ⚙️ Jobs                           (Warteschlange für die n8n-Agenten)
│   ├── 🧑‍🚀 Agenten-Team                    (Rollen, System Prompts, Status)
│   ├── 📜 Agenten-Protokoll               (jeder Lauf: Dauer, Status, Fehler)
│   └── ☀️ Briefings                       (Morgen-Briefing, Wochenrückblick von ALEX)
├── 🎬 Studio                              ← dein Business
│   ├── 🎞 Projekte · ✅ Meine Aufgaben · 👥 Kunden & Kontakte
│   ├── 📅 Content-Kalender · 💡 Ideen · 💶 Finanzen
├── 📚 Wissen & Standards                  ← Gedächtnis von dir und dem Team
│   ├── Brand Guidelines · Review-Kriterien · SOPs (Video-Produktion, Onboarding,
│   │   Repurposing, Angebot & Rechnung) · Prompt-Bibliothek
│   └── 📚 Wissensbasis (Datenbank)
└── ⚙️ System & n8n                        ← Bauplan: Protokoll, IDs, Setup-Anleitung
```

## Das Agenten-Team
| Abteilung | Agent | Aufgabe | Start |
|---|---|---|---|
| Leitung | **ALEX** | Persönlicher Assistent & Agenturleiter: plant, verteilt, fasst zusammen, Briefings | aktiv |
| Leitung | **CreativeDirector** | Quality Gate: prüft kreative Ergebnisse (10 Kriterien, ≥ 7/10) | aktiv |
| Kreation | **ContentWriter** | Posts, Captions, Skripte, Website-Texte, Newsletter | aktiv |
| Produktion | **VideoProducer** (neu) | Video-Konzepte, Shotlists, Drehpläne, Schnittpläne, Packlisten | aktiv |
| Kommunikation | **EmailManager** | E-Mail-Entwürfe, Follow-ups (sendet nie selbst) | aktiv |
| Recherche | **ResearchAnalyst** | Recherche mit Quellen, Wettbewerb, Trends (Claude-Websuche) | aktiv |
| Kreation | BrandStrategist | Positionierung, Messaging | im Aufbau |
| Kreation | DesignConsultant | Designkonzepte, Moodboards, UX | im Aufbau |
| Technik | WebDeveloper | Website/Webflow | im Aufbau |
| Technik | LaunchCoordinator | Launch-Planung, Go-Live | im Aufbau |
| Betrieb | SystemAdmin | Überwachung der Workflows | im Aufbau |

TaskManager und TaskCreator sind keine eigenen Agenten mehr. Ihre Aufgaben übernimmt ALEX
(Planen = Dispatcher, Überwachen = Briefing).

## Das Job-Protokoll (Vertrag zwischen Notion und n8n)
| Status | Wer setzt ihn | Bedeutung |
|---|---|---|
| 📝 Entwurf | Achim/ALEX | noch nicht startklar |
| ▶️ Bereit | ALEX/Achim | Agent darf starten |
| ⚙️ In Bearbeitung | Worker | gesperrt, läuft gerade |
| 🔍 Review | Worker | kreatives Ergebnis wartet auf CreativeDirector |
| ✋ Freigabe | Worker/CD | wartet auf Achim |
| 🔁 Überarbeiten | CD/Achim | zurück an den Agenten (mit Feedback, max. 3 Versuche) |
| ✅ Erledigt | Achim | freigegeben |
| ❌ Fehler | Worker | Lauf fehlgeschlagen, siehe Protokoll |

## n8n-Workflows (werden mitgeliefert, als Import-Datei)
1. **ALEX · Dispatcher** (alle 10 Min.): neue Anfragen → Claude plant Jobs (JSON) → Jobs anlegen.
   Sind alle Jobs einer Anfrage erledigt, schreibt ALEX eine Zusammenfassung.
2. **Agenten · Worker** (alle 5 Min.): nächsten Job mit „Bereit“/„Überarbeiten“ holen → Prompt des
   Agenten laden → Claude → Ergebnis in die Job-Seite → Status → Protokoll.
3. **CreativeDirector · Review** (alle 10 Min.): Jobs in „Review“ bewerten → Score + Feedback →
   „Freigabe“ oder „Überarbeiten“.
4. **ALEX · Morgen-Briefing** (Mo–Fr 07:00): Aufgaben, Deadlines, Freigaben, Content, Follow-ups,
   Fehler → Briefing-Seite.

## Wie du damit arbeitest
1. Auftrag geben: **Formular „Neue Anfrage an ALEX“** (oder Eintrag in „Anfragen an ALEX“).
2. ALEX plant, das Team arbeitet. Du siehst alles im Board „Jobs“.
3. Morgens liest du das **Briefing** und gibst Ergebnisse unter „✋ Wartet auf dich“ frei.
4. Eigene To-dos stehen in **Meine Aufgaben**, Projekte, Kunden und Content im **Studio**.

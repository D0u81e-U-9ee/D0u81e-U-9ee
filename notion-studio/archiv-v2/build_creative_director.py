#!/usr/bin/env python3
"""Implementiert den CreativeDirector-Agenten für Alex 2.0 (nach ALEX_CreativeDirector_Komplett.md).

Legt an (nur additiv, bestehende Felder bleiben unverändert):
  * Eintrag „CreativeDirector“ in der Alex 2.0 Agents Database (inkl. System Prompt)
  * Datenbanken „Creative Briefs“ und „Creative Review Queue“ auf der Alex-2.0-Seite
  * Neue Felder in „Alex 2.0 Aufgaben“: Creative Brief, Creative Review, Asset Type
  * Seiten: ALEX 2.0 Brand Guidelines, Creative Review Criteria, Creative Director Playbook,
    Creative Brief Vorlage
  * Erster Creative Brief + Log-Eintrag
"""
import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import build_workspace as b  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
IDS = json.load(open(os.path.join(HERE, "workspace_ids.json")))
ALEX_PAGE = IDS["alex"]["page"]
AGENTS_DB = "2a7ed125-9f02-8138-aac6-f2c7b47e1ac9"
TASKS_DB = "2a7ed125-9f02-8099-a2eb-d289e0e7d5a4"
LOGS_DB = "2a7ed125-9f02-801a-b585-fc5b2593085c"
PROJEKTE_DB = IDS["databases"]["projekte"]
PROMPT_FILE = os.path.join(HERE, "agents", "CreativeDirector_SystemPrompt.md")


# ---------------------------------------------------------------- Markdown → Blöcke
def inline(text):
    """**fett** und `code` in Rich-Text umwandeln."""
    out, bold, code = [], False, False
    buf = ""
    i = 0
    while i < len(text):
        if text.startswith("**", i) and not code:
            if buf: out.append(b.rt(buf, bold=bold, code=code))
            buf, bold = "", not bold
            i += 2
            continue
        if text[i] == "`":
            if buf: out.append(b.rt(buf, bold=bold, code=code))
            buf, code = "", not code
            i += 1
            continue
        buf += text[i]
        i += 1
    if buf: out.append(b.rt(buf, bold=bold, code=code))
    return out or [b.rt("")]


def md(text):
    blocks, code_buf, in_code = [], [], False
    for line in text.strip("\n").splitlines():
        s = line.rstrip()
        if s.startswith("```"):
            if in_code:
                blocks.append(b.code("\n".join(code_buf)))
                code_buf, in_code = [], False
            else:
                in_code = True
            continue
        if in_code:
            code_buf.append(line); continue
        st = s.strip()
        if not st:
            continue
        if st == "---": blocks.append(b.divider())
        elif st.startswith("### "): blocks.append(b.h3(inline(st[4:])))
        elif st.startswith("## "): blocks.append(b.h2(inline(st[3:])))
        elif st.startswith("# "): blocks.append(b.h1(inline(st[2:])))
        elif st.startswith("- [ ] "): blocks.append(b.todo(inline(st[6:])))
        elif st.startswith(("- ", "✓ ", "✗ ")): blocks.append(b.bullet(inline(st[2:] if st.startswith("- ") else st)))
        elif st[:2].rstrip(".").isdigit() and ". " in st[:4]: blocks.append(b.num(inline(st.split(". ", 1)[1])))
        elif st.startswith("> "): blocks.append(b.quote(inline(st[2:])))
        elif st.startswith("!"):
            emoji, rest = st[1:].split(" ", 1)
            blocks.append(b.callout(inline(rest), emoji, "gray_background"))
        else: blocks.append(b.p(inline(st)))
    return blocks


def chunks(text, n=1900):
    return [b.rt(text[i:i + n]) for i in range(0, len(text), n)]


# ---------------------------------------------------------------- Inhalte
BRAND_GUIDELINES = """
!🎨 Version 1.0 · Owner: **Creative Director** · Quartalsweise prüfen. Platzhalter in [Klammern] bitte ergänzen.
## 1. Brand Overview
### Mission
[Deine Mission Statement]
### Vision
[Deine Vision für ALEX 2.0 / Achim Gutmann Media Manufaktur]
### Values
- Innovation
- Pragmatism
- Excellence
- Transparency
- Human-Centered AI
---
## 2. Brand Personality
**Wer wir sind:**
- Innovativ, aber pragmatisch
- Professional, aber approachable
- Ambitious, aber realistisch
- Technical, aber verständlich
**Wer wir NICHT sind:**
- Hype ohne Substance
- Überkompliziert
- Unpersönlich
- Unrealistisch
---
## 3. Visual Identity
### Logo
[Upload oder Link zu Logo]
✓ Minimum Size: 40px Höhe
✓ Clear Space: 20px rundum
✗ Nicht verzerren
✗ Farben nicht ändern
✗ Keine Effekte hinzufügen
### Color Palette
**Primary:** Brand Blue `#0066CC` · Brand Black `#1A1A1A`
**Secondary:** Accent Orange `#FF6B35` · Success Green `#00C851` · Warning Yellow `#FFB300`
**Neutrals:** White `#FFFFFF` · Light Gray `#F5F5F5` · Dark Gray `#666666`
### Typography
- **Headlines:** Inter Bold · 48 / 36 / 24 px · Line Height 1.2
- **Body:** Inter Regular · 16 px · Line Height 1.6
- **Code:** JetBrains Mono · 14 px
### Imagery Style
- Moderne, klare Fotografie
- Tech-forward, aber nicht kalt
- Echte Menschen und echte Ergebnisse zeigen
- Keine generischen Stock-Fotos
- Datenvisualisierungen nutzen
---
## 4. Tone of Voice
### Prinzipien
- **Confident** ohne arrogant
- **Expert** ohne überheblich
- **Inspirierend** ohne unrealistisch
- **Direkt** ohne unhöflich
### Writing Style
- Aktive Sprache
- Knapp und präzise
- Fachjargon vermeiden (oder erklären)
- Show, don't just tell
- Daten und Beispiele nutzen
### Beispiele
✓ Gut: „ALEX 2.0 orchestriert 20+ KI-Agenten für dein Business. Kein Recruiting. Keine Gehaltsabrechnung. Nur Ergebnisse.“
✗ Schlecht: „Leverage synergistic AI paradigms to disrupt traditional business operations.“
---
## 5. Do's & Don'ts
### ✓ Do's
- Konkrete Ergebnisse zeigen
- Transparent über den Prozess sein
- Balance zwischen Vision und Umsetzung
- Tech-Kompetenz demonstrieren
- Menschlich bleiben trotz KI-Fokus
- Echte Daten und Beispiele nutzen
- Ehrlich über Grenzen sein
### ✗ Don'ts
- KI-Hype ohne Substanz
- Leere Versprechen
- Unerklärter Fachjargon
- Generische Stock-Bilder
- Inkonsistenzen über Kanäle
- Over-Promising
- Vage bleiben
---
## 6. Brand Applications
### Website
- Clean, modern, klare Hierarchie
- Schnell ladend, Mobile-first
- Datengetriebene Designentscheidungen
### Social Media (LinkedIn)
- Professioneller Ton, Value-first
- Daten & Insights, Behind-the-Scenes
- Thought Leadership
### E-Mail
- Klare Betreffzeilen, scanbarer Inhalt
- Starker CTA, persönliche Note
- Mehrwert in jeder Mail
---
## 7. Quality Standards
Jeder Output muss im Schnitt **7+/10** erreichen – siehe Seite „Creative Review Criteria“.
---
## 8. Approval Process
1. Entspricht diesen Guidelines
2. Hat das Creative-Director-Review durchlaufen
3. Erreicht 7+/10 auf den Qualitätskriterien
4. Ist vor Veröffentlichung freigegeben
**Review-Zeiten:** Standard 48 h · Dringend 24 h · Notfall am selben Tag
"""

CRITERIA = [
    ("Brand Alignment", "Passt der Output zur definierten Brand Identity (Personality, Visual Style, Tone)?"),
    ("Strategic Fit", "Unterstützt er die Business-Ziele und das Positioning?"),
    ("Execution Quality", "Erfüllt er professionelle Standards (Handwerk, Details, Fehlerfreiheit)?"),
    ("Consistency", "Ist er konsistent mit anderen Outputs und Kanälen?"),
    ("Impact", "Erzielt er die gewünschte Wirkung bei der Zielgruppe?"),
    ("Innovation", "Bringt er etwas Neues, statt Generisches zu wiederholen?"),
    ("Clarity", "Ist die Botschaft sofort klar?"),
    ("Emotional Resonance", "Berührt er die Zielgruppe?"),
    ("Memorability", "Bleibt er im Kopf?"),
    ("Feasibility", "Ist er realistisch umsetzbar (Zeit, Budget, Technik)?"),
]


def criteria_page():
    blocks = md("""
!⭐ Jedes kreative Asset wird auf 10 Kriterien mit je 1–10 Punkten bewertet. **Minimum für Approval: 7/10 im Durchschnitt.**
## Die 10 Kriterien
""")
    for i, (name, q) in enumerate(CRITERIA, 1):
        blocks.append(b.num([b.rt(f"{name}: ", bold=True), b.rt(q)]))
    blocks += md("""
## Scoring
- **9–10** Herausragend – sofort freigeben, als Referenz sichern
- **7–8** Gut – Approved (ggf. mit kleinen Hinweisen)
- **5–6** Needs Work – konkretes Feedback, Revision Number +1
- **1–4** Rejected – komplett neu aufsetzen, Begründung im Feedback
## Feedback-Regeln
- **Spezifisch:** „Das Logo ist zu komplex, reduziere auf 2 Farben.“
- **Konstruktiv:** „Der Headline fehlt Punch. Versuch: [Vorschlag]“
- **Ehrlich:** „Das erfüllt nicht unsere Standards. Neu aufsetzen.“
## Feedback-Vorlage
```
REVIEW: [Score]/10

✓ On-Brand:
✓ On-Strategy:
✓ Quality:
✗ Verbesserung:

NEXT STEPS:
-
```
""")
    return blocks


PLAYBOOK = """
!🎬 Arbeitsanleitung für den CreativeDirector (berichtet an **ALEX – Master Coordinator**).
## Orchestrierung
```
Creative Director
    ↓ Creative Brief
├─→ BrandStrategist (Brand-E)   – Strategy
├─→ DesignConsultant (Design-A) – Visuals
└─→ ContentWriter (Copy-C)      – Copy
    ↓ Outputs → Creative Review Queue
Creative Director (Quality Check)
    ↓ Approved
ALEX (Final Delivery) → LaunchCoordinator (Launch-L)
```
## Daily Routine
### Morgens (9:00–10:00)
- [ ] Creative Review Queue checken (Filter: Pending Review)
- [ ] Reviews priorisieren (High zuerst)
- [ ] ALEX-Tagesplan auf Creative-Aufgaben prüfen
### Mittags (14:00–15:00)
- [ ] Reviews durchführen und Feedback geben
- [ ] Queue-Status aktualisieren
- [ ] Mit Agenten kommunizieren (Notion-Kommentare)
### Abends (17:00–17:30)
- [ ] Finaler Queue-Check
- [ ] Status an ALEX melden (Eintrag in Alex Agent Logs)
- [ ] Reviews für morgen planen
### Wöchentlich (Montag 10:00)
- [ ] Kommende Creative Briefs prüfen
- [ ] Sync mit Brand / Design / Content
- [ ] Brand Guidelines bei Bedarf aktualisieren
### Monatlich (erster Montag)
- [ ] Qualitätsmetriken auswerten
- [ ] Muster erkennen: Was wird oft rejected?
- [ ] Review-Prozess optimieren
- [ ] Gute Arbeit feiern
## Review-Prozess
1. Asset kommt mit Status **Pending Review** in die Queue (mit Creative Brief verknüpft)
2. Status → **In Review**, Review Date setzen
3. Nach den 10 Kriterien bewerten, Score eintragen
4. Feedback + Next Steps ausfüllen
5. Status → **Approved**, **Needs Work** (Revision Number +1) oder **Rejected**
## Eskalation
- Mehr als 5 Reviews pending → ALEX priorisiert Reviews im Tagesplan
- Deadline in Gefahr → sofort an ALEX melden, Scope oder Termin klären
- Lieber Launch verschieben als mit schlechter Qualität rausgehen
## Kennzahlen
- **Wöchentlich:** 10+ Reviews · Ø Score 7,5+ · First-Try-Approval 60 %+ · Turnaround < 48 h
- **Monatlich:** 40+ Reviews · Qualitätstrend steigend · weniger Revisionen
## Use Cases
### LinkedIn Content Series
1. ContentWriter erstellt Post → Creative Review Queue
2. CreativeDirector wird benachrichtigt (n8n, geplant)
3. Review: On-Brand, Tone, Message, CTA
4. Needs Work + Feedback → Überarbeitung → Resubmit
5. Approved → ContentWriter veröffentlicht
### Website Redesign
1. Creative Brief (Big Idea, Key Message, Tone, Deliverables)
2. DesignConsultant + ContentWriter zuweisen
3. Wöchentlicher Creative Sync über Notion-Kommentare
4. Iterieren bis alle Seiten 8+/10
5. Hand-off an WebDeveloper (Web-W)
### Brand Guidelines weiterentwickeln
1. Mit BrandStrategist: Positioning, Personality, Values
2. Mit DesignConsultant: Logo, Farben, Typografie, Bildstil
3. Mit ContentWriter: Tone of Voice, Writing Guidelines
4. In „ALEX 2.0 Brand Guidelines“ zusammenführen und allen Agenten verlinken
"""

N8N = r"""
## n8n-Integration (ALEX – Tagesplaner)
!⚙️ Diese Schritte finden in n8n statt. Ich habe keinen Zugriff auf dein n8n, deshalb musst du sie dort selbst umsetzen.
1. Node „Aufgaben laden“ duplizieren → umbenennen in **Creative Reviews laden**
2. Database: **Creative Review Queue** · Filter: Status = „Pending Review“ · Sort: Review Priority
3. Mit dem Merge-Node verbinden (Input 3)
4. Code-Node „Aufgaben vorbereiten“ ergänzen:
```
// 3. CREATIVE REVIEWS
let reviewsListe = "";
let pendingReviews = 0;
const reviewItems = allItems.filter(item =>
  item.json.properties && item.json.properties['Asset Type'] && item.json.properties['Review Priority']
);
reviewItems.forEach(item => {
  const props = item.json.properties;
  const name = props.Name?.title[0]?.plain_text || "Unnamed";
  const assetType = props['Asset Type']?.select?.name || "";
  const createdBy = props['Created By']?.select?.name || "";
  const priority = props['Review Priority']?.select?.name || "Medium";
  pendingReviews++;
  reviewsListe += `- ${name} (${assetType}) von ${createdBy} [${priority}]\n`;
});

// im Prompt ergänzen:
// ⚠️ CREATIVE REVIEW QUEUE: ${pendingReviews} Assets warten auf Review
// ${reviewsListe || "Keine pending Reviews"}
// WICHTIG: Priorisiere Creative Reviews wenn > 5 pending

// im return ergänzen:  pending_reviews: pendingReviews,
```
!🐞 Bekannter Fehler: Die Titel deiner Tagespläne und Logs beginnen mit „=“, z. B. „=Tagesplan …“. Im Titelfeld des Notion-Nodes steht ein `=` vor dem Text, obwohl das Feld nicht im Expression-Modus ist. Lösung: Feld auf „Expression“ umstellen oder das führende `=` entfernen.
"""

BRIEF_TEMPLATE = """
!📝 Vorlage – kopieren oder in „Creative Briefs“ als Datenbankvorlage hinterlegen (Datenbank öffnen → Pfeil neben „Neu“ → „+ Neue Vorlage“, Inhalt hier hineinkopieren).
# Creative Brief: [Projekt Name]
## 1. Projekt Overview
- **Was:** [Was wird erstellt]
- **Warum:** [Business-Ziel]
- **Wann:** [Deadline]
- **Wer:** [Zielgruppe]
## 2. Creative Direction
- **Big Idea:** [Zentrale kreative Idee]
- **Key Message:** [Hauptbotschaft]
- **Tone:** [Professional, Inspirational, Educational, Playful, Bold]
- **Style:** [Visueller Stil]
## 3. Deliverables
- [ ] [Deliverable 1]
- [ ] [Deliverable 2]
- [ ] [Deliverable 3]
## 4. Brand Guidelines
- **Colors:** [Farbpalette]
- **Fonts:** [Schriften]
- **Imagery:** [Art Direction für Bilder]
- **Do's & Don'ts:** [Klare Vorgaben]
## 5. Inspiration
- **Referenzen:** [Links zu Beispielen]
- **Moodboard:** [Link zu Moodboard]
## 6. Success Criteria
- [ ] [Kriterium 1]
- [ ] [Kriterium 2]
---
Erstellt von: Creative Director · Datum: [Datum] · Approved by: ALEX
"""


# ---------------------------------------------------------------- Bau
def find_by_title(db, prop, value):
    r = b.call("POST", f"/databases/{db}/query", {"filter": {"property": prop, "title": {"equals": value}}})
    return r["results"][0]["id"] if r["results"] else None


def child_titles(page):
    out = {}
    cur = None
    while True:
        r = b.call("GET", f"/blocks/{page}/children?page_size=100" + (f"&start_cursor={cur}" if cur else ""))
        for bl in r["results"]:
            if bl["type"] in ("child_page", "child_database"):
                out[bl[bl["type"]]["title"]] = bl["id"]
        if not r.get("has_more"):
            return out
        cur = r["next_cursor"]


def dual(db_from, name, db_to, back):
    props = b.call("GET", f"/databases/{db_from}")["properties"]
    if name in props:
        return
    res = b.call("PATCH", f"/databases/{db_from}", {"properties": {name: {"relation": {
        "database_id": db_to, "type": "dual_property", "dual_property": {}}}}})
    synced = res["properties"][name]["relation"]["dual_property"]["synced_property_name"]
    if synced != back:
        b.call("PATCH", f"/databases/{db_to}", {"properties": {synced: {"name": back}}})


def make_db(parent, title, emoji, desc, props):
    return b.call("POST", "/databases", {
        "parent": {"type": "page_id", "page_id": parent}, "is_inline": False,
        "icon": {"type": "emoji", "emoji": emoji}, "title": [b.rt(title)],
        "description": [b.rt(desc)], "properties": props})["id"]


def main():
    existing = child_titles(ALEX_PAGE)
    out = {}

    # 1) Agent-Eintrag
    prompt = open(PROMPT_FILE, encoding="utf-8").read().strip()
    agent = find_by_title(AGENTS_DB, "Agent", "CreativeDirector")
    props = {
        "Agent": {"title": [b.rt("CreativeDirector")]},
        "Full Name": {"rich_text": [b.rt("Creative Director & Brand Experience Lead")]},
        "Role": {"rich_text": [b.rt("Definiert kreative Vision, orchestriert Brand/Design/Content-Agenten, setzt kreative "
                                    "Standards, entscheidet über Look & Feel, quality-checkt alle kreativen Outputs, "
                                    "entwickelt Brand Experience")]},
        "Specialization": {"multi_select": [{"name": "08_Creative Direction & Brand Experience"}]},
        "Tools": {"multi_select": [{"name": n} for n in (
            "01_Anthropic Claude", "06_Figma", "08_Design Systems", "09_Brand Guidelines",
            "10_Creative Frameworks", "11_Moodboards")]},
        "Status": {"select": {"name": "Active"}},
        "System Prompt": {"rich_text": chunks(prompt)},
        "E-Mail": {"email": "creative@gutmann-media.de"},
        "Datum": {"date": {"start": b.TODAY.isoformat()}},
    }
    if agent:
        b.call("PATCH", f"/pages/{agent}", {"properties": props})
    else:
        agent = b.call("POST", "/pages", {"parent": {"database_id": AGENTS_DB},
                                          "icon": {"type": "emoji", "emoji": "🎨"}, "properties": props})["id"]
    out["agent"] = agent
    print("  ✓ Agent CreativeDirector")

    # 2) Creative Briefs
    sel, msel = b.sel, b.msel
    briefs = existing.get("Creative Briefs") or make_db(ALEX_PAGE, "Creative Briefs", "📝",
        "Zentrale Verwaltung aller Creative Briefs – Vorlage: Seite „Creative Brief Vorlage“.", {
        "Name": b.title(),
        "Status": sel(("Draft", "gray"), ("In Review", "yellow"), ("Approved", "green"), ("In Progress", "blue"), ("Completed", "purple")),
        "Projekt-Typ": sel("Brand Identity", "Campaign", "Content Series", "Website", "Product Launch"),
        "Deadline": b.date(),
        "Zielgruppe": b.text(),
        "Big Idea": b.text(),
        "Key Message": b.text(),
        "Tone": msel("Professional", "Inspirational", "Educational", "Playful", "Bold"),
        "Assigned Agents": msel("BrandStrategist", "DesignConsultant", "ContentWriter", "CreativeDirector"),
        "Deliverables": b.text(),
        "Brand Colors": b.text(),
        "Referenzen": b.url(),
        "Success Criteria": b.text(),
        "Brief Document": {"files": {}},
        "Created By": sel("CreativeDirector", "ALEX", "Manual"),
        "Approved By": sel("ALEX", "Manual"),
        "Notes": b.text(),
    })
    out["briefs"] = briefs
    print("  ✓ Datenbank Creative Briefs")

    # 3) Creative Review Queue
    queue = existing.get("Creative Review Queue") or make_db(ALEX_PAGE, "Creative Review Queue", "🔍",
        "Quality-Control-Pipeline. Minimum für Approval: Score 7/10 – siehe „Creative Review Criteria“.", {
        "Name": b.title(),
        "Status": sel(("Pending Review", "yellow"), ("In Review", "blue"), ("Approved", "green"), ("Needs Work", "orange"), ("Rejected", "red")),
        "Asset Type": sel("Logo", "Website", "Content", "Campaign", "Design", "Copy"),
        "Created By": sel("BrandStrategist", "DesignConsultant", "ContentWriter", "Manual"),
        "Review Priority": sel(("High", "red"), ("Medium", "yellow"), ("Low", "gray")),
        "Submitted Date": b.date(),
        "Review Date": b.date(),
        "Asset Link": b.url(),
        "Asset Datei": {"files": {}},
        "Review Criteria": b.text(),
        "Feedback": b.text(),
        "Score": b.number(),
        "Next Steps": b.text(),
        "Revision Number": b.number(),
    })
    out["queue"] = queue
    dual(queue, "Creative Brief", briefs, "Reviews")
    try:
        b.call("PATCH", f"/databases/{queue}", {"properties": {"Approval": b.formula(
            'if(empty(prop("Score")), "", if(prop("Score") >= 7, "✅ freigabefähig", "⛔ unter 7"))')}})
    except RuntimeError as e:
        print("    ! Formel Approval übersprungen:", e)
    print("  ✓ Datenbank Creative Review Queue")

    # 4) Aufgaben erweitern + Verknüpfung zum Studio
    dual(TASKS_DB, "Creative Brief", briefs, "Aufgaben")
    dual(TASKS_DB, "Creative Review", queue, "Aufgaben")
    if "Asset Type" not in b.call("GET", f"/databases/{TASKS_DB}")["properties"]:
        b.call("PATCH", f"/databases/{TASKS_DB}", {"properties": {"Asset Type": sel("Brand", "Design", "Content", "Campaign")}})
    dual(briefs, "Studio-Projekt", PROJEKTE_DB, "Creative Briefs")
    print("  ✓ Alex 2.0 Aufgaben erweitert (Creative Brief, Creative Review, Asset Type)")

    # 5) Dokumentation
    pages = {
        "ALEX 2.0 Brand Guidelines": ("🎨", md(BRAND_GUIDELINES)),
        "Creative Review Criteria": ("⭐", criteria_page()),
        "Creative Director Playbook": ("🎬", md(PLAYBOOK) + md(N8N)),
        "Creative Brief Vorlage": ("📝", md(BRIEF_TEMPLATE)),
    }
    for title, (emoji, blocks) in pages.items():
        out[title] = existing.get(title) or b.create_page(ALEX_PAGE, title, emoji, blocks)
    print("  ✓ Brand Guidelines, Review Criteria, Playbook, Brief-Vorlage")

    # Links im Agent-Eintrag
    b.append(agent, [
        b.callout("Arbeitsgrundlagen des CreativeDirector", "🎨", "purple_background"),
        b.link_page(out["ALEX 2.0 Brand Guidelines"]), b.link_page(out["Creative Review Criteria"]),
        b.link_page(out["Creative Director Playbook"]), b.link_page(out["Creative Brief Vorlage"]),
        b.link_db(briefs), b.link_db(queue),
    ]) if not b.call("GET", f"/blocks/{agent}/children")["results"] else None

    # 6) Erster echter Brief
    if not find_by_title(briefs, "Name", "LinkedIn Content Series Q1 2027"):
        b.call("POST", "/pages", {"parent": {"database_id": briefs}, "properties": {
            "Name": {"title": [b.rt("LinkedIn Content Series Q1 2027")]},
            "Status": {"select": {"name": "Draft"}},
            "Projekt-Typ": {"select": {"name": "Content Series"}},
            "Deadline": {"date": {"start": "2027-03-31"}},
            "Zielgruppe": {"rich_text": [b.rt("Entrepreneurs und Tech-Leads, die KI für ihr Business nutzen wollen")]},
            "Big Idea": {"rich_text": [b.rt("„AI-First Entrepreneurship in Action“ – zeigen, wie ALEX 2.0 echte Business-Probleme löst")]},
            "Key Message": {"rich_text": [b.rt("Du brauchst kein 20-köpfiges Team. Du brauchst 20 intelligente Agenten.")]},
            "Tone": {"multi_select": [{"name": "Professional"}, {"name": "Inspirational"}]},
            "Assigned Agents": {"multi_select": [{"name": "ContentWriter"}, {"name": "DesignConsultant"}]},
            "Deliverables": {"rich_text": [b.rt("12 LinkedIn-Posts (1 pro Woche) · 4 Deep-Dive-Threads · Visual Assets für jeden Post")]},
            "Brand Colors": {"rich_text": [b.rt("Primary #0066CC · Accent #FF6B35")]},
            "Success Criteria": {"rich_text": [b.rt("Engagement Rate > 5 % · 500+ neue Follower · 10+ qualifizierte Leads · Thought Leadership")]},
            "Created By": {"select": {"name": "CreativeDirector"}},
            "Notes": {"rich_text": [b.rt("Fokus auf konkrete Use Cases und Ergebnisse. Show, don't just tell. Mix aus Educational und Inspirational.")]},
        }, "children": md(BRIEF_TEMPLATE)[1:]})
    print("  ✓ Erster Creative Brief")

    # 7) Log
    b.call("POST", "/pages", {"parent": {"database_id": LOGS_DB}, "properties": {
        "Name": {"title": [b.rt(f"CreativeDirector aktiviert – {b.TODAY.strftime('%d.%m.%Y')}")]},
        "Status": {"select": {"name": "Erfolg"}},
        "Agent": {"rich_text": [b.rt("CreativeDirector")]},
        "Details": {"rich_text": [b.rt("Agent angelegt, Creative Briefs + Review Queue erstellt, Aufgaben erweitert, "
                                       "Brand Guidelines/Criteria/Playbook dokumentiert.")]},
        "Alex 2.0 Aufgaben": {"relation": []},
    }})
    print("  ✓ Log-Eintrag")

    IDS["creative_director"] = out
    json.dump(IDS, open(os.path.join(HERE, "workspace_ids.json"), "w"), indent=2)
    print(json.dumps(out, indent=2))


if __name__ == "__main__":
    main()

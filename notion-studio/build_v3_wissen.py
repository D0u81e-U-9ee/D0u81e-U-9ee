#!/usr/bin/env python3
"""Inhalte für „Wissen & Standards“ (v3): Brand Guidelines, Review-Kriterien, Routinen, SOPs, Vorlagen, Prompts."""
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import notion_lib as b  # noqa: E402
from notion_lib import md  # noqa: E402

sys.path.insert(0, os.path.join(HERE, "archiv-v2"))
import build_workspace as v2  # noqa: E402  (SOPs, Prompt-Bibliothek)
import build_creative_director as cd  # noqa: E402  (Kriterien, Brief-Vorlage)

IDS = json.load(open(os.path.join(HERE, "notion_ids.json")))
W = IDS["page_wissen"]

BRAND = """
!🎨 Version 1.0 · Owner: **CreativeDirector** · Die Agenten richten sich danach. Platzhalter in [Klammern] bitte ergänzen – je genauer, desto besser die Ergebnisse.
## 1. Wer wir sind
- **Achim Gutmann Media Manufaktur** – Media Creative Studio in München
- Leistungen: [Imagefilme, Social-Media-Content, Produktvideos, Fotografie, Brand & Web]
- Zielkunden: [z. B. lokale Unternehmen, Gastronomie, Mittelstand, Personal Brands]
- Versprechen: [z. B. „Hochwertiger Content, der wirkt – schnell, persönlich, aus einer Hand.“]
## 2. Persönlichkeit
- Kreativ, aber pragmatisch
- Professionell, aber nahbar
- Handwerklich präzise, aber nicht steif
- Technisch versiert, aber verständlich
## 3. Tonalität
- **Kompetent** ohne Arroganz · **Persönlich** ohne Anbiederung · **Konkret** statt Floskeln · **Inspirierend** ohne Übertreibung
- Aktive Sprache, kurze Sätze, Beispiele und Ergebnisse statt Versprechen
- Anrede: [Du auf Social Media / Sie in Kundenkommunikation]
## 4. Visuelle Identität
- **Farben:** [Primär #…] · [Sekundär #…] · [Akzent #…] · Neutrals: #FFFFFF, #F5F5F5, #1A1A1A
- **Schriften:** [Headline-Font] · [Text-Font]
- **Bildsprache:** echte Menschen, echte Orte, natürliches Licht, cineastischer Look; keine generischen Stockfotos
- **Video-Look:** [z. B. warm, dokumentarisch, ruhige Kamerabewegungen, 24/25 fps]
- **Logo:** [Link] · Schutzzone = Höhe des Buchstabens „A“ · nicht verzerren, keine Effekte
## 5. Do's & Don'ts
### ✓ Do's
- Ergebnisse und Making-ofs zeigen
- Transparent über den Prozess sprechen
- Lokalen Bezug (München/Bayern) nutzen, wo es passt
- Jeden Inhalt mit einem klaren Ziel und Call-to-Action versehen
### ✗ Don'ts
- KI-Hype und leere Superlative („revolutionär“, „next level“)
- Unerklärter Fachjargon
- Versprechen ohne Beleg
- Inkonsistenter Look zwischen Kanälen
## 6. Kanäle
- **Instagram/TikTok:** Reels mit Hook in den ersten 2 Sekunden, Untertitel, 3–5 Hashtags
- **LinkedIn:** persönliche Perspektive, Learnings aus Projekten, Frage am Ende
- **YouTube:** Behind-the-Scenes, Tutorials, Kapitelmarken, starkes Thumbnail
- **Website:** klare Leistungsseiten, Referenzen, schnelle Kontaktaufnahme
- **E-Mail:** kurz, persönlich, ein nächster Schritt
"""

ROUTINE_TAG = """
!☀️ 10 Minuten am Morgen. ALEX hat das Briefing schon geschrieben – du entscheidest.
## Morgens
- [ ] **Briefing** von heute lesen (Cockpit → „Letztes Briefing“)
- [ ] **✋ Wartet auf dich** abarbeiten: Ergebnis prüfen → Erledigt oder Feedback + Überarbeiten
- [ ] **Rückfragen** von ALEX beantworten (Anfragen mit Status „Rückfrage“)
- [ ] Max. 3 eigene Aufgaben mit **Heute** markieren
- [ ] Neue Ideen/Aufträge als **Anfrage an ALEX** übergeben
## Abends (2 Min.)
- [ ] Erledigte Aufgaben abhaken, „Heute“ entfernen
- [ ] Alles, was im Kopf ist, in Anfragen oder Ideen ablegen
"""

ROUTINE_WOCHE = """
!🔁 Freitag, 20 Minuten. Danach ist der Kopf frei fürs Wochenende.
## Rückblick
- [ ] Was wurde abgeschlossen (Aufgaben, Jobs, Projekte)?
- [ ] Was ist liegen geblieben – und warum?
## Projekte & Kunden
- [ ] Jedes aktive Projekt: Status aktuell, nächster Schritt vorhanden?
- [ ] Deadlines der nächsten 14 Tage prüfen
- [ ] Follow-ups für nächste Woche setzen, mindestens 1 Lead kontaktieren
## Finanzen
- [ ] Offene und überfällige Rechnungen prüfen → ggf. Anfrage an ALEX: „Zahlungserinnerung für …“
- [ ] Belege der Woche erfasst
## Content
- [ ] Content der nächsten Woche geplant? → ggf. Anfrage an ALEX
- [ ] Views der veröffentlichten Beiträge eintragen
## Agentur
- [ ] Agenten-Protokoll: Fehler diese Woche? Wenn ja, im System & n8n nachsehen
- [ ] Welcher Agent hat gute Arbeit geliefert, welcher braucht einen besseren Prompt?
## Nächste Woche
- [ ] Die 3 wichtigsten Ergebnisse festlegen
- [ ] Equipment: Akkus laden, Karten sichern/formatieren
"""


def criteria():
    blocks = md("""
!⭐ So bewertet der CreativeDirector jedes kreative Ergebnis. **Freigabe ab 7,0 im Durchschnitt.** Darunter geht der Job mit Feedback zurück an den Agenten (max. 3 Versuche, danach entscheidest du).
## Die 10 Kriterien (je 1–10 Punkte)
""")
    for name, q in cd.CRITERIA:
        blocks.append(b.num([b.rt(f"{name}: ", bold=True), b.rt(q)]))
    blocks += md("""
## Bedeutung
- **9–10** herausragend – als Referenz in der Wissensbasis sichern
- **7–8** gut – Freigabe, ggf. mit kleinen Hinweisen
- **5–6** Überarbeiten – konkretes Feedback
- **1–4** grundsätzlich neu aufsetzen
""")
    return blocks


def prompts_page():
    blocks = md("""
!🤖 Prompts für dich – in eine **Anfrage an ALEX** kopieren (Feld Beschreibung oder Seiteninhalt) oder direkt in Claude nutzen. Ersetze die [Platzhalter].
""")
    for t, prompt in v2.PROMPTS:
        blocks.append(b.toggle([b.rt(t, bold=True)], [b.code_block(prompt)]))
    return blocks


def main():
    pages = {}
    pages["brand"] = b.create_page(W, "Brand Guidelines", "🎨", md(BRAND))
    pages["kriterien"] = b.create_page(W, "Creative Review Kriterien", "⭐", criteria())
    pages["tag"] = b.create_page(W, "Mein Tag mit ALEX", "☀️", md(ROUTINE_TAG))
    pages["woche"] = b.create_page(W, "Wochenrückblick", "🔁", md(ROUTINE_WOCHE))
    pages["sops"] = b.create_page(W, "SOPs & Abläufe", "📋", md(
        "!📋 Wiederholbare Abläufe als Checklisten. Tipp: als Vorlage in Projekte übernehmen oder einen Schritt als Anfrage an ALEX geben."))
    for (emoji, name), blocks in v2.SOPS.items():
        raw = json.dumps(blocks, ensure_ascii=False)
        for old, new in [("Anfrage in 📥 Inbox → Kontakt in 👥 Kunden (Typ: Lead) anlegen", "Anfrage → Kontakt in 👥 Kunden & Kontakte (Typ: Lead) anlegen"),
                         ("Notizen in 📓 Meetings", "Notizen in die Kundenseite (👥 Kunden & Kontakte)"),
                         ("Assets in 🗂 Asset-Bibliothek eintragen", "Learnings in 📚 Wissensbasis eintragen"),
                         ("(Prompt 📧)", "(Anfrage an ALEX → EmailManager)"),
                         ("einen Schritt als Anfrage an ALEX", "einen Schritt als Anfrage an ALEX")]:
            raw = raw.replace(old, new)
        create = json.loads(raw)
        b.create_page(pages["sops"], name, emoji, create)
    pages["brief"] = b.create_page(W, "Creative Brief Vorlage", "📝", md(cd.BRIEF_TEMPLATE.replace(
        "in „Creative Briefs“ als Datenbankvorlage hinterlegen (Datenbank öffnen → Pfeil neben „Neu“ → „+ Neue Vorlage“, Inhalt hier hineinkopieren)",
        "für große Projekte ausfüllen und als Seiteninhalt in die Anfrage an ALEX kopieren")))
    pages["prompts"] = b.create_page(W, "Prompt-Bibliothek", "🤖", prompts_page())
    IDS["wissen_pages"] = pages
    json.dump(IDS, open(os.path.join(HERE, "notion_ids.json"), "w"), indent=2)
    print("  ✓ Wissen & Standards:", ", ".join(pages))


if __name__ == "__main__":
    main()

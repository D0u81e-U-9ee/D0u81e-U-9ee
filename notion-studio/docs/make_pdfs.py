#!/usr/bin/env python3
"""Erzeugt Briefing.pdf und ToDo.pdf für Achim (ALEX · Media Creative Studio)."""
import os

from reportlab.lib import colors
from reportlab.lib.enums import TA_LEFT
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle
from reportlab.lib.units import mm
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.platypus import (KeepTogether, PageBreak, Paragraph, SimpleDocTemplate, Spacer, Table,
                                TableStyle)

HERE = os.path.dirname(os.path.abspath(__file__))
F = "/usr/share/fonts/truetype/dejavu/"
pdfmetrics.registerFont(TTFont("DV", F + "DejaVuSans.ttf"))
pdfmetrics.registerFont(TTFont("DVB", F + "DejaVuSans-Bold.ttf"))
pdfmetrics.registerFont(TTFont("DVI", F + ("DejaVuSans-Oblique.ttf" if os.path.exists(F + "DejaVuSans-Oblique.ttf") else "DejaVuSans.ttf")))
pdfmetrics.registerFont(TTFont("DVM", F + "DejaVuSansMono.ttf"))
from reportlab.pdfbase.pdfmetrics import registerFontFamily  # noqa: E402
registerFontFamily("DV", normal="DV", bold="DVB", italic="DVI", boldItalic="DVB")

INK = colors.HexColor("#1A1A1A")
MUTED = colors.HexColor("#5F6368")
ACCENT = colors.HexColor("#6B4FBB")
SOFT = colors.HexColor("#F3F0FA")
LINE = colors.HexColor("#DDD8EA")
WARN = colors.HexColor("#FFF4E5")

S = {
    "title": ParagraphStyle("t", fontName="DVB", fontSize=22, leading=27, textColor=INK, spaceAfter=4),
    "sub": ParagraphStyle("s", fontName="DV", fontSize=10.5, leading=15, textColor=MUTED, spaceAfter=14),
    "h1": ParagraphStyle("h1", fontName="DVB", fontSize=14, leading=18, textColor=ACCENT, spaceBefore=14, spaceAfter=6, keepWithNext=1),
    "h2": ParagraphStyle("h2", fontName="DVB", fontSize=11.5, leading=15, textColor=INK, spaceBefore=8, spaceAfter=4),
    "p": ParagraphStyle("p", fontName="DV", fontSize=10, leading=14.5, textColor=INK, spaceAfter=5, alignment=TA_LEFT),
    "small": ParagraphStyle("sm", fontName="DV", fontSize=8.5, leading=12, textColor=MUTED),
    "cell": ParagraphStyle("c", fontName="DV", fontSize=9, leading=12.5, textColor=INK),
    "cellb": ParagraphStyle("cb", fontName="DVB", fontSize=9, leading=12.5, textColor=INK),
    "mono": ParagraphStyle("m", fontName="DVM", fontSize=8.6, leading=12, textColor=INK),
    "bullet": ParagraphStyle("b", fontName="DV", fontSize=10, leading=14.5, textColor=INK, leftIndent=12, bulletIndent=0, spaceAfter=2),
}
ROOT_URL = "https://www.notion.so/2a6ed1259f0280abb4d8c14332d2960a"
FORM_URL = "https://www.notion.so/419ed1259f0281f48c16fee5224ffc2e?v=419ed1259f028155bc49000c50086eab"


def P(t, s="p"): return Paragraph(t, S[s])
def B(items): return [Paragraph(i, S["bullet"], bulletText="•") for i in items]


def box(flow, bg=SOFT, border=LINE):
    t = Table([[flow]], colWidths=[170 * mm])
    t.setStyle(TableStyle([("BACKGROUND", (0, 0), (-1, -1), bg), ("BOX", (0, 0), (-1, -1), 0.6, border),
                           ("LEFTPADDING", (0, 0), (-1, -1), 9), ("RIGHTPADDING", (0, 0), (-1, -1), 9),
                           ("TOPPADDING", (0, 0), (-1, -1), 7), ("BOTTOMPADDING", (0, 0), (-1, -1), 7)]))
    return t


def table(rows, widths, header=True):
    data = [[P(c, "cellb" if (header and r == 0) else "cell") for c in row] for r, row in enumerate(rows)]
    t = Table(data, colWidths=[w * mm for w in widths], repeatRows=1 if header else 0)
    st = [("GRID", (0, 0), (-1, -1), 0.4, LINE), ("VALIGN", (0, 0), (-1, -1), "TOP"),
          ("LEFTPADDING", (0, 0), (-1, -1), 5), ("RIGHTPADDING", (0, 0), (-1, -1), 5),
          ("TOPPADDING", (0, 0), (-1, -1), 4), ("BOTTOMPADDING", (0, 0), (-1, -1), 4)]
    if header:
        st.append(("BACKGROUND", (0, 0), (-1, 0), SOFT))
    t.setStyle(TableStyle(st))
    return t


def footer(title):
    def draw(c, d):
        c.saveState()
        c.setFont("DV", 8)
        c.setFillColor(MUTED)
        c.drawString(20 * mm, 12 * mm, f"ALEX · Media Creative Studio — {title}")
        c.drawRightString(190 * mm, 12 * mm, f"Seite {d.page}")
        c.setStrokeColor(LINE)
        c.line(20 * mm, 16 * mm, 190 * mm, 16 * mm)
        c.restoreState()
    return draw


def doc(path, title):
    return SimpleDocTemplate(path, pagesize=A4, leftMargin=20 * mm, rightMargin=20 * mm, topMargin=18 * mm,
                             bottomMargin=22 * mm, title=title, author="Claude für Achim Gutmann")


# =========================================================== BRIEFING
def briefing():
    path = os.path.join(HERE, "Briefing_ALEX_Agentur.pdf")
    s = []
    s += [P("Briefing: ALEX · Media Creative Studio", "title"),
          P("Deine neue Notion-Arbeitsumgebung mit persönlichem Assistenten und KI-Agentur · Stand 15. November 2026", "sub")]
    s.append(box(P("<b>Kurz gesagt:</b> Du gibst ALEX einen Auftrag. ALEX plant ihn, verteilt die Arbeit an sein Team "
                   "aus KI-Agenten in n8n, der CreativeDirector prüft die Qualität, und du gibst am Ende frei. "
                   "Notion ist das Büro dieser Agentur – alles ist sichtbar, nichts geht ohne deine Freigabe nach außen.")))

    s.append(P("1 · Ausgangslage – warum wir neu gebaut haben", "h1"))
    s.append(P("Alex 2.0 hatte die richtige Idee, aber die Agenten haben nie gearbeitet: Sie standen als Einträge in einer "
               "Liste, ohne System Prompt und ohne Workflow. ALEX schrieb nur einen Tagesplan („Agent: X“) – danach passierte nichts. "
               "Dazu kamen zwei vermischte Systeme (Command Center und Alex 2.0), unbemerkte Fehler (Titel mit „=“, eine Vorlage, "
               "die 370 leere Seiten erzeugt hat) und ein manueller statt automatischer Start."))
    s.append(P("<b>Die Lösung im Neuaufbau:</b>"))
    s += B(["Ein fester <b>Auftragsweg</b>: Anfrage → Jobs → Ergebnis → Review → Freigabe.",
            "Ein <b>einziger Worker-Workflow</b> für alle Agenten. Ein Agent ist nur noch ein Datensatz mit Prompt – "
            "ein neuer Agent braucht keinen neuen Workflow.",
            "<b>Klein starten</b>: 6 aktive Agenten, die wirklich arbeiten. 5 weitere stehen bereit.",
            "<b>Fehler sind sichtbar</b>: Status „Fehler“, Protokoll und Hinweis im Morgen-Briefing.",
            "Alles Alte ist gelöscht (30 Tage im Notion-Papierkorb). Ideen, Namen und Erkenntnisse sind gesichert."])

    s.append(P("2 · Was in Notion steht", "h1"))
    s.append(table([
        ["Bereich", "Inhalt", "Wofür"],
        ["<b>Cockpit</b> (Startseite)", "Formular „Neue Anfrage an ALEX“, Live-Ansichten: Wartet auf dich · Heute für dich · "
         "Deine Anfragen · Team bei der Arbeit · Content der Woche · Follow-ups · Briefings", "Dein täglicher Einstieg"],
        ["<b>Agentur</b>", "Anfragen an ALEX · Jobs · Agenten-Team · Agenten-Protokoll · Briefings", "Das Büro von ALEX und dem Team"],
        ["<b>Studio</b>", "Projekte · Meine Aufgaben · Kunden &amp; Kontakte · Content-Kalender · Ideen · Finanzen", "Dein Business"],
        ["<b>Wissen &amp; Standards</b>", "Brand Guidelines · Review-Kriterien · Mein Tag mit ALEX · Wochenrückblick · "
         "SOPs (Video-Produktion, Onboarding, Repurposing, Angebot &amp; Rechnung) · Brief-Vorlage · Prompt-Bibliothek · Wissensbasis",
         "Gedächtnis und Qualitätsmaßstab"],
        ["<b>System &amp; n8n</b>", "Einrichtungsanleitung, Job-Protokoll, IDs, die 4 Workflow-Dateien zum Kopieren", "Bauplan"],
    ], [33, 95, 42]))
    s.append(Spacer(1, 4))
    s.append(P("Die Datenbanken sind miteinander verknüpft (Projekt ⇄ Kunde ⇄ Aufgaben ⇄ Jobs ⇄ Content ⇄ Finanzen). "
               "Automatisch berechnet werden u. a. Projekt- und Anfrage-Fortschritt, Tage bis zur Deadline, Überfällig-Warnungen, "
               "Follow-up-Erinnerungen und Umsatz pro Kunde. Alle Datenbanken sind leer und warten auf deine echten Daten.", "small"))

    s.append(P("3 · Das Agenten-Team", "h1"))
    s.append(table([
        ["Agent", "Rolle", "Liefert", "Status"],
        ["<b>ALEX</b>", "Persönlicher Assistent &amp; Agenturleiter", "Plant Anfragen, verteilt Jobs, Morgen-Briefing", "aktiv"],
        ["<b>CreativeDirector</b>", "Qualitäts-Gatekeeper", "Review nach 10 Kriterien, Freigabe ab 7/10", "aktiv"],
        ["<b>ContentWriter</b>", "Texte &amp; Copy", "Posts, Captions, Skripte, Website, Newsletter", "aktiv"],
        ["<b>VideoProducer</b> (neu)", "Videoproduktion", "Konzepte, Shotlists, Drehpläne, Packlisten", "aktiv"],
        ["<b>ResearchAnalyst</b>", "Recherche (mit Websuche)", "Trends, Wettbewerb, Tools – mit Quellen", "aktiv"],
        ["<b>EmailManager</b>", "Kommunikation", "E-Mail-Entwürfe (sendet nie selbst)", "aktiv"],
        ["BrandStrategist · DesignConsultant · WebDeveloper · LaunchCoordinator · SystemAdmin", "Weitere Spezialisten",
         "Prompts fertig – bei Bedarf auf „Aktiv“ stellen", "im Aufbau"],
    ], [44, 40, 66, 20]))
    s.append(Spacer(1, 3))
    s.append(P("TaskManager und TaskCreator sind keine eigenen Agenten mehr – ALEX übernimmt Planen und Überwachen.", "small"))

    s.append(P("4 · So läuft ein Auftrag", "h1"))
    flow = ("Du ──Anfrage──► ALEX plant ──Jobs──► Agent arbeitet ──► CreativeDirector prüft\n"
            "                                                              │\n"
            "           ► deine Freigabe ◄────────────────────────────────┘")
    s.append(box(Paragraph(flow.replace("\n", "<br/>").replace(" ", "&nbsp;"), S["mono"]), bg=colors.white))
    s.append(Spacer(1, 6))
    s.append(table([
        ["Job-Status", "Bedeutung", "Wer handelt"],
        ["Entwurf", "Noch nicht startklar (z. B. wartet auf einen anderen Job)", "–"],
        ["Bereit", "Agent startet beim nächsten Lauf (alle 5 Min.)", "Worker"],
        ["In Bearbeitung", "Läuft gerade", "Worker"],
        ["Review", "Kreatives Ergebnis wartet auf den CreativeDirector", "Review"],
        ["<b>Freigabe</b>", "<b>Wartet auf dich</b> – Ergebnis steht im Job", "<b>Du</b>"],
        ["Überarbeiten", "Zurück an den Agenten mit Feedback (max. 3 Versuche)", "Worker"],
        ["Erledigt", "Von dir freigegeben", "–"],
        ["Fehler", "Lauf fehlgeschlagen – Grund im Feld „Feedback“ und im Protokoll", "Du (auf „Bereit“ = neuer Versuch)"],
    ], [30, 95, 45]))
    s.append(Spacer(1, 4))
    s.append(P("Rückfragen: Ist eine Anfrage unklar, setzt ALEX sie auf „Rückfrage“ und schreibt seine Frage dazu. "
               "Du trägst die Antwort ins Feld „Antwort an ALEX“ ein und setzt den Status wieder auf „Neu“.", "small"))

    s.append(P("5 · Die vier n8n-Workflows", "h1"))
    s.append(table([
        ["Workflow", "Wann", "Was passiert"],
        ["1 · Dispatcher", "alle 5 Min.", "ALEX plant neue Anfragen als 1–5 Jobs inkl. Reihenfolge – oder stellt eine Rückfrage"],
        ["2 · Worker", "alle 5 Min.", "Der nächste Job wird vom zuständigen Agenten erledigt; Ergebnisse von Vorgänger-Jobs und Feedback fließen ein"],
        ["3 · Review", "alle 5 Min.", "CreativeDirector bewertet kreative Ergebnisse; unter 7/10 → Überarbeiten"],
        ["4 · Morgen-Briefing", "Mo–Fr 07:00", "Aufgaben, Freigaben, Fehler, Deadlines, Content, Follow-ups → Briefing-Seite"],
    ], [33, 25, 112]))
    s.append(Spacer(1, 4))
    s.append(P("Leere Läufe kosten nichts: Claude wird nur aufgerufen, wenn wirklich Arbeit da ist.", "small"))

    s.append(P("6 · Was getestet ist – und was noch nicht", "h1"))
    s.append(box(P("<b>Getestet:</b> Die komplette Kette lief in einer echten n8n-Installation (Version 1.123) gegen dein Notion: "
                   "Anfrage → Recherche → Text mit Recherche-Ergebnis → Review 6/10 (Überarbeiten) → Überarbeitung mit Feedback "
                   "→ Review 8/10 (Freigabe) → Morgen-Briefing. Auch der Fehlerfall greift (Job auf „Fehler“, Grund im Protokoll). "
                   "Testdaten wurden danach entfernt.<br/><br/>"
                   "<b>Noch nicht getestet:</b> Echte Claude-Antworten. Ohne deinen API-Schlüssel kam „Claude“ aus einem Nachbau, "
                   "der jede Anfrage auf gültiges Format geprüft hat. Wie gut die Agenten inhaltlich arbeiten, zeigt erst dein erster echter Lauf.")))

    s.append(P("7 · Voreinstellungen und Kosten", "h1"))
    s += B(["<b>Modell:</b> alle Agenten laufen auf Claude Opus 5.5 (höchste Qualität). Pro Agent im Feld „Modell“ umstellbar.",
            "<b>Ausweichlösung bei Ablehnung</b> ist eingeschaltet: Lehnt Claude eine Anfrage ab, übernimmt automatisch ein anderes Modell.",
            "<b>Kosten:</b> Jeder Lauf schreibt die verbrauchten Tokens ins Agenten-Protokoll – so siehst du genau, was ein Auftrag kostet."])
    s.append(table([
        ["Modell", "Input / 1 Mio. Tokens", "Output / 1 Mio. Tokens", "Einsatz"],
        ["claude-opus-5-5", "$4.00", "$20.00", "Standard, höchste Qualität"],
        ["claude-sonnet-5-5", "$2.00", "$10.00", "Halber Preis, sehr gut für Alltagsjobs"],
        ["claude-haiku-5-5", "$0.10", "$0.50", "Sehr günstig, für einfache Aufgaben"],
    ], [38, 38, 38, 56]))
    s.append(Spacer(1, 3))
    s.append(P("Listenpreise der Anthropic-API, Stand November 2026. Tipp: In der Anthropic-Console ein monatliches Ausgabenlimit setzen.", "small"))

    s.append(P("8 · Wichtige Hinweise", "h1"))
    s += B(["<b>API-Schlüssel:</b> Die alte Seite „API Keys“ wurde mit gelöscht, aber nie geöffnet. Falls dort echte Schlüssel standen, "
            "sind sie weiterhin gültig – bitte beim jeweiligen Anbieter erneuern.",
            "<b>Papierkorb:</b> Die alten Inhalte sind 30 Tage in Notion wiederherstellbar.",
            "<b>Nicht umbenennen:</b> Status- und Feldnamen in Anfragen, Jobs und Agenten-Team werden von n8n gelesen. "
            "Neue Felder hinzufügen ist kein Problem.",
            "<b>Schlüssel gehören nach n8n</b> (Credentials), nie in Notion-Seiten."])

    s.append(P("9 · Wo du was findest", "h1"))
    s.append(table([
        ["Was", "Wo"],
        ["Startseite / Cockpit", f'<link href="{ROOT_URL}" color="#6B4FBB">{ROOT_URL}</link>'],
        ["Formular „Neue Anfrage an ALEX“", f'<link href="{FORM_URL}" color="#6B4FBB">Formular öffnen</link> (auch vom Handy)'],
        ["Einrichtungsanleitung n8n", "Notion → System &amp; n8n"],
        ["Workflow-Dateien", "Notion → System &amp; n8n (zum Kopieren) · Repository: notion-studio/n8n/workflows/"],
        ["Konzept · Erkenntnisse · Prompts", "Repository: notion-studio/KONZEPT.md · docs/ERKENNTNISSE.md · agents/"],
    ], [52, 118], header=True))
    s.append(Spacer(1, 10))
    s.append(P("Nächster Schritt: die To-do-Liste (separates PDF) – Einrichtung in ca. 60 Minuten, dann arbeitet dein Team.", "sub"))
    doc(path, "Briefing – ALEX · Media Creative Studio").build(s, onFirstPage=footer("Briefing"), onLaterPages=footer("Briefing"))
    return path


# =========================================================== TODO
def todo():
    path = os.path.join(HERE, "ToDo_Achim.pdf")
    s = [P("Deine To-dos: ALEX startklar machen", "title"),
         P("Reihenfolge einhalten · Gesamtzeit ca. 60–75 Minuten · danach arbeitet dein Team · Stand 15. November 2026", "sub")]

    def phase(nr, title, time, items, note=None):
        rows = []
        for it in items:
            rows.append([P("☐", "cellb"), P(it, "cell")])
        t = Table(rows, colWidths=[8 * mm, 162 * mm])
        t.setStyle(TableStyle([("VALIGN", (0, 0), (-1, -1), "TOP"), ("LINEBELOW", (0, 0), (-1, -1), 0.3, LINE),
                               ("TOPPADDING", (0, 0), (-1, -1), 4), ("BOTTOMPADDING", (0, 0), (-1, -1), 4),
                               ("LEFTPADDING", (0, 0), (-1, -1), 2)]))
        head = Table([[P(f"<b>Schritt {nr} · {title}</b>", "cellb"), P(time, "small")]], colWidths=[140 * mm, 30 * mm])
        head.setStyle(TableStyle([("BACKGROUND", (0, 0), (-1, -1), SOFT), ("ALIGN", (1, 0), (1, 0), "RIGHT"),
                                  ("VALIGN", (0, 0), (-1, -1), "MIDDLE"), ("TOPPADDING", (0, 0), (-1, -1), 5),
                                  ("BOTTOMPADDING", (0, 0), (-1, -1), 5)]))
        block = [head, t]
        if note:
            block += [Spacer(1, 3), P(note, "small")]
        block.append(Spacer(1, 10))
        return KeepTogether(block)

    s.append(phase(1, "Sicherheit", "5 Min.", [
        "Prüfen, ob auf der gelöschten Seite „API Keys“ echte Schlüssel standen → falls ja: beim jeweiligen Anbieter "
        "<b>neu erzeugen und die alten löschen</b>",
        "Merken: Alte Notion-Inhalte liegen 30 Tage im Papierkorb – danach automatisch weg, nichts zu tun",
    ]))
    s.append(phase(2, "Notion kennenlernen", "15 Min.", [
        f'Startseite öffnen und als <b>Favorit</b> setzen (Stern oben rechts): <link href="{ROOT_URL}" color="#6B4FBB">ALEX · Media Creative Studio</link>',
        "Agentur → <b>Agenten-Team</b> ansehen (Ansicht „Team“): Wer ist aktiv, was kann wer?",
        "Wissen &amp; Standards → <b>Brand Guidelines</b>: alle [Platzhalter] ausfüllen (Leistungen, Zielkunden, Farben, "
        "Schriften, Anrede) – <b>wichtigster Hebel für gute Ergebnisse</b>",
        "Wissen &amp; Standards → „Mein Tag mit ALEX“ einmal lesen",
    ]))
    s.append(phase(3, "Zugänge besorgen", "10 Min.", [
        "<b>Notion-Integration:</b> notion.so/profile/integrations → „Neue Integration“ (intern), Name z. B. „ALEX Agentur“ → Secret kopieren (ntn_…)",
        "In Notion die Seite <b>ALEX · Media Creative Studio</b> öffnen → „…“ → <b>Verbindungen</b> → Integration hinzufügen",
        "<b>Anthropic:</b> console.anthropic.com → API-Key erstellen, Guthaben aufladen, <b>monatliches Ausgabenlimit</b> setzen",
    ], "Schlüssel nirgends in Notion speichern – nur in n8n."))
    s.append(phase(4, "n8n einrichten", "15 Min.", [
        "n8n → Credentials → Neu → <b>Header Auth</b> · Name exakt: <font name='DVM'>Notion · ALEX Agentur</font> · "
        "Header: <font name='DVM'>Authorization</font> · Wert: <font name='DVM'>Bearer ntn_…</font>",
        "n8n → Credentials → Neu → <b>Header Auth</b> · Name exakt: <font name='DVM'>Anthropic · ALEX Agentur</font> · "
        "Header: <font name='DVM'>x-api-key</font> · Wert: dein Anthropic-Key",
        "Die 4 Workflows importieren (Notion → System &amp; n8n → Datei aufklappen → kopieren → in leeren n8n-Workflow einfügen "
        "mit Strg+V → speichern): 1 Dispatcher · 2 Worker · 3 Review · 4 Morgen-Briefing",
        "Stichprobe: in jedem Workflow einen HTTP-Node öffnen – die Credential muss automatisch ausgewählt sein",
    ], "Die Namen müssen exakt stimmen (inkl. Mittelpunkt „·“) – dann verbindet n8n die Zugänge beim Import selbst."))
    s.append(phase(5, "Erster echter Testlauf", "15 Min.", [
        f'<link href="{FORM_URL}" color="#6B4FBB">Formular „Neue Anfrage an ALEX“</link> öffnen und einen kleinen echten Auftrag geben '
        "(z. B. „3 Ideen für Instagram-Reels über meine Arbeit als Videograf“)",
        "n8n → <b>Dispatcher</b> öffnen → „Test workflow“ → in Notion prüfen: Anfrage „In Arbeit“, Jobs angelegt",
        "n8n → <b>Worker</b> manuell starten (einmal pro Job) → Ergebnis erscheint im Job",
        "n8n → <b>Review</b> manuell starten → Score und Feedback erscheinen beim kreativen Job",
        "Ergebnis lesen → passt: Status <b>Erledigt</b> · passt nicht: Feedback schreiben, Status <b>Überarbeiten</b>",
        "n8n → <b>Morgen-Briefing</b> manuell starten → neues Briefing im Cockpit",
        "Agenten-Protokoll prüfen: alle Läufe „Erfolg“? Tokens plausibel?",
    ]))
    s.append(phase(6, "Live schalten", "2 Min.", [
        "In allen 4 Workflows oben rechts <b>Active</b> einschalten",
        "Morgen früh nach 07:00: Briefing im Cockpit lesen",
    ]))
    s.append(phase(7, "Erste Woche", "laufend", [
        "Täglich (10 Min.): Briefing lesen → „Wartet auf dich“ abarbeiten → Rückfragen beantworten → max. 3 Aufgaben „Heute“",
        "Echte Kunden, Projekte und Content-Planung eintragen – je mehr Kontext, desto besser plant ALEX",
        "Ergebnisse eines Agenten nicht gut? System Prompt im Agenten-Team anpassen (wirkt beim nächsten Lauf)",
        "Kosten nach einer Woche prüfen → ggf. einfache Agenten (z. B. EmailManager) auf claude-sonnet-5-5 umstellen",
        "Freitag: Wochenrückblick (Wissen &amp; Standards)",
        "Weitere Agenten bei Bedarf auf „Aktiv“ stellen (z. B. BrandStrategist, DesignConsultant)",
    ]))
    s.append(phase(8, "Optional – Claude hilft weiter", "", [
        "n8n-Adresse + n8n-API-Key an Claude geben → Claude richtet Credentials und Workflows direkt ein (Schritt 4 entfällt)",
        "Anthropic-Key über das sichere Formular hinterlegen → Claude macht einen echten Qualitätstest der Agenten",
    ]))
    s.append(Spacer(1, 4))
    s.append(box(P("<b>Wenn etwas hakt:</b> Notion → System &amp; n8n → „Fehlerbehebung“. Häufigste Ursachen: Integration nicht mit "
                   "der Seite verbunden (401 von Notion), falscher Header-Name oder Key (401 von Anthropic), kein Guthaben. "
                   "Ein Job mit „Fehler“ startet neu, wenn du ihn auf „Bereit“ setzt."), bg=WARN, border=colors.HexColor("#F2C48D")))
    s.append(Spacer(1, 10))
    s.append(P("Notizen: ______________________________________________________________________________", "p"))
    s.append(P("______________________________________________________________________________________", "p"))
    doc(path, "To-do – ALEX startklar machen").build(s, onFirstPage=footer("To-do"), onLaterPages=footer("To-do"))
    return path


if __name__ == "__main__":
    for f in (briefing(), todo()):
        print("✓", f)

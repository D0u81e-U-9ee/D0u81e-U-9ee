#!/usr/bin/env python3
"""Baut die Notion-Arbeitsumgebung „Media Creative Studio“ über die Notion-API.

Nutzung:
    NOTION_TOKEN=secret_xxx python3 build_workspace.py [--parent <page_id>] [--force]

Ohne --parent wird die erste Seite verwendet, die mit der Integration geteilt ist.
"""
import argparse
import datetime as dt
import json
import os
import sys
import time

import requests

API = "https://api.notion.com/v1"
VERSION = "2022-06-28"
TOKEN = os.environ.get("NOTION_TOKEN", "secret_dummy_injected_by_proxy")
HQ_TITLE = "Media Creative Studio HQ"
TODAY = dt.date.today()

S = requests.Session()
S.headers.update({
    "Authorization": f"Bearer {TOKEN}",
    "Notion-Version": VERSION,
    "Content-Type": "application/json",
})


def call(method, path, body=None):
    for attempt in range(6):
        r = S.request(method, API + path, data=json.dumps(body) if body is not None else None)
        if r.status_code == 429 or r.status_code >= 500:
            time.sleep(float(r.headers.get("Retry-After", 1 + attempt * 2)))
            continue
        if r.status_code >= 400:
            raise RuntimeError(f"{method} {path} -> {r.status_code}: {r.text[:800]}")
        time.sleep(0.34)  # ~3 req/s
        return r.json()
    raise RuntimeError(f"{method} {path}: zu viele Wiederholungen")


def d(days):
    return (TODAY + dt.timedelta(days=days)).isoformat()


# ---------------------------------------------------------------- Blöcke
def rt(text, bold=False, italic=False, code=False, color="default", link=None):
    t = {"content": text}
    if link:
        t["link"] = {"url": link}
    return {"type": "text", "text": t,
            "annotations": {"bold": bold, "italic": italic, "code": code, "color": color}}


def _txt(x):
    return x if isinstance(x, list) else [rt(x)]


def h1(x): return {"type": "heading_1", "heading_1": {"rich_text": _txt(x)}}
def h2(x): return {"type": "heading_2", "heading_2": {"rich_text": _txt(x)}}
def h3(x): return {"type": "heading_3", "heading_3": {"rich_text": _txt(x)}}
def p(x=""): return {"type": "paragraph", "paragraph": {"rich_text": _txt(x) if x else []}}
def bullet(x, children=None):
    b = {"type": "bulleted_list_item", "bulleted_list_item": {"rich_text": _txt(x)}}
    if children: b["bulleted_list_item"]["children"] = children
    return b
def num(x): return {"type": "numbered_list_item", "numbered_list_item": {"rich_text": _txt(x)}}
def todo(x, checked=False): return {"type": "to_do", "to_do": {"rich_text": _txt(x), "checked": checked}}
def quote(x): return {"type": "quote", "quote": {"rich_text": _txt(x)}}
def divider(): return {"type": "divider", "divider": {}}
def toc(): return {"type": "table_of_contents", "table_of_contents": {}}
def code(x, lang="plain text"): return {"type": "code", "code": {"rich_text": [rt(x)], "language": lang}}
def callout(x, emoji="💡", color="gray_background", children=None):
    b = {"type": "callout", "callout": {"rich_text": _txt(x), "icon": {"type": "emoji", "emoji": emoji}, "color": color}}
    if children: b["callout"]["children"] = children
    return b
def toggle(x, children):
    return {"type": "toggle", "toggle": {"rich_text": _txt(x), "children": children}}
def link_page(pid): return {"type": "link_to_page", "link_to_page": {"type": "page_id", "page_id": pid}}
def link_db(did): return {"type": "link_to_page", "link_to_page": {"type": "database_id", "database_id": did}}
def columns(*cols):
    return {"type": "column_list", "column_list": {"children": [
        {"type": "column", "column": {"children": list(c)}} for c in cols]}}


def append(block_id, blocks):
    for i in range(0, len(blocks), 90):
        call("PATCH", f"/blocks/{block_id}/children", {"children": blocks[i:i + 90]})


def create_page(parent_id, title, emoji, blocks=(), cover=None):
    body = {"parent": {"page_id": parent_id},
            "icon": {"type": "emoji", "emoji": emoji},
            "properties": {"title": {"title": [rt(title)]}}}
    if cover:
        body["cover"] = {"type": "external", "external": {"url": cover}}
    page = call("POST", "/pages", body)
    if blocks:
        append(page["id"], list(blocks))
    return page["id"]


# ---------------------------------------------------------------- Schema-Helfer
def opts(*names):
    colors = ["blue", "green", "yellow", "orange", "red", "purple", "pink", "brown", "gray", "default"]
    out = []
    for i, n in enumerate(names):
        if isinstance(n, tuple):
            out.append({"name": n[0], "color": n[1]})
        else:
            out.append({"name": n, "color": colors[i % len(colors)]})
    return out


def sel(*n): return {"select": {"options": opts(*n)}}
def msel(*n): return {"multi_select": {"options": opts(*n)}}
def title(): return {"title": {}}
def text(): return {"rich_text": {}}
def date(): return {"date": {}}
def number(fmt="number"): return {"number": {"format": fmt}}
def url(): return {"url": {}}
def email(): return {"email": {}}
def phone(): return {"phone_number": {}}
def check(): return {"checkbox": {}}
def created(): return {"created_time": {}}
def edited(): return {"last_edited_time": {}}
def formula(expr): return {"formula": {"expression": expr}}


# ---------------------------------------------------------------- Datenbanken
STATUS_DONE = "Erledigt"

DBS = [
    {
        "key": "kunden", "name": "Kunden & Kontakte", "emoji": "👥",
        "desc": "Mini-CRM: Kunden, Leads, Partner & Freelancer – mit Follow-up-Erinnerung.",
        "props": {
            "Name": title(),
            "Firma": text(),
            "Typ": sel(("Kunde", "green"), ("Lead", "yellow"), ("Partner", "blue"), ("Freelancer", "purple")),
            "Status": sel(("Aktiv", "green"), ("Warm", "orange"), ("Kalt", "gray"), ("Archiv", "default")),
            "E-Mail": email(), "Telefon": phone(), "Website": url(),
            "Letzter Kontakt": date(), "Nächster Follow-up": date(),
            "Notizen": text(),
        },
        "formulas": {
            "⏰ Follow-up": 'if(!empty(prop("Nächster Follow-up")) and prop("Nächster Follow-up") <= now(), "⏰ Jetzt melden", "")',
        },
        "rows": [
            {"Name": "Lena Hoffmann", "Firma": "Café Aroma GmbH", "Typ": "Kunde", "Status": "Aktiv",
             "E-Mail": "lena@cafe-aroma.example", "Letzter Kontakt": d(-6), "Nächster Follow-up": d(1),
             "Notizen": "Möchte zusätzlich Social-Clips zum Imagefilm."},
            {"Name": "Jonas Weber", "Firma": "Weber Sports", "Typ": "Lead", "Status": "Warm",
             "E-Mail": "jonas@webersports.example", "Letzter Kontakt": d(-14), "Nächster Follow-up": d(-1),
             "Notizen": "Interesse an Produktvideos für Webshop."},
            {"Name": "Mira Kaya", "Firma": "Freelance Sound Design", "Typ": "Freelancer", "Status": "Aktiv",
             "E-Mail": "mira@sound.example", "Notizen": "Sounddesign & Mischung, Tagessatz 450 €."},
        ],
    },
    {
        "key": "ziele", "name": "Ziele", "emoji": "🎯",
        "desc": "Quartals- und Jahresziele. Projekte zahlen auf Ziele ein.",
        "props": {
            "Ziel": title(),
            "Zeitraum": sel("Q1 2027", "Q2 2027", "Q3 2027", "Q4 2026", "Jahr 2027"),
            "Status": sel(("Aktiv", "blue"), ("Erreicht", "green"), ("Pausiert", "gray")),
            "Bereich": sel("Business", "Content", "Skills", "Persönlich"),
            "Messgröße": text(),
        },
        "rows": [
            {"Ziel": "3 neue Stammkunden gewinnen", "Zeitraum": "Q4 2026", "Status": "Aktiv", "Bereich": "Business",
             "Messgröße": "3 unterschriebene Rahmenverträge"},
            {"Ziel": "YouTube-Kanal auf 1.000 Abos", "Zeitraum": "Q1 2027", "Status": "Aktiv", "Bereich": "Content",
             "Messgröße": "1.000 Abonnenten, 1 Video/Woche"},
        ],
    },
    {
        "key": "projekte", "name": "Projekte", "emoji": "🎞",
        "desc": "Alle Produktionen von der Idee bis zur Auslieferung. Fortschritt berechnet sich aus erledigten Aufgaben.",
        "props": {
            "Projekt": title(),
            "Status": sel(("Idee", "gray"), ("Planung", "yellow"), ("Produktion", "orange"), ("Postproduktion", "purple"),
                          ("Review", "pink"), ("Abgeschlossen", "green"), ("Pausiert", "brown")),
            "Typ": sel("Video", "Foto", "Podcast", "Design", "Social Media", "Musik", "Website", "Animation"),
            "Priorität": sel(("🔥 Hoch", "red"), ("Mittel", "yellow"), ("Niedrig", "gray")),
            "Start": date(), "Deadline": date(),
            "Budget": number("euro"),
            "Ordner / Drive": url(),
            "Briefing": text(),
            "Kunde": {"rel": "kunden", "back": "Projekte"},
            "Ziel": {"rel": "ziele", "back": "Projekte"},
        },
        "formulas": {
            "⏳ Tage bis Deadline": 'if(empty(prop("Deadline")), "", if(prop("Status") == "Abgeschlossen", "✅", format(dateBetween(prop("Deadline"), now(), "days")) + " Tage"))',
        },
        "rows": [
            {"Projekt": "Imagefilm Café Aroma", "Status": "Produktion", "Typ": "Video", "Priorität": "🔥 Hoch",
             "Start": d(-10), "Deadline": d(12), "Budget": 4800, "Kunde": ["Lena Hoffmann"],
             "Ziel": ["3 neue Stammkunden gewinnen"],
             "Briefing": "90-Sek.-Imagefilm + 3 Social-Cutdowns (9:16). Look: warm, natürlich, dokumentarisch."},
            {"Projekt": "Produktvideos Weber Sports", "Status": "Planung", "Typ": "Video", "Priorität": "Mittel",
             "Start": d(5), "Deadline": d(35), "Budget": 2500, "Kunde": ["Jonas Weber"],
             "Briefing": "5 Produktclips à 15 Sek. für den Webshop, weißer Hintergrund + Lifestyle."},
            {"Projekt": "YouTube-Serie „Behind the Lens“", "Status": "Idee", "Typ": "Social Media", "Priorität": "Mittel",
             "Start": d(2), "Deadline": d(60), "Ziel": ["YouTube-Kanal auf 1.000 Abos"],
             "Briefing": "Wöchentliche Folgen über Workflows, Gear und Making-ofs."},
        ],
    },
    {
        "key": "aufgaben", "name": "Aufgaben", "emoji": "✅",
        "desc": "Alle To-dos. Setze ‚Heute‘ für deine Top-3 des Tages.",
        "props": {
            "Aufgabe": title(),
            "Status": sel(("Offen", "gray"), ("In Arbeit", "blue"), ("Warten auf", "yellow"), (STATUS_DONE, "green")),
            "Priorität": sel(("🔥 Hoch", "red"), ("Mittel", "yellow"), ("Niedrig", "gray")),
            "Fällig": date(),
            "Heute": check(),
            "Bereich": sel("Produktion", "Kunde", "Content", "Admin", "Finanzen", "Lernen", "Privat"),
            "Aufwand": sel(("⚡ 15 Min", "green"), ("🕐 1 Std", "yellow"), ("🕓 Halber Tag", "orange"), ("📆 Ganzer Tag", "red")),
            "Notiz": text(),
            "Projekt": {"rel": "projekte", "back": "Aufgaben"},
        },
        "formulas": {
            "Erledigt-Wert": f'if(prop("Status") == "{STATUS_DONE}", 1, 0)',
            "⚠️ Warnung": f'if(prop("Status") != "{STATUS_DONE}" and !empty(prop("Fällig")) and prop("Fällig") < today(), "⚠️ Überfällig", "")',
        },
        "rows": [
            {"Aufgabe": "Drehplan & Shotlist finalisieren", "Status": STATUS_DONE, "Priorität": "🔥 Hoch", "Fällig": d(-3),
             "Bereich": "Produktion", "Aufwand": "🕐 1 Std", "Projekt": ["Imagefilm Café Aroma"]},
            {"Aufgabe": "Drehtag 2: Barista-Szenen", "Status": "Offen", "Priorität": "🔥 Hoch", "Fällig": d(2),
             "Bereich": "Produktion", "Aufwand": "📆 Ganzer Tag", "Projekt": ["Imagefilm Café Aroma"]},
            {"Aufgabe": "Musik lizenzieren (Artlist/Epidemic)", "Status": "Offen", "Priorität": "Mittel", "Fällig": d(0),
             "Heute": True, "Bereich": "Produktion", "Aufwand": "⚡ 15 Min", "Projekt": ["Imagefilm Café Aroma"]},
            {"Aufgabe": "Rohschnitt Version 1", "Status": "Offen", "Priorität": "🔥 Hoch", "Fällig": d(6),
             "Bereich": "Produktion", "Aufwand": "📆 Ganzer Tag", "Projekt": ["Imagefilm Café Aroma"]},
            {"Aufgabe": "Angebot an Weber Sports schicken", "Status": "In Arbeit", "Priorität": "🔥 Hoch", "Fällig": d(-1),
             "Heute": True, "Bereich": "Kunde", "Aufwand": "🕐 1 Std", "Projekt": ["Produktvideos Weber Sports"]},
            {"Aufgabe": "Konzept & Folgenliste „Behind the Lens“", "Status": "Offen", "Priorität": "Mittel", "Fällig": d(4),
             "Bereich": "Content", "Aufwand": "🕓 Halber Tag", "Projekt": ["YouTube-Serie „Behind the Lens“"]},
            {"Aufgabe": "Belege des Monats hochladen", "Status": "Offen", "Priorität": "Niedrig", "Fällig": d(5),
             "Heute": True, "Bereich": "Finanzen", "Aufwand": "⚡ 15 Min"},
        ],
    },
    {
        "key": "content", "name": "Content-Kalender", "emoji": "📅",
        "desc": "Planung und Veröffentlichung für alle Kanäle. Tipp: als Kalender- oder Board-Ansicht nutzen.",
        "props": {
            "Titel": title(),
            "Status": sel(("Idee", "gray"), ("Skript", "yellow"), ("Dreh", "orange"), ("Schnitt", "purple"),
                          ("Geplant", "blue"), ("Veröffentlicht", "green")),
            "Plattform": msel(("YouTube", "red"), ("Instagram", "pink"), ("TikTok", "default"), ("LinkedIn", "blue"),
                              ("Blog", "brown"), ("Podcast", "purple")),
            "Format": sel("Reel / Short", "Long-Form", "Post", "Karussell", "Story", "Artikel", "Episode"),
            "Veröffentlichung": date(),
            "Hook": text(),
            "Call-to-Action": text(),
            "Link": url(),
            "Views": number(),
            "Projekt": {"rel": "projekte", "back": "Content"},
        },
        "rows": [
            {"Titel": "Making-of Café Aroma (Reel)", "Status": "Idee", "Plattform": ["Instagram", "TikTok"],
             "Format": "Reel / Short", "Veröffentlichung": d(15), "Hook": "So entsteht ein Imagefilm in 2 Tagen ☕🎥",
             "Projekt": ["Imagefilm Café Aroma"]},
            {"Titel": "Behind the Lens #1 – Mein Kamera-Setup", "Status": "Skript", "Plattform": ["YouTube"],
             "Format": "Long-Form", "Veröffentlichung": d(9), "Hook": "Dieses 1.500-€-Setup schlägt meine alte Cinema-Cam",
             "Call-to-Action": "Abonnieren für Folge 2", "Projekt": ["YouTube-Serie „Behind the Lens“"]},
            {"Titel": "5 Tipps für bessere Interviews", "Status": "Geplant", "Plattform": ["LinkedIn", "Instagram"],
             "Format": "Karussell", "Veröffentlichung": d(3), "Hook": "Gute Interviews entstehen vor der Aufnahme."},
        ],
    },
    {
        "key": "ideen", "name": "Ideen-Pool", "emoji": "💡",
        "desc": "Jede Idee ist willkommen. Bewerten, reifen lassen, in Projekte oder Content überführen.",
        "props": {
            "Idee": title(),
            "Kategorie": sel("Video", "Foto", "Content", "Business", "Kunde", "Experiment"),
            "Bewertung": sel(("⭐", "gray"), ("⭐⭐", "yellow"), ("⭐⭐⭐", "orange"), ("⭐⭐⭐⭐", "red"), ("⭐⭐⭐⭐⭐", "purple")),
            "Status": sel(("Neu", "blue"), ("Reift", "yellow"), ("Umgesetzt", "green"), ("Verworfen", "gray")),
            "Inspiration / Link": url(),
            "Beschreibung": text(),
            "Erstellt": created(),
        },
        "rows": [
            {"Idee": "Hyperlapse durch die Altstadt bei Nacht", "Kategorie": "Video", "Bewertung": "⭐⭐⭐⭐",
             "Status": "Reift", "Beschreibung": "Für Portfolio und Stock-Footage."},
            {"Idee": "Paket „Social Starter“ für lokale Läden", "Kategorie": "Business", "Bewertung": "⭐⭐⭐⭐⭐",
             "Status": "Neu", "Beschreibung": "1 Drehtag → 10 Kurzclips, Festpreis."},
        ],
    },
    {
        "key": "assets", "name": "Asset-Bibliothek", "emoji": "🗂",
        "desc": "Footage, Fotos, Musik, LUTs, Templates & Fonts – mit Speicherort und Lizenz.",
        "props": {
            "Asset": title(),
            "Typ": sel("Footage", "Foto", "Audio / SFX", "Musik", "Grafik", "Template", "LUT / Preset", "Font", "3D"),
            "Lizenz": sel(("Eigenes Material", "green"), ("Lizenzfrei", "blue"), ("Lizenziert (Abo)", "yellow"),
                          ("Creative Commons", "purple"), ("Nur für Kunde", "red")),
            "Speicherort": url(),
            "Tags": msel("Stadt", "Natur", "People", "Food", "Corporate", "Cinematic", "Upbeat", "Ruhig"),
            "Lizenz gültig bis": date(),
            "Projekt": {"rel": "projekte", "back": "Assets"},
        },
        "rows": [
            {"Asset": "Cinematic Warm LUT Pack", "Typ": "LUT / Preset", "Lizenz": "Lizenzfrei", "Tags": ["Cinematic"]},
            {"Asset": "B-Roll Café Aroma (Drehtag 1)", "Typ": "Footage", "Lizenz": "Nur für Kunde",
             "Tags": ["Food", "People"], "Projekt": ["Imagefilm Café Aroma"]},
        ],
    },
    {
        "key": "equipment", "name": "Equipment", "emoji": "🧰",
        "desc": "Inventar mit Status, Wert und Seriennummer – wichtig für Versicherung und Packlisten.",
        "props": {
            "Gerät": title(),
            "Kategorie": sel("Kamera", "Objektiv", "Licht", "Audio", "Grip / Stativ", "Drohne", "Computer", "Speicher", "Software"),
            "Status": sel(("Verfügbar", "green"), ("Im Einsatz", "blue"), ("Verliehen", "yellow"), ("Defekt", "red"), ("In Reparatur", "orange")),
            "Kaufpreis": number("euro"),
            "Kaufdatum": date(),
            "Seriennummer": text(),
            "Versichert": check(),
        },
        "rows": [
            {"Gerät": "Sony FX3", "Kategorie": "Kamera", "Status": "Verfügbar", "Kaufpreis": 4200, "Versichert": True},
            {"Gerät": "Sigma 24-70 f/2.8", "Kategorie": "Objektiv", "Status": "Verfügbar", "Kaufpreis": 1100, "Versichert": True},
            {"Gerät": "Aputure 300d II", "Kategorie": "Licht", "Status": "Im Einsatz", "Kaufpreis": 750},
            {"Gerät": "Rode Wireless PRO", "Kategorie": "Audio", "Status": "Verfügbar", "Kaufpreis": 380},
        ],
    },
    {
        "key": "finanzen", "name": "Finanzen", "emoji": "💶",
        "desc": "Einnahmen & Ausgaben, Rechnungsstatus. Tipp: Ansicht nach Monat gruppieren und Summe von ‚Betrag (±)‘ anzeigen.",
        "props": {
            "Posten": title(),
            "Typ": sel(("Einnahme", "green"), ("Ausgabe", "red")),
            "Betrag": number("euro"),
            "Datum": date(),
            "Kategorie": sel("Honorar", "Lizenz", "Equipment", "Software / Abo", "Reise", "Freelancer", "Marketing", "Sonstiges"),
            "Status": sel(("Offen", "yellow"), ("Bezahlt", "green"), ("Überfällig", "red"), ("Entwurf", "gray")),
            "Rechnungsnr.": text(),
            "Projekt": {"rel": "projekte", "back": "Finanzen"},
            "Kunde": {"rel": "kunden", "back": "Finanzen"},
        },
        "formulas": {
            "Betrag (±)": 'if(prop("Typ") == "Ausgabe", -prop("Betrag"), prop("Betrag"))',
        },
        "rows": [
            {"Posten": "Anzahlung Imagefilm (50 %)", "Typ": "Einnahme", "Betrag": 2400, "Datum": d(-10),
             "Kategorie": "Honorar", "Status": "Bezahlt", "Rechnungsnr.": "RE-2026-041",
             "Projekt": ["Imagefilm Café Aroma"], "Kunde": ["Lena Hoffmann"]},
            {"Posten": "Musiklizenz Artlist (Jahresabo)", "Typ": "Ausgabe", "Betrag": 199, "Datum": d(-4),
             "Kategorie": "Software / Abo", "Status": "Bezahlt"},
            {"Posten": "Sounddesign Mira Kaya", "Typ": "Ausgabe", "Betrag": 450, "Datum": d(10),
             "Kategorie": "Freelancer", "Status": "Offen", "Projekt": ["Imagefilm Café Aroma"]},
        ],
    },
    {
        "key": "meetings", "name": "Meetings & Journal", "emoji": "📓",
        "desc": "Kunden-Calls, Brainstormings, Weekly Reviews und Journal-Einträge.",
        "props": {
            "Titel": title(),
            "Typ": sel(("Kunden-Call", "blue"), ("Brainstorming", "yellow"), ("Weekly Review", "purple"),
                       ("Journal", "green"), ("Dreh-Briefing", "orange")),
            "Datum": date(),
            "Zusammenfassung": text(),
            "Nächste Schritte": text(),
            "Projekt": {"rel": "projekte", "back": "Meetings"},
            "Teilnehmer": {"rel": "kunden", "back": "Meetings"},
        },
        "rows": [
            {"Titel": "Kick-off Imagefilm", "Typ": "Kunden-Call", "Datum": d(-12),
             "Zusammenfassung": "Zielgruppe 25–40, Fokus auf Handwerk & Atmosphäre.",
             "Nächste Schritte": "Shotlist bis Freitag, Drehtermine abstimmen.",
             "Projekt": ["Imagefilm Café Aroma"], "Teilnehmer": ["Lena Hoffmann"]},
        ],
    },
    {
        "key": "wissen", "name": "Wissen & Vorlagen", "emoji": "📚",
        "desc": "Tutorials, Checklisten, Prompts, Presets – dein Studio-Gedächtnis.",
        "props": {
            "Titel": title(),
            "Kategorie": sel("Tutorial", "Checkliste", "Prompt", "Vorlage", "Referenz", "Gear-Tipp"),
            "Tool": msel("Premiere Pro", "DaVinci Resolve", "After Effects", "Lightroom", "Photoshop", "Final Cut", "Notion", "Claude"),
            "Link": url(),
            "Notiz": text(),
        },
        "rows": [
            {"Titel": "Color Grading Basics in Resolve", "Kategorie": "Tutorial", "Tool": ["DaVinci Resolve"],
             "Notiz": "Node-Struktur: Balance → Look → Vignette → Output."},
            {"Titel": "Export-Settings Social Media", "Kategorie": "Referenz", "Tool": ["Premiere Pro", "DaVinci Resolve"],
             "Notiz": "Reels/TikTok: 1080×1920, H.264, 20–25 Mbit/s. YouTube: 4K H.265, 45 Mbit/s."},
        ],
    },
    {
        "key": "inbox", "name": "Inbox", "emoji": "📥",
        "desc": "Alles schnell hier rein. Täglich in der Tagesplanung leeren und einsortieren.",
        "props": {
            "Eintrag": title(),
            "Art": sel(("Idee", "yellow"), ("Aufgabe", "blue"), ("Link", "purple"), ("Notiz", "gray"), ("Anfrage", "red")),
            "Verarbeitet": check(),
            "Link": url(),
            "Erfasst": created(),
        },
        "rows": [
            {"Eintrag": "Anfrage Hochzeitsvideo von Website – zurückrufen", "Art": "Anfrage"},
            {"Eintrag": "Neues DJI-Mic ansehen", "Art": "Link"},
            {"Eintrag": "Idee: Timelapse-Serie Jahreszeiten", "Art": "Idee"},
        ],
    },
]


def create_db(parent_id, spec, ids):
    props = {}
    rels = {}
    for name, schema in spec["props"].items():
        if "rel" in schema:
            rels[name] = schema
            continue
        props[name] = schema
    body = {
        "parent": {"type": "page_id", "page_id": parent_id},
        "icon": {"type": "emoji", "emoji": spec["emoji"]},
        "title": [rt(spec["name"])],
        "description": [rt(spec["desc"])],
        "is_inline": False,
        "properties": props,
    }
    db = call("POST", "/databases", body)
    ids[spec["key"]] = db["id"]
    print(f"  ✓ Datenbank {spec['emoji']} {spec['name']}")

    # Relationen (zweiseitig) anlegen und Gegen-Property umbenennen
    for name, r in rels.items():
        target = ids[r["rel"]]
        res = call("PATCH", f"/databases/{db['id']}", {"properties": {
            name: {"relation": {"database_id": target, "type": "dual_property", "dual_property": {}}}}})
        synced = res["properties"][name]["relation"]["dual_property"]["synced_property_name"]
        if synced != r["back"]:
            call("PATCH", f"/databases/{target}", {"properties": {synced: {"name": r["back"]}}})

    for name, expr in spec.get("formulas", {}).items():
        try:
            call("PATCH", f"/databases/{db['id']}", {"properties": {name: formula(expr)}})
        except RuntimeError as e:
            print(f"    ! Formel „{name}“ übersprungen: {e}")
    return db["id"]


def value(schema_type, v, rel_db_key, pages):
    if schema_type == "title":
        return {"title": [rt(v)]}
    if schema_type == "rich_text":
        return {"rich_text": [rt(v)]}
    if schema_type == "select":
        return {"select": {"name": v}}
    if schema_type == "multi_select":
        return {"multi_select": [{"name": x} for x in v]}
    if schema_type == "date":
        return {"date": {"start": v}}
    if schema_type == "number":
        return {"number": v}
    if schema_type == "url":
        return {"url": v}
    if schema_type == "email":
        return {"email": v}
    if schema_type == "phone_number":
        return {"phone_number": v}
    if schema_type == "checkbox":
        return {"checkbox": bool(v)}
    if schema_type == "relation":
        return {"relation": [{"id": pages[rel_db_key][x]} for x in v]}
    raise ValueError(schema_type)


def fill_rows(spec, ids, pages):
    pages[spec["key"]] = {}
    title_name = next(n for n, s in spec["props"].items() if "title" in s)
    for row in spec["rows"]:
        props = {}
        for k, v in row.items():
            schema = spec["props"][k]
            if "rel" in schema:
                props[k] = value("relation", v, schema["rel"], pages)
            else:
                props[k] = value(next(iter(schema)), v, None, pages)
        page = call("POST", "/pages", {"parent": {"database_id": ids[spec["key"]]}, "properties": props})
        pages[spec["key"]][row[title_name]] = page["id"]


def integrate_tasks(tasks_db, ids):
    """Bindet eine bestehende Aufgaben-DB ein – nur NEUE Felder, bestehende bleiben unverändert."""
    db = call("GET", f"/databases/{tasks_db}")
    props = db["properties"]
    due = "Deadline" if "Deadline" in props else ("Fällig" if "Fällig" in props else None)
    status_done = STATUS_DONE
    ids["aufgaben"] = tasks_db
    if "Studio-Projekt" not in props:
        res = call("PATCH", f"/databases/{tasks_db}", {"properties": {"Studio-Projekt": {"relation": {
            "database_id": ids["projekte"], "type": "dual_property", "dual_property": {}}}}})
        synced = res["properties"]["Studio-Projekt"]["relation"]["dual_property"]["synced_property_name"]
        if synced != "Aufgaben":
            call("PATCH", f"/databases/{ids['projekte']}", {"properties": {synced: {"name": "Aufgaben"}}})
    new = {}
    if "Heute" not in props:
        new["Heute"] = check()
    if new:
        call("PATCH", f"/databases/{tasks_db}", {"properties": new})
    f = {"Erledigt-Wert": f'if(prop("Status") == "{status_done}", 1, 0)'}
    if due:
        f["⚠️ Warnung"] = (f'if(prop("Status") != "{status_done}" and !empty(prop("{due}")) and '
                           f'prop("{due}") < today(), "⚠️ Überfällig", "")')
    for name, expr in f.items():
        if name in props:
            continue
        try:
            call("PATCH", f"/databases/{tasks_db}", {"properties": {name: formula(expr)}})
        except RuntimeError as e:
            print(f"    ! Formel „{name}“ übersprungen: {e}")
    print("  ✓ Bestehende Aufgaben-DB eingebunden (nur neue Felder ergänzt)")


def find_child_dbs(page_id, depth=0):
    found = []
    cur = None
    while True:
        r = call("GET", f"/blocks/{page_id}/children?page_size=100" + (f"&start_cursor={cur}" if cur else ""))
        for bl in r["results"]:
            if bl["type"] == "child_database":
                found.append(bl["id"])
            elif bl["has_children"] and bl["type"] not in ("child_page",) and depth < 3:
                found += find_child_dbs(bl["id"], depth + 1)
        if not r.get("has_more"):
            return found
        cur = r["next_cursor"]


def link_hq_from(page_id, hq):
    blocks = [callout([rt("Neu: ", bold=True), rt("Das Media Creative Studio HQ ist deine neue Startseite.")],
                      "🎬", "orange_background"), link_page(hq)]
    first = call("GET", f"/blocks/{page_id}/children?page_size=1")["results"]
    body = {"children": blocks}
    if first:
        body["after"] = first[0]["id"]
    call("PATCH", f"/blocks/{page_id}/children", body)


def add_rollups(ids):
    try:
        call("PATCH", f"/databases/{ids['projekte']}", {"properties": {
            "Fortschritt": {"rollup": {"relation_property_name": "Aufgaben",
                                       "rollup_property_name": "Erledigt-Wert", "function": "average"}}}})
        call("PATCH", f"/databases/{ids['projekte']}", {"properties": {
            "📊 Fortschritt": formula('if(empty(prop("Fortschritt")), "", format(round(prop("Fortschritt") * 100)) + " %")')}})
        print("  ✓ Projekt-Fortschritt (Rollup)")
    except RuntimeError as e:
        print(f"  ! Rollup übersprungen: {e}")
    try:
        call("PATCH", f"/databases/{ids['kunden']}", {"properties": {
            "Umsatz gesamt": {"rollup": {"relation_property_name": "Finanzen",
                                         "rollup_property_name": "Betrag (±)", "function": "sum"}}}})
        print("  ✓ Kunden-Umsatz (Rollup)")
    except RuntimeError as e:
        print(f"  ! Umsatz-Rollup übersprungen: {e}")


# ---------------------------------------------------------------- Seiteninhalte
def page_start(ids):
    return [
        callout([rt("Willkommen in deinem Studio-Betriebssystem. ", bold=True),
                 rt("Diese Seite erklärt, wie alles zusammenspielt. Lies sie einmal – danach brauchst du nur noch das HQ.")],
                "🧭", "blue_background"),
        h2("Das Prinzip: Erfassen → Planen → Machen → Reflektieren"),
        num([rt("Erfassen: ", bold=True), rt("Alles, was dir einfällt, sofort in die 📥 Inbox (auch per Handy-App oder Web-Clipper).")]),
        num([rt("Planen: ", bold=True), rt("Jeden Morgen ☀️ Tagesplanung – Inbox leeren, Top-3 Aufgaben mit ‚Heute‘ markieren.")]),
        num([rt("Machen: ", bold=True), rt("Im Projekt arbeiten. Aufgaben, Assets, Content und Meetings hängen am Projekt.")]),
        num([rt("Reflektieren: ", bold=True), rt("Jeden Freitag 🔁 Wochenrückblick – Projekte, Follow-ups, Finanzen, Content.")]),
        h2("Wie die Datenbanken verbunden sind"),
        code("Ziel ─┐\nKunde ─┼─► PROJEKT ◄── Aufgaben (→ Fortschritt %)\n       │     ├── Content-Kalender\n       │     ├── Assets\n       │     ├── Finanzen ──► Kunde (Umsatz gesamt)\n       │     └── Meetings ──► Teilnehmer (Kontakte)"),
        h2("Empfohlene Ansichten (einmalig 2 Minuten)"),
        p("Notion erlaubt das Anlegen von Ansichten nicht per API. Diese Ansichten lohnen sich – klicke in der Datenbank auf „+ Ansicht hinzufügen“:"),
        todo("✅ Aufgaben → Board nach Status · Filter „Heute“ ist angehakt · Kalender nach „Deadline“ (bzw. „Fällig“)"),
        todo("🎞 Projekte → Board nach Status · Timeline von Start bis Deadline"),
        todo("📅 Content-Kalender → Kalender nach „Veröffentlichung“ · Board nach Status"),
        todo("💶 Finanzen → Tabelle gruppiert nach Monat (Datum) mit Summe „Betrag (±)“"),
        todo("👥 Kunden → Filter „⏰ Follow-up“ ist nicht leer"),
        todo("📥 Inbox → Filter „Verarbeitet“ ist nicht angehakt"),
        h2("Tipps"),
        bullet("Setze das HQ in die Favoriten (⭐ oben rechts) – so ist es immer einen Klick entfernt."),
        bullet("Datenbankvorlagen: Lege in „Projekte“ eine Vorlage mit der Video-Produktions-Checkliste an (siehe SOPs)."),
        bullet("Die Beispieldaten kannst du jederzeit löschen oder überschreiben."),
    ]


def page_tag():
    return [
        callout("Deine 5-Minuten-Morgenroutine. Dupliziere diese Checkliste nicht – hake einfach ab und setze sie morgens wieder zurück.",
                "☀️", "yellow_background"),
        h2("1 · Inbox leeren (2 Min.)"),
        todo("📥 Inbox öffnen: jeden Eintrag einsortieren → Aufgabe, Idee, Content oder löschen"),
        todo("„Verarbeitet“ anhaken"),
        h2("2 · Fokus setzen (2 Min.)"),
        todo("⚠️ Überfällige Aufgaben prüfen: neu terminieren oder erledigen"),
        todo("Maximal 3 Aufgaben mit „Heute“ markieren (1× groß, 2× klein)"),
        todo("Kalender checken: Drehs, Calls, Deadlines"),
        h2("3 · Kommunikation (1 Min.)"),
        todo("👥 Kunden mit „⏰ Jetzt melden“ – heute eine Nachricht schicken"),
        todo("📅 Content, der heute veröffentlicht wird, freigeben"),
        h2("Abends (optional)"),
        todo("„Heute“-Haken bei erledigten Aufgaben entfernen, Status auf Erledigt"),
        todo("Kurzer Journal-Eintrag in 📓 Meetings & Journal: Was lief gut? Was lerne ich?"),
        quote("Tipp: Frag Claude oder Notion AI „Plane meinen Tag aus meinen Aufgaben mit Heute und Überfällig“."),
    ]


def page_woche():
    return [
        callout("Jeden Freitag 20 Minuten. Lege danach einen Eintrag „Weekly Review“ in 📓 Meetings & Journal an.",
                "🔁", "purple_background"),
        h2("Rückblick"),
        todo("Was habe ich diese Woche abgeschlossen? (Aufgaben → Erledigt)"),
        todo("Was ist liegen geblieben – und warum?"),
        todo("Highlights / Learnings notieren"),
        h2("Projekte"),
        todo("Jedes aktive Projekt: Status aktuell? Nächster Schritt als Aufgabe vorhanden?"),
        todo("Deadlines der nächsten 14 Tage prüfen (⏳ Tage bis Deadline)"),
        todo("Assets gesichert? (3-2-1-Backup: 3 Kopien, 2 Medien, 1 extern)"),
        h2("Kunden & Akquise"),
        todo("Follow-ups für nächste Woche terminieren"),
        todo("Mindestens 1 Lead aktiv kontaktieren"),
        h2("Finanzen"),
        todo("Offene/überfällige Rechnungen nachfassen"),
        todo("Belege der Woche erfasst"),
        h2("Content"),
        todo("Content der nächsten Woche: Skript/Dreh/Schnitt im Plan?"),
        todo("Performance (Views) der veröffentlichten Beiträge eintragen"),
        h2("Ziele & nächste Woche"),
        todo("🎯 Ziele prüfen: Zahlt die nächste Woche darauf ein?"),
        todo("Die 3 wichtigsten Ergebnisse für nächste Woche festlegen"),
        todo("Equipment: Akkus laden, Karten formatieren, Defekte melden"),
    ]


PROMPTS = [
    ("☀️ Tagesplan erstellen",
     "Schau dir meine Aufgaben an, die heute fällig, überfällig oder mit „Heute“ markiert sind. Erstelle einen realistischen "
     "Tagesplan mit Zeitblöcken (Deep Work am Vormittag), berücksichtige den Aufwand und nenne die eine wichtigste Aufgabe."),
    ("📝 Briefing → Aufgabenliste",
     "Hier ist ein Kundenbriefing: [Briefing einfügen]. Zerlege es in Phasen (Pre-Production, Produktion, Post, Delivery) "
     "und erstelle eine Aufgabenliste mit Aufwandsschätzung und Fälligkeitsdaten bis zur Deadline [Datum]."),
    ("🎬 Shotlist generieren",
     "Erstelle eine Shotlist für [Projekt]: Ziel, Zielgruppe [..], Länge [..]. Spalten: Nr., Szene, Einstellung, "
     "Kamerabewegung, Objektiv, Ton, Dauer, Notiz. Plane B-Roll und Sicherheits-Shots ein."),
    ("✍️ Skript / Hook schreiben",
     "Schreibe 5 Hooks und ein 60-Sekunden-Skript für ein Reel zum Thema [Thema] für [Plattform]. Ton: [locker/professionell]. "
     "Struktur: Hook (0–3 s), Problem, Lösung, Call-to-Action."),
    ("♻️ Content repurposen",
     "Aus diesem Long-Form-Video/Transkript: [Text] – erstelle 3 Shorts-Ideen mit Timecodes, 1 LinkedIn-Post, "
     "1 Instagram-Karussell (7 Slides) und 1 Newsletter-Absatz."),
    ("📧 Kunden-Mail formulieren",
     "Formuliere eine freundliche, professionelle E-Mail an [Kunde]: Anlass [Follow-up / Angebot / Freigabe / Zahlungserinnerung]. "
     "Kurz, klar, mit konkretem nächsten Schritt."),
    ("💶 Angebot kalkulieren",
     "Kalkuliere ein Angebot für [Projekt]: Drehtage [x], Schnitttage [y], Freelancer [..], Equipment, Lizenzen, Puffer 15 %. "
     "Tagessatz [€]. Gib eine Positionsliste und eine Gesamtsumme netto/brutto (19 % MwSt.) aus."),
    ("🔁 Wochenrückblick auswerten",
     "Fasse meine erledigten Aufgaben, Projektstatus und Finanzen dieser Woche zusammen. Was lief gut, was blockiert, "
     "welche 3 Prioritäten schlägst du für nächste Woche vor?"),
    ("💡 Ideen bewerten",
     "Bewerte diese Ideen aus meinem Ideen-Pool nach Aufwand, Wirkung und Passung zu meinen Zielen [Ziele]. "
     "Gib eine Rangliste mit Begründung und einen ersten Schritt je Idee."),
]


def page_prompts():
    blocks = [
        callout("Kopiere einen Prompt in Notion AI (Leertaste in einer leeren Zeile oder ‚Q&A‘) oder in Claude. "
                "Ersetze die [Platzhalter].", "🤖", "green_background"),
    ]
    for t, prompt in PROMPTS:
        blocks.append(toggle([rt(t, bold=True)], [code(prompt)]))
    blocks += [
        divider(),
        h3("Mit Claude verbinden"),
        p("Verbinde Notion in Claude (Einstellungen → Connectors → Notion). Dann kann Claude direkt in diesem Workspace "
          "Aufgaben anlegen, Tagespläne schreiben und Projekte zusammenfassen."),
    ]
    return blocks


SOPS = {
    ("🎥", "Video-Produktion"): [
        callout("Standardablauf für jede Videoproduktion. Ideal als Datenbankvorlage in 🎞 Projekte.", "🎥", "orange_background"),
        h2("1 · Pre-Production"),
        todo("Briefing & Ziel klären (Zielgruppe, Kanal, Länge, Formate 16:9 / 9:16)"),
        todo("Angebot & Auftragsbestätigung"),
        todo("Konzept / Treatment, Moodboard"),
        todo("Skript & Shotlist"),
        todo("Location, Drehgenehmigungen, Releases (Model/Location)"),
        todo("Drehplan, Crew & Equipment buchen"),
        h2("2 · Produktion"),
        todo("Packliste Equipment (Akkus, Karten, Ton, Licht)"),
        todo("Tonprobe, Weißabgleich, Framerate/Shutter prüfen"),
        todo("Backup vor Ort (2 Kopien)"),
        h2("3 · Postproduktion"),
        todo("Ingest & Ordnerstruktur, 3-2-1-Backup"),
        todo("Rohschnitt → Feedback-Runde 1"),
        todo("Feinschnitt, Musik, Sounddesign"),
        todo("Color Grading, Untertitel, Grafiken"),
        todo("Feedback-Runde 2 → Final"),
        h2("4 · Delivery"),
        todo("Exporte in allen Formaten & Plattform-Specs"),
        todo("Übergabe (Link), Rechnung stellen"),
        todo("Projekt archivieren, Assets in 🗂 Asset-Bibliothek eintragen"),
        todo("Testimonial anfragen, Making-of für Content nutzen"),
    ],
    ("🤝", "Kunden-Onboarding"): [
        callout("Vom Erstkontakt bis zum Projektstart – professionell und wiederholbar.", "🤝", "blue_background"),
        num("Anfrage in 📥 Inbox → Kontakt in 👥 Kunden (Typ: Lead) anlegen"),
        num("Erstgespräch (20–30 Min.): Ziel, Budget, Timing, Entscheider – Notizen in 📓 Meetings"),
        num("Angebot innerhalb von 48 h senden, Follow-up in 3 Tagen setzen"),
        num("Zusage → Typ: Kunde, Projekt in 🎞 Projekte anlegen und verknüpfen"),
        num("Anzahlungsrechnung (30–50 %) in 💶 Finanzen"),
        num("Kick-off-Termin, Briefing-Dokument, gemeinsamer Ordner"),
        num("Feedback-Prozess erklären: max. 2 Korrekturschleifen inklusive"),
    ],
    ("♻️", "Content-Repurposing"): [
        callout("Aus einem Inhalt viele machen.", "♻️", "green_background"),
        bullet("1 Long-Form-Video (YouTube) →",
               [bullet("3–5 Shorts/Reels/TikToks"), bullet("1 LinkedIn-Post + 1 Karussell"),
                bullet("1 Blogartikel/Newsletter"), bullet("Thumbnails & Stills für Stories")]),
        bullet("Jeder Kunden-Dreh → Making-of-Reel (mit Freigabe des Kunden)"),
        bullet("Jede Frage von Kunden → ein Tipp-Post"),
        todo("Alle Ableger in 📅 Content-Kalender anlegen und mit dem Projekt verknüpfen"),
    ],
    ("🧾", "Angebot & Rechnung"): [
        callout("Sauber kalkulieren, pünktlich bezahlt werden.", "🧾", "red_background"),
        h3("Angebot"),
        todo("Positionen: Konzeption, Drehtage, Schnitt, Grading, Ton, Lizenzen, Reisekosten"),
        todo("Nutzungsrechte definieren (Kanäle, Dauer, Region)"),
        todo("Puffer einplanen (10–15 %), Gültigkeit 30 Tage"),
        h3("Rechnung"),
        todo("Rechnungsnummer fortlaufend (RE-JJJJ-NNN) in 💶 Finanzen"),
        todo("Pflichtangaben: Steuernr./USt-IdNr., Leistungsdatum, Netto/MwSt./Brutto"),
        todo("Zahlungsziel 14 Tage, Status ‚Offen‘"),
        todo("Nach 14 Tagen unbezahlt → Status ‚Überfällig‘, freundliche Erinnerung (Prompt 📧)"),
    ],
}


def db_ref(did):
    """Seitenlink für Full-Page-Datenbanken, @-Erwähnung für Inline-Datenbanken."""
    db = call("GET", f"/databases/{did}")
    if not db.get("is_inline"):
        return link_db(did)
    return p([{"type": "mention", "mention": {"type": "database", "database": {"id": did}}}])


def build_hq(hq, sub, ids, alex=None):
    def dbl(key): return db_ref(ids[key])
    blocks = [
        callout([rt("Guten Tag! ", bold=True),
                 rt("Das ist dein Studio-HQ. Starte jeden Tag mit ☀️ Tagesplanung und jede Woche mit 🔁 Wochenrückblick.")],
                "🎬", "gray_background"),
        h2("⚡ Schnellstart"),
        columns(
            [h3("📥 Erfassen"), dbl("inbox"), dbl("ideen")],
            [h3("☀️ Routinen"), link_page(sub["tag"]), link_page(sub["woche"])],
            [h3("🤖 Assistent"), link_page(sub["prompts"]), link_page(sub["start"])],
        ),
        divider(),
        h2("🎬 Produktion"),
        columns(
            [dbl("projekte"), dbl("aufgaben")],
            [dbl("content"), dbl("assets")],
            [dbl("ziele"), dbl("meetings")],
        ),
        h2("💼 Business"),
        columns(
            [dbl("kunden")],
            [dbl("finanzen")],
            [dbl("equipment")],
        ),
        h2("📚 Wissen & Abläufe"),
        columns(
            [dbl("wissen")],
            [link_page(sub["sops"])],
            [link_page(sub["system"])],
        ),
    ]
    if alex:
        blocks += [
            h2("🤖 Alex 2.0 – dein Assistent"),
            callout("Alex' Aufgaben sind die Aufgaben dieses Studios: Verknüpfe sie über „Studio-Projekt“ mit einem Projekt "
                    "und markiere deine Top-3 mit „Heute“.", "🤖", "purple_background"),
            columns([link_page(alex["page"])] + [db_ref(x) for x in alex["dbs"][:2]],
                    [db_ref(x) for x in alex["dbs"][2:]] or [p()]),
        ]
    blocks += [
        divider(),
        quote([rt("„Kreativität braucht Struktur, damit sie frei sein kann.“", italic=True)]),
    ]
    append(hq, blocks)


def find_parent():
    res = call("POST", "/search", {"filter": {"property": "object", "value": "page"}, "page_size": 50})
    pages = [r for r in res["results"] if r.get("parent", {}).get("type") in ("workspace", "page_id")]
    if not pages:
        sys.exit("Keine Seite mit der Integration geteilt. Bitte eine Seite in Notion über ‚… → Verbindungen‘ teilen.")
    for pg in pages:
        t = "".join(x["plain_text"] for x in pg["properties"].get("title", {}).get("title", []))
        if t.strip() == HQ_TITLE:
            return None, pg["id"]
    # bevorzuge Top-Level-Seiten
    pages.sort(key=lambda r: r["parent"]["type"] != "workspace")
    return pages[0]["id"], None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--parent")
    ap.add_argument("--force", action="store_true")
    ap.add_argument("--samples", action="store_true", help="Beispieldaten anlegen")
    ap.add_argument("--tasks-db", help="bestehende Aufgaben-DB (z. B. Alex 2.0 Aufgaben) einbinden statt neu anlegen")
    ap.add_argument("--alex-page", help="Seite von Alex 2.0 – wird im HQ verlinkt")
    ap.add_argument("--link-from", help="Seite, auf der oben ein Link zum HQ eingefügt wird")
    a = ap.parse_args()

    me = call("GET", "/users/me")
    print(f"Verbunden als Integration: {me.get('name')}")
    parent, existing = (a.parent, None) if a.parent else find_parent()
    if existing and not a.force:
        sys.exit(f"„{HQ_TITLE}“ existiert bereits ({existing}). Mit --force neu bauen.")
    if not parent:
        parent = call("GET", f"/pages/{existing}")["parent"].get("page_id")
    print(f"Baue unter Seite {parent} …")

    hq = create_page(parent, HQ_TITLE, "🎬",
                     cover="https://images.unsplash.com/photo-1492691527719-9d1e07e534b4?w=1600")
    print("  ✓ HQ")
    sub = {
        "start": create_page(hq, "Start hier", "🧭", page_start({})),
        "tag": create_page(hq, "Tagesplanung", "☀️", page_tag()),
        "woche": create_page(hq, "Wochenrückblick", "🔁", page_woche()),
        "prompts": create_page(hq, "Assistent-Prompts", "🤖", page_prompts()),
    }
    sub["sops"] = create_page(hq, "SOPs & Workflows", "📋", [
        callout("Wiederholbare Abläufe als Checklisten. Tipp: als Datenbankvorlage in Projekten hinterlegen.", "📋")])
    for (emoji, name), blocks in SOPS.items():
        create_page(sub["sops"], name, emoji, blocks)
    print("  ✓ Routinen, Prompts & SOPs")

    sub["system"] = create_page(hq, "System (Datenbanken)", "🗄", [
        callout("Hier liegen alle Datenbanken. Im Alltag arbeitest du über das HQ – diese Seite ist der Maschinenraum.",
                "🗄", "gray_background")])
    ids, pages = {}, {}
    specs = [x for x in DBS if not (a.tasks_db and x["key"] == "aufgaben")]
    for spec in specs:
        create_db(sub["system"], spec, ids)
        if spec["key"] == "projekte" and a.tasks_db:
            integrate_tasks(a.tasks_db, ids)
    add_rollups(ids)
    if a.samples and not a.tasks_db:
        for spec in DBS:
            fill_rows(spec, ids, pages)
        print("  ✓ Beispieldaten")

    alex = None
    if a.alex_page:
        alex = {"page": a.alex_page, "dbs": find_child_dbs(a.alex_page)}
    build_hq(hq, sub, ids, alex)
    if a.link_from:
        link_hq_from(a.link_from, hq)
    out = {"hq": hq, "pages": sub, "databases": ids}
    with open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "workspace_ids.json"), "w") as f:
        json.dump(out, f, indent=2)
    print(f"\nFertig! HQ: https://www.notion.so/{hq.replace('-', '')}")


if __name__ == "__main__":
    main()

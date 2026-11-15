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
def formula(expr): return {"formula": {"expression": expr}}
def files(): return {"files": {}}


def chunks(text, n=1900):
    return [rt(text[i:i + n]) for i in range(0, len(text), n)] or [rt("")]


def code_block(text, lang="plain text"):
    return {"type": "code", "code": {"rich_text": chunks(text), "language": lang}}


def mention_db(did):
    return {"type": "mention", "mention": {"type": "database", "database": {"id": did}}}


def mention_page(pid):
    return {"type": "mention", "mention": {"type": "page", "page": {"id": pid}}}


def inline(text):
    """**fett** und `code` → Rich Text."""
    out, bold, code_, buf, i = [], False, False, "", 0
    while i < len(text):
        if text.startswith("**", i) and not code_:
            if buf: out.append(rt(buf, bold=bold, code=code_))
            buf, bold, i = "", not bold, i + 2; continue
        if text[i] == "`":
            if buf: out.append(rt(buf, bold=bold, code=code_))
            buf, code_, i = "", not code_, i + 1; continue
        buf += text[i]; i += 1
    if buf: out.append(rt(buf, bold=bold, code=code_))
    return out or [rt("")]


def md(text):
    """Kleines Markdown → Notion-Blöcke (Überschriften, Listen, To-dos, Zitate, Callouts !emoji, Code, Tabellen→Code)."""
    blocks, buf, in_code, lang = [], [], False, "plain text"
    for line in text.strip("\n").splitlines():
        s = line.rstrip()
        if s.startswith("```"):
            if in_code:
                blocks.append(code_block("\n".join(buf), lang)); buf, in_code = [], False
            else:
                in_code, lang = True, (s[3:].strip() or "plain text")
            continue
        if in_code:
            buf.append(line); continue
        st = s.strip()
        if not st: continue
        if st == "---": blocks.append(divider())
        elif st.startswith("### "): blocks.append(h3(inline(st[4:])))
        elif st.startswith("## "): blocks.append(h2(inline(st[3:])))
        elif st.startswith("# "): blocks.append(h1(inline(st[2:])))
        elif st.startswith("- [ ] "): blocks.append(todo(inline(st[6:])))
        elif st.startswith("- [x] "): blocks.append(todo(inline(st[6:]), True))
        elif st.startswith("- "): blocks.append(bullet(inline(st[2:])))
        elif st[0].isdigit() and ". " in st[:4]: blocks.append(num(inline(st.split(". ", 1)[1])))
        elif st.startswith("> "): blocks.append(quote(inline(st[2:])))
        elif st.startswith("!"):
            emoji, rest = st[1:].split(" ", 1)
            blocks.append(callout(inline(rest), emoji, "gray_background"))
        else: blocks.append(p(inline(st)))
    return blocks

"""Leert den Notion-Workspace: alles in den Papierkorb, nur die Root-Seite bleibt als leere Hülle."""
import sys, json
sys.path.insert(0, '.'); import notion_lib as b
ROOT = sys.argv[1]
MODE = sys.argv[2] if len(sys.argv) > 2 else 'dry'

objs, cur = [], None
while True:
    body = {'page_size': 100}
    if cur: body['start_cursor'] = cur
    r = b.call('POST', '/search', body)
    objs += r['results']
    if not r['has_more']: break
    cur = r['next_cursor']

def title(o):
    if o['object'] == 'database': return ''.join(x['plain_text'] for x in o['title'])
    for v in o['properties'].values():
        if v['type'] == 'title': return ''.join(x['plain_text'] for x in v['title'])
    return ''

top = [o for o in objs if o['parent']['type'] == 'workspace' and o['id'] != ROOT]
print(f'Sichtbare Objekte: {len(objs)} | oberste Ebene (ohne Root): {len(top)}')
for o in top: print('  ', o['object'], o['id'], repr(title(o)))

# Kinder der Root-Seite
kids, cur = [], None
while True:
    r = b.call('GET', f'/blocks/{ROOT}/children?page_size=100' + (f'&start_cursor={cur}' if cur else ''))
    kids += r['results']
    if not r['has_more']: break
    cur = r['next_cursor']
print(f'Blöcke auf der Root-Seite: {len(kids)}')

if MODE == 'delete':
    fails = []
    for o in top:
        try:
            b.call('PATCH', f"/{'databases' if o['object']=='database' else 'pages'}/{o['id']}", {'archived': True})
        except RuntimeError as e:
            fails.append((o['id'], title(o), str(e)[:200]))
    for k in kids:
        try:
            b.call('DELETE', f"/blocks/{k['id']}")
        except RuntimeError as e:
            fails.append((k['id'], k['type'], str(e)[:200]))
    b.call('PATCH', f'/pages/{ROOT}', {'properties': {'title': {'title': [b.rt('ALEX · Media Creative Studio')]}},
                                        'icon': {'type': 'emoji', 'emoji': '🏠'}})
    print('Fehler:', fails)
    # Kontrolle
    r = b.call('POST', '/search', {'page_size': 100})
    rest = [(o['object'], title(o), o['parent']['type']) for o in r['results']]
    print('Danach sichtbar:', len(rest), rest[:20])

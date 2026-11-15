// ===== ALEX Agentur · Hilfsfunktionen (in jeden Code-Node eingebettet) =====
const LIMIT = 1900;
function rtChunks(text) {
  text = String(text ?? '');
  const out = [];
  for (let i = 0; i < text.length; i += LIMIT) out.push({ type: 'text', text: { content: text.slice(i, i + LIMIT) } });
  return out.length ? out : [{ type: 'text', text: { content: '' } }];
}
function plain(prop) {
  if (!prop) return '';
  const arr = prop.title || prop.rich_text || [];
  return arr.map(t => t.plain_text ?? t.text?.content ?? '').join('');
}
function sel(prop) { return prop?.select?.name || ''; }
function relIds(prop) { return (prop?.relation || []).map(r => r.id); }
function num(prop) {
  if (!prop) return null;
  if (prop.type === 'number') return prop.number;
  if (prop.type === 'rollup') return prop.rollup?.number ?? null;
  if (prop.type === 'formula') return prop.formula?.number ?? null;
  return null;
}
function dateStr(prop) { return prop?.date?.start || ''; }
function inlineRT(s) {
  const out = [];
  const push = (content, ann, link) => {
    for (let i = 0; i < content.length; i += LIMIT) {
      const t = { type: 'text', text: { content: content.slice(i, i + LIMIT) } };
      if (link) t.text.link = { url: link };
      if (ann) t.annotations = ann;
      out.push(t);
    }
  };
  const re = /(\*\*[^*]+\*\*|`[^`]+`|\[[^\]]+\]\([^)\s]+\))/g;
  let last = 0, m;
  while ((m = re.exec(s))) {
    if (m.index > last) push(s.slice(last, m.index));
    const tok = m[0];
    if (tok.startsWith('**')) push(tok.slice(2, -2), { bold: true });
    else if (tok.startsWith('`')) push(tok.slice(1, -1), { code: true });
    else {
      const mm = tok.match(/^\[([^\]]+)\]\(([^)\s]+)\)$/);
      if (mm && /^https?:\/\//.test(mm[2])) push(mm[1], null, mm[2]); else push(tok);
    }
    last = m.index + tok.length;
  }
  if (last < s.length) push(s.slice(last));
  return (out.length ? out : [{ type: 'text', text: { content: '' } }]).slice(0, 100);
}
function block(type, text, extra) { return { type, [type]: Object.assign({ rich_text: inlineRT(text) }, extra || {}) }; }
function callout(text, emoji, color) {
  return { type: 'callout', callout: { rich_text: rtChunks(text).slice(0, 100), icon: { type: 'emoji', emoji: emoji || '💡' }, color: color || 'gray_background' } };
}
function mdToBlocks(md, maxBlocks) {
  maxBlocks = maxBlocks || 95;
  const lines = String(md || '').replace(/\r/g, '').split('\n');
  const blocks = [];
  let i = 0;
  while (i < lines.length) {
    const line = lines[i];
    const t = line.trim();
    if (!t) { i++; continue; }
    if (t.startsWith('```')) {
      const lang = t.slice(3).trim();
      const buf = []; i++;
      while (i < lines.length && !lines[i].trim().startsWith('```')) { buf.push(lines[i]); i++; }
      i++;
      const allowed = ['javascript', 'python', 'json', 'html', 'css', 'bash', 'markdown', 'typescript', 'sql', 'yaml'];
      blocks.push({ type: 'code', code: { rich_text: rtChunks(buf.join('\n')).slice(0, 100), language: allowed.includes(lang) ? lang : 'plain text' } });
      continue;
    }
    if (t.startsWith('|')) {
      const rows = [];
      while (i < lines.length && lines[i].trim().startsWith('|')) {
        const r = lines[i].trim();
        if (!/^\|?\s*:?-{2,}/.test(r.replace(/\|/g, '|'))) {
          const cells = r.replace(/^\|/, '').replace(/\|\s*$/, '').split('|').map(c => c.trim());
          if (!cells.every(c => /^:?-{2,}:?$/.test(c))) rows.push(cells);
        }
        i++;
      }
      if (rows.length) {
        const width = Math.min(Math.max(...rows.map(r => r.length)), 20);
        blocks.push({ type: 'table', table: { table_width: width, has_column_header: true, has_row_header: false,
          children: rows.slice(0, 100).map(r => ({ type: 'table_row', table_row: { cells: Array.from({ length: width }, (_, k) => inlineRT(r[k] || '')) } })) } });
      }
      continue;
    }
    let m;
    if ((m = t.match(/^(#{1,6})\s+(.*)$/))) {
      const lvl = Math.min(m[1].length, 3);
      blocks.push(block('heading_' + lvl, m[2]));
    } else if (/^[-*]\s+\[( |x|X)\]\s+/.test(t)) {
      const checked = /^[-*]\s+\[(x|X)\]/.test(t);
      blocks.push(block('to_do', t.replace(/^[-*]\s+\[( |x|X)\]\s+/, ''), { checked }));
    } else if (/^[-*•]\s+/.test(t)) {
      blocks.push(block('bulleted_list_item', t.replace(/^[-*•]\s+/, '')));
    } else if (/^\d+[.)]\s+/.test(t)) {
      blocks.push(block('numbered_list_item', t.replace(/^\d+[.)]\s+/, '')));
    } else if (t.startsWith('>')) {
      blocks.push(block('quote', t.replace(/^>\s?/, '')));
    } else if (/^(-{3,}|\*{3,}|_{3,})$/.test(t)) {
      blocks.push({ type: 'divider', divider: {} });
    } else {
      blocks.push(block('paragraph', t));
    }
    i++;
  }
  if (blocks.length > maxBlocks) {
    const kept = blocks.slice(0, maxBlocks - 1);
    kept.push(callout('Das Ergebnis ist länger als eine Notion-Anfrage erlaubt. Den vollständigen Text findest du im Feld „Ergebnis“.', '✂️', 'yellow_background'));
    return kept;
  }
  return blocks;
}
function blocksToText(results) {
  return (results || []).map(bl => {
    const v = bl[bl.type] || {};
    const txt = (v.rich_text || []).map(t => t.plain_text || '').join('');
    if (!txt) return '';
    if (bl.type.startsWith('heading')) return '## ' + txt;
    if (bl.type === 'bulleted_list_item') return '- ' + txt;
    if (bl.type === 'numbered_list_item') return '1. ' + txt;
    if (bl.type === 'to_do') return (v.checked ? '- [x] ' : '- [ ] ') + txt;
    return txt;
  }).filter(Boolean).join('\n');
}
function claudeText(r) { return (r.content || []).filter(c => c.type === 'text').map(c => c.text).join('\n').trim(); }
function claudeError(r) {
  if (!r) return 'Keine Antwort';
  if (r.error) return typeof r.error === 'object' ? (r.error.message || JSON.stringify(r.error)).slice(0, 1800) : String(r.error).slice(0, 1800);
  if (!Array.isArray(r.content)) return 'Unerwartete Antwort: ' + JSON.stringify(r).slice(0, 500);
  if (r.stop_reason === 'refusal') return 'Claude hat abgelehnt: ' + (r.stop_details?.explanation || r.stop_details?.category || '');
  return '';
}
function withFallback(body) {
  if (CFG.fallbacks && /opus|sonnet|fable/.test(body.model)) body.fallbacks = 'default';
  return body;
}
function berlinNow() { return new Date().toLocaleString('de-DE', { timeZone: 'Europe/Berlin' }); }
function berlinDate() { return new Date().toLocaleDateString('de-DE', { timeZone: 'Europe/Berlin', weekday: 'long', year: 'numeric', month: 'long', day: 'numeric' }); }
function isoToday() { return new Date().toLocaleDateString('sv-SE', { timeZone: 'Europe/Berlin' }); }
function logEntry(o) {
  const props = {
    Lauf: { title: rtChunks(String(o.titel).slice(0, 200)) },
    Workflow: { select: { name: o.workflow } },
    Status: { select: { name: o.status } },
    'Dauer (Sek)': { number: o.dauer ?? null },
    'Tokens Input': { number: o.usage?.input_tokens ?? null },
    'Tokens Output': { number: o.usage?.output_tokens ?? null },
    Modell: { rich_text: rtChunks(o.modell || '') },
    Fehler: { rich_text: rtChunks(o.fehler || '') },
    'n8n-Ausführung': { rich_text: rtChunks(String(o.lauf || '')) },
  };
  if (o.agent_id) props.Agent = { relation: [{ id: o.agent_id }] };
  if (o.job_id) props.Job = { relation: [{ id: o.job_id }] };
  return { parent: { database_id: CFG.db.protokoll }, properties: props };
}
// ===== Ende Hilfsfunktionen =====

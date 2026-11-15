// Mock der Claude Messages API für Ende-zu-Ende-Tests der n8n-Workflows (keine echten Kosten).
// Prüft jede Anfrage auf die Felder, die die echte API verlangt, und antwortet im echten Antwortformat.
const http = require('http');
const fs = require('fs');
const LOG = process.argv[2] || 'mock_requests.jsonl';
let reviewCount = 0;
const reply = (res, code, obj) => { res.writeHead(code, { 'content-type': 'application/json' }); res.end(JSON.stringify(obj)); };
const msg = (text, model) => ({ id: 'msg_mock_' + Date.now(), type: 'message', role: 'assistant', model,
  content: [{ type: 'text', text }], stop_reason: 'end_turn', stop_sequence: null,
  usage: { input_tokens: 1234, output_tokens: 567 } });
http.createServer((req, res) => {
  let raw = '';
  req.on('data', c => raw += c);
  req.on('end', () => {
    let b; try { b = JSON.parse(raw); } catch { return reply(res, 400, { type: 'error', error: { type: 'invalid_request_error', message: 'invalid JSON' } }); }
    const problems = [];
    if (req.url !== '/v1/messages') problems.push('path ' + req.url);
    if (!req.headers['x-api-key']) problems.push('x-api-key fehlt');
    if (req.headers['anthropic-version'] !== '2026-10-21') problems.push('anthropic-version');
    if (!b.model || !b.max_tokens || !Array.isArray(b.messages) || !b.messages.length) problems.push('model/max_tokens/messages');
    for (const k of ['temperature', 'top_p', 'top_k', 'thinking']) if (k in b) problems.push(k + ' darf nicht gesendet werden');
    if (typeof b.system !== 'string' || b.system.length < 200) problems.push('system prompt fehlt/zu kurz');
    if (b.output_config?.format && b.output_config.format.type !== 'json_schema') problems.push('format');
    fs.appendFileSync(LOG, JSON.stringify({ t: new Date().toISOString(), problems, model: b.model, effort: b.output_config?.effort,
      schema: !!b.output_config?.format, tools: b.tools, fallbacks: b.fallbacks, user: b.messages[0].content.slice(0, 3000) }) + '\n');
    if (problems.length) return reply(res, 400, { type: 'error', error: { type: 'invalid_request_error', message: problems.join('; ') } });
    const schema = b.output_config?.format?.schema;
    const user = b.messages[0].content;
    if (schema?.properties?.jobs) {
      const names = schema.properties.jobs.items.properties.agent.enum;
      const jobs = [];
      if (names.includes('ResearchAnalyst')) jobs.push({ titel: 'Recherche: Trends Food-Reels 2027', agent: 'ResearchAnalyst',
        auftrag: 'Recherchiere die 5 wichtigsten Trends für Instagram-Reels in der Gastronomie (2026/2027) mit Quellen.', prioritaet: 'Normal', faellig: '', wartet_auf: [] });
      jobs.push({ titel: '3 Instagram-Reels für Café Aroma (Skripte)', agent: 'ContentWriter',
        auftrag: 'Schreibe 3 Reel-Skripte (je 20–30 Sek.) für Café Aroma auf Basis der Recherche. Zielgruppe 25–40, Ton warm und nahbar.', prioritaet: 'Hoch', faellig: '', wartet_auf: jobs.length ? [0] : [] });
      return reply(res, 200, msg(JSON.stringify({ verstanden: 'Achim möchte Social-Content für Café Aroma.', rueckfrage: '',
        plan: 'Erst Trends recherchieren, dann darauf aufbauend 3 Reel-Skripte schreiben.', jobs }), b.model));
    }
    if (schema?.properties?.kriterien) {
      reviewCount++;
      const v = reviewCount === 1 ? 6 : 8;
      const kriterien = Object.fromEntries(schema.properties.kriterien.required.map(k => [k, v]));
      return reply(res, 200, msg(JSON.stringify({ kriterien, entscheidung: v >= 7 ? 'freigabe' : 'ueberarbeiten',
        staerken: 'Klare Struktur, passende Tonalität.', feedback: v >= 7 ? '' : '- Hook von Reel 2 ist zu allgemein – mit einer konkreten Szene starten\n- CTA fehlt bei Reel 3' }), b.model));
    }
    if (/Morgen-Briefing/.test(user)) {
      return reply(res, 200, msg('📋 ' + new Date().toLocaleDateString('de-DE') + '\n🎯 Fokus: Café-Aroma-Content freigeben.\n\n## ✅ Deine Top 3 heute\n1. **Reel-Skripte freigeben** – 15 Min\n2. Angebot Weber Sports – 1 Std\n3. Belege hochladen – 15 Min\n\n💪 Weniger, aber besser.', b.model));
    }
    const rev = /ÜBERARBEITUNG/.test(user);
    const text = `## Ergebnis\n${rev ? '**Überarbeitete Fassung** – Feedback umgesetzt.\n\n' : ''}### Reel 1 – „Der erste Schluck“\n| Szene | Bild | Sprechertext | Dauer |\n|---|---|---|---|\n| 1 | Close-up Milchschaum | „Kennst du diesen Moment?“ | 3 s |\n| 2 | Barista lacht | „Bei uns ist er jeden Morgen.“ | 4 s |\n\n- Hook: „Kennst du diesen Moment?“\n- CTA: **Komm vorbei** – [Website](https://example.com)\n\n## Zusammenfassung\n3 Reel-Skripte geliefert${rev ? ' (überarbeitet)' : ''}. Nächster Schritt: Freigabe und Drehplanung.\n\n## Offene Fragen\nKeine`;
    return reply(res, 200, msg(text, b.model));
  });
}).listen(8787, '127.0.0.1', () => console.log('mock claude on :8787'));

// Briefing · als Seite in „Briefings“ speichern
const { alex_id, start, lauf_id } = $('Briefing-Prompt bauen').first().json;
const r = $input.first().json;
let fehler = claudeError(r), text = fehler ? '' : claudeText(r);
if (!fehler && !text) fehler = 'Leere Antwort';
const heute = isoToday();
const fokus = (text.match(/🎯\s*(.+)/) || [])[1] || '';
const children = fehler ? [callout('Das Briefing konnte nicht erstellt werden: ' + fehler, '❌', 'red_background')] : mdToBlocks(text, 98);
const page = { parent: { database_id: CFG.db.briefings }, icon: { type: 'emoji', emoji: '☀️' },
  properties: { Briefing: { title: rtChunks('Briefing · ' + berlinDate()) }, Datum: { date: { start: heute } },
    Typ: { select: { name: 'Morgen-Briefing' } }, Fokus: { rich_text: rtChunks(fokus.slice(0, 500)) } }, children };
const log = logEntry({ titel: 'Briefing · ' + heute, workflow: 'Briefing', status: fehler ? 'Fehler' : 'Erfolg',
  dauer: Math.round((Date.now() - new Date(start)) / 1000), usage: r.usage, modell: r.model, fehler, lauf: lauf_id, agent_id: alex_id });
return [{ json: { page, log } }];

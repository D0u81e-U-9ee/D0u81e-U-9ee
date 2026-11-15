// Briefing · alle Daten sammeln und ALEX schreiben lassen
const get = n => $(n).first().json.results || [];
const heute = isoToday();
const aufgaben = get('Meine Aufgaben').map(p => { const P = p.properties; return {
  t: plain(P['Aufgabe']), s: sel(P['Status']), pr: sel(P['Priorität']), f: dateStr(P['Fällig']), h: P['Heute']?.checkbox }; });
const jobs = get('Offene Jobs').map(p => { const P = p.properties; return {
  t: plain(P['Job']), s: sel(P['Status']), pr: sel(P['Priorität']), f: dateStr(P['Fällig']), z: plain(P['Zusammenfassung']).slice(0, 200),
  alt: (Date.now() - new Date(p.last_edited_time)) / 3600000 }; });
const content = get('Content diese Woche').map(p => { const P = p.properties; return `${dateStr(P['Veröffentlichung'])} · ${plain(P['Titel'])} (${(P['Plattform']?.multi_select || []).map(x => x.name).join(', ')}) – ${sel(P['Status'])}`; });
const follow = get('Follow-ups').map(p => { const P = p.properties; return `${plain(P['Name'])}${plain(P['Firma']) ? ' (' + plain(P['Firma']) + ')' : ''} – fällig ${dateStr(P['Nächster Follow-up'])}`; });
const projekte = get('Projekte mit Deadline').map(p => { const P = p.properties; return `${plain(P['Projekt'])} – Deadline ${dateStr(P['Deadline'])} – ${sel(P['Status'])}`; });
const anfragen = get('Anfragen für dich').map(p => { const P = p.properties; return `${plain(P['Anfrage'])} – ${sel(P['Status'])}${sel(P['Status']) === 'Rückfrage' ? ': ' + plain(P['Rückfrage von ALEX']).slice(0, 300) : ''}`; });
const alexPage = get('ALEX laden')[0];
if (!alexPage) throw new Error('Agent „ALEX“ nicht gefunden');
const fmtA = x => `- ${x.f && x.f < heute ? '⚠️ ÜBERFÄLLIG ' : ''}${x.h ? '☀️ ' : ''}${x.t} [${x.pr || 'Normal'}]${x.f ? ' · fällig ' + x.f : ''} · ${x.s}`;
const freigabe = jobs.filter(j => j.s === 'Freigabe');
const fehler = jobs.filter(j => j.s === 'Fehler');
const haengt = jobs.filter(j => j.s === 'In Bearbeitung' && j.alt > 1);
const aktiv = jobs.filter(j => ['Bereit', 'In Bearbeitung', 'Review', 'Überarbeiten'].includes(j.s));
const user = `Schreibe Achims Morgen-Briefing für ${berlinDate()} (${heute}) im vorgegebenen Format. Nutze nur diese Daten:

## Meine Aufgaben (offen)
${aufgaben.map(fmtA).join('\n') || '–'}
## Jobs warten auf Freigabe
${freigabe.map(j => `- ${j.t}: ${j.z}`).join('\n') || '–'}
## Jobs mit Fehler
${fehler.map(j => `- ${j.t}`).join('\n') || '–'}
## Jobs hängen (seit > 1 Std. „In Bearbeitung“)
${haengt.map(j => `- ${j.t}`).join('\n') || '–'}
## Team arbeitet an
${aktiv.map(j => `- ${j.t} (${j.s})`).join('\n') || '–'}
## Anfragen, die auf Achim warten
${anfragen.join('\n') || '–'}
## Projekte mit Deadline in 14 Tagen
${projekte.join('\n') || '–'}
## Content in den nächsten 7 Tagen
${content.join('\n') || '–'}
## Follow-ups fällig
${follow.join('\n') || '–'}

Gib nur das Briefing aus (Markdown, max. 2500 Zeichen). Lass leere Abschnitte weg.`;
const A = alexPage.properties;
const body = withFallback({ model: sel(A['Modell']) || CFG.defaultModel, max_tokens: 8000, system: plain(A['System Prompt']),
  messages: [{ role: 'user', content: user }], output_config: { effort: 'medium' } });
return [{ json: { claude_body: body, alex_id: alexPage.id, start: new Date().toISOString(), lauf_id: String($execution.id) } }];

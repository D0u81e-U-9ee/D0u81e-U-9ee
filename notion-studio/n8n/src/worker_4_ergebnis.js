// Worker · Antwort auswerten → Job-Update, Ergebnis-Blöcke, Protokoll
const job = $('Job auswählen').first().json;
const { agent } = $('Prompt bauen').first().json;
const r = $input.first().json;
const now = new Date();
const dauer = Math.round((now - new Date(job.start)) / 1000);
let fehler = claudeError(r), text = '', status, logStatus = 'Erfolg';
if (!fehler) {
  text = claudeText(r);
  if (!text) fehler = 'Leere Antwort (stop_reason: ' + r.stop_reason + ')';
}
if (fehler) { status = 'Fehler'; logStatus = 'Fehler'; }
else {
  status = agent.review && agent.name !== 'CreativeDirector' ? 'Review' : 'Freigabe';
  if (r.stop_reason === 'max_tokens' || r.stop_reason === 'pause_turn') logStatus = 'Teilweise';
}
const m = text.match(/##\s*Zusammenfassung\s*\n([\s\S]*?)(\n##\s|$)/i);
const summary = (m ? m[1] : text.slice(0, 600)).trim().slice(0, 1900);
const versuch = (job.versuch || 0) + 1;
const props = {
  Status: { select: { name: status } },
  Beendet: { date: { start: now.toISOString() } },
  Versuch: { number: versuch },
  'Lauf-ID': { rich_text: rtChunks(job.lauf_id) },
};
if (text) {
  props['Ergebnis'] = { rich_text: rtChunks(text).slice(0, 100) };
  props['Zusammenfassung'] = { rich_text: rtChunks(summary) };
}
if (fehler) props['Feedback'] = { rich_text: rtChunks('Technischer Fehler: ' + fehler + ' – Status auf „Bereit“ setzen, um es erneut zu versuchen.') };
const blocks = [{ type: 'divider', divider: {} },
  { type: 'heading_2', heading_2: { rich_text: rtChunks(status === 'Fehler' ? `❌ Fehler – Versuch ${versuch}` : `📄 Ergebnis – Versuch ${versuch} · ${agent.name} · ${berlinNow()}`) } }];
if (status === 'Fehler') blocks.push(callout(fehler, '❌', 'red_background'));
else {
  if (logStatus === 'Teilweise') blocks.push(callout('Die Antwort wurde abgeschnitten (stop_reason: ' + r.stop_reason + '). Ggf. „Max Tokens“ des Agenten erhöhen.', '⚠️', 'yellow_background'));
  blocks.push(...mdToBlocks(text, 95));
}
const log = logEntry({ titel: `Worker · ${agent.name} · ${job.titel}`, workflow: 'Worker', status: logStatus, dauer,
  usage: r.usage, modell: r.model || agent.model, fehler, lauf: job.lauf_id, agent_id: job.agent_id, job_id: job.job_id });
return [{ json: { job_id: job.job_id, props, blocks, log } }];

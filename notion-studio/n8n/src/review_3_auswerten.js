// Review · Score berechnen und Job weiterleiten (Freigabe / Überarbeiten)
const job = $('Review-Job auswählen').first().json;
const { cd_id, KEYS } = $('Review-Prompt bauen').first().json;
const r = $input.first().json;
const dauer = Math.round((Date.now() - new Date(job.start)) / 1000);
let fehler = claudeError(r), data = null;
if (!fehler) { try { data = JSON.parse(claudeText(r)); } catch (e) { fehler = 'Antwort ist kein gültiges JSON: ' + e.message; } }
const labels = { brand_alignment: 'Brand Alignment', strategic_fit: 'Strategic Fit', execution_quality: 'Execution Quality',
  consistency: 'Consistency', impact: 'Impact', innovation: 'Innovation', clarity: 'Clarity',
  emotional_resonance: 'Emotional Resonance', memorability: 'Memorability', feasibility: 'Feasibility' };
let props, blocks;
if (fehler) {
  props = { Status: { select: { name: 'Freigabe' } },
    Feedback: { rich_text: rtChunks('⚠️ Automatisches Review fehlgeschlagen (' + fehler.slice(0, 300) + '). Bitte selbst prüfen.') } };
  blocks = [callout('Automatisches Review fehlgeschlagen – bitte selbst prüfen. ' + fehler, '⚠️', 'yellow_background')];
} else {
  const vals = KEYS.map(k => Math.max(1, Math.min(10, Number(data.kriterien[k]) || 1)));
  const score = Math.round(vals.reduce((a, b) => a + b, 0) / vals.length * 10) / 10;
  let status, hinweis = '';
  if (score >= 7) status = 'Freigabe';
  else if (job.versuch >= 3) { status = 'Freigabe'; hinweis = '⚠️ Nach 3 Versuchen noch unter 7 – bitte selbst entscheiden.\n\n'; }
  else status = 'Überarbeiten';
  const fb = hinweis + (status === 'Überarbeiten' ? data.feedback : ('Stärken: ' + data.staerken + (data.feedback ? '\n\nHinweise: ' + data.feedback : '')));
  props = { Status: { select: { name: status } }, Score: { number: score }, Feedback: { rich_text: rtChunks(fb).slice(0, 100) } };
  blocks = [
    { type: 'heading_3', heading_3: { rich_text: rtChunks(`⭐ Review CreativeDirector · Score ${score}/10 · ${status === 'Überarbeiten' ? 'Überarbeiten' : 'Freigabe empfohlen'}`) } },
    callout(data.staerken || '—', '💪', 'green_background'),
    ...KEYS.map((k, i) => ({ type: 'bulleted_list_item', bulleted_list_item: { rich_text: rtChunks(`${labels[k]}: ${vals[i]}/10`) } })),
  ];
  if (data.feedback) blocks.push(...mdToBlocks((hinweis ? hinweis : '') + data.feedback, 40));
}
const log = logEntry({ titel: `Review · ${job.titel}`, workflow: 'Review', status: fehler ? 'Fehler' : 'Erfolg', dauer,
  usage: r.usage, modell: r.model, fehler, lauf: job.lauf_id, agent_id: cd_id, job_id: job.job_id });
return [{ json: { job_id: job.job_id, props, blocks, log } }];

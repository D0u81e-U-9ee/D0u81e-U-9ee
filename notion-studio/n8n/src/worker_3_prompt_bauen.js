// Worker · Prompt für den zuständigen Agenten bauen
const job = $('Job auswählen').first().json;
const A = $('Agent laden').first().json.properties;
const pages = $input.all().map(i => i.json).filter(p => p && p.id);
const preds = pages.filter(p => p.id !== job.job_id);
const agent = {
  name: plain(A['Agent']), prompt: plain(A['System Prompt']),
  model: sel(A['Modell']) || CFG.defaultModel, effort: sel(A['Effort']) || 'medium',
  web: A['Websuche']?.checkbox === true, review: A['Review durch CD']?.checkbox === true,
  maxTokens: num(A['Max Tokens']) || 16000,
};
if (!agent.prompt) throw new Error('Agent ' + agent.name + ' hat keinen System Prompt');
let user = `# JOB: ${job.titel}\nHeute: ${berlinDate()}\n\n## Auftrag\n${job.auftrag || '(Kein Auftragstext – arbeite auf Basis des Titels und triff sinnvolle Annahmen.)'}\n`;
if (preds.length) {
  user += `\n## Ergebnisse vorheriger Jobs (nutze sie als Grundlage)\n`;
  for (const p of preds) user += `\n### ${plain(p.properties['Job'])}\n${plain(p.properties['Ergebnis']) || plain(p.properties['Zusammenfassung']) || '(kein Ergebnis)'}\n`;
}
if (job.status === 'Überarbeiten') {
  user += `\n## ÜBERARBEITUNG – Versuch ${job.versuch + 1}\n### Feedback (vollständig umsetzen)\n${job.feedback || '(kein Feedback angegeben)'}\n### Dein bisheriges Ergebnis\n${job.ergebnis_alt || '(nicht vorhanden)'}\n\nLiefere das komplette überarbeitete Ergebnis, nicht nur die Änderungen.\n`;
}
user += `\nAntworte genau in der Struktur: ## Ergebnis / ## Zusammenfassung / ## Offene Fragen`;
const body = withFallback({
  model: agent.model, max_tokens: agent.maxTokens, system: agent.prompt,
  messages: [{ role: 'user', content: user }], output_config: { effort: agent.effort },
});
if (agent.web) body.tools = [{ type: 'web_search_20260209', name: 'web_search', max_uses: 5 }];
return [{ json: { claude_body: body, agent } }];

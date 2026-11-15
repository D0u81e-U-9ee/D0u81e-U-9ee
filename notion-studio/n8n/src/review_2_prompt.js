// Review · Prompt für den CreativeDirector (strukturierte JSON-Antwort)
const job = $('Review-Job auswählen').first().json;
const agents = $input.first().json.results || [];
const cd = agents.find(a => plain(a.properties['Agent']) === 'CreativeDirector');
if (!cd) throw new Error('Agent „CreativeDirector“ nicht im Agenten-Team gefunden');
const autor = agents.find(a => a.id === job.agent_id);
const A = cd.properties;
const KEYS = ['brand_alignment', 'strategic_fit', 'execution_quality', 'consistency', 'impact',
  'innovation', 'clarity', 'emotional_resonance', 'memorability', 'feasibility'];
const schema = {
  type: 'object', additionalProperties: false, required: ['kriterien', 'entscheidung', 'staerken', 'feedback'],
  properties: {
    kriterien: { type: 'object', additionalProperties: false, required: KEYS,
      properties: Object.fromEntries(KEYS.map(k => [k, { type: 'integer' }])) },
    entscheidung: { type: 'string', enum: ['freigabe', 'ueberarbeiten'] },
    staerken: { type: 'string' },
    feedback: { type: 'string' },
  },
};
const user = `Prüfe das Ergebnis dieses Jobs als Creative Director.

# JOB: ${job.titel}
Agent: ${autor ? plain(autor.properties['Agent']) : 'unbekannt'} · Versuch ${job.versuch}

## Auftrag
${job.auftrag || '(kein Auftragstext)'}

## Ergebnis
${job.ergebnis || '(leer)'}

Bewerte jedes der 10 Kriterien mit einer ganzen Zahl von 1 bis 10.
- "entscheidung": "freigabe", wenn der Durchschnitt mindestens 7 ist, sonst "ueberarbeiten".
- "staerken": 1–3 Sätze, was gut ist.
- "feedback": bei "ueberarbeiten" max. 5 konkrete, umsetzbare Punkte als Markdown-Liste, nach Wirkung sortiert; bei "freigabe" kurze Hinweise für Achim (oder leer).`;
const body = withFallback({
  model: sel(A['Modell']) || CFG.defaultModel, max_tokens: 8000, system: plain(A['System Prompt']),
  messages: [{ role: 'user', content: user }],
  output_config: { effort: sel(A['Effort']) || 'high', format: { type: 'json_schema', schema } },
});
return [{ json: { claude_body: body, cd_id: cd.id, KEYS } }];

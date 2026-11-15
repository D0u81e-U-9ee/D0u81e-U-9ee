// Dispatcher · ALEX plant die Anfrage (strukturierte JSON-Antwort)
const a = $('Anfrage auswählen').first().json;
const seite = blocksToText($('Seiteninhalt laden').first().json.results);
const agents = ($input.first().json.results || []).map(p => ({ id: p.id, name: plain(p.properties['Agent']),
  rolle: plain(p.properties['Rolle']), kann: plain(p.properties['Kann']), prompt: plain(p.properties['System Prompt']),
  modell: sel(p.properties['Modell']), effort: sel(p.properties['Effort']) })).filter(x => x.name);
const alex = agents.find(x => x.name === 'ALEX');
if (!alex) throw new Error('Agent „ALEX“ ist nicht aktiv');
const names = agents.map(x => x.name);
const schema = {
  type: 'object', additionalProperties: false, required: ['verstanden', 'rueckfrage', 'plan', 'jobs'],
  properties: {
    verstanden: { type: 'string', description: 'Ziel der Anfrage in 1–2 Sätzen' },
    rueckfrage: { type: 'string', description: 'Eine präzise Rückfrage an Achim – oder leerer String' },
    plan: { type: 'string', description: 'Kurzer Plan in 1–3 Sätzen' },
    jobs: { type: 'array', items: { type: 'object', additionalProperties: false,
      required: ['titel', 'agent', 'auftrag', 'prioritaet', 'faellig', 'wartet_auf'],
      properties: {
        titel: { type: 'string' }, agent: { type: 'string', enum: names }, auftrag: { type: 'string' },
        prioritaet: { type: 'string', enum: ['Hoch', 'Normal', 'Niedrig'] },
        faellig: { type: 'string', description: 'YYYY-MM-DD oder leerer String' },
        wartet_auf: { type: 'array', items: { type: 'integer' }, description: '0-basierte Indizes früherer Jobs dieser Liste' },
      } } },
  },
};
const user = `NEUE ANFRAGE VON ACHIM – plane die Umsetzung. Heute: ${berlinDate()} (${isoToday()})

# ${a.titel}
- Art: ${a.art || '–'} · Priorität: ${a.prio} · Gewünscht bis: ${a.gewuenscht || '–'}

## Beschreibung
${a.beschreibung || '–'}

## Seiteninhalt (Details von Achim)
${seite || '–'}
${a.rueckfrage ? `\n## Deine frühere Rückfrage\n${a.rueckfrage}\n## Achims Antwort\n${a.antwort || '(keine Antwort – plane mit sinnvollen Annahmen)'}\n` : ''}
## Aktive Agenten
${agents.map(x => `- ${x.name} – ${x.rolle}: ${x.kann}`).join('\n')}

Regeln: 1–5 Jobs; „rueckfrage“ nur, wenn ohne Antwort nichts Sinnvolles entstehen kann (dann „jobs“ leer). Der Auftrag jedes Jobs muss für sich allein verständlich sein.`;
const body = withFallback({
  model: alex.modell || CFG.defaultModel, max_tokens: 16000, system: alex.prompt,
  messages: [{ role: 'user', content: user }],
  output_config: { effort: alex.effort || 'high', format: { type: 'json_schema', schema } },
});
return [{ json: { claude_body: body, agents: agents.map(({ id, name }) => ({ id, name })), alex_id: alex.id } }];

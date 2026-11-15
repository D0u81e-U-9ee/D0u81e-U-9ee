// Review · ältesten Job im Status „Review“ nehmen
const p = ($input.first().json.results || [])[0];
if (!p) return [];
const P = p.properties;
return [{ json: {
  job_id: p.id, titel: plain(P['Job']), auftrag: plain(P['Auftrag']), ergebnis: plain(P['Ergebnis']),
  versuch: num(P['Versuch']) || 1, agent_id: relIds(P['Agent'])[0] || null,
  start: new Date().toISOString(), lauf_id: String($execution.id),
} }];

// Worker · Job auswählen: höchste Priorität, keine offenen Vorgänger, Agent gesetzt
const res = $input.first().json.results || [];
const rank = { Hoch: 0, Normal: 1, Niedrig: 2 };
const cands = res.map(p => ({
  p,
  prio: rank[sel(p.properties['Priorität'])] ?? 1,
  order: num(p.properties['Reihenfolge']) ?? 99,
  open: num(p.properties['Offene Vorgänger']) ?? 0,
  agent: relIds(p.properties['Agent'])[0],
})).filter(c => c.agent && !c.open).sort((a, b) => a.prio - b.prio || a.order - b.order);
if (!cands.length) return [];
const job = cands[0].p, P = job.properties;
return [{ json: {
  job_id: job.id, titel: plain(P['Job']), status: sel(P['Status']),
  auftrag: plain(P['Auftrag']), feedback: plain(P['Feedback']), ergebnis_alt: plain(P['Ergebnis']),
  versuch: num(P['Versuch']) || 0, agent_id: cands[0].agent,
  vorgaenger: relIds(P['Wartet auf']), start: new Date().toISOString(), lauf_id: String($execution.id),
} }];

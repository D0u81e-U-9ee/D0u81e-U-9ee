// Dispatcher · Abhängigkeiten setzen (Index → neue Job-ID), danach „Bereit“
const plan = $('Jobs aufteilen').all().map(i => i.json);
const created = $input.all().map(i => i.json.id);
return plan.filter(j => j.deps.length && created[j.index]).map(j => ({ json: {
  id: created[j.index],
  props: { 'Wartet auf': { relation: j.deps.map(d => ({ id: created[d] })).filter(x => x.id) }, Status: { select: { name: 'Bereit' } } },
} }));

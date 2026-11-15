// Dispatcher · Plan auswerten → Anfrage-Update, Job-Entwürfe, Protokoll
const a = $('Anfrage auswählen').first().json;
const { agents, alex_id } = $('Plan-Prompt bauen').first().json;
const r = $input.first().json;
const dauer = Math.round((Date.now() - new Date(a.start)) / 1000);
let fehler = claudeError(r), plan = null;
if (!fehler) { try { plan = JSON.parse(claudeText(r)); } catch (e) { fehler = 'Antwort ist kein gültiges JSON: ' + e.message; } }
const byName = Object.fromEntries(agents.map(x => [x.name, x.id]));
let anfrage_props, jobs = [];
if (fehler) {
  anfrage_props = { Status: { select: { name: 'Rückfrage' } },
    'Rückfrage von ALEX': { rich_text: rtChunks('⚠️ Ich konnte die Anfrage nicht planen (technischer Fehler: ' + fehler.slice(0, 400) + '). Setze den Status wieder auf „Neu“, um es erneut zu versuchen.') } };
} else if (plan.rueckfrage && plan.rueckfrage.trim()) {
  anfrage_props = { Status: { select: { name: 'Rückfrage' } },
    'Rückfrage von ALEX': { rich_text: rtChunks(plan.rueckfrage) }, 'ALEX-Plan': { rich_text: rtChunks(plan.verstanden) } };
} else {
  const valid = (plan.jobs || []).filter(j => byName[j.agent]).slice(0, 5);
  jobs = valid.map((j, i) => {
    const deps = (j.wartet_auf || []).filter(d => Number.isInteger(d) && d >= 0 && d < i);
    const props = {
      Job: { title: rtChunks(j.titel.slice(0, 200)) },
      Status: { select: { name: deps.length ? 'Entwurf' : 'Bereit' } },
      Agent: { relation: [{ id: byName[j.agent] }] },
      Anfrage: { relation: [{ id: a.anfrage_id }] },
      Priorität: { select: { name: j.prioritaet || a.prio } },
      Reihenfolge: { number: i + 1 }, Versuch: { number: 0 },
      Auftrag: { rich_text: rtChunks(j.auftrag).slice(0, 100) },
    };
    if (/^\d{4}-\d{2}-\d{2}$/.test(j.faellig || '')) props['Fällig'] = { date: { start: j.faellig } };
    else if (a.gewuenscht) props['Fällig'] = { date: { start: a.gewuenscht } };
    if (a.projekt_id) props.Projekt = { relation: [{ id: a.projekt_id }] };
    return { index: i, deps, body: { parent: { database_id: CFG.db.jobs }, properties: props,
      children: [callout('Auftrag von ALEX an ' + j.agent, '📋', 'blue_background'), ...mdToBlocks(j.auftrag, 60)] } };
  });
  const liste = valid.map((j, i) => `${i + 1}. ${j.titel} → ${j.agent}`).join('\n');
  anfrage_props = { Status: { select: { name: jobs.length ? 'In Arbeit' : 'Wartet auf dich' } },
    'ALEX-Plan': { rich_text: rtChunks(`${plan.verstanden}\n\n${plan.plan}\n\n${liste}`).slice(0, 100) } };
}
const log = logEntry({ titel: `Dispatcher · ${a.titel}`, workflow: 'Dispatcher', status: fehler ? 'Fehler' : 'Erfolg', dauer,
  usage: r.usage, modell: r.model, fehler, lauf: a.lauf_id, agent_id: alex_id });
return [{ json: { anfrage_id: a.anfrage_id, anfrage_props, jobs, log } }];

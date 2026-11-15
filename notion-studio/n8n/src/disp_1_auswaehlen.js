// Dispatcher · älteste Anfrage mit Status „Neu“
const p = ($input.first().json.results || [])[0];
if (!p) return [];
const P = p.properties;
return [{ json: {
  anfrage_id: p.id, titel: plain(P['Anfrage']), art: sel(P['Art']), prio: sel(P['Priorität']) || 'Normal',
  gewuenscht: dateStr(P['Gewünscht bis']), beschreibung: plain(P['Beschreibung']),
  rueckfrage: plain(P['Rückfrage von ALEX']), antwort: plain(P['Antwort an ALEX']),
  projekt_id: relIds(P['Projekt'])[0] || null, kunde_id: relIds(P['Kunde'])[0] || null,
  start: new Date().toISOString(), lauf_id: String($execution.id),
} }];

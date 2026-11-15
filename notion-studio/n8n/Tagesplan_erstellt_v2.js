// ALEX 2.0 – Node "Tagesplan erstellt" (v2): vollständiges Logging
const a = $('Aufgaben vorbereiten').first().json;
const ausfuehrungszeit = Math.round((Date.now() - Date.parse(a.timestamp)) / 1000);
return [{ json: {
  log_titel: "Tagesplan erstellt – " + new Date().toLocaleString('de-DE', { timeZone: 'Europe/Berlin' }),
  agent: "ALEX",
  aktion: "Tagesplan erstellt",
  status: "Erfolg",
  details: `${a.offene_aufgaben} Aufgaben (${a.ueberfaellige_aufgaben} überfällig), ${a.aktive_agenten} Agenten, ${a.pending_reviews} Reviews offen`,
  verarbeitete_items: a.offene_aufgaben,
  ausfuehrungszeit,
}}];
// Im Node "Log Eintrag erstellt":
//   Verarbeitete Items      → {{ $json.verarbeitete_items }}
//   Ausführungszeit (Sek)   → {{ $json.ausfuehrungszeit }}
//   Name (Titel)            → {{ $json.log_titel }}   (Feld im Expression-Modus, KEIN führendes "=" als Text!)
// Im Node "Tagesplan erstellen":  Name → Tagesplan {{ $('Aufgaben vorbereiten').first().json.datum_heute }}

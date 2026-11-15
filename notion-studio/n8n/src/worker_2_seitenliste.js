// Worker · Seiten, die geladen werden: der Job selbst + Vorgänger-Jobs (deren Ergebnisse sind Kontext)
const job = $('Job auswählen').first().json;
return [{ json: { id: job.job_id } }, ...job.vorgaenger.map(id => ({ json: { id } }))];

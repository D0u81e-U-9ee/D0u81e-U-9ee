// ALEX 2.0 – Node "Aufgaben vorbereiten" (v2)
// Eingänge über Merge: "Aufgaben laden", "alleAgenten", optional "Creative Reviews laden"
// Fixes ggü. v1: Specialization ist Multi-Select · Agenten nur über "Full Name" erkennen
// (Aufgaben haben auch ein "Agent"-Feld) · Status "Vorschlag" wird nicht eingeplant
// Neu: Überfällig/Heute/Studio-Projekt, Creative Review Queue, Vorschläge des TaskCreator
const allItems = $input.all();
const P = (item) => item.json.properties || {};
const text = (p) => (p?.title || p?.rich_text || []).map(t => t.plain_text).join("");

const taskItems   = allItems.filter(i => P(i).Kategorie && P(i).Priorität);
const agentItems  = allItems.filter(i => P(i)['Full Name']);
const reviewItems = allItems.filter(i => P(i)['Review Priority']);

const heute = new Date();
const heuteISO = heute.toISOString().slice(0, 10);

// 1. AUFGABEN
let aufgabenListe = "", vorschlaege = 0, offeneCount = 0, ueberfaellig = 0;
const prioRang = { "Hoch": 0, "Mittel": 1, "Niedrig": 2 };
const offene = [];
taskItems.forEach(item => {
  const p = P(item);
  const name = text(p.Name) || "";
  const status = p.Status?.select?.name || "Offen";
  if (!name || status === "Erledigt") return;
  if (status === "Vorschlag") { vorschlaege++; return; }
  const deadline = p.Deadline?.date?.start || "";
  const overdue = deadline && deadline < heuteISO;
  offene.push({
    name, status, deadline, overdue,
    prio: p.Priorität?.select?.name || "Mittel",
    kategorie: p.Kategorie?.select?.name || "",
    heute: p.Heute?.checkbox === true,
  });
});
offene.sort((a, b) => (b.overdue - a.overdue) || (b.heute - a.heute) ||
  ((prioRang[a.prio] ?? 1) - (prioRang[b.prio] ?? 1)) || (a.deadline || "9999").localeCompare(b.deadline || "9999"));
offene.forEach(t => {
  offeneCount++;
  if (t.overdue) ueberfaellig++;
  let z = `- ${t.overdue ? "⚠️ ÜBERFÄLLIG " : ""}${t.heute ? "☀️ " : ""}${t.name} [${t.prio}] | Status: ${t.status}`;
  if (t.kategorie) z += ` | Kategorie: ${t.kategorie}`;
  if (t.deadline) z += ` | Deadline: ${t.deadline}`;
  aufgabenListe += z + "\n";
});

// 2. AGENTEN
let agentenListe = "", activeAgents = 0;
agentItems.forEach(item => {
  const p = P(item);
  const title = text(p.Agent);
  if (p.Status?.select?.name !== "Active" || !title) return;
  activeAgents++;
  const spec = (p.Specialization?.multi_select || []).map(o => o.name).join(", ");
  agentenListe += `- **${title}** (${text(p['Full Name'])}): ${text(p.Role)}` + (spec ? ` | Spezialisierung: ${spec}` : "") + "\n";
});
if (!agentenListe) agentenListe = "Keine aktiven Agenten gefunden";

// 3. CREATIVE REVIEWS
let reviewsListe = "", pendingReviews = 0;
reviewItems.forEach(item => {
  const p = P(item);
  if (!["Pending Review", "In Review"].includes(p.Status?.select?.name)) return;
  pendingReviews++;
  reviewsListe += `- ${text(p.Name) || "Unbenannt"} (${p['Asset Type']?.select?.name || ""}) von ${p['Created By']?.select?.name || "?"} [${p['Review Priority']?.select?.name || "Medium"}]\n`;
});

// 4. DATUM & PROMPT
const heuteFormatiert = heute.toLocaleDateString('de-DE', { weekday: 'long', year: 'numeric', month: 'long', day: 'numeric', timeZone: 'Europe/Berlin' });
const prompt = `SYSTEM: Du bist ALEX, Master Coordinator von ALEX 2.0 (Achim Gutmann Media Manufaktur). Datum: ${heuteFormatiert}

VERFÜGBARE AGENTEN (nutze exakt diese Namen!):
${agentenListe}
AUFGABEN (bereits priorisiert, ${offeneCount} offen, davon ${ueberfaellig} überfällig):
${aufgabenListe || "Keine offenen Aufgaben"}
CREATIVE REVIEW QUEUE: ${pendingReviews} offen
${reviewsListe || "Keine offenen Reviews"}
VORSCHLÄGE DES TASKCREATOR (warten auf Freigabe): ${vorschlaege}

REGELN:
- Weise JEDE Top-Aufgabe dem PASSENDEN Agenten aus der Liste zu. Kategorien (z. B. "Alex System") sind KEINE Agenten.
- Überfällige und mit ☀️ markierte Aufgaben zuerst. Max. 6 Std. Fokusarbeit.
- Wenn mehr als 5 Creative Reviews offen sind: CreativeDirector-Review als Top-Aufgabe.

FORMAT (max. 2000 Zeichen):
📋 ${heuteFormatiert}
🎯 [Tagesziel in 1 Satz]
⚠️ [Überfällig/Risiken – nur falls vorhanden]
✅ Top 3:
1. [Aufgabe] → Agent: **[AgentName]** – [Zeit]
2. [Aufgabe] → Agent: **[AgentName]** – [Zeit]
3. [Aufgabe] → Agent: **[AgentName]** – [Zeit]
🔍 Creative Reviews: ${pendingReviews} offen${vorschlaege ? `\n💡 ${vorschlaege} Aufgaben-Vorschläge warten auf Freigabe` : ""}
💪 [kurzer Motivationssatz]`;

return [{ json: {
  prompt, offene_aufgaben: offeneCount, ueberfaellige_aufgaben: ueberfaellig,
  aktive_agenten: activeAgents, pending_reviews: pendingReviews, vorschlaege,
  datum_heute: heuteFormatiert, datum_iso: heuteISO, timestamp: heute.toISOString(),
}}];

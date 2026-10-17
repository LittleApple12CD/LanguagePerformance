async function loadCsv(path) {
  const res = await fetch(path);
  const text = await res.text();
  const lines = text.trim().split('\n');
  const rows = [];
  for (let i = 1; i < lines.length; i++) {
    const line = lines[i].trim();
    if (!line) continue;
    rows.push(line.split(',').map(s => parseFloat(s.trim())));
  }
  return rows;
}
use std::fs;

pub fn load_csv_str(content: &str) -> Vec<Vec<f64>> {
    let mut rows = Vec::new();
    for (i, line) in content.lines().enumerate() {
        if i == 0 || line.trim().is_empty() { continue; }
        let row: Vec<f64> = line.split(',')
            .map(|s| s.trim().parse().unwrap_or(0.0))
            .collect();
        rows.push(row);
    }
    rows
}
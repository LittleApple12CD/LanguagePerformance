use macroquad::prelude::*;
use crate::{nbody::NBody, squares2d::Squares2D, cubes3d::Cubes3D, measurer::Measurer};

pub const WIDTH: f32 = 1280.0;
pub const HEIGHT: f32 = 720.0;
pub const DURATION: f64 = 30.0;
pub const STACK_NAME: &str = "rust-macroquad";

pub struct Game {
    pub nbody: NBody,
    pub squares: Squares2D,
    pub cubes: Cubes3D,
    pub scene_idx: usize,
    pub paused: bool,
    pub hud_show: bool,
    pub quit: bool,
    frame_times: [f64; 240],
    frame_count: usize,
    frame_head: usize,
    max_fps: f64,
    pub startup_ms: f64,
    first_frame: bool,
    pub measurer: Measurer,
    last_time: f64,
    font: Font,
}

impl Game {
    pub fn new(particles: Vec<Vec<f64>>, squares: Vec<Vec<f64>>, cubes: Vec<Vec<f64>>, font: Font) -> Self {
        Self {
            nbody: NBody::new(particles, 1000),
            squares: Squares2D::new(squares, 1000),
            cubes: Cubes3D::new(cubes, 200),
            scene_idx: 0, paused: false, hud_show: true, quit: false,
            frame_times: [0.0; 240], frame_count: 0, frame_head: 0,
            max_fps: 0.0, startup_ms: 0.0, first_frame: true,
            measurer: Measurer::new(DURATION),
            last_time: get_time(),
            font,
        }
    }

    pub fn update(&mut self) {
        let now = get_time();
        let dt = (now - self.last_time).max(0.001);
        self.last_time = now;
        let dt_ms = dt * 1000.0;

        self.frame_times[self.frame_head] = dt_ms;
        self.frame_head = (self.frame_head + 1) % 240;
        if self.frame_count < 240 { self.frame_count += 1; }
        let inst_fps = 1000.0 / dt_ms;
        if inst_fps > self.max_fps { self.max_fps = inst_fps; }

        if self.measurer.active { self.measurer.tick(dt); }
        if self.measurer.just_finished {
            self.measurer.just_finished = false;
            self.write_result();
        }

        // 输入
        if is_key_pressed(KeyCode::Escape) {
            if !self.measurer.frames.is_empty() { self.write_result(); }
            self.quit = true;
        }
        if is_key_pressed(KeyCode::Key1) { self.scene_idx = 0; self.reset_max(); }
        if is_key_pressed(KeyCode::Key2) { self.scene_idx = 1; self.reset_max(); }
        if is_key_pressed(KeyCode::Key3) { self.scene_idx = 2; self.reset_max(); }
        if is_key_pressed(KeyCode::Space) { self.paused = !self.paused; }
        if is_key_pressed(KeyCode::R) {
            match self.scene_idx {
                0 => self.nbody.reset(),
                1 => self.squares.reset(),
                _ => self.cubes.reset(),
            }
            self.reset_max();
        }
        if is_key_pressed(KeyCode::I) { self.hud_show = !self.hud_show; }
        if is_key_pressed(KeyCode::S) {
            if !self.measurer.active { self.measurer.start(); self.reset_max(); }
        }

        if !self.paused {
            match self.scene_idx {
                0 => self.nbody.update(dt),
                1 => self.squares.update(dt),
                _ => self.cubes.update(dt),
            }
        }
    }

    pub fn draw(&self) {
        clear_background(Color::new(20.0/255.0, 20.0/255.0, 28.0/255.0, 1.0));
        match self.scene_idx {
            0 => self.nbody.draw(),
            1 => self.squares.draw(),
            _ => self.cubes.draw(),
        }
        if self.hud_show { self.draw_hud(); }
    }

    fn draw_hud(&self) {
        let s = self.stats();
        let test = ["T1 - N-Body", "T2 - 2D Squares", "T3 - 3D Cubes"][self.scene_idx];
        let objects = [self.nbody.n, self.squares.n, self.cubes.n][self.scene_idx];
        let lines = [
            format!("Stack:    {}", STACK_NAME),
            format!("Test:     {}", test),
            format!("FPS:      {:6.1}", s[0]),
            format!("Frame:    {:5.2} ms (avg) / {:5.2} ms (p99)", 1000.0 / s[0], s[1]),
            format!("Max FPS:  {:6.1}", s[3]),
            format!("1% Low:   {:6.1}", s[2]),
            format!("Objects:  {}", objects),
            format!("Paused:   {}", self.paused),
            if self.measurer.active { format!("Measuring: {}s", self.measurer.left() as i32) } else { "Measuring: no".to_string() },
            "[1] T1  [2] T2  [3] T3  [Space] Pause  [R] Reset".to_string(),
            "[I] Info  [S] Measure  [Esc] Quit".to_string(),
        ];
        for (i, line) in lines.iter().enumerate() {
            draw_text_ex(line, 8.0, 12.0 + (i as f32) * 18.0, TextParams {
                font: Some(&self.font), font_size: 14, color: Color::new(0.9, 0.9, 0.9, 1.0), ..Default::default()
            });
        }
    }

    fn stats(&self) -> [f64; 4] {
        if self.frame_count == 0 { return [0.0, 0.0, 0.0, self.max_fps]; }
        let mut sorted: Vec<f64> = self.frame_times[..self.frame_count].to_vec();
        sorted.sort_by(|a, b| a.partial_cmp(b).unwrap());
        let avg: f64 = sorted.iter().sum::<f64>() / self.frame_count as f64;
        let p99 = sorted[((self.frame_count as f64 * 0.99) as usize).min(self.frame_count - 1)];
        let k = ((self.frame_count as f64 * 0.01) as usize).max(1);
        let slow_sum: f64 = sorted[self.frame_count - k..].iter().sum();
        let low1 = 1000.0 / (slow_sum / k as f64);
        [1000.0 / avg, p99, low1, self.max_fps]
    }

    fn reset_max(&mut self) { self.max_fps = 0.0; self.frame_head = 0; self.frame_count = 0; }

    fn write_result(&self) {
        let s = self.stats();
        let test = ["T1", "T2", "T3"][self.scene_idx];
        let objects = [self.nbody.n, self.squares.n, self.cubes.n][self.scene_idx];
        let max_ms = self.frame_times[..self.frame_count].iter().cloned().fold(0.0f64, f64::max);
        let json = format!(
            r#"{{
  "stack": "{}",
  "test": "{}",
  "objects": {},
  "resolution": [{}, {}],
  "avg_fps": {},
  "min_fps": {},
  "max_fps": {},
  "avg_frame_ms": {},
  "p99_frame_ms": {},
  "one_percent_low_fps": {},
  "startup_ms": {},
  "duration_s": {},
  "runtime": "Rust + Macroquad",
  "os": "Windows"
}}
"#, STACK_NAME, test, objects, WIDTH as i32, HEIGHT as i32,
            s[0], 1000.0 / max_ms, s[3], 1000.0 / s[0], s[1], s[2], self.startup_ms, DURATION);
        let _ = std::fs::create_dir_all("results");
        let fn_name = format!("results/{}-{}-{}.json", STACK_NAME, test, objects);
        let _ = std::fs::write(fn_name, json);
    }
}
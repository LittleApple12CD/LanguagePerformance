use macroquad::prelude::*;

pub struct Squares2D {
    pub n: usize,
    pub x: Vec<f64>, pub y: Vec<f64>,
    pub vx: Vec<f64>, pub vy: Vec<f64>,
    pub size: Vec<f64>,
    pub colors: Vec<Color>,
    pub x0: i32, pub x1: i32, pub y0: i32, pub y1: i32,
    all: Vec<Vec<f64>>,
}

impl Squares2D {
    pub fn new(data: Vec<Vec<f64>>, n: usize) -> Self {
        let mut s = Self {
            n, x: vec![0.0; n], y: vec![0.0; n], vx: vec![0.0; n], vy: vec![0.0; n],
            size: vec![0.0; n], colors: vec![Color::new(0.0, 0.0, 0.0, 1.0); n],
            x0: (crate::game::WIDTH as i32) / 2 - 200, x1: (crate::game::WIDTH as i32) / 2 + 200,
            y0: (crate::game::HEIGHT as i32) / 2 - 200, y1: (crate::game::HEIGHT as i32) / 2 + 200,
            all: data,
        };
        s.reset(); s
    }

    pub fn reset(&mut self) {
        for i in 0..self.n {
            self.x[i] = self.all[i][0] + self.x0 as f64;
            self.y[i] = self.all[i][1] + self.y0 as f64;
            self.vx[i] = self.all[i][2]; self.vy[i] = self.all[i][3];
            self.size[i] = self.all[i][4];
            self.colors[i] = Color::new(self.all[i][5] as f32 / 255.0, self.all[i][6] as f32 / 255.0, self.all[i][7] as f32 / 255.0, 1.0);
        }
    }

    pub fn update(&mut self, _dt: f64) {
        for i in 0..self.n {
            self.x[i] += self.vx[i]; self.y[i] += self.vy[i];
            if self.x[i] < self.x0 as f64 || self.x[i] > self.x1 as f64 {
                self.vx[i] = -self.vx[i];
                self.x[i] = self.x[i].clamp(self.x0 as f64, self.x1 as f64);
            }
            if self.y[i] < self.y0 as f64 || self.y[i] > self.y1 as f64 {
                self.vy[i] = -self.vy[i];
                self.y[i] = self.y[i].clamp(self.y0 as f64, self.y1 as f64);
            }
        }
    }

    pub fn draw(&self) {
        for i in 0..self.n {
            draw_rectangle(self.x[i] as f32, self.y[i] as f32, self.size[i] as f32, self.size[i] as f32, self.colors[i]);
        }
    }
}
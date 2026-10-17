use macroquad::prelude::*;

pub struct NBody {
    pub n: usize,
    pub x: Vec<f64>, pub y: Vec<f64>,
    pub vx: Vec<f64>, pub vy: Vec<f64>,
    pub mass: Vec<f64>,
    pub g: f64, pub dt: f64, pub a_max: f64, pub scale: f64,
    all: Vec<Vec<f64>>,
    ax: Vec<f64>, ay: Vec<f64>,
}

impl NBody {
    pub fn new(data: Vec<Vec<f64>>, n: usize) -> Self {
        let mut b = Self {
            n, x: vec![0.0; n], y: vec![0.0; n],
            vx: vec![0.0; n], vy: vec![0.0; n], mass: vec![0.0; n],
            g: 0.0002, dt: 0.016, a_max: 3.0, scale: 0.4,
            all: data,
            ax: vec![0.0; n], ay: vec![0.0; n],
        };
        b.reset();
        b
    }

    pub fn reset(&mut self) {
        for i in 0..self.n {
            self.x[i] = self.all[i][0];
            self.y[i] = self.all[i][1];
            self.vx[i] = self.all[i][2];
            self.vy[i] = self.all[i][3];
            self.mass[i] = self.all[i][4];
        }
    }

    pub fn update(&mut self, _frame_dt: f64) {
        let n = self.n;
        let g = self.g; let dt = self.dt; let a_max = self.a_max;
        // 清零加速度
        for i in 0..n { self.ax[i] = 0.0; self.ay[i] = 0.0; }
        // O(N^2) 引力
        for i in 0..n {
            let xi = self.x[i]; let yi = self.y[i];
            let mut axi = 0.0; let mut ayi = 0.0;
            for j in 0..n {
                if i == j { continue; }
                let dx = self.x[j] - xi;
                let dy = self.y[j] - yi;
                let d2 = dx * dx + dy * dy;
                if d2 < 1e-12 { continue; }
                let inv = 1.0 / d2;
                let f = g * inv * inv.sqrt() * self.mass[j];
                axi += f * dx; ayi += f * dy;
            }
            self.ax[i] = axi; self.ay[i] = ayi;
        }
        for i in 0..n {
            let a2 = self.ax[i] * self.ax[i] + self.ay[i] * self.ay[i];
            if a2 > a_max * a_max {
                let s = a_max / a2.sqrt();
                self.ax[i] *= s; self.ay[i] *= s;
            }
            self.vx[i] += self.ax[i] * dt;
            self.vy[i] += self.ay[i] * dt;
            self.x[i] += self.vx[i] * dt;
            self.y[i] += self.vy[i] * dt;
        }
    }

    pub fn draw(&self) {
        let cx = crate::game::WIDTH as f64 / 2.0;
        let cy = crate::game::HEIGHT as f64 / 2.0;
        for i in 0..self.n {
            let px = (self.x[i] * self.scale * crate::game::WIDTH as f64 / 2.0 + cx) as f32;
            let py = (-self.y[i] * self.scale * crate::game::WIDTH as f64 / 2.0 + cy) as f32;
            if px < 0.0 || px >= crate::game::WIDTH || py < 0.0 || py >= crate::game::HEIGHT { continue; }
            draw_circle(px, py, 2.0, Color::new(180.0/255.0, 220.0/255.0, 1.0, 1.0));
        }
    }
}
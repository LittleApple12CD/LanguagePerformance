use macroquad::prelude::*;
use rayon::prelude::*;

pub struct NBody {
    pub n: usize,
    pub x: Vec<f64>, pub y: Vec<f64>,
    pub vx: Vec<f64>, pub vy: Vec<f64>,
    pub mass: Vec<f64>,
    pub g: f64, pub a_max: f64, pub scale: f64,
    all: Vec<Vec<f64>>,
    ax: Vec<f64>, ay: Vec<f64>,
}

impl NBody {
    pub fn new(data: Vec<Vec<f64>>, n: usize) -> Self {
        let mut b = Self {
            n, x: vec![0.0; n], y: vec![0.0; n],
            vx: vec![0.0; n], vy: vec![0.0; n], mass: vec![0.0; n],
            g: 0.0002, a_max: 3.0, scale: 0.4,
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

    pub fn update(&mut self, frame_dt: f64) {
        let n = self.n;
        let g = self.g; 
        // 限制最大步长
        let dt = frame_dt.min(0.016);
        let a_max = self.a_max;

        let xs = &self.x;
        let ys = &self.y;
        let ms = &self.mass;
        let axs = &mut self.ax;
        let ays = &mut self.ay;

        axs.par_iter_mut()
            .zip(ays.par_iter_mut())
            .enumerate()
            .for_each(|(i, (axi_out, ayi_out))| {
                let xi = xs[i];
                let yi = ys[i];
                let mut axi = 0.0;
                let mut ayi = 0.0;
                
                for j in 0..n {
                    let dx = xs[j] - xi;
                    let dy = ys[j] - yi;

                    let mut d2 = dx * dx + dy * dy;
                    if d2 < 0.0001 { d2 = 0.0001; }
                    
                    let inv = 1.0 / d2;
                    // 开方倒数，极其暴力
                    let f = g * inv * inv.sqrt() * ms[j];
                    axi += f * dx;
                    ayi += f * dy;
                }
                *axi_out = axi;
                *ayi_out = ayi;
            });

        // 串行更新位置
        for i in 0..n {
            let mut axi = self.ax[i];
            let mut ayi = self.ay[i];
            let a2 = axi * axi + ayi * ayi;
            if a2 > a_max * a_max {
                let s = a_max / a2.sqrt();
                axi *= s; ayi *= s;
            }
            self.vx[i] += axi * dt;
            self.vy[i] += ayi * dt;
            self.x[i] += self.vx[i] * dt;
            self.y[i] += self.vy[i] * dt;
        }
    }

    pub fn draw(&self) {
        let cx = crate::game::WIDTH as f64 / 2.0;
        let cy = crate::game::HEIGHT as f64 / 2.0;
        let color = Color::new(180.0/255.0, 220.0/255.0, 1.0, 1.0);
        for i in 0..self.n {
            let px = (self.x[i] * self.scale * crate::game::WIDTH as f64 / 2.0 + cx) as f32;
            let py = (-self.y[i] * self.scale * crate::game::WIDTH as f64 / 2.0 + cy) as f32;
            if px < 0.0 || px >= crate::game::WIDTH || py < 0.0 || py >= crate::game::HEIGHT { continue; }

            draw_rectangle(px - 2.0, py - 2.0, 4.0, 4.0, color);
        }
    }
}
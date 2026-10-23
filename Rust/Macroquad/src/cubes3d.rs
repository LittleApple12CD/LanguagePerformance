use macroquad::prelude::*;

pub struct Cubes3D {
    pub n: usize,
    pub px: Vec<f64>, pub py: Vec<f64>, pub pz: Vec<f64>,
    pub ax: Vec<f64>, pub ay: Vec<f64>, pub az: Vec<f64>,
    pub speed: Vec<f64>,
    pub colors: Vec<Color>,
    pub angles: Vec<f64>,
    all: Vec<Vec<f64>>,
    camera: Camera3D,
}

impl Cubes3D {
    pub fn new(data: Vec<Vec<f64>>, n: usize) -> Self {
        let mut c = Self {
            n,
            px: vec![0.0; n], py: vec![0.0; n], pz: vec![0.0; n],
            ax: vec![0.0; n], ay: vec![0.0; n], az: vec![0.0; n],
            speed: vec![0.0; n],
            colors: vec![Color::new(0.0, 0.0, 0.0, 1.0); n],
            angles: vec![0.0; n],
            all: data,
            camera: Camera3D {
                position: vec3(0.0, 0.0, -8.0),
                up: vec3(0.0, 1.0, 0.0),
                target: vec3(0.0, 0.0, 0.0),
                fovy: 60.0f32.to_radians(),
                projection: Projection::Perspective,
                ..Default::default()
            },
        };
        c.reset(); c
    }

    pub fn reset(&mut self) {
        for i in 0..self.n {
            self.px[i] = self.all[i][0]; self.py[i] = self.all[i][1]; self.pz[i] = self.all[i][2];
            self.ax[i] = self.all[i][3]; self.ay[i] = self.all[i][4]; self.az[i] = self.all[i][5];
            self.speed[i] = self.all[i][6];
            self.colors[i] = Color::new(self.all[i][7] as f32 / 255.0, self.all[i][8] as f32 / 255.0, self.all[i][9] as f32 / 255.0, 1.0);
            self.angles[i] = 0.0;
        }
    }

    pub fn update(&mut self, _dt: f64) {
        let t = macroquad::prelude::get_time();
        for i in 0..self.n { 
            self.angles[i] = self.speed[i] * t; 
        }
    }

    pub fn draw(&self) {
        set_camera(&self.camera);
        let s = 0.25f32;

        let verts = [
            vec3(-s, -s, -s), vec3(s, -s, -s), vec3(s, s, -s), vec3(-s, s, -s),
            vec3(-s, -s, s), vec3(s, -s, s), vec3(s, s, s), vec3(-s, s, s),
        ];
        let edges = [
            (0,1),(1,2),(2,3),(3,0),
            (4,5),(5,6),(6,7),(7,4),
            (0,4),(1,5),(2,6),(3,7),
        ];

        for i in 0..self.n {
            let axis = vec3(self.ax[i] as f32, self.ay[i] as f32, self.az[i] as f32);
            let angle = self.angles[i] as f32;
            let pos = vec3(self.px[i] as f32, self.py[i] as f32, self.pz[i] as f32);

            let rot = Mat4::from_axis_angle(axis.normalize_or_zero(), angle);
            let model = Mat4::from_translation(pos) * rot;

            let mut transformed = [Vec3::ZERO; 8];
            for k in 0..8 {
                transformed[k] = model.transform_point3(verts[k]);
            }

            for &(a, b) in &edges {
                draw_line_3d(transformed[a], transformed[b], self.colors[i]);
            }
        }
        set_default_camera();
    }
}
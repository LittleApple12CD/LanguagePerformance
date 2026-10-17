use bevy::prelude::*;
use rayon::prelude::*;
use crate::game::{NBodyData, GameState, WIDTH, HEIGHT};

#[derive(Component)]
pub struct Particle;

pub fn setup_nbody(
    mut commands: Commands,
    mut meshes: ResMut<Assets<Mesh>>,
    mut materials: ResMut<Assets<ColorMaterial>>,
    data: Res<crate::CsvData>,
) {
    let mesh = meshes.add(Circle::new(2.0));
    let material = materials.add(Color::srgb(180.0/255.0, 220.0/255.0, 1.0));

    let mut ids = Vec::with_capacity(1000);
    for i in 0..1000 {
        let id = commands.spawn((
            Particle,
            Mesh2d(mesh.clone()),
            MeshMaterial2d(material.clone()),
            Transform::from_xyz(data.particles[i][0] as f32, data.particles[i][1] as f32, 0.0),
        )).id();
        ids.push(id);
    }

    let nbody = NBodyData {
        x: data.particles.iter().take(1000).map(|r| r[0] as f32).collect(),
        y: data.particles.iter().take(1000).map(|r| r[1] as f32).collect(),
        vx: data.particles.iter().take(1000).map(|r| r[2] as f32).collect(),
        vy: data.particles.iter().take(1000).map(|r| r[3] as f32).collect(),
        mass: data.particles.iter().take(1000).map(|r| r[4] as f32).collect(),
        g: 0.0002, dt: 0.016, a_max: 3.0, scale: 0.4,
        ax: vec![0.0; 1000], ay: vec![0.0; 1000],
        entity_ids: ids,
        initial_x: data.particles.iter().take(1000).map(|r| r[0] as f32).collect(),
        initial_y: data.particles.iter().take(1000).map(|r| r[1] as f32).collect(),
        initial_vx: data.particles.iter().take(1000).map(|r| r[2] as f32).collect(),
        initial_vy: data.particles.iter().take(1000).map(|r| r[3] as f32).collect(),
    };
    commands.insert_resource(nbody);
}

pub fn update_nbody(
    mut nbody: ResMut<NBodyData>,
    mut query: Query<&mut Transform, With<Particle>>,
    game: Res<GameState>,
    _time: Res<Time>,
) {
    if game.scene_idx != 0 || game.paused { return; }

    let n = nbody.x.len();
    let dt = nbody.dt;
    let g = nbody.g;
    let a_max = nbody.a_max;

    // 一次性拿到不可变切片，避免在内层循环反复借用 ResMut
    let xs = &nbody.x;
    let ys = &nbody.y;
    let ms = &nbody.mass;

    // 并行计算加速度，避免写 nbody.ax/ay 造成别名冲突
    let acc: Vec<(f32, f32)> = (0..n)
        .into_par_iter()
        .map(|i| {
            let xi = xs[i];
            let yi = ys[i];
            let mut axi = 0.0f32;
            let mut ayi = 0.0f32;
            for j in 0..n {
                if i == j { continue; }
                let dx = xs[j] - xi;
                let dy = ys[j] - yi;
                let d2 = dx * dx + dy * dy;
                if d2 < 1e-12 { continue; }
                let inv = 1.0 / d2;
                let f = g * inv * inv.sqrt() * ms[j];
                axi += f * dx;
                ayi += f * dy;
            }
            (axi, ayi)
        })
        .collect();

    // 写回 + 限幅 + 积分（串行，很轻）
    for i in 0..n {
        let (mut axi, mut ayi) = acc[i];
        let a2 = axi * axi + ayi * ayi;
        if a2 > a_max * a_max {
            let s = a_max / a2.sqrt();
            axi *= s;
            ayi *= s;
        }
        nbody.ax[i] = axi;
        nbody.ay[i] = ayi;
        nbody.vx[i] += axi * dt;
        nbody.vy[i] += ayi * dt;
        nbody.x[i] += nbody.vx[i] * dt;
        nbody.y[i] += nbody.vy[i] * dt;
    }

    let cx = WIDTH / 2.0;
    let cy = HEIGHT / 2.0;
    for (i, &id) in nbody.entity_ids.iter().enumerate() {
        if let Ok(mut transform) = query.get_mut(id) {
            transform.translation.x = nbody.x[i] * nbody.scale * WIDTH / 2.0 + cx;
            transform.translation.y = -nbody.y[i] * nbody.scale * WIDTH / 2.0 + cy;
        }
    }
}

pub fn reset_nbody(
    mut nbody: ResMut<NBodyData>,
    mut game: ResMut<GameState>,
) {
    if !game.reset_requested || game.scene_idx != 0 { return; }
    let n = nbody.x.len();
    for i in 0..n {
        nbody.x[i] = nbody.initial_x[i];
        nbody.y[i] = nbody.initial_y[i];
        nbody.vx[i] = nbody.initial_vx[i];
        nbody.vy[i] = nbody.initial_vy[i];
    }
    game.reset_requested = false;
}
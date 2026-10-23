use bevy::prelude::*;
use crate::game::{GameState, WIDTH, HEIGHT};

#[derive(Component)]
pub struct SquareData {
    pub vx: f32,
    pub vy: f32,
    pub initial_x: f32,
    pub initial_y: f32,
    pub initial_vx: f32,
    pub initial_vy: f32,
}

pub fn setup_squares(
    mut commands: Commands,
    mut meshes: ResMut<Assets<Mesh>>,
    mut materials: ResMut<Assets<ColorMaterial>>,
    data: Res<crate::CsvData>,
) {
    let x0 = WIDTH / 2.0 - 200.0;
    let y0 = HEIGHT / 2.0 - 200.0;

    for row in data.squares.iter().take(1000) {
        let size = row[4] as f32;
        let color = Color::srgb(row[5] as f32/255.0, row[6] as f32/255.0, row[7] as f32/255.0);
		
        let px = row[0] as f32 + x0;
        let py = row[1] as f32 + y0;
        let vx = row[2] as f32;
        let vy = row[3] as f32;

        commands.spawn((
            Mesh2d(meshes.add(Rectangle::new(size, size))),
            MeshMaterial2d(materials.add(color)),
            Transform::from_xyz(px, py, 0.0),
            SquareData {
                vx,
                vy,
                initial_x: px,
                initial_y: py,
                initial_vx: vx,
                initial_vy: vy,
            },
        ));
    }
}

pub fn update_squares(
    mut query: Query<(&mut Transform, &mut SquareData)>,
    game: Res<GameState>,
) {
    if game.scene_idx != 1 || game.paused { return; }

    let x0 = WIDTH / 2.0 - 200.0;
    let x1 = WIDTH / 2.0 + 200.0;
    let y0 = HEIGHT / 2.0 - 200.0;
    let y1 = HEIGHT / 2.0 + 200.0;

    query.par_iter_mut().for_each(|(mut transform, mut data)| {
        transform.translation.x += data.vx;
        transform.translation.y += data.vy;

        if transform.translation.x < x0 {
            transform.translation.x = x0;
            data.vx = -data.vx;
        } else if transform.translation.x > x1 {
            transform.translation.x = x1;
            data.vx = -data.vx;
        }

        if transform.translation.y < y0 {
            transform.translation.y = y0;
            data.vy = -data.vy;
        } else if transform.translation.y > y1 {
            transform.translation.y = y1;
            data.vy = -data.vy;
        }
    });
}

pub fn reset_squares(
    mut query: Query<(&mut Transform, &mut SquareData)>,
    mut game: ResMut<GameState>,
) {
    if !game.reset_requested || game.scene_idx != 1 { return; }
    for (mut transform, mut data) in &mut query {
        transform.translation.x = data.initial_x;
        transform.translation.y = data.initial_y;
        data.vx = data.initial_vx;
        data.vy = data.initial_vy;
    }
    game.reset_requested = false;
}
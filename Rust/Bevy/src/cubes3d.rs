use bevy::prelude::*;
use bevy::math::Dir3;
use crate::game::GameState;

#[derive(Component)]
pub struct Rotatable {
    pub speed: f32,
    pub axis: Dir3,
    pub initial_angle: f32,
}

pub fn setup_cubes(
    mut commands: Commands,
    mut meshes: ResMut<Assets<Mesh>>,
    mut materials: ResMut<Assets<StandardMaterial>>,
    data: Res<crate::CsvData>,
) {
    commands.spawn((
        Camera3d::default(),
        Transform::from_xyz(0.0, 0.0, -10.0).looking_at(Vec3::ZERO, Vec3::Y),
    ));

    commands.spawn((
        DirectionalLight::default(),
        Transform::from_xyz(10.0, 10.0, 10.0).looking_at(Vec3::ZERO, Vec3::Y),
    ));

    let cube_mesh = meshes.add(Cuboid::new(0.5, 0.5, 0.5));

    for row in data.cubes.iter().take(200) {
        let color = Color::srgb(row[7] as f32/255.0, row[8] as f32/255.0, row[9] as f32/255.0);

        commands.spawn((
            Mesh3d(cube_mesh.clone()),
            MeshMaterial3d(materials.add(color)),
            Transform::from_xyz(row[0] as f32, row[1] as f32, row[2] as f32),
            Rotatable {
                speed: row[6] as f32,
                axis: Dir3::new(Vec3::new(row[3] as f32, row[4] as f32, row[5] as f32).normalize())
                    .unwrap_or(Dir3::Y),
                initial_angle: 0.0,
            },
        ));
    }
}

pub fn update_cubes(
    mut query: Query<(&mut Transform, &Rotatable)>,
    game: Res<GameState>,
    time: Res<Time>,
) {
    if game.scene_idx != 2 || game.paused { return; }

    let t = time.elapsed_secs();
    for (mut transform, rot) in &mut query {
        let angle = rot.initial_angle + rot.speed * t;
        transform.rotation = Quat::from_axis_angle(*rot.axis, angle);
    }
}

pub fn reset_cubes(
    mut query: Query<&mut Transform, With<Rotatable>>,
    mut game: ResMut<GameState>,
) {
    if !game.reset_requested || game.scene_idx != 2 { return; }
    for mut transform in &mut query {
        transform.rotation = Quat::IDENTITY;
    }
    game.reset_requested = false;
}
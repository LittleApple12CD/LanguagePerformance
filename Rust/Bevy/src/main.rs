#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

mod csv;
mod game;
mod measurer;
mod nbody;
mod squares2d;
mod cubes3d;

use bevy::prelude::*;
use bevy::input::ButtonInput;
use bevy::window::{
    WindowPlugin, WindowResolution, Window, PresentMode,
    WindowResizeConstraints, EnabledButtons,
};
use bevy_window_utils::{WindowUtils, WindowUtilsPlugin};

use game::{CsvData, GameState, MeasurerRes, FontRes, HudText, NBodyData,
           STACK_NAME, DURATION, WIDTH, HEIGHT};

fn main() {
    App::new()
        .add_plugins(WindowUtilsPlugin)
        .add_plugins(
            DefaultPlugins
                .set(WindowPlugin {
                    primary_window: Some(Window {
                        title: "Perf Compare - rust-bevy".into(),
                        resolution: WindowResolution::new(1280.0, 720.0),
                        resizable: false,
                        present_mode: PresentMode::AutoNoVsync,
                        resize_constraints: WindowResizeConstraints {
                            min_width: 1280.0,
                            min_height: 720.0,
                            max_width: 1280.0,
                            max_height: 720.0,
                        },
                        enabled_buttons: EnabledButtons {
                            maximize: false,
                            minimize: true,
                            close: true,
                        },
                        ..default()
                    }),
                    ..default()
                })
                .set(AssetPlugin {
                    watch_for_changes: false,
                        ..default()
                })
                .disable::<bevy::audio::AudioPlugin>()
        )
        .init_resource::<GameState>()
        .insert_resource(MeasurerRes(measurer::Measurer::new(DURATION)))
        .add_systems(Startup, (
            setup_window_icon,
            setup_data,
            setup_camera_and_font,
            nbody::setup_nbody,
            squares2d::setup_squares,
            cubes3d::setup_cubes,
            setup_hud,
            setup_visibility,
        ).chain())
        .add_systems(Update, (
            handle_input,
            update_frame_stats,
            nbody::update_nbody,
            nbody::reset_nbody,
            squares2d::update_squares,
            squares2d::reset_squares,
            cubes3d::update_cubes,
            cubes3d::reset_cubes,
            update_hud,
            update_scene_visibility,
        ))
        .run();
}

// ============================================================
// 以下是你原本的 Startup 和 Update 系统，保持原样即可
// ============================================================

fn setup_window_icon(
    asset_server: Res<AssetServer>,
    mut window_utils: ResMut<WindowUtils>,
) {
    window_utils.window_icon = Some(asset_server.load("data/icon.png"));
}

fn setup_data(mut commands: Commands) {
    let particles = csv::load_csv("assets/data/particles.csv");
    let squares = csv::load_csv("assets/data/squares.csv");
    let cubes = csv::load_csv("assets/data/cubes.csv");
    commands.insert_resource(CsvData { particles, squares, cubes });
}

fn setup_camera_and_font(
    mut commands: Commands,
    asset_server: Res<AssetServer>,
) {
    commands.spawn((
        Camera2d,
        Transform::from_xyz(WIDTH / 2.0, HEIGHT / 2.0, 0.0),
    ));
    let font = asset_server.load("data/fonts/arial.ttf");
    commands.insert_resource(FontRes(font));
}

fn setup_hud(
    mut commands: Commands,
    font: Res<FontRes>,
) {
    let text = commands.spawn((
        Text::new(""),
        TextFont {
            font: font.0.clone(),
            font_size: 14.0,
            ..default()
        },
        TextColor(Color::srgb(0.9, 0.9, 0.9)),
        Node {
            position_type: PositionType::Absolute,
            top: Val::Px(8.0),
            left: Val::Px(8.0),
            ..default()
        },
        Visibility::Visible,
    )).id();
    commands.insert_resource(HudText(text));
}

fn setup_visibility(
    mut commands: Commands,
    nbody: Res<NBodyData>,
    squares: Query<Entity, With<squares2d::SquareData>>,
    cubes: Query<Entity, With<cubes3d::Rotatable>>,
) {
    for &id in &nbody.entity_ids {
        commands.entity(id).insert(Visibility::Visible);
    }
    for e in &squares {
        commands.entity(e).insert(Visibility::Hidden);
    }
    for e in &cubes {
        commands.entity(e).insert(Visibility::Hidden);
    }
}

fn handle_input(
    keys: Res<ButtonInput<KeyCode>>,
    mut game: ResMut<GameState>,
    mut measurer: ResMut<MeasurerRes>,
    mut exit: EventWriter<AppExit>,
) {
    if keys.just_pressed(KeyCode::Escape) {
        if !measurer.0.frames.is_empty() {
            write_result(&game, &measurer.0);
        }
        exit.send(AppExit::Success);
    }
    if keys.just_pressed(KeyCode::Digit1) { game.scene_idx = 0; reset_max(&mut game); }
    if keys.just_pressed(KeyCode::Digit2) { game.scene_idx = 1; reset_max(&mut game); }
    if keys.just_pressed(KeyCode::Digit3) { game.scene_idx = 2; reset_max(&mut game); }
    if keys.just_pressed(KeyCode::Space) { game.paused = !game.paused; }
    if keys.just_pressed(KeyCode::KeyR) {
        game.reset_requested = true;
        reset_max(&mut game);
    }
    if keys.just_pressed(KeyCode::KeyI) { game.hud_show = !game.hud_show; }
    if keys.just_pressed(KeyCode::KeyS) {
        if !measurer.0.active {
            measurer.0.start();
            reset_max(&mut game);
        }
    }
}

fn update_frame_stats(
    time: Res<Time>,
    mut game: ResMut<GameState>,
    mut measurer: ResMut<MeasurerRes>,
) {
    let dt = time.delta_secs_f64();
    let dt_ms = dt * 1000.0;

    let head = game.frame_head;
    game.frame_times[head] = dt_ms;

    let new_head = (head + 1) % 240;
    game.frame_head = new_head;
    if game.frame_count < 240 {
        game.frame_count += 1;
    }
    let inst_fps = 1000.0 / dt_ms;
    if inst_fps > game.max_fps {
        game.max_fps = inst_fps;
    }

    if measurer.0.active {
        measurer.0.tick(dt);
    }
    if measurer.0.just_finished {
        measurer.0.just_finished = false;
        write_result(&game, &measurer.0);
    }

    if game.first_frame {
        game.startup_ms = time.elapsed_secs_f64() * 1000.0;
        game.first_frame = false;
    }
}

fn reset_max(game: &mut GameState) {
    game.max_fps = 0.0;
    game.frame_head = 0;
    game.frame_count = 0;
}

fn update_hud(
    game: Res<GameState>,
    hud: Res<HudText>,
    measurer: Res<MeasurerRes>,
    nbody: Option<Res<NBodyData>>,
    mut query: Query<(&mut Text, &mut Visibility)>,
) {
    let Ok((mut text, mut vis)) = query.get_mut(hud.0) else { return; };

    if !game.hud_show {
        *vis = Visibility::Hidden;
        return;
    }
    *vis = Visibility::Visible;

    let s = stats(&game);
    let test = ["T1 - N-Body", "T2 - 2D Squares", "T3 - 3D Cubes"][game.scene_idx];
    let objects = match game.scene_idx {
        0 => nbody.as_ref().map(|n| n.x.len()).unwrap_or(0),
        1 => 1000,
        _ => 200,
    };

    let measuring = if measurer.0.active {
        format!("Measuring: {}s", measurer.0.left() as i32)
    } else {
        "Measuring: no".to_string()
    };

    **text = format!(
        "Stack:    {}\n\
         Test:     {}\n\
         FPS:      {:6.1}\n\
         Frame:    {:5.2} ms (avg) / {:5.2} ms (p99)\n\
         Max FPS:  {:6.1}\n\
         1% Low:   {:6.1}\n\
         Objects:  {}\n\
         Paused:   {}\n\
         {}\n\
         [1] T1  [2] T2  [3] T3  [Space] Pause  [R] Reset\n\
         [I] Info  [S] Measure  [Esc] Quit",
        STACK_NAME, test, s[0], 1000.0/s[0], s[1], s[3], s[2], objects, game.paused, measuring
    );
}

fn stats(game: &GameState) -> [f64; 4] {
    if game.frame_count == 0 {
        return [0.0, 0.0, 0.0, game.max_fps];
    }
    let mut sorted: Vec<f64> = game.frame_times[..game.frame_count].to_vec();
    sorted.sort_by(|a, b| a.partial_cmp(b).unwrap());
    let avg: f64 = sorted.iter().sum::<f64>() / game.frame_count as f64;
    let p99 = sorted[((game.frame_count as f64 * 0.99) as usize).min(game.frame_count - 1)];
    let k = ((game.frame_count as f64 * 0.01) as usize).max(1);
    let slow_sum: f64 = sorted[game.frame_count - k..].iter().sum();
    let low1 = 1000.0 / (slow_sum / k as f64);
    [1000.0 / avg, p99, low1, game.max_fps]
}

fn write_result(game: &GameState, _measurer: &measurer::Measurer) {
    let s = stats(game);
    let test = ["T1", "T2", "T3"][game.scene_idx];
    let objects = match game.scene_idx { 0 => 1000, 1 => 1000, _ => 200 };
    let max_ms = game.frame_times[..game.frame_count]
        .iter().cloned().fold(0.0f64, f64::max);
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
  "runtime": "Rust + Bevy",
  "os": "Windows"
}}
"#, STACK_NAME, test, objects, WIDTH as i32, HEIGHT as i32,
        s[0], 1000.0 / max_ms, s[3], 1000.0 / s[0], s[1], s[2],
        game.startup_ms, DURATION);
    let _ = std::fs::create_dir_all("results");
    let fn_name = format!("results/{}-{}-{}.json", STACK_NAME, test, objects);
    let _ = std::fs::write(fn_name, json);
}

fn update_scene_visibility(
    game: Res<GameState>,
    mut last_scene: Local<usize>,
    mut nbody_q: Query<&mut Visibility, (
        With<nbody::Particle>,
        Without<squares2d::SquareData>,
        Without<cubes3d::Rotatable>,
    )>,
    mut squares_q: Query<&mut Visibility, (
        With<squares2d::SquareData>,
        Without<nbody::Particle>,
        Without<cubes3d::Rotatable>,
    )>,
    mut cubes_q: Query<&mut Visibility, (
        With<cubes3d::Rotatable>,
        Without<nbody::Particle>,
        Without<squares2d::SquareData>,
    )>,
) {
    if *last_scene == game.scene_idx { return; }
    *last_scene = game.scene_idx;

    let (n_vis, s_vis, c_vis) = match game.scene_idx {
        0 => (Visibility::Visible, Visibility::Hidden, Visibility::Hidden),
        1 => (Visibility::Hidden, Visibility::Visible, Visibility::Hidden),
        _ => (Visibility::Hidden, Visibility::Hidden, Visibility::Visible),
    };
    for mut v in &mut nbody_q { *v = n_vis; }
    for mut v in &mut squares_q { *v = s_vis; }
    for mut v in &mut cubes_q { *v = c_vis; }
}
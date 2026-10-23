use bevy::prelude::*;

pub const WIDTH: f32 = 1280.0;
pub const HEIGHT: f32 = 720.0;
pub const DURATION: f64 = 30.0;
pub const STACK_NAME: &str = "rust-bevy";

#[derive(Resource)]
pub struct CsvData {
    pub particles: Vec<Vec<f64>>,
    pub squares: Vec<Vec<f64>>,
    pub cubes: Vec<Vec<f64>>,
}

#[derive(Resource)]
pub struct GameState {
    pub scene_idx: usize,
    pub paused: bool,
    pub hud_show: bool,
    pub reset_requested: bool,
    pub frame_times: [f64; 240],
    pub frame_count: usize,
    pub frame_head: usize,
    pub max_fps: f64,
    pub startup_ms: f64,
    pub first_frame: bool,
}

impl Default for GameState {
    fn default() -> Self {
        Self {
            scene_idx: 0,
            paused: false,
            hud_show: true,
            reset_requested: false,
            frame_times: [0.0; 240],
            frame_count: 0,
            frame_head: 0,
            max_fps: 0.0,
            startup_ms: 0.0,
            first_frame: true,
        }
    }
}

#[derive(Resource)]
pub struct MeasurerRes(pub crate::measurer::Measurer);

#[derive(Resource)]
pub struct FontRes(pub Handle<Font>);

#[derive(Resource)]
pub struct HudText(pub Entity);

#[derive(Resource)]
pub struct NBodyData {
    pub x: Vec<f32>, pub y: Vec<f32>,
    pub vx: Vec<f32>, pub vy: Vec<f32>,
    pub mass: Vec<f32>,
    pub g: f32, pub dt: f32, pub a_max: f32, pub scale: f32,
    pub ax: Vec<f32>, pub ay: Vec<f32>,
    pub entity_ids: Vec<Entity>,
    pub initial_x: Vec<f32>,
    pub initial_y: Vec<f32>,
    pub initial_vx: Vec<f32>,
    pub initial_vy: Vec<f32>,
}
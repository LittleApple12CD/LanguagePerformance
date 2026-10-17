#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

mod csv;
mod game;
mod measurer;
mod nbody;
mod squares2d;
mod cubes3d;

use macroquad::prelude::*;
use macroquad::miniquad::conf::{Icon, Conf};

fn window_conf() -> Conf {
    let icon_bytes = include_bytes!("../data/icon.png");
    let img = image::load_from_memory(icon_bytes).unwrap();

    let small_img = img.resize_exact(16, 16, image::imageops::FilterType::Lanczos3).to_rgba8();
    let medium_img = img.resize_exact(32, 32, image::imageops::FilterType::Lanczos3).to_rgba8();
    let big_img = img.resize_exact(64, 64, image::imageops::FilterType::Lanczos3).to_rgba8();

    let mut small = [0u8; 1024];
    let mut medium = [0u8; 4096];
    let mut big = [0u8; 16384];

    small.copy_from_slice(&small_img.into_raw());
    medium.copy_from_slice(&medium_img.into_raw());
    big.copy_from_slice(&big_img.into_raw());

    let icon = Icon { small, medium, big };

    Conf {
        window_title: "Perf Compare - rust-macroquad".to_owned(),
        window_width: 1280,
        window_height: 720,
        window_resizable: false,
        icon: Some(icon),
        platform: macroquad::miniquad::conf::Platform {
            swap_interval: Some(0),
            ..Default::default()
        },
        ..Default::default()
    }
}

#[macroquad::main(window_conf)]
async fn main() {
    // 字体
    let font_bytes = include_bytes!("../data/fonts/arial.ttf");
    let font = load_ttf_font_from_bytes(font_bytes).expect("Cannot load font");

    // 数据
    let particles = csv::load_csv_str(include_str!("../data/particles.csv"));
    let squares = csv::load_csv_str(include_str!("../data/squares.csv"));
    let cubes = csv::load_csv_str(include_str!("../data/cubes.csv"));

    let mut game = game::Game::new(particles, squares, cubes, font);

    game.startup_ms = get_time() * 1000.0;

    while !game.quit {
        game.update();
        game.draw();
        next_frame().await;
    }
}
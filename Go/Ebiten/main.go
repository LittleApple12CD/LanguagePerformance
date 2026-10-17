package main

import (
    "bytes"
    "embed"
    "image"
    "image/color"
    "log"

    "github.com/hajimehoshi/ebiten/v2"
    "github.com/hajimehoshi/ebiten/v2/text/v2"
)

//go:embed data/particles.csv data/squares.csv data/cubes.csv
//go:embed data/fonts/arial.ttf
//go:embed data/icon.png
var assets embed.FS

var hudFace *text.GoTextFace
var whiteImage *ebiten.Image

func main() {
    whiteImage = ebiten.NewImage(1, 1)
    whiteImage.Fill(color.White)

    // 字体
    fontData, err := assets.ReadFile("data/fonts/arial.ttf")
    if err != nil {
        log.Fatal(err)
    }
    faceSource, err := text.NewGoTextFaceSource(bytes.NewReader(fontData))
    if err != nil {
        log.Fatal(err)
    }
    hudFace = &text.GoTextFace{Source: faceSource, Size: 14}

    // 窗口
    ebiten.SetWindowSize(1280, 720)
    ebiten.SetWindowTitle("Perf Compare - go-ebiten")
    ebiten.SetWindowResizingMode(ebiten.WindowResizingModeDisabled)
    ebiten.SetVsyncEnabled(false)
    ebiten.SetMaxTPS(1000)

    // 窗口图标（任务栏/标题栏）
    iconData, err := assets.ReadFile("data/icon.png")
    if err == nil {
        iconImg, _, err := image.Decode(bytes.NewReader(iconData))
        if err == nil {
            ebiten.SetWindowIcon([]image.Image{iconImg})
        }
    }

    game, err := NewGame(assets)
    if err != nil {
        log.Fatal(err)
    }

    if err := ebiten.RunGame(game); err != nil {
        log.Fatal(err)
    }
}
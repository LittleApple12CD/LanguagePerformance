package main

import (
	"embed"
	"fmt"
	"image/color"
	"os"
	"sort"
	"time"

	"github.com/hajimehoshi/ebiten/v2"
	"github.com/hajimehoshi/ebiten/v2/text/v2"
	"github.com/hajimehoshi/ebiten/v2/inpututil"
)

const (
	GameW     = 1280
	GameH     = 720
	Duration  = 30.0
	StackName = "go-ebiten"
)

type Game struct {
	t1, t2, t3 *interface{}
	nbody      *NBody
	squares    *Squares2D
	cubes      *Cubes3D
	sceneIdx   int
	paused     bool
	hudShow    bool
	quit       bool

	frameTimes [240]float64
	frameCount int
	frameHead  int
	maxFps     float64
	startupMs  float64
	firstFrame bool

	measurer *Measurer

	lastTime time.Time
}

func NewGame(assets embed.FS) (*Game, error) {
	pData, _ := assets.ReadFile("data/particles.csv")
	sData, _ := assets.ReadFile("data/squares.csv")
	cData, _ := assets.ReadFile("data/cubes.csv")

	particles, err := LoadCSV(pData)
	if err != nil {
		return nil, err
	}
	squares, _ := LoadCSV(sData)
	cubes, _ := LoadCSV(cData)

	g := &Game{
		nbody:      NewNBody(particles, 1000),
		squares:    NewSquares2D(squares, 1000),
		cubes:      NewCubes3D(cubes, 200),
		hudShow:    true,
		firstFrame: true,
		measurer:   NewMeasurer(Duration),
		lastTime:   time.Now(),
	}
	return g, nil
}

func (g *Game) Update() error {
	now := time.Now()
	dt := now.Sub(g.lastTime).Seconds()
	g.lastTime = now
	if dt <= 0 {
		dt = 0.001
	}
	dtMs := dt * 1000

	g.frameTimes[g.frameHead] = dtMs
	g.frameHead = (g.frameHead + 1) % len(g.frameTimes)
	if g.frameCount < len(g.frameTimes) {
		g.frameCount++
	}
	instFps := 1000.0 / dtMs
	if instFps > g.maxFps {
		g.maxFps = instFps
	}

	// 测量
	if g.measurer.Active {
		g.measurer.Tick(dt)
	}
	if g.measurer.JustFinished {
		g.measurer.JustFinished = false
		g.WriteResult()
	}

	// 输入
    if inpututil.IsKeyJustPressed(ebiten.KeyEscape) {
        if len(g.measurer.Frames) > 0 {
            g.WriteResult()
        }
        g.quit = true
        return ebiten.Termination
    }
    if inpututil.IsKeyJustPressed(ebiten.Key1) {
        g.sceneIdx = 0
        g.ResetMax()
    }
    if inpututil.IsKeyJustPressed(ebiten.Key2) {
        g.sceneIdx = 1
        g.ResetMax()
    }
    if inpututil.IsKeyJustPressed(ebiten.Key3) {
        g.sceneIdx = 2
        g.ResetMax()
    }
    if inpututil.IsKeyJustPressed(ebiten.KeySpace) {
        g.paused = !g.paused
    }
    if inpututil.IsKeyJustPressed(ebiten.KeyR) {
        switch g.sceneIdx {
        case 0:
            g.nbody.Reset()
        case 1:
            g.squares.Reset()
        case 2:
            g.cubes.Reset()
        }
        g.ResetMax()
    }
    if inpututil.IsKeyJustPressed(ebiten.KeyI) {
        g.hudShow = !g.hudShow
    }
    if inpututil.IsKeyJustPressed(ebiten.KeyS) {
        if !g.measurer.Active {
            g.measurer.Start()
            g.ResetMax()
        }
    }

	// 更新场景
	if !g.paused {
		switch g.sceneIdx {
		case 0:
			g.nbody.Update(dt)
		case 1:
			g.squares.Update(dt)
		case 2:
			g.cubes.Update(dt)
		}
	}
	return nil
}

func (g *Game) Draw(screen *ebiten.Image) {
	screen.Fill(color.RGBA{20, 20, 28, 255})

	switch g.sceneIdx {
	case 0:
		g.nbody.Draw(screen)
	case 1:
		g.squares.Draw(screen)
	case 2:
		g.cubes.Draw(screen)
	}

	if g.hudShow {
		g.DrawHud(screen)
	}
}

func (g *Game) Layout(outsideWidth, outsideHeight int) (int, int) {
	return GameW, GameH
}

func (g *Game) DrawHud(screen *ebiten.Image) {
    s := g.Stats()
    test := []string{"T1 - N-Body", "T2 - 2D Squares", "T3 - 3D Cubes"}[g.sceneIdx]
    objects := []int{g.nbody.n, g.squares.n, g.cubes.n}[g.sceneIdx]

    measuringLine := "Measuring: no"
    if g.measurer.Active {
        measuringLine = fmt.Sprintf("Measuring: %ds", int(g.measurer.Left()))
    }

    lines := []string{
        fmt.Sprintf("Stack:    %s", StackName),
        fmt.Sprintf("Test:     %s", test),
        fmt.Sprintf("FPS:      %6.1f", s[0]),
        fmt.Sprintf("Frame:    %5.2f ms (avg) / %5.2f ms (p99)", 1000/s[0], s[1]),
        fmt.Sprintf("Max FPS:  %6.1f", s[3]),
        fmt.Sprintf("1%% Low:   %6.1f", s[2]),
        fmt.Sprintf("Objects:  %d", objects),
        fmt.Sprintf("Paused:   %t", g.paused),
        measuringLine,
        "[1] T1  [2] T2  [3] T3  [Space] Pause  [R] Reset",
        "[I] Info  [S] Measure  [Esc] Quit",
    }

    for i, line := range lines {
        op := &text.DrawOptions{}
        op.GeoM.Translate(8, float64(8+i*18))
        text.Draw(screen, line, hudFace, op)
    }
}

func (g *Game) Stats() [4]float64 {
    if g.frameCount == 0 {
        return [4]float64{0, 0, 0, g.maxFps}
    }
    sorted := make([]float64, g.frameCount)
    copy(sorted, g.frameTimes[:g.frameCount])
    sort.Float64s(sorted)
    sum := 0.0
    for _, v := range sorted {
        sum += v
    }
    avg := sum / float64(g.frameCount)
    p99 := sorted[int(float64(g.frameCount)*0.99)]
    k := int(float64(g.frameCount) * 0.01)
    if k < 1 {
        k = 1
    }
    slowSum := 0.0
    for i := g.frameCount - k; i < g.frameCount; i++ {
        slowSum += sorted[i]
    }
    low1 := 1000.0 / (slowSum / float64(k))
    return [4]float64{1000.0 / avg, p99, low1, g.maxFps}
}

func (g *Game) ResetMax() {
	g.maxFps = 0
	g.frameHead = 0
	g.frameCount = 0
}

func (g *Game) WriteResult() {
	s := g.Stats()
	test := []string{"T1", "T2", "T3"}[g.sceneIdx]
	objects := []int{g.nbody.n, g.squares.n, g.cubes.n}[g.sceneIdx]
	maxMs := 0.0
	for i := 0; i < g.frameCount; i++ {
		if g.frameTimes[i] > maxMs {
			maxMs = g.frameTimes[i]
		}
	}

	json := fmt.Sprintf(
		`{
  "stack": "%s",
  "test": "%s",
  "objects": %d,
  "resolution": [%d, %d],
  "avg_fps": %f,
  "min_fps": %f,
  "max_fps": %f,
  "avg_frame_ms": %f,
  "p99_frame_ms": %f,
  "one_percent_low_fps": %f,
  "startup_ms": %f,
  "duration_s": %f,
  "runtime": "Go + Ebiten",
  "os": "Windows"
}
`, StackName, test, objects, GameW, GameH,
		s[0], 1000.0/maxMs, s[3], 1000.0/s[0], s[1], s[2],
		g.startupMs, Duration)

	os.MkdirAll("results", 0755)
	fn := fmt.Sprintf("results/%s-%s-%d.json", StackName, test, objects)
	os.WriteFile(fn, []byte(json), 0644)
	fmt.Println("Wrote", fn)
}

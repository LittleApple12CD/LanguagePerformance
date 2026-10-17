package main

import (
	"image/color"
	"math"

	"github.com/hajimehoshi/ebiten/v2"
	"github.com/hajimehoshi/ebiten/v2/vector"
)

type Cubes3D struct {
	n          int
	px, py, pz []float64
	ax, ay, az []float64
	speed      []float64
	colors     []color.RGBA
	angles     []float64
	all        [][]float64
}

func NewCubes3D(data [][]float64, n int) *Cubes3D {
	c := &Cubes3D{all: data, n: n}
	c.Reset()
	return c
}

func (c *Cubes3D) Reset() {
	c.px = make([]float64, c.n)
	c.py = make([]float64, c.n)
	c.pz = make([]float64, c.n)
	c.ax = make([]float64, c.n)
	c.ay = make([]float64, c.n)
	c.az = make([]float64, c.n)
	c.speed = make([]float64, c.n)
	c.colors = make([]color.RGBA, c.n)
	c.angles = make([]float64, c.n)
	for i := 0; i < c.n; i++ {
		c.px[i] = c.all[i][0]
		c.py[i] = c.all[i][1]
		c.pz[i] = c.all[i][2]
		c.ax[i] = c.all[i][3]
		c.ay[i] = c.all[i][4]
		c.az[i] = c.all[i][5]
		c.speed[i] = c.all[i][6]
		c.colors[i] = color.RGBA{
			uint8(c.all[i][7]), uint8(c.all[i][8]), uint8(c.all[i][9]), 255,
		}
	}
}

func (c *Cubes3D) Update(dt float64) {
	for i := 0; i < c.n; i++ {
		c.angles[i] += c.speed[i] * dt
	}
}

func (c *Cubes3D) Draw(screen *ebiten.Image) {
	fov := 60.0 * math.Pi / 180.0
	f := 1.0 / math.Tan(fov/2)
	aspect := float64(GameW) / float64(GameH)
	camZ := -8.0
	s := 0.25

	verts := [8][3]float64{
		{-s, -s, -s}, {s, -s, -s}, {s, s, -s}, {-s, s, -s},
		{-s, -s, s}, {s, -s, s}, {s, s, s}, {-s, s, s},
	}
	edges := [12][2]int{
		{0, 1}, {1, 2}, {2, 3}, {3, 0},
		{4, 5}, {5, 6}, {6, 7}, {7, 4},
		{0, 4}, {1, 5}, {2, 6}, {3, 7},
	}

	for i := 0; i < c.n; i++ {
		a := c.angles[i]
		axx, ayy, azz := c.ax[i], c.ay[i], c.az[i]
		cosA, sinA := math.Cos(a), math.Sin(a)
		r00 := cosA + axx*axx*(1-cosA)
		r01 := axx*ayy*(1-cosA) - azz*sinA
		r02 := axx*azz*(1-cosA) + ayy*sinA
		r10 := ayy*axx*(1-cosA) + azz*sinA
		r11 := cosA + ayy*ayy*(1-cosA)
		r12 := ayy*azz*(1-cosA) - axx*sinA
		r20 := azz*axx*(1-cosA) - ayy*sinA
		r21 := azz*ayy*(1-cosA) + axx*sinA
		r22 := cosA + azz*azz*(1-cosA)

		var sx, sy [8]float32
		ok := true
		for k := 0; k < 8; k++ {
			bx, by, bz := verts[k][0], verts[k][1], verts[k][2]
			wx := r00*bx + r01*by + r02*bz + c.px[i]
			wy := r10*bx + r11*by + r12*bz + c.py[i]
			wz := r20*bx + r21*by + r22*bz + c.pz[i]
			ez := wz - camZ
			if ez <= 0.1 {
				ok = false
				break
			}
			ndcX := (wx * f / aspect) / ez
			ndcY := (wy * f) / ez
			sx[k] = float32((ndcX*0.5 + 0.5) * float64(GameW))
			sy[k] = float32((1.0 - (ndcY*0.5 + 0.5)) * float64(GameH))
		}
		if !ok {
			continue
		}
		for _, e := range edges {
			vector.StrokeLine(screen, sx[e[0]], sy[e[0]], sx[e[1]], sy[e[1]],
				2, c.colors[i], false)
		}
	}
}

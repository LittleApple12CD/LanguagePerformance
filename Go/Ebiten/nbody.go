package main

import (
	"math"
	"runtime"
	"sync"

	"github.com/hajimehoshi/ebiten/v2"
)

type NBody struct {
	n            int
	x, y, vx, vy []float64
	mass         []float64
	G, dt, aMax  float64
	scale        float64
	all          [][]float64

	ax, ay []float64

	vertices []ebiten.Vertex
	indices  []uint16
}

func NewNBody(data [][]float64, n int) *NBody {
	b := &NBody{
		all:      data,
		n:        n,
		G:        0.0002,
		dt:       0.016,
		aMax:     3.0,
		scale:    0.4,
		ax:       make([]float64, n),
		ay:       make([]float64, n),
		vertices: make([]ebiten.Vertex, n*4),
		indices:  make([]uint16, n*6),
	}
	b.Reset()
	return b
}

func (b *NBody) Reset() {
	n := b.n
	b.x = make([]float64, n)
	b.y = make([]float64, n)
	b.vx = make([]float64, n)
	b.vy = make([]float64, n)
	b.mass = make([]float64, n)
	for i := 0; i < n; i++ {
		b.x[i] = b.all[i][0]
		b.y[i] = b.all[i][1]
		b.vx[i] = b.all[i][2]
		b.vy[i] = b.all[i][3]
		b.mass[i] = b.all[i][4]
	}
}

func (b *NBody) Update(_ float64) {
	n := b.n
	x, y, mass := b.x, b.y, b.mass
	ax, ay := b.ax, b.ay
	G := b.G
	dt := b.dt
	aMax := b.aMax

	numWorkers := runtime.NumCPU()
	chunk := (n + numWorkers - 1) / numWorkers

	var wg sync.WaitGroup
	for w := 0; w < numWorkers; w++ {
		start := w * chunk
		end := start + chunk
		if end > n {
			end = n
		}
		if start >= end {
			break
		}
		wg.Add(1)
		go func(s, e int) {
			defer wg.Done()
			for i := s; i < e; i++ {
				xi := x[i]
				yi := y[i]
				var axi, ayi float64
				for j := 0; j < n; j++ {
					dx := x[j] - xi
					dy := y[j] - yi
					d2 := dx*dx + dy*dy + 1e-12
					inv := 1.0 / d2
					f := G * inv * math.Sqrt(inv) * mass[j]
					axi += f * dx
					ayi += f * dy
				}
				ax[i] = axi
				ay[i] = ayi
			}
		}(start, end)
	}
	wg.Wait()

	for i := 0; i < n; i++ {
		a2 := ax[i]*ax[i] + ay[i]*ay[i]
		if a2 > aMax*aMax {
			s := aMax / math.Sqrt(a2)
			ax[i] *= s
			ay[i] *= s
		}
		b.vx[i] += ax[i] * dt
		b.vy[i] += ay[i] * dt
		b.x[i] += b.vx[i] * dt
		b.y[i] += b.vy[i] * dt
	}
}

func (b *NBody) Draw(screen *ebiten.Image) {
	if whiteImage == nil {
		return
	}

	n := b.n
	cx := float64(GameW) / 2
	cy := float64(GameH) / 2
	r := float32(180.0 / 255.0)
	g := float32(220.0 / 255.0)
	bl := float32(1.0)
	half := float32(2.0)

	for i := 0; i < n; i++ {
		px := float32(b.x[i]*b.scale*GameW/2 + cx)
		py := float32(-b.y[i]*b.scale*GameW/2 + cy)

		base := i * 4
		// 左上
		b.vertices[base+0] = ebiten.Vertex{
			DstX: px - half, DstY: py - half,
			SrcX: 0, SrcY: 0,
			ColorR: r, ColorG: g, ColorB: bl, ColorA: 1,
		}
		// 右上
		b.vertices[base+1] = ebiten.Vertex{
			DstX: px + half, DstY: py - half,
			SrcX: 1, SrcY: 0,
			ColorR: r, ColorG: g, ColorB: bl, ColorA: 1,
		}
		// 右下
		b.vertices[base+2] = ebiten.Vertex{
			DstX: px + half, DstY: py + half,
			SrcX: 1, SrcY: 1,
			ColorR: r, ColorG: g, ColorB: bl, ColorA: 1,
		}
		// 左下
		b.vertices[base+3] = ebiten.Vertex{
			DstX: px - half, DstY: py + half,
			SrcX: 0, SrcY: 1,
			ColorR: r, ColorG: g, ColorB: bl, ColorA: 1,
		}

		ib := i * 6
		b.indices[ib+0] = uint16(base + 0)
		b.indices[ib+1] = uint16(base + 1)
		b.indices[ib+2] = uint16(base + 2)
		b.indices[ib+3] = uint16(base + 0)
		b.indices[ib+4] = uint16(base + 2)
		b.indices[ib+5] = uint16(base + 3)
	}

	screen.DrawTriangles(b.vertices, b.indices, whiteImage, nil)
}
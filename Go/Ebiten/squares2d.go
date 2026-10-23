package main

import (
	"image/color"
	"math"

	"github.com/hajimehoshi/ebiten/v2"
)

type Squares2D struct {
	n              int
	x, y, vx, vy   []float64
	size           []float64
	colors         []color.RGBA
	x0, x1, y0, y1 int
	all            [][]float64
	vertices []ebiten.Vertex
	indices  []uint16
}

func NewSquares2D(data [][]float64, n int) *Squares2D {
	s := &Squares2D{
		all: data, n: n,
		x0: GameW/2 - 200, x1: GameW/2 + 200,
		y0: GameH/2 - 200, y1: GameH/2 + 200,
	}
    s.vertices = make([]ebiten.Vertex, n*4)
    s.indices = make([]uint16, n*6)    
	s.Reset()
	return s
}

func (s *Squares2D) Reset() {
	s.x = make([]float64, s.n)
	s.y = make([]float64, s.n)
	s.vx = make([]float64, s.n)
	s.vy = make([]float64, s.n)
	s.size = make([]float64, s.n)
	s.colors = make([]color.RGBA, s.n)
	for i := 0; i < s.n; i++ {
		s.x[i] = s.all[i][0] + float64(s.x0)
		s.y[i] = s.all[i][1] + float64(s.y0)
		s.vx[i] = s.all[i][2]
		s.vy[i] = s.all[i][3]
		s.size[i] = s.all[i][4]
		s.colors[i] = color.RGBA{
			uint8(s.all[i][5]), uint8(s.all[i][6]), uint8(s.all[i][7]), 255,
		}
	}
}

func (s *Squares2D) Update(_ float64) {
	for i := 0; i < s.n; i++ {
		s.x[i] += s.vx[i]
		s.y[i] += s.vy[i]
		if s.x[i] < float64(s.x0) || s.x[i] > float64(s.x1) {
			s.vx[i] = -s.vx[i]
			s.x[i] = math.Max(float64(s.x0), math.Min(float64(s.x1), s.x[i]))
		}
		if s.y[i] < float64(s.y0) || s.y[i] > float64(s.y1) {
			s.vy[i] = -s.vy[i]
			s.y[i] = math.Max(float64(s.y0), math.Min(float64(s.y1), s.y[i]))
		}
	}
}

func (s *Squares2D) Draw(screen *ebiten.Image) {
	for i := 0; i < s.n; i++ {
		px, py := float32(s.x[i]), float32(s.y[i])
		sz := float32(s.size[i])
		c := s.colors[i]
		r, g, b := float32(c.R)/255.0, float32(c.G)/255.0, float32(c.B)/255.0
		a := float32(1.0)

		base := i * 4
		s.vertices[base+0] = ebiten.Vertex{DstX: px, DstY: py, SrcX: 0, SrcY: 0, ColorR: r, ColorG: g, ColorB: b, ColorA: a}
		s.vertices[base+1] = ebiten.Vertex{DstX: px + sz, DstY: py, SrcX: 1, SrcY: 0, ColorR: r, ColorG: g, ColorB: b, ColorA: a}
		s.vertices[base+2] = ebiten.Vertex{DstX: px + sz, DstY: py + sz, SrcX: 1, SrcY: 1, ColorR: r, ColorG: g, ColorB: b, ColorA: a}
		s.vertices[base+3] = ebiten.Vertex{DstX: px, DstY: py + sz, SrcX: 0, SrcY: 1, ColorR: r, ColorG: g, ColorB: b, ColorA: a}

		ib := i * 6
		s.indices[ib+0] = uint16(base + 0)
		s.indices[ib+1] = uint16(base + 1)
		s.indices[ib+2] = uint16(base + 2)
		s.indices[ib+3] = uint16(base + 0)
		s.indices[ib+4] = uint16(base + 2)
		s.indices[ib+5] = uint16(base + 3)
	}
	screen.DrawTriangles(s.vertices, s.indices, whiteImage, nil)
}
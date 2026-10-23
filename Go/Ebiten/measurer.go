package main

import "math"

type Measurer struct {
	duration     float64
	Active       bool
	Elapsed      float64
	Frames       []float64
	JustFinished bool
}

func NewMeasurer(d float64) *Measurer {
	return &Measurer{duration: d}
}

func (m *Measurer) Start() {
	m.Active = true
	m.Elapsed = 0
	m.Frames = m.Frames[:0]
	m.JustFinished = false
}

func (m *Measurer) Tick(dt float64) {
	if !m.Active {
		return
	}
	m.Frames = append(m.Frames, dt)
	m.Elapsed += dt
	if m.Elapsed >= m.duration {
		m.Active = false
		m.JustFinished = true
	}
}

func (m *Measurer) Left() float64 {
	if !m.Active {
		if m.JustFinished {
			return 0
		}
		return m.duration
	}
	return math.Max(0, m.duration-m.Elapsed)
}

pub struct Measurer {
    pub duration: f64,
    pub active: bool,
    pub elapsed: f64,
    pub frames: Vec<f64>,
    pub just_finished: bool,
}

impl Measurer {
    pub fn new(d: f64) -> Self {
        Self { duration: d, active: false, elapsed: 0.0, frames: Vec::new(), just_finished: false }
    }

    pub fn start(&mut self) {
        self.active = true;
        self.elapsed = 0.0;
        self.frames.clear();
        self.just_finished = false;
    }

    pub fn tick(&mut self, dt: f64) {
        if !self.active { return; }
        self.frames.push(dt);
        self.elapsed += dt;
        if self.elapsed >= self.duration {
            self.active = false;
            self.just_finished = true;
        }
    }

    pub fn left(&self) -> f64 {
        if !self.active {
            return if self.just_finished { 0.0 } else { self.duration };
        }
        (self.duration - self.elapsed).max(0.0)
    }
}
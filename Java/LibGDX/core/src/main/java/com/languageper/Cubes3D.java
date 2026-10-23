package com.languageper;

import com.badlogic.gdx.Gdx;
import com.badlogic.gdx.graphics.Color;
import com.badlogic.gdx.graphics.GL20;
import com.badlogic.gdx.graphics.PerspectiveCamera;
import com.badlogic.gdx.graphics.VertexAttributes;
import com.badlogic.gdx.graphics.g3d.Environment;
import com.badlogic.gdx.graphics.g3d.Material;
import com.badlogic.gdx.graphics.g3d.Model;
import com.badlogic.gdx.graphics.g3d.ModelBatch;
import com.badlogic.gdx.graphics.g3d.ModelInstance;
import com.badlogic.gdx.graphics.g3d.attributes.ColorAttribute;
import com.badlogic.gdx.graphics.g3d.environment.DirectionalLight;
import com.badlogic.gdx.graphics.g3d.utils.ModelBuilder;
import com.badlogic.gdx.math.Matrix4;
import com.badlogic.gdx.math.Vector3;
import com.badlogic.gdx.utils.Array;
import com.languageper.Main;

public class Cubes3D {
    double[] px, py, pz;
    double[] ax_, ay_, az_;
    double[] speed;
    float[][] colors;
    double[] angles;
    int n;
    double[][] all;

    Model cubeModel;
    Array<ModelInstance> instances = new Array<>();
    ModelBatch modelBatch;
    Environment environment;
    PerspectiveCamera camera;
	
	private final Vector3 tmpAxis = new Vector3();

    public Cubes3D(double[][] data, int n) {
        this.all = data;
        this.n = n;
        initGL();
        reset();
    }

    void initGL() {
        ModelBuilder mb = new ModelBuilder();
        cubeModel = mb.createBox(
                0.5f, 0.5f, 0.5f,
                new Material(ColorAttribute.createDiffuse(Color.WHITE)),
                VertexAttributes.Usage.Position | VertexAttributes.Usage.Normal
        );

        modelBatch = new ModelBatch();

        environment = new Environment();
        environment.set(new ColorAttribute(ColorAttribute.AmbientLight, 0.4f, 0.4f, 0.4f, 1f));
        environment.add(new DirectionalLight().set(0.8f, 0.8f, 0.8f, -1f, -0.8f, -0.2f));

        camera = new PerspectiveCamera(60, Main.WIDTH, Main.HEIGHT);
        camera.position.set(0, 0, -8f);
        camera.lookAt(0, 0, 0);
        camera.near = 0.1f;
        camera.far = 100f;
        camera.update();
    }

    public void reset() {
        px = new double[n]; py = new double[n]; pz = new double[n];
        ax_ = new double[n]; ay_ = new double[n]; az_ = new double[n];
        speed = new double[n];
        colors = new float[n][3];
        angles = new double[n];
        for (int i = 0; i < n; i++) {
            px[i] = all[i][0]; py[i] = all[i][1]; pz[i] = all[i][2];
            ax_[i] = all[i][3]; ay_[i] = all[i][4]; az_[i] = all[i][5];
            speed[i] = all[i][6];
            colors[i][0] = (float) (all[i][7] / 255.0);
            colors[i][1] = (float) (all[i][8] / 255.0);
            colors[i][2] = (float) (all[i][9] / 255.0);
        }
        rebuildInstances();
    }

    void rebuildInstances() {
        instances.clear();
        for (int i = 0; i < n; i++) {
            ModelInstance mi = new ModelInstance(cubeModel);
            mi.materials.get(0).set(ColorAttribute.createDiffuse(
                    colors[i][0], colors[i][1], colors[i][2], 1f));
            instances.add(mi);
        }
    }

    public void update(double dt) {
        for (int i = 0; i < n; i++) angles[i] += speed[i] * dt;
    }

    public void draw() {
        for (int i = 0; i < n; i++) {
            ModelInstance mi = instances.get(i);
            mi.transform.idt();
            mi.transform.translate((float) px[i], (float) py[i], (float) pz[i]);
            tmpAxis.set((float) ax_[i], (float) ay_[i], (float) az_[i]);
            mi.transform.rotate(tmpAxis, (float) Math.toDegrees(angles[i]));
        }

        modelBatch.begin(camera);
        modelBatch.render(instances, environment);
        modelBatch.end();
    }

    public void dispose() {
        modelBatch.dispose();
        cubeModel.dispose();
    }
}
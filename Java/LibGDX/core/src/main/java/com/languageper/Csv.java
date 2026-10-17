package com.languageper;

import com.badlogic.gdx.Gdx;
import com.badlogic.gdx.files.FileHandle;

import java.io.BufferedReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

public class Csv {
    public static double[][] load(String path) throws IOException {
        FileHandle fh = Gdx.files.internal(path);
        if (!fh.exists()) {
            fh = Gdx.files.local(path);
        }
        List<double[]> rows = new ArrayList<>();
        try (BufferedReader br = new BufferedReader(fh.reader("UTF-8"))) {
            br.readLine(); // header
            String line;
            while ((line = br.readLine()) != null) {
                if (line.trim().isEmpty()) continue;
                String[] parts = line.split(",");
                double[] row = new double[parts.length];
                for (int i = 0; i < parts.length; i++) {
                    row[i] = Double.parseDouble(parts[i].trim());
                }
                rows.add(row);
            }
        }
        double[][] out = new double[rows.size()][];
        for (int i = 0; i < rows.size(); i++) out[i] = rows.get(i);
        return out;
    }
}
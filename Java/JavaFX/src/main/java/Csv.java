import java.io.*;
import java.nio.file.*;
import java.util.*;

public class Csv {
    public static double[][] load(String path) throws IOException {
        InputStream in = Csv.class.getClassLoader().getResourceAsStream(path);
        if (in == null) {
            in = Files.newInputStream(Paths.get(path));
        }
        List<double[]> rows = new ArrayList<>();
        try (BufferedReader br = new BufferedReader(new InputStreamReader(in, "UTF-8"))) {
            String line = br.readLine(); // header
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
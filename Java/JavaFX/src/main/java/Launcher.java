import javafx.application.Application;

public class Launcher {
    public static void main(String[] args) {
        System.setProperty("prism.vsync", "false");
        System.setProperty("javafx.animation.pulse", "1000");
        Application.launch(Main.class, args);
    }
}
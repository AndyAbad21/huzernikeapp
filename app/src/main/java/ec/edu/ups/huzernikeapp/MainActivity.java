package ec.edu.ups.huzernikeapp;

import androidx.appcompat.app.AppCompatActivity;

import android.graphics.Bitmap;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.Spinner;
import android.widget.TextView;

import java.io.File;


import ec.edu.ups.huzernikeapp.databinding.ActivityMainBinding;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'huzernikeapp' library on application startup.
    static {
        System.loadLibrary("huzernikeapp");
    }

    private ActivityMainBinding binding;
    private DrawView drawView;
    private String selectedMoment = "Momentos de Hu"; // Opción por defecto

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        // Obtener referencias a los componentes
        drawView = findViewById(R.id.drawView);
        TextView resultTextView = findViewById(R.id.sample_text);
        Spinner momentSelector = findViewById(R.id.moment_selector);
        Button classifyButton = findViewById(R.id.button);
        Button clearButton = findViewById(R.id.button2);

        // Configurar Spinner con las opciones
        ArrayAdapter<CharSequence> adapter = ArrayAdapter.createFromResource(this,
                R.array.moment_options, android.R.layout.simple_spinner_item);
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        momentSelector.setAdapter(adapter);

        // Detectar la opción seleccionada en el Spinner
        momentSelector.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                selectedMoment = parent.getItemAtPosition(position).toString();
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
                selectedMoment = "Momentos de Hu"; // Opción por defecto
            }
        });

        // Configurar acción para el botón de clasificar
        classifyButton.setOnClickListener(view -> {
            Bitmap bitmap = drawView.getBitmap();
            String datasetPath = "/storage/emulated/0/Documents/figureshu.csv";

            // Verificar si el archivo del dataset existe
            File file = new File(datasetPath);

            if (!file.exists()) {
                Log.e("FILE_CHECK", "El archivo NO existe en Java.");
            } else if (!file.canRead()) {
                Log.e("FILE_CHECK", "El archivo existe pero NO tiene permisos de lectura.");
            } else {
                Log.e("FILE_CHECK", "El archivo SÍ existe y es accesible desde Java.");
            }

            if (!file.exists()) {
                resultTextView.setText("Error: No se encontró el dataset en " + datasetPath);
                Log.e("CLASSIFY", "El dataset no existe en la ruta proporcionada.");
                return;
            }

            if ("Momentos de Hu".equals(selectedMoment)) {
                String result = classifyImage(bitmap, datasetPath);
                resultTextView.setText(result);
            } else if ("Momentos de Zernike".equals(selectedMoment)) {
                resultTextView.setText("Cálculo de Momentos de Zernike aún no implementado.");
            }
        });

        // Configurar acción para el botón de limpiar
        clearButton.setOnClickListener(view -> {
            drawView.clearCanvas(); // Limpia el contenido del DrawView
            resultTextView.setText(""); // Limpia el texto del TextView
        });
    }

    /**
     * Método nativo para clasificar una imagen usando Momentos de Hu con el dataset.
     * @param bitmap El Bitmap generado por el DrawView.
     * @param datasetPath La ruta del archivo CSV con los datos de entrenamiento.
     * @return Un String con el resultado de la clasificación.
     */
    public native String classifyImage(Bitmap bitmap, String datasetPath);
}
#pragma once

#include <JuceHeader.h>

// La ventana de la app. Es un AudioAppComponent: un componente gráfico que además produce audio.
// Mixer de 7 stems: cada canal es un reproductor del ejemplo 01 con su fader de ganancia
// (ejemplo 03), y al final se aplica una ganancia general (master).
class MainComponent : public juce::AudioAppComponent
{
public:
    MainComponent();
    ~MainComponent() override;

    // Audio (se llaman desde el hilo de audio)
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    // Interfaz gráfica (se llaman desde el hilo de la interfaz)
    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    // ---- Los 7 stems, en el MISMO orden en que el notebook los exporta ----
    static constexpr int numStems = 7;

    // ---- Botones globales (ASCII, sin iconos) ----
    juce::TextButton loadButton { "..." };  // carga los 7 stems de la carpeta del proyecto
    juce::TextButton playButton { ">" };
    juce::TextButton stopButton { "||" };

    juce::AudioFormatManager formatManager;                          // sabe leer wav, aiff, mp3, flac...
    juce::AudioTransportSource transport [numStems];                 // play, stop y posición de cada stem
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource [numStems]; // el archivo abierto de cada stem

    void loadStems();                                   // carga los 7 archivos <stem>.wav
    void openFile (int stem, const juce::File& file);   // igual que el ejemplo 01, por canal
    // ---- Fin del reproductor ----

    // ==== Ganancia: una por stem (fader) y una general (master) ====
    // Los cambia la interfaz y los usa el audio (std::atomic, ver ejemplo 02 del curso).
    std::atomic<float> gain [numStems];
    std::atomic<float> masterGain { 1.0f };

    juce::Slider gainSlider [numStems];   // faders verticales
    juce::Label  gainLabel  [numStems];   // el nombre del stem, de vocals a instrumental
    juce::Slider masterSlider;
    juce::Label  masterLabel { {}, "master" };

    juce::AudioBuffer<float> tempBuffer;  // acá suena cada stem antes de sumarlo al buffer de salida

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};

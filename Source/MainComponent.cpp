#include "MainComponent.h"

// Los 7 stems, en el MISMO orden en que el notebook (stem_separation.ipynb) los exporta.
// El botón "..." carga <nombre>.wav de la subcarpeta stems/ (desde donde se corre la app).
static const char* stemNames[] = { "vocals", "drums", "bass", "guitar", "piano", "other", "instrumental" };

MainComponent::MainComponent()
{
    // ---- Botones globales: afectan a los 7 canales a la vez ----
    addAndMakeVisible (loadButton);
    addAndMakeVisible (playButton);
    addAndMakeVisible (stopButton);

    loadButton.onClick = [this] { loadStems(); };
    playButton.onClick = [this]
    {
        for (auto& t : transport)
            t.start();
    };
    stopButton.onClick = [this]
    {
        for (auto& t : transport)
            t.stop();
    };

    formatManager.registerBasicFormats();

    // ==== Un fader vertical por stem ====
    for (int i = 0; i < numStems; ++i)
    {
        gain[i] = 1.0f;

        gainSlider[i].setSliderStyle (juce::Slider::LinearVertical); // fader vertical, como un mixer de vivo
        gainSlider[i].setRange (0.0, 1.0, 0.01);                     // 0 = silencio, 1 = volumen original
        gainSlider[i].setValue (1.0);
        gainSlider[i].setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 20);
        gainSlider[i].onValueChange = [this, i] { gain[i] = (float) gainSlider[i].getValue(); };

        gainLabel[i].setText (stemNames[i], juce::dontSendNotification);
        gainLabel[i].setJustificationType (juce::Justification::centred);

        addAndMakeVisible (gainSlider[i]);
        addAndMakeVisible (gainLabel[i]);
    }

    // ==== Fader general (master) ====
    masterSlider.setSliderStyle (juce::Slider::LinearVertical);
    masterSlider.setRange (0.0, 1.0, 0.01);
    masterSlider.setValue (1.0);
    masterSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 20);
    masterSlider.onValueChange = [this] { masterGain = (float) masterSlider.getValue(); };
    masterLabel.setJustificationType (juce::Justification::centred);

    addAndMakeVisible (masterSlider);
    addAndMakeVisible (masterLabel);

    setSize (900, 450);
    setAudioChannels (0, 2); // 0 entradas, 2 salidas (estéreo)
}

MainComponent::~MainComponent()
{
    shutdownAudio();

    for (int i = 0; i < numStems; ++i)
        transport[i].setSource (nullptr);
}

//==============================================================================
// Audio

// Se llama antes de empezar a sonar: acá conocemos el sample rate y el tamaño del bloque.
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    tempBuffer.setSize (2, samplesPerBlockExpected); // buffer auxiliar para sumar los stems

    for (auto& t : transport)
        t.prepareToPlay (samplesPerBlockExpected, sampleRate);
}

// Se llama una y otra vez desde el hilo de audio: cada vez hay que llenar un bloque de muestras.
void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    auto* buffer = bufferToFill.buffer;
    if (buffer->getNumChannels() < 2)
        return; // este ejemplo necesita estéreo

    // Empezamos con silencio y vamos SUMANDO cada stem (así se mezcla).
    buffer->clear (bufferToFill.startSample, bufferToFill.numSamples);

    for (int stem = 0; stem < numStems; ++stem)
    {
        // 1. El reproductor del stem llena el buffer temporal (o silencio si está en stop).
        tempBuffer.clear();
        transport[stem].getNextAudioBlock (juce::AudioSourceChannelInfo (&tempBuffer, 0, bufferToFill.numSamples));

        // 2. Ganancia del fader: cada muestra se multiplica por el número del fader
        //    y se SUMA al buffer de salida (igual que el ejemplo 03, pero sumando).
        float g = gain[stem];

        for (int channel = 0; channel < buffer->getNumChannels(); ++channel)
        {
            for (int i = 0; i < bufferToFill.numSamples; ++i)
            {
                int index = bufferToFill.startSample + i;
                float mixed = buffer->getSample (channel, index) + tempBuffer.getSample (channel, i) * g;
                buffer->setSample (channel, index, mixed);
            }
        }
    }

    // ==== Ganancia general (master): multiplica TODO el buffer por el mismo número ====
    float m = masterGain;

    for (int channel = 0; channel < buffer->getNumChannels(); ++channel)
    {
        for (int i = 0; i < bufferToFill.numSamples; ++i)
        {
            int index = bufferToFill.startSample + i;
            buffer->setSample (channel, index, buffer->getSample (channel, index) * m);
        }
    }
}

void MainComponent::releaseResources()
{
    for (auto& t : transport)
        t.releaseResources();
}

//==============================================================================
// Interfaz gráfica

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void MainComponent::resized()
{
    auto area = getLocalBounds().reduced (20);

    // Fila de botones globales
    auto row = area.removeFromTop (30);
    loadButton.setBounds (row.removeFromLeft (60));
    row.removeFromLeft (10);
    playButton.setBounds (row.removeFromLeft (60));
    row.removeFromLeft (10);
    stopButton.setBounds (row.removeFromLeft (60));

    area.removeFromTop (20);

    // Columna del master a la derecha
    auto masterColumn = area.removeFromRight (90);
    masterLabel.setBounds (masterColumn.removeFromBottom (20));
    masterSlider.setBounds (masterColumn.reduced (15, 0));

    // Una columna por stem: fader vertical arriba, nombre abajo
    auto columnWidth = area.getWidth() / numStems;

    for (int i = 0; i < numStems; ++i)
    {
        auto column = area.removeFromLeft (columnWidth);
        gainLabel[i].setBounds (column.removeFromBottom (20));
        gainSlider[i].setBounds (column.reduced (15, 0));
    }
}

//==============================================================================
// Reproductor: cargar los 7 stems y abrir archivos (igual que el ejemplo 01, por canal)

void MainComponent::loadStems()
{
    // Los stems ya vienen con nombre normalizado del notebook: los busca en la
    // subcarpeta stems/ (desde donde se corre la app, o sea la raíz del proyecto).
    auto directory = juce::File::getCurrentWorkingDirectory().getChildFile ("stems");

    for (int i = 0; i < numStems; ++i)
    {
        auto file = directory.getChildFile (stemNames[i]).withFileExtension ("wav");

        if (file.existsAsFile())
            openFile (i, file);
    }
}

void MainComponent::openFile (int stem, const juce::File& file)
{
    auto* reader = formatManager.createReaderFor (file);
    if (reader == nullptr)
        return; // no es un archivo de audio que se pueda leer

    transport[stem].stop();
    transport[stem].setSource (nullptr);

    readerSource[stem] = std::make_unique<juce::AudioFormatReaderSource> (reader, true);
    readerSource[stem]->setLooping (true); // al llegar al final vuelve a empezar

    transport[stem].setSource (readerSource[stem].get(), 0, nullptr, reader->sampleRate);
}



#pragma once

//==============================================================================
class RotaryLookAndFeel : public LookAndFeel_V4
{
public:
    RotaryLookAndFeel()
    {
        setColour(Slider::thumbColourId, Colours::red);
    }
    void drawRotarySlider(Graphics& g, int x, int y, int width, int height, float sliderPos,
        const float rotaryStartAngle, const float rotaryEndAngle, Slider&) override
    {
        auto radius = jmin(width / 2, height / 2) - 4.0f;
        auto centreX = x + width * 0.5f;
        auto centreY = y + height * 0.5f;
        auto rx = centreX - radius;
        auto ry = centreY - radius;
        auto rw = radius * 2.0f;
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        // fill
        g.setColour(Colours::black);
        g.fillEllipse(rx, ry, rw, rw);

        // outline
        g.setColour(Colours::white);
        g.drawEllipse(rx, ry, rw, rw, 1.0f);

        Path p;
        auto pointerLength = radius * 0.33f;
        auto pointerThickness = 2.0f;
        p.addRectangle(-pointerThickness * 0.5f, -radius, pointerThickness, pointerLength);
        p.applyTransform(AffineTransform::rotation(angle).translated(centreX, centreY));

        // pointer
        g.setColour(Colours::white);
        g.fillPath(p);
    }

};

class MainContentComponent  : public Component,
                              private MidiInputCallback,
                              private MidiKeyboardStateListener
{
public:
    float keywidth = 200.0f;

    MainContentComponent()
      : keyboardComponent (keyboardState, MidiKeyboardComponent::horizontalKeyboard),
        startTime (Time::getMillisecondCounterHiRes() * 0.001)
    {
        setOpaque (true);
        keyboardComponent.setColour(MidiKeyboardComponent::ColourIds::whiteNoteColourId, Colours::white);
        keyboardComponent.setColour(MidiKeyboardComponent::ColourIds::blackNoteColourId, Colours::black);
        keyboardComponent.setColour(MidiKeyboardComponent::ColourIds::keySeparatorLineColourId, Colours::white);
        keyboardComponent.setColour(MidiKeyboardComponent::ColourIds::textLabelColourId, Colours::white);
        keyboardComponent.setColour(MidiKeyboardComponent::ColourIds::shadowColourId, Colours::black);
        keyboardComponent.setColour(MidiKeyboardComponent::ColourIds::upDownButtonArrowColourId, Colours::indianred);

        //drywetSlider.setSliderStyle(Slider::SliderStyle::Rotary);
        //drywetSlider.setColour(Slider::ColourIds::thumbColourId, Colours::white);
        //drywetLabel.setJustificationType(Justification::centredBottom);
        //drywetLabel.attachToComponent(&drywetSlider, false);
        //drywetSlider.setRange(0.00f, 1.00f, 0.01f);
        //drywetSlider.setTextBoxStyle(Slider::TextBoxBelow, true, 50, 20);
        //drywetLabel.setText("dry/wet", dontSendNotification);
        //drywetLabel.setColour(Label::textColourId, Colours::white);
        //drywetSlider.setColour(Slider::ColourIds::textBoxTextColourId, Colours::grey);
        //drywetSlider.setValue(0.5f);
        //drywetSlider.setBounds(10, 10, 180, 180);
        //addAndMakeVisible(drywetSlider);
        //addAndMakeVisible(drywetLabel);

        if (!sender.connect("127.0.0.1", 57120))
            showConnectionErrorMessage("Error: could not connect to UDP port  57120.");

        //Slider
        setLookAndFeel(&rotaryLookAndFeel);

        //dry wet
        drywet_slider.setSliderStyle(Slider::SliderStyle::Rotary);
        drywet_slider.setRange(0.0f, 1.0f, 0.01f);
        drywet_slider.setTextBoxStyle(Slider::TextBoxBelow, true, 100, 25);
        addAndMakeVisible(drywet_slider);
        drywet_slider.onValueChange = [this]
        {
            // create and send an OSC message with an address and a float value:
            if (!sender.send("/~bus_drywet", (float)drywet_slider.getValue())) // [5]
                showConnectionErrorMessage("Error: could not send OSC message.");
        };

        drywet_label.setColour(Label::textColourId, Colours::orange);
        drywet_label.attachToComponent(&drywet_slider, false);
        drywet_label.setText("dry / wet", dontSendNotification);
        drywet_label.setJustificationType(Justification::centredBottom);
        addAndMakeVisible(drywet_label);

        //Reverb amount
        reverb_slider.setSliderStyle(Slider::SliderStyle::Rotary);
        reverb_slider.setColour(Slider::ColourIds::thumbColourId, Colours::white);
        reverb_slider.setRange(0.0f, 1.0f, 0.01f);
        reverb_slider.setTextBoxStyle(Slider::TextBoxBelow, true, 100, 25);
        addAndMakeVisible(reverb_slider);
        reverb_slider.onValueChange = [this]
        {
            // create and send an OSC message with an address and a float value:
            if (!sender.send("/juce/drywet", (float)reverb_slider.getValue())) // [5]
                showConnectionErrorMessage("Error: could not send OSC message.");
        };

        reverb_label.setColour(Label::textColourId, Colours::orange);
        reverb_label.setText("reverb", dontSendNotification);
        reverb_label.attachToComponent(&reverb_slider, true);
        addAndMakeVisible(reverb_label);

        //Gain
        gain_slider.setSliderStyle(Slider::SliderStyle::Rotary);
        gain_slider.setColour(Slider::ColourIds::thumbColourId, Colours::white);
        gain_slider.setRange(0.0f, 1.0f, 0.01f);
        gain_slider.setTextBoxStyle(Slider::TextBoxBelow, false, 100, 25);
        addAndMakeVisible(gain_slider);
        gain_slider.onValueChange = [this]
        {
            // create and send an OSC message with an address and a float value:
            if (!sender.send("/~bus_dryIn1Amp", (float)gain_slider.getValue())) // [5]
                showConnectionErrorMessage("Error: could not send OSC message.");
        };

        gain_label.setColour(Label::textColourId, Colours::orange);
        gain_label.setText("gain", dontSendNotification);
        gain_label.attachToComponent(&gain_slider, true);
        addAndMakeVisible(gain_label);

        room_slider.setSliderStyle(Slider::SliderStyle::Rotary);
        room_slider.setColour(Slider::ColourIds::thumbColourId, Colours::white);
        room_slider.setRange(0.0f, 1.0f, 0.01f);
        room_slider.setTextBoxStyle(Slider::TextBoxBelow, false, 100, 25);
        addAndMakeVisible(room_slider);
        room_slider.onValueChange = [this]
        {
            // create and send an OSC message with an address and a float value:
            if (!sender.send("/juce/drywet", (float)room_slider.getValue())) // [5]
                showConnectionErrorMessage("Error: could not send OSC message.");
        };

        room_label.setColour(Label::textColourId, Colours::orange);
        room_label.setText("room", dontSendNotification);
        room_label.attachToComponent(&room_slider, true);
        addAndMakeVisible(room_label);
       
  
        addAndMakeVisible (midiInputListLabel);
        midiInputListLabel.setText ("MIDI Input:", dontSendNotification);
        midiInputListLabel.attachToComponent (&midiInputList, true);

        addAndMakeVisible (midiInputList);
        midiInputList.setTextWhenNoChoicesAvailable ("No MIDI Inputs Enabled");
        auto midiInputs = MidiInput::getAvailableDevices();

        StringArray midiInputNames;
        for (auto input : midiInputs)
            midiInputNames.add (input.name);

        midiInputList.addItemList (midiInputNames, 1);
        midiInputList.onChange = [this] { setMidiInput (midiInputList.getSelectedItemIndex()); };

        // find the first enabled device and use that by default
        for (auto input : midiInputs)
        {
            if (deviceManager.isMidiInputDeviceEnabled (input.identifier))
            {
                setMidiInput (midiInputs.indexOf (input));
                break;
            }
        }

        // if no enabled devices were found just use the first one in the list
        if (midiInputList.getSelectedId() == 0)
            setMidiInput (0);

        addAndMakeVisible (keyboardComponent);
        keyboardState.addListener (this);

  

        setSize (500, 300);
    }

    ~MainContentComponent() override
    {
        keyboardState.removeListener (this);
        deviceManager.removeMidiInputDeviceCallback (MidiInput::getAvailableDevices()[midiInputList.getSelectedItemIndex()].identifier, this);
    }

    void paint (Graphics& g) override
    {
        g.fillAll (Colours::black);
        g.setColour(Colours::white); //setting the colour for future drawing
        g.setFont(Font("Lucida Sans", 20.0f, Font::plain));
        g.drawFittedText("harmonizer", 250, 20, 10, 150, Justification::bottomLeft, 12);
    }

   

    void resized() override
    {
       const auto startY = 0.12f;
       const auto startX = 0.45f;
       const auto dialWidth = 1.0f;
       const auto dialHeight = 0.2f;
       
       keyboardComponent.setBoundsRelative(0, 0.8f, dialWidth, dialHeight);
       //drywet_slider.setBoundsRelative(startX + dialWidth * 3, startY - 0.2f, dialWidth, dialHeight);
        auto area = getLocalBounds();

        auto border = 4;

        auto dialArea = area.removeFromTop(area.getHeight() / 2);


        auto four = area.getWidth() / 4;


        midiInputList.setBoundsRelative(startX +0.15f, 0.6f, dialWidth/3, dialHeight/2);

        drywet_slider.setBoundsRelative(startX- 0.40f, startY, dialWidth/5, dialHeight*2);
        gain_slider.setBoundsRelative(startX - 0.20f, startY, dialWidth / 5, dialHeight * 2);
        reverb_slider.setBoundsRelative(startX + 0.10f, startY, dialWidth / 5, dialHeight * 2);
        room_slider.setBoundsRelative(startX + 0.30f, startY, dialWidth / 5, dialHeight * 2);

    }

private:

    void showConnectionErrorMessage(const String& messageText)
    {
        AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,
            "Connection error",
            messageText,
            "OK");
    }
    static String getMidiMessageDescription (const MidiMessage& m)
    {
        if (m.isNoteOn())           return "Note on "          + MidiMessage::getMidiNoteName (m.getNoteNumber(), true, true, 3);
        if (m.isNoteOff())          return "Note off "         + MidiMessage::getMidiNoteName (m.getNoteNumber(), true, true, 3);
        if (m.isProgramChange())    return "Program change "   + String (m.getProgramChangeNumber());
        if (m.isPitchWheel())       return "Pitch wheel "      + String (m.getPitchWheelValue());
        if (m.isAftertouch())       return "After touch "      + MidiMessage::getMidiNoteName (m.getNoteNumber(), true, true, 3) +  ": " + String (m.getAfterTouchValue());
        if (m.isChannelPressure())  return "Channel pressure " + String (m.getChannelPressureValue());
        if (m.isAllNotesOff())      return "All notes off";
        if (m.isAllSoundOff())      return "All sound off";
        if (m.isMetaEvent())        return "Meta event";

        if (m.isController())
        {
            String name (MidiMessage::getControllerName (m.getControllerNumber()));

            if (name.isEmpty())
                name = "[" + String (m.getControllerNumber()) + "]";

            return "Controller " + name + ": " + String (m.getControllerValue());
        }

        return String::toHexString (m.getRawData(), m.getRawDataSize());
    }

    void logMessage (const String& m)
    {
        midiMessagesBox.moveCaretToEnd();
        midiMessagesBox.insertTextAtCaret (m + newLine);
    }

    /** Starts listening to a MIDI input device, enabling it if necessary. */
    void setMidiInput (int index)
    {
        auto list = MidiInput::getAvailableDevices();

        deviceManager.removeMidiInputDeviceCallback(list[lastInputIndex].identifier, this);

        auto newInput = list[index];

        if (! deviceManager.isMidiInputDeviceEnabled (newInput.identifier))
            deviceManager.setMidiInputDeviceEnabled (newInput.identifier, true);

        deviceManager.addMidiInputDeviceCallback (newInput.identifier, this);
        midiInputList.setSelectedId (index + 1, dontSendNotification);

        lastInputIndex = index;
    }

    // These methods handle callbacks from the midi device + on-screen keyboard..
    void handleIncomingMidiMessage (MidiInput* source, const MidiMessage& message) override
    {
        const ScopedValueSetter<bool> scopedInputFlag (isAddingFromMidiInput, true);
        keyboardState.processNextMidiEvent (message);
        postMessageToList (message, source->getName());
    }

    void handleNoteOn (MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override
    {
        if (! isAddingFromMidiInput)
        {
            auto m = MidiMessage::noteOn (midiChannel, midiNoteNumber, velocity);
            m.setTimeStamp (Time::getMillisecondCounterHiRes() * 0.001);
            postMessageToList (m, "On-Screen Keyboard");
        }
    }

    void handleNoteOff (MidiKeyboardState*, int midiChannel, int midiNoteNumber, float /*velocity*/) override
    {
        if (! isAddingFromMidiInput)
        {
            auto m = MidiMessage::noteOff (midiChannel, midiNoteNumber);
            m.setTimeStamp (Time::getMillisecondCounterHiRes() * 0.001);
            postMessageToList (m, "On-Screen Keyboard");
        }
    }

    // This is used to dispach an incoming message to the message thread
    class IncomingMessageCallback   : public CallbackMessage
    {
    public:
        IncomingMessageCallback (MainContentComponent* o, const MidiMessage& m, const String& s)
           : owner (o), message (m), source (s)
        {}

        void messageCallback() override
        {
            if (owner != nullptr)
                owner->addMessageToList (message, source);
        }

        Component::SafePointer<MainContentComponent> owner;
        MidiMessage message;
        String source;
    };

    void postMessageToList (const MidiMessage& message, const String& source)
    {
        (new IncomingMessageCallback (this, message, source))->post();
    }

    void addMessageToList (const MidiMessage& message, const String& source)
    {
        auto time = message.getTimeStamp() - startTime;

        auto hours   = ((int) (time / 3600.0)) % 24;
        auto minutes = ((int) (time / 60.0)) % 60;
        auto seconds = ((int) time) % 60;
        auto millis  = ((int) (time * 1000.0)) % 1000;

        auto timecode = String::formatted ("%02d:%02d:%02d.%03d",
                                           hours,
                                           minutes,
                                           seconds,
                                           millis);

        auto description = getMidiMessageDescription (message);

        String midiMessageString (timecode + "  -  " + description + " (" + source + ")"); // [7]
        logMessage (midiMessageString);
    }

    //==============================================================================
    AudioDeviceManager deviceManager;           // [1]
    ComboBox midiInputList;                     // [2]
    Label midiInputListLabel;
    int lastInputIndex = 0;                     // [3]
    bool isAddingFromMidiInput = false;   
    
    RotaryLookAndFeel rotaryLookAndFeel;
    Slider drywet_slider;
    Label drywet_label;
    Slider reverb_slider;
    Label reverb_label;
    Slider gain_slider;
    Label gain_label;
    Slider room_slider;
    Label room_label;

    MidiKeyboardState keyboardState;            // [5]
    MidiKeyboardComponent keyboardComponent;    // [6]
    TextEditor midiMessagesBox;
    double startTime;
    OSCSender sender;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};

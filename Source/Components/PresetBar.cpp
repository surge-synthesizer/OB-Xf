#include "../PluginEditor.h"
#include "PresetBar.h"


PresetBar::PresetBar(ObxdAudioProcessorEditor &gui)
    : editor(gui)
{
    presetNameLb = std::make_unique<CustomLabel>("preset_label", "---");
    addAndMakeVisible(presetNameLb.get());

#ifdef JUCE_MAC
    presetNameLb->setFont (juce::Font(
        juce::FontOptions()
            .withName("Helvetica Neue")
            .withStyle("Bold")
            .withHeight(16.0f)));
#endif

#ifdef JUCE_WINDOWS
    presetNameLb->setFont(juce::Font(
        juce::FontOptions()
        .withName("Arial")
        .withStyle("Bold")
        .withHeight(16.0f)));
#endif

#ifdef JUCE_LINUX
    presetNameLb->setFont (juce::Font(
        juce::FontOptions()
            .withName("DejaVu Sans")
            .withStyle("Bold")
            .withHeight(16.0f)));
#endif

    presetNameLb->setJustificationType(juce::Justification::centred);
    presetNameLb->setEditable(false, false, false);
    presetNameLb->setColour(juce::TextEditor::textColourId, juce::Colours::black);
    presetNameLb->setColour(juce::TextEditor::backgroundColourId,
                            juce::Colours::black.brighter(0.1f));
    presetNameLb->setBounds(60, 8, 351, 24);

    previousBtn = std::make_unique<juce::ImageButton>("previous_button");
    addAndMakeVisible(previousBtn.get());
    previousBtn->setButtonText(juce::String());
    previousBtn->addListener(this);
    previousBtn->setImages(false, true, true,
                           juce::Image(), 1.000f, juce::Colour(0x00000000),
                           juce::Image(), 1.000f, juce::Colour(0x00000000),
                           juce::Image(), 1.000f, juce::Colour(0x00000000));
    previousBtn->setBounds(24, 8, 30, 24);

    nextBtn = std::make_unique<juce::ImageButton>("next_button");
    addAndMakeVisible(nextBtn.get());
    nextBtn->setButtonText(juce::String());
    nextBtn->addListener(this);
    nextBtn->setImages(false, true, true,
                       juce::Image(), 1.000f, juce::Colour(0x00000000),
                       juce::Image(), 1.000f, juce::Colour(0x00000000),
                       juce::Image(), 1.000f, juce::Colour(0x00000000));
    nextBtn->setBounds(417, 8, 30, 24);

    presetNameLb->leftClicked = [this](juce::Point<int> pos) {
        if (leftClicked)
            leftClicked(pos);
    };

    setSize(471, 40);
    startTimer(50);
}

PresetBar::~PresetBar()
{
    stopTimer();
}

void PresetBar::paint(juce::Graphics &g)
{
    g.setColour(juce::Colour(0xff797979));
    g.drawRoundedRectangle(60.0f, 6.0f, 351.0f, 27.0f, 3.0f, 1.0f);

    const float prevBtnCenterX = previousBtn->getX() + previousBtn->getWidth() / 2.0f;
    const float prevBtnCenterY = previousBtn->getY() + previousBtn->getHeight() / 2.0f;
    const float nextBtnCenterX = nextBtn->getX() + nextBtn->getWidth() / 2.0f;
    const float nextBtnCenterY = nextBtn->getY() + nextBtn->getHeight() / 2.0f;

    constexpr float arrowWidth = 8.0f;
    constexpr float arrowHeight = 12.0f;

    juce::Path prevArrow;
    prevArrow.startNewSubPath(prevBtnCenterX - arrowWidth / 2.0f, prevBtnCenterY);
    prevArrow.lineTo(prevBtnCenterX + arrowWidth / 2.0f, prevBtnCenterY - arrowHeight / 2.0f);
    prevArrow.lineTo(prevBtnCenterX + arrowWidth / 2.0f, prevBtnCenterY + arrowHeight / 2.0f);
    prevArrow.closeSubPath();
    g.fillPath(prevArrow);

    juce::Path nextArrow;
    nextArrow.startNewSubPath(nextBtnCenterX + arrowWidth / 2.0f, nextBtnCenterY);
    nextArrow.lineTo(nextBtnCenterX - arrowWidth / 2.0f, nextBtnCenterY - arrowHeight / 2.0f);
    nextArrow.lineTo(nextBtnCenterX - arrowWidth / 2.0f, nextBtnCenterY + arrowHeight / 2.0f);
    nextArrow.closeSubPath();
    g.fillPath(nextArrow);
}

void PresetBar::resized()
{
}

void PresetBar::buttonClicked(juce::Button *buttonThatWasClicked)
{
    if (buttonThatWasClicked == previousBtn.get())
    {
        editor.prevProgram();
    }
    else if (buttonThatWasClicked == nextBtn.get())
    {
        editor.nextProgram();
    }
}

void PresetBar::timerCallback()
{
    update();
}

void PresetBar::update() const
{
    presetNameLb->setText(editor.getCurrentProgramName(),
                          juce::NotificationType::dontSendNotification);
}
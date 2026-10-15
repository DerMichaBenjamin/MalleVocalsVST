#include "PluginEditor.h"
#include <iostream>
#include <stdexcept>
void require(bool ok, const char *msg) {
  if (!ok)
    throw std::runtime_error(msg);
}
juce::TextButton *button(juce::Component &root, const juce::String &text) {
  for (auto *c : root.getChildren()) {
    if (auto *b = dynamic_cast<juce::TextButton *>(c))
      if (b->getButtonText() == text)
        return b;
    if (auto *b = button(*c, text))
      return b;
  }
  return nullptr;
}
int main() {
  juce::ScopedJuceInitialiser_GUI gui;
  try {
    MalleProcessor p;
    p.prepareToPlay(48000, 256);
    p.setValue("tune.root", 7);
    p.setValue("input", -3);
    p.state.copyState();
    p.undo.clearUndoHistory();
    p.applyPreset(14);
    require(p.getValue("tune.root") == 7, "key lock");
    require(p.getValue("input") == -3, "input lock");
    float pressure = p.getValue("druck.amount");
    require(pressure > 0, "preset applies");
    require(p.undo.undo(), "preset undo exists");
    require(p.getValue("druck.amount") == 0, "preset undo restores macro");
    require(p.undo.redo(), "preset redo");
    require(p.getValue("druck.amount") == pressure,
            "preset redo restores macro");
    juce::MemoryBlock saved;
    p.getStateInformation(saved);
    MalleProcessor restored;
    restored.setStateInformation(saved.getData(), int(saved.getSize()));
    for (auto &d : malle::parameters())
      require(std::abs(p.getValue(d.id) - restored.getValue(d.id)) <
                  1e-5f * (1 + std::abs(p.getValue(d.id))),
              "parameter recall");
    auto editor = std::unique_ptr<juce::AudioProcessorEditor>(p.createEditor());
    auto before = p.state.copyState().createCopy();
    auto *b = button(*editor, "Advanced");
    require(b != nullptr, "simple is default");
    auto screenshots =
        juce::File::getCurrentWorkingDirectory().getChildFile("screenshots");
    screenshots.createDirectory();
    auto saveImage = [&](juce::String name) {
      auto image = editor->createComponentSnapshot(editor->getLocalBounds());
      juce::FileOutputStream stream(screenshots.getChildFile(name));
      juce::PNGImageFormat png;
      require(png.writeImageToStream(image, stream), "PNG snapshot");
    };
    saveImage("simple.png");
    b->onClick();
    require(before.isEquivalentTo(p.state.copyState()),
            "view switch preserves all state");
    saveImage("advanced.png");
    auto file = juce::File::getSpecialLocation(juce::File::tempDirectory)
                    .getNonexistentChildFile("malle-fixture", ".wav");
    juce::WavAudioFormat fmt;
    std::unique_ptr<juce::AudioFormatWriter> w(fmt.createWriterFor(
        new juce::FileOutputStream(file), 48000, 2, 24, {}, 0));
    require(w != nullptr, "fixture WAV");
    juce::AudioBuffer<float> fixture(2, 4800);
    for (int i = 0; i < 4800; ++i)
      for (int c = 0; c < 2; ++c)
        fixture.setSample(c, i,
                          .1f * std::sin(2 * malle::pi * 220 * i / 48000));
    w->writeFromAudioSampleBuffer(fixture, 0, 4800);
    w.reset();
    juce::String error;
    require(p.importSample(file, error), "sample import");
    p.getStateInformation(saved);
    file.deleteFile();
    MalleProcessor recalled;
    recalled.setStateInformation(saved.getData(), int(saved.getSize()));
    require(!recalled.sampleMissing.load(),
            "embedded sample recall without file");
    require(recalled.sampleName.contains("malle-fixture"),
            "sample name recall");
    p.prepareToPlay(48000, 256);
    recalled.prepareToPlay(48000, 256);
    juce::AudioBuffer<float> a(2, 256), c(2, 256);
    juce::MidiBuffer midi;
    for (int n = 0; n < 40; ++n) {
      for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 256; ++i) {
          float x = .1f * std::sin(2 * malle::pi * 220 * (n * 256 + i) / 48000);
          a.setSample(ch, i, x);
          c.setSample(ch, i, x);
        }
      p.processBlock(a, midi);
      recalled.processBlock(c, midi);
      for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 256; ++i)
          require(std::abs(a.getSample(ch, i) - c.getSample(ch, i)) < 1e-6f,
                  "offline recall output exact");
    }
    std::cout
        << "PASS: locks, preset Undo/Redo, full recall, view-state identity, "
           "embedded sample recall, exact offline output, UI screenshots\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}

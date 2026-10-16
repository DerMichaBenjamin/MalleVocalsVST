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
    {
      MalleProcessor catalog;
      for (auto key : {"lock.key", "lock.input", "lock.routing"})
        catalog.state.state.setProperty(key, false, nullptr);
      for (int i=0;i<int(malle::presets().size());++i) {
        catalog.applyPreset(i);
        for (size_t j=0;j<malle::parameters().size();++j) {
          float expected=malle::presets()[size_t(i)].values[j];
          require(std::abs(catalog.getValue(malle::parameters()[j].id)-expected)
                      < 1e-4f*(1+std::abs(expected)), "all global presets load actual values");
        }
        catalog.undo.clearUndoHistory();
      }
    }
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
    auto originalRoute = p.routing();
    p.moveModule(0, 8);
    require(p.routing() != originalRoute, "routing moves");
    require(p.undo.undo(), "routing undo");
    require(p.routing() == originalRoute, "routing undo exact");
    require(p.undo.redo(), "routing redo");
    require(p.routing()[8] == 0, "routing redo exact");
    p.deesserPreset(1);
    require(p.getValue("deess.low") == 2200 && p.getValue("deess.high") == 6500,
            "SCH preset band");
    p.deesserPreset(2);
    require(p.getValue("deess.mode") == 1, "wideband preset");
    require(p.undo.undo(), "deesser preset undo");
    require(p.getValue("deess.mode") == 0, "deesser undo mode");
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
      auto destination = screenshots.getChildFile(name);
      destination.deleteFile();
      juce::FileOutputStream stream(destination);
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
    p.setValue("stadium.slot",1);
    p.setValue("stadium.1.root",5);
    p.setValue("stadium.1.level",-9);
    require(p.importSample(file,error),"second crowd slot import");
    auto bankFile=file.withFileExtension("mallecrowd");
    require(p.saveCrowdBank(bankFile,error),"crowd bank export");
    auto bankXml=juce::XmlDocument::parse(bankFile);
    auto bankTree=juce::ValueTree::fromXml(*bankXml);
    for(auto child:bankTree) {
      auto text=child.getProperty("data").toString();
      if(text.isEmpty())continue;
      juce::MemoryBlock bytes;require(bytes.fromBase64Encoding(text),"native bank encoding");
      child.setProperty("data",juce::Base64::toBase64(bytes.getData(),bytes.getSize()),nullptr);
      child.setProperty("encoding","base64",nullptr);
    }
    bankTree.createXml()->writeTo(bankFile);
    MalleProcessor bankLoaded;
    require(bankLoaded.loadCrowdBank(bankFile,error),"crowd bank import");
    require(bankLoaded.getValue("stadium.1.root")==5 && bankLoaded.getValue("stadium.1.level")==-9,"per slot settings survive bank export");
    require(bankLoaded.crowdSlotName(0).contains("malle-fixture") && bankLoaded.crowdSlotName(1).contains("malle-fixture"),"bank contains two slots");
    bankLoaded.undo.undo();
    require(bankLoaded.crowdSlotName(1)=="Leer / Testtextur","bank undo restores empty bank");
    bankLoaded.undo.redo();
    require(bankLoaded.crowdSlotName(1).contains("malle-fixture"),"bank redo restores samples");
    p.clearCrowdSlot();
    require(p.crowdSlotName(1)=="Leer / Testtextur","clear slot");
    p.undo.undo();
    require(p.crowdSlotName(1).contains("malle-fixture"),"clear slot undo");
    p.setValue("stadium.slot",0);
    bankFile.deleteFile();
    // Legacy 0.2 XML sample properties still restore into slot 1.
    auto old=p.state.copyState().createCopy();
    old.setProperty("schema",2,nullptr);
    old.setProperty("sample.data",old.getProperty("crowd.0.data"),nullptr);
    old.setProperty("sample.name",old.getProperty("crowd.0.name"),nullptr);
    for(int i=0;i<malle::crowdSlots;++i)for(auto field:{"data","name"})
      old.removeProperty(juce::Identifier(juce::String(malle::crowdProperty(i,field))),nullptr);
    auto oldFile=file.withFileExtension("mallevocals");
    old.createXml()->writeTo(oldFile);
    MalleProcessor legacy;
    require(legacy.loadPreset(oldFile),"legacy preset loads");
    require(legacy.crowdSlotName(0).contains("malle-fixture"),"legacy sample migrates");
    oldFile.deleteFile();
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

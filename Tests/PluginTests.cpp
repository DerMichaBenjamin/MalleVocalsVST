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
juce::CallOutBox* findCallout(juce::Component& root) {
  for(auto* c:root.getChildren()){
    if(auto* popup=dynamic_cast<juce::CallOutBox*>(c))return popup;
    if(auto* popup=findCallout(*c))return popup;
  }
  return nullptr;
}
int parameterControls(juce::Component& root) {
  int count=dynamic_cast<ValueControl*>(&root)!=nullptr || dynamic_cast<ParamChoice*>(&root)!=nullptr || dynamic_cast<ParamToggle*>(&root)!=nullptr ? 1 : 0;
  for(auto* child:root.getChildren())count+=parameterControls(*child);
  return count;
}
int main() {
  juce::ScopedJuceInitialiser_GUI gui;
  try {
    {
      MalleProcessor monoStereo;
      auto layout=monoStereo.getBusesLayout();
      layout.inputBuses.set(0,juce::AudioChannelSet::mono());
      layout.inputBuses.set(1,juce::AudioChannelSet::mono());
      layout.outputBuses.set(0,juce::AudioChannelSet::stereo());
      require(monoStereo.setBusesLayout(layout),"mono input stereo output accepted with lead sidechain");
      for(auto& d:malle::parameters())if(d.id.find(".on")!=std::string::npos)monoStereo.setValue(d.id,0);
      monoStereo.setValue("monitor.on",1);monoStereo.prepareToPlay(48000,64);
      juce::AudioBuffer<float> audio(2,129);juce::MidiBuffer midi;
      for(int i=0;i<129;++i){audio.setSample(0,i,.1f);audio.setSample(1,i,.05f);}
      monoStereo.processBlock(audio,midi);
      for(int i=0;i<129;++i)require(std::abs(audio.getSample(0,i)-.1f)<1e-6f && std::abs(audio.getSample(1,i)-.1f)<1e-6f,"mono duplicated to stereo across oversized callback");
      require(std::abs(monoStereo.engine.meters.sidechain.load()-.05f)<1e-6f,"mono stereo output never overwrites lead sidechain");
    }
    {
      MalleProcessor catalog;
      for (auto key : {"lock.key", "lock.input", "lock.routing"})
        catalog.state.state.setProperty(key, false, nullptr);
      for (int i=0;i<int(malle::presets().size());++i) {
        catalog.applyPreset(i);
        for (size_t j=0;j<malle::parameters().size();++j) {
          float expected=malle::parameters()[j].id=="monitor.on"?1.f:malle::presets()[size_t(i)].values[j];
          require(std::abs(catalog.getValue(malle::parameters()[j].id)-expected)
                      < 1e-4f*(1+std::abs(expected)), "all global presets load actual values");
        }
        catalog.undo.clearUndoHistory();
      }
    }
    std::cerr<<"Native preset catalog passed"<<std::endl;
    MalleProcessor p;
    p.prepareToPlay(48000, 256);
    require(p.getLatencySamples()==0,"fresh instance recording latency zero");
    p.setValue("monitor.on",0);p.prepareToPlay(48000,256);
    require(p.getLatencySamples()>0,"mix mode reports pitch latency");
    p.setValue("monitor.on",1);p.prepareToPlay(48000,256);
    p.setValue("tune.root", 7);
    p.setValue("input", -3);
    p.state.copyState();
    p.undo.clearUndoHistory();
    auto initialPressure=p.getValue("druck.amount");
    p.applyPreset(14);
    require(p.getValue("tune.root") == 7, "key lock");
    require(p.getValue("input") == -3, "input lock");
    float pressure = p.getValue("druck.amount");
    require(pressure > 0, "preset applies");
    require(p.undo.undo(), "preset undo exists");
    require(std::abs(p.getValue("druck.amount")-initialPressure)<1e-5f, "preset undo restores macro");
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
    auto lockedRoute=p.routing();
    p.applyPreset(0);
    require(p.routing()==lockedRoute,"routing lock preserves actual module order");
    p.deesserPreset(1);
    require(p.getValue("deess.low") == 2200 && p.getValue("deess.high") == 6500,
            "SCH preset band");
    p.deesserPreset(2);
    require(p.getValue("deess.mode") == 1, "wideband preset");
    require(p.undo.undo(), "deesser preset undo");
    require(p.getValue("deess.mode") == 0, "deesser undo mode");
    p.undo.beginNewTransaction("0.5 details");
    p.setValue("klang.colour",-.35f);p.setValue("stadium.pitch",-5);p.setValue("duck.threshold",-45);
    p.state.copyState();p.undo.beginNewTransaction();
    require(p.undo.undo(),"new character controls undo");
    require(std::abs(p.getValue("stadium.pitch"))<1e-6f,"crowd pitch undone");
    require(p.undo.redo() && p.getValue("stadium.pitch")==-5,"crowd pitch redo");
    juce::MemoryBlock saved;
    p.getStateInformation(saved);
    MalleProcessor restored;
    restored.setStateInformation(saved.getData(), int(saved.getSize()));
    for (auto &d : malle::parameters())
      require(std::abs(p.getValue(d.id) - restored.getValue(d.id)) <
                  1e-5f * (1 + std::abs(p.getValue(d.id))),
              "parameter recall");
    auto editor = std::unique_ptr<juce::AudioProcessorEditor>(p.createEditor());
    require(editor->getWidth()<=1040 && editor->getHeight()<=813,"compact default editor");
    require(button(*editor,"Hall / Delay einstellen")!=nullptr,"Simple space controls available");
    auto before = p.state.copyState().createCopy();
    auto *b = button(*editor, "Advanced");
    require(b != nullptr, "simple is default");
    auto screenshots =
        juce::File::getCurrentWorkingDirectory().getChildFile("screenshots");
    screenshots.createDirectory();
    auto saveImage = [&](juce::String name,juce::Component* view=nullptr) {
      auto* target=view?view:editor.get();
      auto image = target->createComponentSnapshot(target->getLocalBounds());
      auto destination = screenshots.getChildFile(name);
      destination.deleteFile();
      juce::FileOutputStream stream(destination);
      juce::PNGImageFormat png;
      require(png.writeImageToStream(image, stream), "PNG snapshot");
    };
    saveImage("simple.png");
    p.setValue("quality.on",0);
    auto spaceBefore=p.state.copyState().createCopy();
    button(*editor,"Hall / Delay einstellen")->onClick();
    require(spaceBefore.isEquivalentTo(p.state.copyState()),"opening Simple room panel is audio/state neutral");
    auto* panel=findCallout(*editor);
    require(panel!=nullptr,"native Simple room panel opens inside plugin");
    saveImage("hall-delay.png",panel);panel->setVisible(false);panel->dismiss();
    p.setValue("quality.on",1);
    b->onClick();
    require(before.isEquivalentTo(p.state.copyState()),
            "view switch preserves all state");
    saveImage("advanced.png");
    auto* stadiumTab=button(*editor,"Stadium");require(stadiumTab!=nullptr,"stadium navigation");
    stadiumTab->onClick();
    auto* bankSaveButton=button(*editor,"Bank speichern");
    require(bankSaveButton && bankSaveButton->getParentComponent()->isVisible(),"crowd bank actions visible in advanced");
    saveImage("stadium.png");
    button(*editor,"Klang & Dynamik")->onClick();
    {
      auto temporary=std::make_unique<MalleProcessor>();
      auto temporaryEditor=std::unique_ptr<juce::AudioProcessorEditor>(temporary->createEditor());
      button(*temporaryEditor,"Hall / Delay einstellen")->onClick();
      juce::Component::SafePointer<juce::CallOutBox> popup(findCallout(*temporaryEditor));
      require(popup!=nullptr,"lifetime fixture opens actual popup");
      temporaryEditor.reset();
      require(popup==nullptr || parameterControls(*popup)==0,"closing editor detaches all nested popup parameters before processor destruction");
      temporary.reset();
    }
    std::cerr<<"Native UI/views/popup lifetime passed"<<std::endl;
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
    bankLoaded.applyPreset(0);
    require(bankLoaded.getValue("stadium.1.root")==5 && bankLoaded.getValue("stadium.1.level")==-9,"factory voicing retains imported asset metadata");
    p.clearCrowdSlot();
    require(p.crowdSlotName(1)=="Leer / Testtextur","clear slot");
    p.undo.undo();
    require(p.crowdSlotName(1).contains("malle-fixture"),"clear slot undo");
    p.setValue("stadium.slot",0);
    bankFile.deleteFile();
    // Legacy 0.2 XML sample properties still restore into slot 1.
    auto old=p.state.copyState().createCopy();
    old.setProperty("schema",2,nullptr);
    for(auto id:{"quality.on","monitor.on","druck.match.on","klang.match.on","waerme.match.on","breite.match.on","macro.v2.on","klang.colour","stadium.pitch","stadium.low","stadium.high","duck.threshold"})
      old.removeChild(old.getChildWithProperty("id",id),nullptr);
    old.setProperty("sample.data",old.getProperty("crowd.0.data"),nullptr);
    old.setProperty("sample.name",old.getProperty("crowd.0.name"),nullptr);
    for(int i=0;i<malle::crowdSlots;++i)for(auto field:{"data","name"})
      old.removeProperty(juce::Identifier(juce::String(malle::crowdProperty(i,field))),nullptr);
    auto oldFile=file.withFileExtension("mallevocals");
    old.createXml()->writeTo(oldFile);
    MalleProcessor legacy;
    require(legacy.loadPreset(oldFile),"legacy preset loads");
    require(legacy.getValue("monitor.on")==1 && legacy.getValue("quality.on")==0 && legacy.getValue("druck.match.on")==0,"legacy user preset DSP defaults; explicit recording mode preserved");
    require(legacy.crowdSlotName(0).contains("malle-fixture"),"legacy sample migrates");
    juce::MemoryBlock legacyProject;
    juce::AudioProcessor::copyXmlToBinary(*old.createXml(),legacyProject);
    MalleProcessor legacyDaw;
    legacyDaw.setStateInformation(legacyProject.getData(),int(legacyProject.getSize()));
    require(legacyDaw.getValue("monitor.on")==0 && legacyDaw.getValue("quality.on")==0 && legacyDaw.getValue("druck.match.on")==0,"old DAW projects retain legacy DSP and mix latency");
    require(legacyDaw.getValue("macro.v2.on")==0 && legacy.getValue("macro.v2.on")==0,"legacy DAW and preset retain old character");
    oldFile.deleteFile();
    std::cerr<<"Native banks and legacy migration passed"<<std::endl;
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

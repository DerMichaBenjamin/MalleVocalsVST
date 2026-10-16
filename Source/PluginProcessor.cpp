#include "PluginProcessor.h"
#include "PluginEditor.h"
juce::AudioProcessorValueTreeState::ParameterLayout MalleProcessor::layout() {
  juce::AudioProcessorValueTreeState::ParameterLayout l;
  for (auto &p : malle::parameters()) {
    juce::NormalisableRange<float> r(p.min, p.max,
                                     malle::discrete(p.id) ? 1.f : 0.f);
    if (p.logarithmic)
      r.setSkewForCentre(std::sqrt(p.min * p.max));
    auto attr =
        juce::AudioParameterFloatAttributes()
            .withLabel(p.unit)
            .withStringFromValueFunction([id = p.id, unit = p.unit](float v,
                                                                    int) {
              if (id == "tune.root" || id == "stadium.root")
                return juce::String(juce::StringArray{
                    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#",
                    "B"}[juce::jlimit(0, 11, int(v))]);
              if (id == "tune.scale")
                return juce::String(
                    juce::StringArray{"Chromatisch", "Dur", "Moll",
                                      "Custom"}[juce::jlimit(0, 3, int(v))]);
              if (id.find(".on") != std::string::npos || id == "bypass")
                return juce::String(v > .5f ? "An" : "Aus");
              if (id == "druck.amount" || id == "klang.amount" ||
                  id == "waerme.amount" || id == "breite.amount" ||
                  id == "raum.amount" || id == "bewegung.amount" ||
                  id == "fx.mix" || id == "stadium.mix" || id == "throw.return")
                return juce::String(v * 100, 0) + " %";
              return juce::String(v, malle::discrete(id)            ? 0
                                     : unit == "Hz"                 ? 0
                                     : unit == "dB" || unit == "ms" ? 1
                                                                    : 2);
            })
            .withValueFromStringFunction([id = p.id](const juce::String &s) {
              if (s.equalsIgnoreCase("An"))
                return 1.f;
              if (s.equalsIgnoreCase("Aus"))
                return 0.f;
              if (id == "tune.root" || id == "stadium.root") {
                juce::StringArray names{"C",  "C#", "D",  "D#", "E",  "F",
                                        "F#", "G",  "G#", "A",  "A#", "B"};
                int i = names.indexOf(s, true);
                if (i >= 0)
                  return float(i);
              }
              if (id == "tune.scale") {
                juce::StringArray names{"Chromatisch", "Dur", "Moll", "Custom"};
                int i = names.indexOf(s, true);
                if (i >= 0)
                  return float(i);
              }
              if (id == "druck.amount" || id == "klang.amount" ||
                  id == "waerme.amount" || id == "breite.amount" ||
                  id == "raum.amount" || id == "bewegung.amount" ||
                  id == "fx.mix" || id == "stadium.mix" || id == "throw.return")
                return s.getFloatValue() / 100;
              return s.getFloatValue();
            });
    l.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(p.id, 1), p.section + ": " + p.name, r, p.def, attr));
  }
  return l;
}
namespace {
bool isRoutingParameter(const std::string& id) {
  return id.rfind("route.",0)==0 || id=="duck.source" || id=="parallel.tap" || id=="pump.mode" || id=="pump.target";
}
bool isCrowdAssetParameter(const std::string& id) {
  if(id=="stadium.slot" || id=="stadium.loopSmooth")return true;
  for(int slot=0;slot<malle::crowdSlots;++slot)for(auto field:{"root","loop","fade","level"})
    if(id==malle::crowdParameter(slot,field))return true;
  return false;
}
bool decodeCrowdData(const juce::String& encoded,juce::MemoryBlock& data) {
  auto bytes=encoded.upToFirstOccurrenceOf(".",false,false).getLargeIntValue();
  if(bytes<44 || bytes>48*1024*1024 || encoded.length()>68*1024*1024)return false;
  return data.fromBase64Encoding(encoded);
}
juce::Identifier crowdProp(int slot,const char* field) {
  return juce::Identifier(juce::String(malle::crowdProperty(slot,field)));
}
}
MalleProcessor::MalleProcessor()
    : AudioProcessor(
          BusesProperties()
              .withInput("Vocal", juce::AudioChannelSet::stereo(), true)
              .withOutput("Vocal", juce::AudioChannelSet::stereo(), true)
              .withInput("Lead Sidechain", juce::AudioChannelSet::stereo(),
                         false)),
      state(*this, &undo, "MalleVocals", layout()), base(malle::defaults()),
      resolved(base) {
  for (auto &p : malle::parameters())
    raw.push_back(state.getRawParameterValue(p.id));
  state.state.setProperty("schema", 4, nullptr);
  state.state.setProperty("lock.key", true, nullptr);
  state.state.setProperty("lock.input", true, nullptr);
  state.state.setProperty("lock.routing", true, nullptr);
  setValue("quality.on",1);
  setValue("monitor.on",1);
  for(auto section : {"druck","klang","waerme","breite"})setValue(std::string(section)+".match.on",1);
  setValue("druck.amount",.45f);setValue("klang.amount",.55f);setValue("waerme.amount",.3f);setValue("raum.amount",.35f);
  // Optional private bank installed by the user; never embedded in public builds.
  auto bankRoot=juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
 #if JUCE_MAC
  bankRoot=bankRoot.getChildFile("Application Support");
 #endif
  auto bank=bankRoot.getChildFile("Rohstoff Audio/Malle Vocals/Micha.mallecrowd");
  if(bank.existsAsFile()){juce::String error;loadCrowdBank(bank,error);}
  undo.clearUndoHistory();
  startTimerHz(10);
}
MalleProcessor::~MalleProcessor() { stopTimer(); }
void MalleProcessor::prepareToPlay(double sr, int block) {
  engine.prepare(sr, block, getMainBusNumOutputChannels());
  for(size_t i=0;i<base.size();++i)base[i]=raw[i]->load();
  malle::resolveInto(base,resolved);engine.setParameters(resolved);
  setLatencySamples(engine.latency());
}
bool MalleProcessor::isBusesLayoutSupported(const BusesLayout &l) const {
  auto in = l.getMainInputChannelSet(), out = l.getMainOutputChannelSet();
  return (in == juce::AudioChannelSet::mono() ||
          in == juce::AudioChannelSet::stereo()) &&
         in == out &&
         (l.inputBuses.size() < 2 || l.inputBuses[1].isDisabled() ||
          l.inputBuses[1] == juce::AudioChannelSet::mono() ||
          l.inputBuses[1] == juce::AudioChannelSet::stereo());
}
void MalleProcessor::processBlock(juce::AudioBuffer<float> &b,
                                  juce::MidiBuffer &) {
  process(b, false);
}
void MalleProcessor::processBlockBypassed(juce::AudioBuffer<float> &b,
                                          juce::MidiBuffer &) {
  process(b, true);
}
void MalleProcessor::process(juce::AudioBuffer<float> &b, bool bypass) {
  juce::ScopedNoDenormals no;
  for (size_t i = 0; i < base.size(); ++i)
    base[i] = raw[i]->load();
  malle::resolveInto(base, resolved);
  engine.setParameters(resolved);
  if(getLatencySamples()!=engine.latency())setLatencySamples(engine.latency());
  malle::Transport tr;
  if (auto *ph = getPlayHead())
    if (auto pos = ph->getPosition()) {
      tr.bpm = pos->getBpm().orFallback(120);
      tr.ppq = pos->getPpqPosition().orFallback(0);
      tr.hasPosition = pos->getPpqPosition().hasValue();
      tr.playing = pos->getIsPlaying();
    }
  auto main = getBusBuffer(b, false, 0);
  auto side = getBusBuffer(b, true, 1);
  bool enabled = getBus(true, 1)->isEnabled();
  sidechainEnabled.store(enabled);
  engine.process(main.getArrayOfWritePointers(), main.getNumChannels(),
                 main.getNumSamples(),
                 enabled ? side.getArrayOfReadPointers() : nullptr,
                 enabled ? side.getNumChannels() : 0, tr, bypass);
}
juce::AudioProcessorEditor *MalleProcessor::createEditor() {
  return new MalleEditor(*this);
}
void MalleProcessor::getStateInformation(juce::MemoryBlock &m) {
  auto tree = state.copyState();
  tree.setProperty("schema", 4, nullptr);
  tree.setProperty("preset", currentPreset, nullptr);
  if (auto xml = tree.createXml())
    copyXmlToBinary(*xml, m);
}
void MalleProcessor::setStateInformation(const void *d, int n) {
  if (auto xml = getXmlFromBinary(d, n)) {
    auto tree = juce::ValueTree::fromXml(*xml);
    if (tree.hasType("MalleVocals") &&
        int(tree.getProperty("schema", 2)) <= 4) {
      if (int(tree.getProperty("schema", 1)) < 2) {
        auto oldTune = tree.getChildWithProperty("id", "tune.on");
        auto creative = tree.getChildWithProperty("id", "creative.on");
        if (!creative.isValid()) {
          juce::ValueTree n("PARAM");
          n.setProperty("id", "creative.on", nullptr);
          n.setProperty("value", float(oldTune.getProperty("value", 0)),
                        nullptr);
          tree.addChild(n, -1, nullptr);
        }
        std::array<int, 11> legacy{0, 1, 10, 2, 3, 4, 5, 6, 7, 8, 9};
        for (int i = 0; i < 11; ++i) {
          juce::ValueTree n("PARAM");
          n.setProperty("id", "route." + juce::String(i), nullptr);
          n.setProperty("value", legacy[i], nullptr);
          tree.addChild(n, -1, nullptr);
        }
        tree.setProperty("schema", 4, nullptr);
      }
      // Missing appended parameters in schema <=3 must load legacy DSP, never
      // inherit new-instance recording or gain-match defaults.
      for(auto& d : malle::parameters())if(!tree.getChildWithProperty("id",juce::String(d.id)).isValid()) {
        juce::ValueTree param("PARAM");param.setProperty("id",juce::String(d.id),nullptr);
        param.setProperty("value",d.def,nullptr);tree.addChild(param,-1,nullptr);
      }
      state.replaceState(tree);
      currentPreset = tree.getProperty("preset", "User").toString();
      restoreSample();
    }
  }
}
void MalleProcessor::setValue(const std::string &id, float v) {
  auto *p = state.getParameter(id);
  p->setValueNotifyingHost(p->convertTo0to1(v));
}
float MalleProcessor::getValue(const std::string &id) const {
  return state.getRawParameterValue(id)->load();
}
void MalleProcessor::applyPreset(int n) {
  if (n < 0 || n >= int(malle::presets().size()))
    return;
  undo.beginNewTransaction("Preset");
  auto &pr = malle::presets()[size_t(n)];
  auto &defs = malle::parameters();
  for (size_t i = 0; i < defs.size(); ++i) {
    auto id = defs[i].id;
    bool key = id.rfind("tune.root", 0) == 0 ||
               id.rfind("tune.scale", 0) == 0 || id.rfind("note.", 0) == 0;
    bool input = id == "input";
    bool routing = isRoutingParameter(id);
    if(isCrowdAssetParameter(id) || id=="monitor.on")continue;
    if ((key && bool(state.state.getProperty("lock.key"))) ||
        (input && bool(state.state.getProperty("lock.input"))) ||
        (routing && bool(state.state.getProperty("lock.routing"))))
      continue;
    setValue(id, pr.values[i]);
  }
  juce::MemoryBlock baseline;
  for (auto &d : malle::parameters()) {
    float v = getValue(d.id);
    baseline.append(&v, sizeof(v));
  }
  state.state.setProperty("preset.baseline", baseline.toBase64Encoding(),
                          &undo);
  currentPreset = pr.name;
  state.state.setProperty("preset", currentPreset, &undo);
  state.copyState();
  undo.beginNewTransaction();
}
bool MalleProcessor::importSample(const juce::File &f, juce::String &error) {
  juce::AudioFormatManager formats;
  formats.registerBasicFormats();
  std::unique_ptr<juce::AudioFormatReader> r(formats.createReaderFor(f));
  if (!r) {
    error = "Audioformat nicht lesbar (WAV/AIFF/FLAC).";
    return false;
  }
  if (r->sampleRate > 192000 || r->lengthInSamples > r->sampleRate * 30 ||
      r->lengthInSamples < 64) {
    error = "Bitte 64 Samples bis 30 Sekunden importieren.";
    return false;
  }
  size_t retainedBytes = 0;
  for (auto &s : retainedSamples)
    retainedBytes += (s->left.size() + s->right.size()) * sizeof(float);
  if (retainedBytes + size_t(r->lengthInSamples) * 8 > 128 * 1024 * 1024) {
    error = "Sample-Speichergrenze 128 MB erreicht. Projekt speichern und "
            "Instanz neu laden.";
    return false;
  }
  juce::AudioBuffer<float> b(2, int(r->lengthInSamples));
  r->read(&b, 0, b.getNumSamples(), 0, true, true);
  juce::MemoryBlock encoded;
  auto *stream = new juce::MemoryOutputStream(encoded, false);
  juce::WavAudioFormat wav;
  std::unique_ptr<juce::AudioFormatWriter> w(
      wav.createWriterFor(stream, r->sampleRate, 2, 24, {}, 0));
  if (!w) {
    delete stream;
    error = "WAV konnte nicht erstellt werden.";
    return false;
  }
  w->writeFromAudioSampleBuffer(b, 0, b.getNumSamples());
  w.reset();
  undo.beginNewTransaction("Sample importieren");
  int slot=int(getValue("stadium.slot"));
  state.state.setProperty(crowdProp(slot,"data"), encoded.toBase64Encoding(), &undo);
  state.state.setProperty(crowdProp(slot,"name"), f.getFileName(), &undo);
  undo.beginNewTransaction();
  restoreSample();
  return true;
}
juce::String MalleProcessor::crowdSlotName(int slot) const {
  auto key=crowdProp(slot,"name");
  auto name=state.state.getProperty(key).toString();
  if(slot==0 && !state.state.hasProperty(crowdProp(slot,"data")))
    name=state.state.getProperty("sample.name").toString();
  return name.isEmpty() ? "Leer / Testtextur" : name;
}
void MalleProcessor::restoreSample() {
  for(int slot=0;slot<malle::crowdSlots;++slot) {
    auto key=crowdProp(slot,"data");
    auto encoded=state.state.getProperty(key).toString();
    if(slot==0 && !state.state.hasProperty(key))
      encoded=state.state.getProperty("sample.data").toString();
    if(encoded==loadedSamples[size_t(slot)])continue;
    if(encoded.isEmpty()) {
      engine.setSample(slot,nullptr); loadedSamples[size_t(slot)]=encoded;continue;
    }
    bool found=false;
    for(size_t i=0;i<sampleKeys.size();++i)if(sampleKeys[i]==encoded) {
      engine.setSample(slot,retainedSamples[i].get());found=true;break;
    }
    if(!found) {
      juce::MemoryBlock data;
      if(!decodeCrowdData(encoded,data)){sampleMissing=true;continue;}
      juce::AudioFormatManager formats;formats.registerBasicFormats();
      std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(
          std::make_unique<juce::MemoryInputStream>(data,true)));
      if(!reader || reader->sampleRate>192000 || reader->lengthInSamples<64 ||
         reader->lengthInSamples>reader->sampleRate*30){sampleMissing=true;continue;}
      size_t bytes=0;for(auto& s:retainedSamples)bytes+=(s->left.size()+s->right.size())*sizeof(float);
      if(bytes+size_t(reader->lengthInSamples)*8>128*1024*1024){sampleMissing=true;continue;}
      auto s=std::make_unique<malle::Sample>();s->sampleRate=reader->sampleRate;
      s->name=crowdSlotName(slot).toStdString();
      juce::AudioBuffer<float> audio(2,int(reader->lengthInSamples));
      if(!reader->read(&audio,0,audio.getNumSamples(),0,true,true)){sampleMissing=true;continue;}
      s->left.assign(audio.getReadPointer(0),audio.getReadPointer(0)+audio.getNumSamples());
      s->right.assign(audio.getReadPointer(1),audio.getReadPointer(1)+audio.getNumSamples());
      engine.setSample(slot,s.get());retainedSamples.push_back(std::move(s));sampleKeys.push_back(encoded);
    }
    loadedSamples[size_t(slot)]=encoded;
  }
  int slot=std::clamp(int(getValue("stadium.slot")),0,malle::crowdSlots-1);
  auto encoded=state.state.getProperty(crowdProp(slot,"data")).toString();
  if(slot==0 && !state.state.hasProperty(crowdProp(slot,"data")))
    encoded=state.state.getProperty("sample.data").toString();
  waveform.fill(0);sampleName=encoded.isEmpty()?"Synthetic test crowd (original)":crowdSlotName(slot);
  sampleMissing=encoded!=loadedSamples[size_t(slot)];
  if(!encoded.isEmpty())for(size_t k=0;k<sampleKeys.size();++k)if(sampleKeys[k]==encoded) {
    auto& s=*retainedSamples[k];
    for(size_t i=0;i<waveform.size();++i) {
      size_t a=i*s.left.size()/waveform.size(),b=(i+1)*s.left.size()/waveform.size();
      for(size_t j=a;j<b;++j)waveform[i]=std::max(waveform[i],std::abs(s.left[j]));
    }
    break;
  }
}
void MalleProcessor::clearCrowdSlot() {
  int slot=int(getValue("stadium.slot"));state.copyState();
  undo.beginNewTransaction("Crowd-Platz leeren");
  state.state.setProperty(crowdProp(slot,"data"),juce::String{},&undo);
  state.state.setProperty(crowdProp(slot,"name"),juce::String{},&undo);
  undo.beginNewTransaction();restoreSample();
}
bool MalleProcessor::saveCrowdBank(const juce::File& file,juce::String& error) {
  auto full=state.copyState();juce::ValueTree bank("MalleCrowdBank");
  bank.setProperty("schema",1,nullptr);
  for(int slot=0;slot<malle::crowdSlots;++slot) {
    juce::ValueTree child("SAMPLE");child.setProperty("slot",slot,nullptr);
    auto encoded=full.getProperty(crowdProp(slot,"data")).toString();
    if(slot==0 && !full.hasProperty(crowdProp(slot,"data")))encoded=full.getProperty("sample.data").toString();
    child.setProperty("data",encoded,nullptr);child.setProperty("name",crowdSlotName(slot),nullptr);
    for(auto field:{"root","loop","fade","level"})child.setProperty(field,getValue(malle::crowdParameter(slot,field)),nullptr);
    bank.addChild(child,-1,nullptr);
  }
  bank.setProperty("loopCrossfade",getValue("stadium.loopSmooth"),nullptr);
  auto xml=bank.createXml();if(!xml || !xml->writeTo(file)){error="Crowd-Bank konnte nicht gespeichert werden.";return false;}return true;
}
bool MalleProcessor::loadCrowdBank(const juce::File& file,juce::String& error) {
  if(file.getSize()>160*1024*1024){error="Crowd-Bank ist zu gross.";return false;}
  auto xml=juce::XmlDocument::parse(file);if(!xml){error="Crowd-Bank ist nicht lesbar.";return false;}
  auto bank=juce::ValueTree::fromXml(*xml);
  if(!bank.hasType("MalleCrowdBank") || int(bank.getProperty("schema",0))!=1 || bank.getNumChildren()>malle::crowdSlots){error="Unbekanntes Crowd-Bankformat.";return false;}
  std::array<bool,malle::crowdSlots> seen{};size_t required=0;
  for(auto child:bank) {
    int slot=int(child.getProperty("slot",-1));
    if(slot<0 || slot>=malle::crowdSlots || seen[size_t(slot)]){error="Ungueltige Crowd-Plaetze.";return false;}seen[size_t(slot)]=true;
    auto encoded=child.getProperty("data").toString();if(encoded.isEmpty())continue;
    if(child.getProperty("encoding").toString()=="base64") {
      if(encoded.length()>68*1024*1024){error="Audiodaten sind zu gross.";return false;}
      juce::MemoryBlock decoded;
      juce::MemoryOutputStream stream(decoded,false);
      if(!juce::Base64::convertFromBase64(stream,encoded)){error="Ungueltiges Base64.";return false;}
      stream.flush();encoded=decoded.toBase64Encoding();child.setProperty("data",encoded,nullptr);
    }
    juce::MemoryBlock data; if(!decodeCrowdData(encoded,data)){error="Ungueltige Audiodaten.";return false;}
    juce::AudioFormatManager formats;formats.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> r(formats.createReaderFor(std::make_unique<juce::MemoryInputStream>(data,true)));
    if(!r || r->sampleRate>192000 || r->lengthInSamples<64 || r->lengthInSamples>r->sampleRate*30){error="Sample muss 64 Frames bis 30 Sekunden / maximal 192 kHz enthalten.";return false;}
    bool cached=std::find(sampleKeys.begin(),sampleKeys.end(),encoded)!=sampleKeys.end();
    if(!cached)required+=size_t(r->lengthInSamples)*8;
  }
  size_t held=0;for(auto& s:retainedSamples)held+=(s->left.size()+s->right.size())*sizeof(float);
  if(held+required>128*1024*1024){error="Sample-Speichergrenze 128 MB. Projekt speichern und Instanz neu laden.";return false;}
  state.copyState();undo.beginNewTransaction("Crowd-Bank laden");
  for(int slot=0;slot<malle::crowdSlots;++slot) {
    auto child=bank.getChildWithProperty("slot",slot);
    state.state.setProperty(crowdProp(slot,"data"),child.getProperty("data"),&undo);
    state.state.setProperty(crowdProp(slot,"name"),child.getProperty("name"),&undo);
    for(auto field:{"root","loop","fade","level"}) {
      auto id=malle::crowdParameter(slot,field);auto& d=malle::parameters()[size_t(malle::index(id))];
      float v=float(child.getProperty(field,d.def));setValue(id,std::isfinite(v)?std::clamp(v,d.min,d.max):d.def);
    }
  }
  setValue("stadium.loopSmooth",float(bank.getProperty("loopCrossfade",1))>.5f?1.f:0.f);
  setValue("stadium.slot",0);state.copyState();undo.beginNewTransaction();restoreSample();return true;
}
void MalleProcessor::timerCallback() { restoreSample(); }
void MalleProcessor::savePreset(const juce::File &f) {
  auto tree = state.copyState();
  tree.setProperty("schema", 4, nullptr);
  if (auto x = tree.createXml())
    x->writeTo(f);
}
bool MalleProcessor::loadPreset(const juce::File &f) {
  auto xml = juce::XmlDocument::parse(f);
  if (!xml)
    return false;
  auto t = juce::ValueTree::fromXml(*xml);
  if (!t.hasType("MalleVocals"))
    return false;
  undo.beginNewTransaction("User Preset");
  for (auto &pd : malle::parameters()) {
    auto child = t.getChildWithProperty("id", juce::String(pd.id));
    if(pd.id=="monitor.on")continue;
    if(!child.isValid() && int(t.getProperty("schema",1))<4){setValue(pd.id,pd.def);continue;}
    if (child.isValid()) {
      bool key = pd.id == "tune.root" || pd.id == "tune.scale" ||
                 pd.id.rfind("note.", 0) == 0;
      bool input = pd.id == "input", routing = isRoutingParameter(pd.id);
      if ((key && bool(state.state.getProperty("lock.key"))) ||
          (input && bool(state.state.getProperty("lock.input"))) ||
          (routing && bool(state.state.getProperty("lock.routing"))))
        continue;
      setValue(pd.id, float(child.getProperty("value", pd.def)));
    }
  }
  state.state.setProperty("sample.data", t.getProperty("sample.data"), &undo);
  state.state.setProperty("sample.name", t.getProperty("sample.name"), &undo);
  for(int slot=0;slot<malle::crowdSlots;++slot)for(auto field:{"data","name"}) {
    auto key=crowdProp(slot,field);
    if(t.hasProperty(key))state.state.setProperty(key,t.getProperty(key),&undo);
    else state.state.removeProperty(key,&undo);
  }
  currentPreset = f.getFileNameWithoutExtension();
  state.copyState();
  undo.beginNewTransaction();
  restoreSample();
  return true;
}
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
  return new MalleProcessor();
}

std::array<int, 11> MalleProcessor::routing() const {
  std::array<int, 11> r{};
  std::array<bool, 11> seen{};
  for (int i = 0; i < 11; ++i) {
    int v = int(getValue("route." + std::to_string(i)));
    if (v < 0 || v > 10 || seen[size_t(v)]) {
      for (int j = 0; j < 11; ++j)
        r[j] = j;
      return r;
    }
    seen[size_t(v)] = true;
    r[i] = v;
  }
  return r;
}
void MalleProcessor::moveModule(int from, int to) {
  if (from < 0 || from > 10 || to < 0 || to > 10 || from == to)
    return;
  state.copyState();
  undo.beginNewTransaction("Direktmodul verschieben");
  auto r = routing();
  int id = r[size_t(from)];
  if (from < to)
    for (int i = from; i < to; ++i)
      r[i] = r[i + 1];
  else
    for (int i = from; i > to; --i)
      r[i] = r[i - 1];
  r[size_t(to)] = id;
  for (int i = 0; i < 11; ++i)
    setValue("route." + std::to_string(i), float(r[i]));
  state.copyState();
  undo.beginNewTransaction();
}
void MalleProcessor::deesserPreset(int n) {
  if (n < 0 || n > 2)
    return;
  state.copyState();
  undo.beginNewTransaction("De-Esser Startabstimmung");
  setValue("deess.preset", float(n));
  setValue("deess.low", n == 0 ? 5000 : n == 1 ? 2200 : 3000);
  setValue("deess.high", n == 0 ? 11000 : n == 1 ? 6500 : 14000);
  setValue("deess.threshold", n == 1 ? -27 : -24);
  setValue("deess.amount", n == 2 ? .35f : .6f);
  setValue("deess.mode", n == 2 ? 1 : 0);
  setValue("deess.on", 1);
  state.copyState();
  undo.beginNewTransaction();
}

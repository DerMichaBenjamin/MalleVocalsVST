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
  state.state.setProperty("schema", 2, nullptr);
  state.state.setProperty("lock.key", true, nullptr);
  state.state.setProperty("lock.input", true, nullptr);
  state.state.setProperty("lock.routing", true, nullptr);
  startTimerHz(10);
}
MalleProcessor::~MalleProcessor() { stopTimer(); }
void MalleProcessor::prepareToPlay(double sr, int block) {
  engine.prepare(sr, block, getMainBusNumOutputChannels());
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
  tree.setProperty("schema", 2, nullptr);
  tree.setProperty("preset", currentPreset, nullptr);
  if (auto xml = tree.createXml())
    copyXmlToBinary(*xml, m);
}
void MalleProcessor::setStateInformation(const void *d, int n) {
  if (auto xml = getXmlFromBinary(d, n)) {
    auto tree = juce::ValueTree::fromXml(*xml);
    if (tree.hasType("MalleVocals") &&
        int(tree.getProperty("schema", 2)) <= 2) {
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
        tree.setProperty("schema", 2, nullptr);
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
    bool routing = id.rfind("duck.", 0) == 0;
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
  state.state.setProperty("sample.data", encoded.toBase64Encoding(), &undo);
  state.state.setProperty("sample.name", f.getFileName(), &undo);
  undo.beginNewTransaction();
  restoreSample();
  return true;
}
void MalleProcessor::restoreSample() {
  auto encoded = state.state.getProperty("sample.data").toString();
  if (encoded == loadedSample)
    return;
  loadedSample = encoded;
  if (encoded.isEmpty()) {
    engine.setSample(nullptr);
    sampleName = "Synthetic test crowd (original)";
    return;
  }
  juce::MemoryBlock data;
  if (!data.fromBase64Encoding(encoded)) {
    sampleMissing = true;
    return;
  }
  juce::AudioFormatManager formats;
  formats.registerBasicFormats();
  std::unique_ptr<juce::AudioFormatReader> r(formats.createReaderFor(
      std::make_unique<juce::MemoryInputStream>(data, true)));
  if (!r) {
    sampleMissing = true;
    return;
  }
  if (r->sampleRate > 192000 || r->lengthInSamples < 64 ||
      r->lengthInSamples > r->sampleRate * 30) {
    sampleMissing = true;
    return;
  }
  auto key = encoded;
  for (size_t i = 0; i < sampleKeys.size(); ++i)
    if (sampleKeys[i] == key) {
      engine.setSample(retainedSamples[i].get());
      sampleName = retainedSamples[i]->name;
      sampleMissing = false;
      return;
    }
  auto s = std::make_unique<malle::Sample>();
  s->sampleRate = r->sampleRate;
  s->name = state.state.getProperty("sample.name").toString().toStdString();
  juce::AudioBuffer<float> b(2, int(r->lengthInSamples));
  r->read(&b, 0, b.getNumSamples(), 0, true, true);
  s->left.assign(b.getReadPointer(0), b.getReadPointer(0) + b.getNumSamples());
  s->right.assign(b.getReadPointer(1), b.getReadPointer(1) + b.getNumSamples());
  sampleName = s->name;
  auto *ptr = s.get();
  for (size_t i = 0; i < waveform.size(); ++i) {
    size_t a = i * s->left.size() / waveform.size(),
           b = (i + 1) * s->left.size() / waveform.size();
    float peak = 0;
    for (size_t j = a; j < b; ++j)
      peak = std::max(peak, std::abs(s->left[j]));
    waveform[i] = peak;
  }
  retainedSamples.push_back(std::move(s));
  sampleKeys.push_back(key);
  engine.setSample(ptr);
  sampleMissing = false;
}
void MalleProcessor::timerCallback() { restoreSample(); }
void MalleProcessor::savePreset(const juce::File &f) {
  auto tree = state.copyState();
  tree.setProperty("schema", 2, nullptr);
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
    if (child.isValid()) {
      bool key = pd.id == "tune.root" || pd.id == "tune.scale" ||
                 pd.id.rfind("note.", 0) == 0;
      bool input = pd.id == "input", routing = pd.id.rfind("duck.", 0) == 0;
      if ((key && bool(state.state.getProperty("lock.key"))) ||
          (input && bool(state.state.getProperty("lock.input"))) ||
          (routing && bool(state.state.getProperty("lock.routing"))))
        continue;
      setValue(pd.id, float(child.getProperty("value", pd.def)));
    }
  }
  state.state.setProperty("sample.data", t.getProperty("sample.data"), &undo);
  state.state.setProperty("sample.name", t.getProperty("sample.name"), &undo);
  currentPreset = f.getFileNameWithoutExtension();
  state.copyState();
  undo.beginNewTransaction();
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

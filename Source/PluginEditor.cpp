#include "PluginEditor.h"
MalleEditor::Control::Control(MalleProcessor &p, const malle::Param &d) {
  label.setText((d.id.find(".amount") != std::string::npos &&
                         d.section == d.id.substr(0, d.id.find('.'))
                     ? d.section
                 : d.name == "Aktiv" ? d.section + " Aktiv"
                                     : d.name) +
                    (d.unit.empty() ? "" : " (" + d.unit + ")"),
                juce::dontSendNotification);
  label.setFont(juce::FontOptions(16.f));
  label.setJustificationType(juce::Justification::centred);
  addAndMakeVisible(label);
  slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 100, 24);
  slider.setDoubleClickReturnValue(true, d.def);
  slider.setTooltip(
      d.section + ": " + d.name +
      ". Doppelklick = Reset; Shift = fein; Zahl direkt editieren.");
  slider.setNumDecimalPlacesToDisplay(malle::discrete(d.id) ? 0
                                      : d.max <= 1          ? 2
                                                            : 1);
  addAndMakeVisible(slider);
  attachment =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          p.state, d.id, slider);
  slider.onDragStart = [&p] { p.undo.beginNewTransaction("Parameter"); };
  slider.onDragEnd = [&p] { p.undo.beginNewTransaction(); };
}
void MalleEditor::Control::resized() {
  auto b = getLocalBounds();
  label.setBounds(b.removeFromTop(32));
  slider.setBounds(b);
}
juce::Point<float> EqView::point(int b) const {
  float f = proc.getValue("eq" + std::to_string(b) + ".freq"),
        g = proc.getValue("eq" + std::to_string(b) + ".gain");
  return {float(getWidth()) * std::log(f / 30) / std::log(20000.f / 30),
          getHeight() * (.5f - g / 40)};
}
void EqView::paint(juce::Graphics &g) {
  g.fillAll(juce::Colour(0xff101b26));
  g.setColour(juce::Colour(0xff324451));
  for (int i = 1; i < 5; ++i)
    g.drawHorizontalLine(getHeight() * i / 5, 0, float(getWidth()));
  juce::Path spectrum;
  for (int i = 0; i < 32; ++i) {
    float x = getWidth() * i / 31.f,
          y = getHeight() *
              (1 - juce::jlimit(
                       0.f, 1.f,
                       (malle::db(proc.engine.meters.spectrum[i].load()) + 70) /
                           70));
    if (i == 0)
      spectrum.startNewSubPath(x, y);
    else
      spectrum.lineTo(x, y);
  }
  g.setColour(juce::Colour(0xff637b91));
  g.strokePath(spectrum, juce::PathStrokeType(1.5f));
  for (int i = 0; i < 32; ++i) {
    float red = proc.engine.meters.reduction[size_t(i)].load();
    g.setColour(juce::Colour(0x99ed7762));
    g.fillRect(i * getWidth() / 32, getHeight() - int(red * 3),
               std::max(1, getWidth() / 32 - 1), int(red * 3));
  }
  auto base = malle::defaults();
  for (size_t i = 0; i < base.size(); ++i)
    base[i] = proc.getValue(malle::parameters()[i].id);
  auto effective = malle::resolve(base);
  std::array<malle::Biquad, 4> filters;
  for (int b = 0; b < 4; ++b) {
    int i = malle::index("eq0.freq") + b * 6;
    filters[b].set(
        3 + int(effective[size_t(i + 3)]), effective[size_t(i)],
        effective[size_t(i + 2)], effective[size_t(i + 1)],
        float(proc.getSampleRate() > 0 ? proc.getSampleRate() : 48000));
  }
  juce::Path response;
  for (int x = 0; x < getWidth(); x += 3) {
    float freq = 30 * std::pow(20000.f / 30, float(x) / getWidth());
    std::complex<float> z = std::polar(
        1.f,
        -2 * malle::pi * freq /
            float(proc.getSampleRate() > 0 ? proc.getSampleRate() : 48000));
    std::complex<float> h = 1.f;
    for (auto &f : filters)
      h *= (f.b0 + f.b1 * z + f.b2 * z * z) / (1.f + f.a1 * z + f.a2 * z * z);
    float y = getHeight() * (.5f - malle::db(std::abs(h)) / 40);
    if (x == 0)
      response.startNewSubPath(float(x), y);
    else
      response.lineTo(float(x), y);
  }
  g.setColour(juce::Colour(0xff6bd3cd));
  g.strokePath(response, juce::PathStrokeType(2));
  g.setColour(juce::Colour(0xfff2b44b));
  for (int b = 0; b < 4; ++b) {
    auto p = point(b);
    g.fillEllipse(p.x - 7, p.y - 7, 14, 14);
    g.drawText(juce::String(b + 1), int(p.x) + 9, int(p.y) - 10, 30, 20,
               juce::Justification::left);
  }
  g.setFont(14);
  g.drawText("30 Hz    |    EQ: ziehen = Hz/Gain, Mausrad = Q    |    20 kHz",
             10, 3, getWidth() - 20, 22, juce::Justification::left);
}
void EqView::mouseDown(const juce::MouseEvent &e) {
  selected = -1;
  float dist = 30;
  for (int b = 0; b < 4; ++b) {
    float d = point(b).getDistanceFrom(e.position);
    if (d < dist) {
      dist = d;
      selected = b;
    }
  }
  if (selected >= 0) {
    proc.undo.beginNewTransaction("EQ Band");
    for (auto suffix : {".freq", ".gain"})
      proc.state.getParameter("eq" + std::to_string(selected) + suffix)
          ->beginChangeGesture();
  }
}
void EqView::mouseDrag(const juce::MouseEvent &e) {
  if (selected < 0)
    return;
  proc.setValue(
      "eq" + std::to_string(selected) + ".freq",
      30 * std::pow(20000.f / 30,
                    juce::jlimit(0.f, 1.f, e.position.x / getWidth())));
  proc.setValue(
      "eq" + std::to_string(selected) + ".gain",
      juce::jlimit(-18.f, 18.f, (.5f - e.position.y / getHeight()) * 40));
  repaint();
}
void EqView::mouseUp(const juce::MouseEvent &) {
  if (selected >= 0)
    for (auto suffix : {".freq", ".gain"})
      proc.state.getParameter("eq" + std::to_string(selected) + suffix)
          ->endChangeGesture();
  proc.undo.beginNewTransaction();
}
void EqView::mouseWheelMove(const juce::MouseEvent &,
                            const juce::MouseWheelDetails &w) {
  if (selected < 0)
    return;
  auto id = "eq" + std::to_string(selected) + ".q";
  proc.setValue(id, juce::jlimit(.2f, 12.f, proc.getValue(id) + w.deltaY * 3));
}
MalleEditor::MalleEditor(MalleProcessor &p)
    : AudioProcessorEditor(p), proc(p), eq(p), wave(p) {
  look.setColour(juce::ResizableWindow::backgroundColourId,
                 juce::Colour(0xff14202c));
  look.setColour(juce::Slider::rotarySliderFillColourId,
                 juce::Colour(0xffe9b65f));
  look.setColour(juce::Slider::thumbColourId, juce::Colour(0xffffd990));
  look.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2c4656));
  setLookAndFeel(&look);
  setResizable(true, true);
  setResizeLimits(950, 720, 1800, 1300);
  for (auto *c : std::initializer_list<juce::Component *>{
           &mode,    &undo,      &redo,        &import,      &save,
           &load,    &favorite,  &preview,     &throwButton, &wave,
           &lockKey, &lockInput, &lockRouting, &search,      &preset,
           &section, &status,    &sample,      &viewport,    &eq})
    addAndMakeVisible(c);
  viewport.setViewedComponent(&content, false);
  search.setTextToShowWhenEmpty("Suche (* = Favoriten)", juce::Colours::grey);
  search.onTextChange = [this] { populatePresets(); };
  populatePresets();
  preset.onChange = [this] { proc.applyPreset(preset.getSelectedId() - 1); };
  mode.onClick = [this] {
    advanced = !advanced;
    mode.setButtonText(advanced ? "Simple" : "Advanced");
    rebuild();
  };
  undo.onClick = [this] {proc.state.copyState();proc.undo.undo(); };
  redo.onClick = [this] {proc.state.copyState();proc.undo.redo(); };
  import.onClick = [this] {
    chooser = std::make_unique<juce::FileChooser>(
        "Crowd importieren", juce::File{}, "*.wav;*.aif;*.aiff;*.flac");
    auto safe = juce::Component::SafePointer<MalleEditor>(this);
    chooser->launchAsync(juce::FileBrowserComponent::openMode |
                             juce::FileBrowserComponent::canSelectFiles,
                         [safe](const juce::FileChooser &fc) {
                           if (safe && fc.getResult().existsAsFile())
                             safe->filesDropped(
                                 {fc.getResult().getFullPathName()}, 0, 0);
                         });
  };
  throwButton.onStateChange = [this] {
    proc.setValue("throw.trigger", throwButton.isDown() ? 1.f : 0.f);
  };
  preview.setClickingTogglesState(true);
  preview.onClick = [this] {
    proc.engine.audition.store(preview.getToggleState());
  };
  save.onClick = [this] { choose(true); };
  load.onClick = [this] { choose(false); };
  favorite.setClickingTogglesState(true);
  favorite.onClick = [this] {
    auto key = "fav." + juce::String(preset.getSelectedId());
    proc.state.state.setProperty(key, !bool(proc.state.state.getProperty(key)),
                                 &proc.undo);
  };
  auto lock = [this](juce::ToggleButton &b, const char *key) {
    b.setToggleState(bool(proc.state.state.getProperty(key)),
                     juce::dontSendNotification);
    b.onClick = [this, &b, key] {
      proc.undo.beginNewTransaction("Lock");
      proc.state.state.setProperty(key, b.getToggleState(), &proc.undo);
    };
  };
  lock(lockKey, "lock.key");
  lock(lockInput, "lock.input");
  lock(lockRouting, "lock.routing");
  juce::StringArray groups;
  for (auto &d : malle::parameters())
    if (!groups.contains(d.section))
      groups.add(d.section);
  section.addItemList(groups, 1);
  section.setSelectedId(1);
  section.onChange = [this] {
    if (advanced)
      rebuild();
  };
  for (int i = 0; i < 6; ++i) {
    addAndMakeVisible(variants[i]);
    for (int j = 0; j < 20; ++j)
      variants[i].addItem(malle::variants()[j], j + 1);
    std::string key = std::array<std::string, 6>{
        "druck", "klang", "waerme", "breite", "raum", "bewegung"}[i];
    variantAttachments[i] = std::make_unique<
        juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        proc.state, key + ".variant", variants[i]);
  }
  setSize(1120, 850);
  rebuild();
  startTimerHz(20);
  timerCallback();
}
MalleEditor::~MalleEditor() {
  stopTimer();
  proc.engine.audition.store(false);
  setLookAndFeel(nullptr);
}
void MalleEditor::populatePresets() {
  int selected = preset.getSelectedId();
  preset.clear(juce::dontSendNotification);
  auto q = search.getText();bool onlyFavorites=q.startsWith("*");if(onlyFavorites)q=q.substring(1).trim();
  int i = 0;
  for (auto &p : malle::presets()) {
    ++i;
    bool fav=bool(proc.state.state.getProperty("fav."+juce::String(i)));
    juce::String text = (fav?"* ":"")+p.category + " / " + p.name;
    if (text.containsIgnoreCase(q)&&(!onlyFavorites||fav))
      preset.addItem(text, i);
  }
  if (selected == 0) {
    int i = 0;
    for (auto &p : malle::presets()) {
      ++i;
      if (juce::String(p.name) == proc.currentPreset) {
        selected = i;
        break;
      }
    }
  }
  preset.setSelectedId(selected, juce::dontSendNotification);
}
void MalleEditor::rebuild() {
  controls.clear();
  auto display = malle::parameters();
  if (!advanced)
    std::stable_sort(display.begin(), display.end(), [](auto &a, auto &b) {
      auto macro = [](auto &d) {
        return d.id.find(".amount") != std::string::npos &&
               d.section == d.id.substr(0, d.id.find('.'));
      };
      return macro(a) > macro(b);
    });
  for (auto &d : display) {
    bool show = advanced ? juce::String(d.section) == section.getText()
                         : (d.id == "input" || d.id == "output" ||
                            d.id == "fx.mix" || d.id == "tune.on" ||
                            d.id == "tune.root" || d.id == "tune.scale" ||
                            d.id == "stadium.mix" || d.id == "stadium.on" ||
                            d.id == "throw.trigger" || d.id == "res.amount" ||
                            d.id == "res.on" || d.id == "air.high" ||
                            d.id == "air.presence" || d.id == "air.on" ||
                            d.id.find(".amount") != std::string::npos &&
                                d.section == d.id.substr(0, d.id.find('.')));
    if (show) {
      auto c = std::make_unique<Control>(proc, d);
      content.addAndMakeVisible(*c);
      controls.push_back(std::move(c));
    }
  }
  for (auto &v : variants)
    v.setVisible(!advanced);
  section.setVisible(advanced);
  resized();
}
void MalleEditor::resized() {
  int w = getWidth();
  mode.setBounds(w - 135, 20, 110, 32);
  undo.setBounds(w - 340, 20, 90, 32);
  redo.setBounds(w - 240, 20, 90, 32);
  preset.setBounds(24, 83, w - 610, 32);
  search.setBounds(w - 570, 83, 180, 32);
  save.setBounds(w - 375, 83, 170, 32);
  load.setBounds(w - 190, 83, 165, 32);
  lockKey.setBounds(24, 125, 145, 28);
  lockInput.setBounds(175, 125, 140, 28);
  lockRouting.setBounds(320, 125, 150, 28);
  favorite.setBounds(480, 125, 90, 28);
  section.setBounds(w - 270, 125, 245, 28);
  eq.setBounds(24, 165, w - 48, 135);
  import.setBounds(24, 312, 200, 32);
  sample.setBounds(235, 312, w - 540, 32);
  preview.setBounds(w - 140, 312, 115, 32);
  throwButton.setBounds(w - 290, 312, 140, 32);
  wave.setBounds(24, 355, w - 48, 42);
  int top = 408;
  for (int i = 0; i < 6; ++i)
    variants[i].setBounds(24 + i * (w - 48) / 6, top, (w - 60) / 6, 30);
  viewport.setBounds(20, advanced ? 408 : 450, w - 40,
                     getHeight() - (advanced ? 473 : 515));
  int cw = viewport.getWidth() - 20, columns = advanced ? 4 : 6,
      width = cw / columns, height = 155;
  for (size_t i = 0; i < controls.size(); ++i)
    controls[i]->setBounds(int(i) % columns * width, int(i) / columns * height,
                           width - 8, height - 4);
  content.setSize(cw, int((controls.size() + columns - 1) / columns) * height);
  status.setBounds(24, getHeight() - 52, w - 48, 44);
}
void MalleEditor::paint(juce::Graphics &g) {
  g.fillAll(juce::Colour(0xff14202c));
  g.setColour(juce::Colours::white);
  g.setFont(30);
  g.drawText("MALLE VOCALS", 24, 14, 360, 38, juce::Justification::left);
  g.setFont(14);
  g.setColour(juce::Colour(0xffc5d3dd));
  g.drawText("ROHSTOFF AUDIO / MICHA BENJAMIN   /   0.1.0 ALPHA", 24, 53, 620,
             22, juce::Justification::left);
}
void MalleEditor::timerCallback() {
  auto &m = proc.engine.meters;
  juce::String sc =
      (proc.getValue("duck.source") > .5f || proc.getValue("pump.mode") > .5f)
          ? (proc.sidechainEnabled.load()
                 ? (m.sidechain.load() > 1e-6f ? "SC Signal"
                                               : "SC: kein Signal")
                 : "SC: Eingang fehlt")
          : "Ducking intern";
  status.setText(
      "IN " + juce::String(malle::db(m.input.load()), 1) + " dB  |  OUT " +
          juce::String(malle::db(m.output.load()), 1) + " dB  |  GR FET " +
          juce::String(m.fet.load(), 1) + " / Level " +
          juce::String(m.level.load(), 1) + " dB  |  MIDI " +
          juce::String(m.pitch.load(), 1) + "  |  " + sc +
          "\nInput EQ > Tune* > FET > Level > DeEss > EQ > Multiband > "
          "Resonanz > Air > Sat > Doubler/Stadium > Raum",
      juce::dontSendNotification);
  sample.setText(proc.sampleName +
                     (proc.sampleMissing.load() ? " - FEHLER" : ""),
                 juce::dontSendNotification);
  lockKey.setToggleState(bool(proc.state.state.getProperty("lock.key")),juce::dontSendNotification);lockInput.setToggleState(bool(proc.state.state.getProperty("lock.input")),juce::dontSendNotification);lockRouting.setToggleState(bool(proc.state.state.getProperty("lock.routing")),juce::dontSendNotification);favorite.setToggleState(bool(proc.state.state.getProperty("fav."+juce::String(preset.getSelectedId()))),juce::dontSendNotification);
  undo.setEnabled(proc.undo.canUndo());
  redo.setEnabled(proc.undo.canRedo());
  eq.repaint();
  wave.repaint();
}
void MalleEditor::filesDropped(const juce::StringArray &files, int, int) {
  if (files.isEmpty())
    return;
  juce::String error;
  if (!proc.importSample(juce::File(files[0]), error))
    juce::AlertWindow::showMessageBoxAsync(
        juce::MessageBoxIconType::WarningIcon, "Sample Import", error);
}
void MalleEditor::choose(bool saving) {
  chooser = std::make_unique<juce::FileChooser>(
      saving ? "Preset speichern" : "Preset laden",
      juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
      "*.mallevocals");
  auto safe = juce::Component::SafePointer<MalleEditor>(this);
  chooser->launchAsync(
      (saving ? juce::FileBrowserComponent::saveMode |
                    juce::FileBrowserComponent::warnAboutOverwriting
              : juce::FileBrowserComponent::openMode) |
          juce::FileBrowserComponent::canSelectFiles,
      [safe, saving](const juce::FileChooser &fc) {
        if (!safe || fc.getResult() == juce::File{})
          return;
        if (saving)
          safe->proc.savePreset(
              fc.getResult().withFileExtension("mallevocals"));
        else
          safe->proc.loadPreset(fc.getResult());
      });
}

void WaveView::paint(juce::Graphics &g) {
  g.fillAll(juce::Colour(0xff101b26));
  g.setColour(juce::Colour(0xff6bd3cd));
  for (int i = 0; i < 512; ++i) {
    float x = getWidth() * i / 512.f,
          peak = std::min(1.f, proc.waveform[size_t(i)]);
    g.drawVerticalLine(int(x), getHeight() * (.5f - peak * .45f),
                       getHeight() * (.5f + peak * .45f));
  }
  g.setFont(juce::FontOptions(14));
  g.drawText("Crowd-Wellenform / Audiodatei hier ablegen", 10, 2,
             getWidth() - 20, 20, juce::Justification::left);
}

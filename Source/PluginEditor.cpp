#include "PluginEditor.h"
namespace {
const juce::Colour orange(0xffffb53d), teal(0xff4eddd4), ink(0xff101416);
const std::array<std::string, 6> macroIds{"druck",  "klang", "waerme",
                                          "breite", "raum",  "bewegung"};
const std::array<juce::String, 6> macroNames{
    "DRUCK", "KLANG", "WAERME", "BREITE", "HALL & DELAY", "PUMPEN"};
const std::array<juce::String, 11> nodeNames{
    "Input EQ",  "Tune",     "FET", "Leveler",    "De-Esser",       "Klang EQ",
    "Multiband", "Resonanz", "Air", "Saettigung", "Pitch / Formant"};
const std::array<std::string, 11> nodeOn{
    "inputEQ.on", "tune.on", "fet.on", "level.on", "deess.on",   "eq.on",
    "mb.on",      "res.on",  "air.on", "sat.on",   "creative.on"};
const std::array<juce::String, 11> nodeSections{
    "Input EQ",  "Tune",       "FET", "Leveling",   "De-Esser",     "Klang EQ",
    "Multiband", "Resonanzen", "Air", "Saturation", "Pitch/Formant"};
const std::array<juce::String, 10> parallelNames{
    "Parallel-Kompressor", "Doubler / Chor", "Stadium", "Kurzer Raum",
    "Short Delay",         "Long Delay",     "Hall",    "Throw Delay",
    "FX-Ducking",          "Pumpen"};
const std::array<std::string, 10> parallelOn{
    "parallel.on", "double.on", "stadium.on", "room.on", "short.on",
    "long.on",     "reverb.on", "throw.on",   "duck.on", "pump.on"};
const std::array<juce::String, 10> parallelSection{
    "Parallel", "Doubler", "Stadium", "room",    "short",
    "long",     "reverb",  "throw",   "Ducking", "Bewegung"};
} // namespace
void GraphiteLook::drawRotarySlider(juce::Graphics &g, int x, int y, int w,
                                    int h, float pos, float start, float end,
                                    juce::Slider &s) {
  float radius = std::min(w, h) * .38f, cx = x + w * .5f, cy = y + h * .5f;
  juce::Rectangle<float> r(cx - radius, cy - radius, radius * 2, radius * 2);
  auto accent =
      bool(s.getProperties().getWithDefault("teal", false)) ? teal : orange;
  g.setColour(juce::Colours::black.withAlpha(.7f));
  g.fillEllipse(r.expanded(5).translated(2, 5));
  juce::ColourGradient gradient(juce::Colour(0xff62666a), cx - radius,
                                cy - radius, juce::Colour(0xff080a0b),
                                cx + radius, cy + radius, false);
  g.setGradientFill(gradient);
  g.fillEllipse(r.expanded(3));
  g.setColour(juce::Colour(0xff090c0d));
  g.fillEllipse(r);
  juce::ColourGradient face(juce::Colour(0xff2a2d2e), cx - radius * .6f,
                            cy - radius * .7f, juce::Colour(0xff07090a),
                            cx + radius, cy + radius, false);
  g.setGradientFill(face);
  g.fillEllipse(r.reduced(4));
  g.setColour(juce::Colour(0xff171c1e));
  juce::Path track;
  track.addCentredArc(cx, cy, radius + 8, radius + 8, 0, start, end, true);
  g.strokePath(track, juce::PathStrokeType(5));
  g.setColour(accent);
  juce::Path arc;
  arc.addCentredArc(cx, cy, radius + 8, radius + 8, 0, start,
                    start + pos * (end - start), true);
  g.strokePath(arc, juce::PathStrokeType(4));
  for (int i = 0; i <= 10; ++i) {
    float a = start + (end - start) * i / 10;
    g.setColour(juce::Colour(0xffa5a9aa));
    g.drawLine(
        cx + std::sin(a) * (radius + 15), cy - std::cos(a) * (radius + 15),
        cx + std::sin(a) * (radius + 19), cy - std::cos(a) * (radius + 19), 1);
  }
  float a = start + pos * (end - start);
  g.setColour(accent.brighter());
  g.drawLine(cx + std::sin(a) * radius * .55f, cy - std::cos(a) * radius * .55f,
             cx + std::sin(a) * (radius - 5), cy - std::cos(a) * (radius - 5),
             3);
}
void GraphiteLook::drawButtonBackground(juce::Graphics &g, juce::Button &b,
                                        const juce::Colour &, bool over,
                                        bool down) {
  auto r = b.getLocalBounds().toFloat().reduced(1);
  auto c = b.getToggleState() || down
               ? orange
               : juce::Colour(over ? 0xff353b3d : 0xff22282a);
  g.setGradientFill(juce::ColourGradient(c.brighter(.25f), 0, 0, c.darker(.45f),
                                         0, float(b.getHeight()), false));
  g.fillRoundedRectangle(r, 4);
  g.setColour(b.getToggleState() ? orange.brighter()
                                 : juce::Colour(0xff657073));
  g.drawRoundedRectangle(r, 4, 1);
}
ValueControl::ValueControl(MalleProcessor &p, std::string id,
                           juce::String title, bool isTeal) {
  auto &d = malle::parameters()[size_t(malle::index(id))];
  if (title.isEmpty())
    title = d.name;
  label.setText(title + (d.unit.empty() ? "" : " (" + d.unit + ")"),
                juce::dontSendNotification);
  label.setFont(juce::FontOptions(15));
  label.setJustificationType(juce::Justification::centred);
  addAndMakeVisible(label);
  slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 100, 24);
  slider.setDoubleClickReturnValue(true, d.def);
  slider.getProperties().set("teal", isTeal);
  slider.setTooltip(d.section + ": " + d.name +
                    ". Doppelklick: Reset. Shift: fein. Zahl editierbar." +
                    ((id.find(".decay")!=std::string::npos || id=="reverb.type") ? " Party-Klang 0.4 muss aktiv sein." : ""));
  addAndMakeVisible(slider);
  attach =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          p.state, id, slider);
  slider.onDragStart = [&p] {
    p.state.copyState();
    p.undo.beginNewTransaction("Parameter");
  };
  slider.onDragEnd = [&p] {
    p.state.copyState();
    p.undo.beginNewTransaction();
  };
}
void ValueControl::resized() {
  auto r = getLocalBounds();
  label.setBounds(r.removeFromTop(24));
  slider.setBounds(r);
}
ParamChoice::ParamChoice(MalleProcessor &p, std::string id, juce::String title,
                         juce::StringArray names, std::vector<float> v)
    : values(std::move(v)) {
  label.setText(title, juce::dontSendNotification);
  label.setFont(juce::FontOptions(14));
  addAndMakeVisible(label);
  box.addItemList(names, 1);
  addAndMakeVisible(box);
  if (values.empty())
    for (int i = 0; i < names.size(); ++i)
      values.push_back(float(i));
  auto *parameter = p.state.getParameter(id);
  attach = std::make_unique<juce::ParameterAttachment>(
      *parameter,
      [this](float value) {
        int best = -1;
        for (size_t i = 0; i < values.size(); ++i)
          if (std::abs(value - values[i]) < .001f)
            best = int(i);
        box.setSelectedId(best + 1, juce::dontSendNotification);
        if (best < 0)
          box.setText(juce::String(value, 2) + " (Custom)",
                      juce::dontSendNotification);
      },
      &p.undo);
  box.onChange = [this, &p] {
    int i = box.getSelectedId() - 1;
    if (i >= 0) {
      p.undo.beginNewTransaction("Auswahl");
      attach->setValueAsCompleteGesture(values[size_t(i)]);
      p.state.copyState();
      p.undo.beginNewTransaction();
    }
  };
  attach->sendInitialUpdate();
}
void ParamChoice::resized() {
  auto r = getLocalBounds();
  label.setBounds(r.removeFromTop(20));
  box.setBounds(r.reduced(1));
}
ParamToggle::ParamToggle(MalleProcessor &p, std::string id, juce::String text)
    : TextButton(text) {
  setClickingTogglesState(true);
  attach =
      std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
          p.state, id, *this);
}
void Card::paint(juce::Graphics &g) {
  auto r = getLocalBounds().toFloat().reduced(1);
  g.setGradientFill(juce::ColourGradient(
      juce::Colour(0xff303638), 0, 0, juce::Colour(0xff151a1c),
      float(getWidth()), float(getHeight()), false));
  g.fillRoundedRectangle(r, 5);
  g.setColour(juce::Colour(0xff080b0c));
  g.drawRoundedRectangle(r, 5, 2);
  g.setColour(juce::Colour(0xff596164));
  g.drawRoundedRectangle(r.reduced(2), 3, .6f);
  g.setFont(juce::FontOptions(17, juce::Font::bold));
  g.setColour(teal ? ::teal : orange);
  g.drawText(title, 14, 8, getWidth() - 30, 25, juce::Justification::left);
}
namespace {
class SpaceControls final : public Card {
  GraphiteLook look;
  std::vector<std::unique_ptr<juce::Component>> controls;
public:
  explicit SpaceControls(MalleProcessor& p):Card("HALL & DELAY / 100% WET RETURNS",true) {
    setLookAndFeel(&look);
    look.setColour(juce::Label::textColourId,juce::Colour(0xffeef0f0));
    look.setColour(juce::TextButton::textColourOffId,juce::Colours::white);
    look.setColour(juce::TextButton::textColourOnId,juce::Colours::black);
    look.setColour(juce::Slider::textBoxBackgroundColourId,juce::Colour(0xff101719));
    look.setColour(juce::Slider::textBoxOutlineColourId,juce::Colour(0xff515c60));
    look.setColour(juce::ComboBox::backgroundColourId,juce::Colour(0xff111719));
    look.setColour(juce::ComboBox::outlineColourId,juce::Colour(0xff586063));
    look.setColour(juce::PopupMenu::backgroundColourId,juce::Colour(0xff172023));
    setSize(800,665);
    auto revision=std::make_unique<ParamToggle>(p,"quality.on","Party-Klang 0.4");
    revision->setTooltip("Neue Makro-Kennlinien und Hall-Dauer/Typ. Aus: bisheriger Klang alter Projekte.");
    addAndMakeVisible(*revision);revision->setBounds(592,7,194,27);controls.push_back(std::move(revision));
    for(int col=0;col<4;++col){
      std::string id=std::array<std::string,4>{"room","reverb","short","long"}[col];
      auto put=[&](std::unique_ptr<juce::Component> c,int y,int h){
        addAndMakeVisible(*c);c->setBounds(14+col*196,y,184,h);controls.push_back(std::move(c));
      };
      put(std::make_unique<ParamToggle>(p,id+".on",std::array<juce::String,4>{"Kurzer Raum","Hall","Short Delay","Long Delay"}[col]),43,29);
      put(std::make_unique<ValueControl>(p,id+".send","Send",true),80,112);
      if(col<2){
        put(std::make_unique<ValueControl>(p,id+".decay","Dauer",true),200,112);
        put(std::make_unique<ValueControl>(p,id+".time","Pre-Delay",true),320,112);
        if(col==1)put(std::make_unique<ParamChoice>(p,"reverb.type","Hall-Typ",juce::StringArray{"Room","Plate","Hall"}),446,56);
      }else{
        put(std::make_unique<ValueControl>(p,id+".feedback","Feedback",true),200,112);
        put(std::make_unique<ValueControl>(p,id+".time","Zeit (Sync aus)",true),320,112);
        put(std::make_unique<ParamToggle>(p,id+".sync","Tempo Sync"),439,26);
        put(std::make_unique<ParamChoice>(p,id+".beats","Rhythmus",juce::StringArray{"1/16","1/8","1/8 punktiert","1/4","1/4 triolisch","1/2"},std::vector<float>{.25f,.5f,.75f,1.f,2.f/3.f,2.f}),470,52);
      }
    }
    auto addFooter=[&](std::unique_ptr<juce::Component> c,int x,int y,int w,int h){addAndMakeVisible(*c);c->setBounds(x,y,w,h);controls.push_back(std::move(c));};
    addFooter(std::make_unique<ParamToggle>(p,"duck.on","FX Ducking ON"),14,535,170,28);
    addFooter(std::make_unique<ParamChoice>(p,"duck.source","Quelle",juce::StringArray{"Eigene Vocals","Externer Lead"}),14,574,174,60);
    addFooter(std::make_unique<ValueControl>(p,"duck.amount","Ducking 0-100%",true),210,535,176,120);
    addFooter(std::make_unique<ValueControl>(p,"duck.threshold","Gesang ab",true),406,535,176,120);
    addFooter(std::make_unique<ValueControl>(p,"duck.release","Rueckkehr",true),602,535,176,120);
  }
  void detachParameters() {controls.clear();}
  ~SpaceControls() override {setLookAndFeel(nullptr);}
};
}
MacroCard::MacroCard(MalleProcessor &p, int n)
    : Card(macroNames[n], n == 4), proc(p), slot(n) {
  auto id = macroIds[n];
  trim=std::make_unique<ValueControl>(p,id+".trim","Pegel",n==4);
  addAndMakeVisible(*trim);
  if(n<4){match=std::make_unique<ParamToggle>(p,id+".match.on","Pegelgleich");addAndMakeVisible(*match);}
  if(n==4){
    duck=std::make_unique<ValueControl>(p,"duck.amount","FX-Ducking",true);addAndMakeVisible(*duck);
    addAndMakeVisible(spaceDetails);
    spaceDetails.onClick=[this]{

      if(closePopup)closePopup();
      auto controls=std::make_unique<SpaceControls>(proc);
      juce::Component::SafePointer<SpaceControls> panel(controls.get());
      auto* parent=getTopLevelComponent();
      auto target=parent->getLocalArea(&spaceDetails,spaceDetails.getLocalBounds());
      // Embed in the plugin window: no separate desktop/display is required.
      auto& callout=juce::CallOutBox::launchAsynchronously(std::move(controls),target,parent);
      juce::Component::SafePointer<juce::CallOutBox> popup(&callout);
      // The modal callback can outlive its editor. Detach all parameter
      // references synchronously before the processor may be destroyed.
      closePopup=[panel,popup]{if(panel)panel->detachParameters();if(popup){popup->setVisible(false);popup->dismiss();}};
    };
  }
  amount =
      std::make_unique<ValueControl>(p, id + ".amount", "Intensitaet", n == 4);
  on = std::make_unique<ParamToggle>(p, id + ".on", "ON");
  juce::StringArray names;
  for (auto &v : malle::variants())
    names.add(v);
  variant =
      std::make_unique<ParamChoice>(p, id + ".variant", "Variante", names);
  for (auto *c : std::initializer_list<juce::Component *>{
           amount.get(), on.get(), variant.get()})
    addAndMakeVisible(c);
  if (n == 1) {
    colour=std::make_unique<ValueControl>(p,"klang.colour","Dunkel / Hell");addAndMakeVisible(*colour);
    res = std::make_unique<ValueControl>(p, "res.amount", "Resonanz");
    air = std::make_unique<ValueControl>(p, "air.high", "Air");
    resOn = std::make_unique<ParamToggle>(p, "res.on", "Res ON");
    airOn = std::make_unique<ParamToggle>(p, "air.on", "Air ON");
    addAndMakeVisible(*resOn);
    addAndMakeVisible(*airOn);
    addAndMakeVisible(*res);
    addAndMakeVisible(*air);
  }
  if (n == 5) {
    mode = std::make_unique<ParamChoice>(
        p, "pump.mode", "Steuerung", juce::StringArray{"Tempo", "Extern SC"});
    rate = std::make_unique<ParamChoice>(
        p, "pump.beats", "Raster",
        juce::StringArray{"1/16", "1/8", "1/4", "1/2"},
        std::vector<float>{.25f, .5f, 1, 2});
    addAndMakeVisible(*mode);
    addAndMakeVisible(*rate);
  }
}
MacroCard::~MacroCard() {if(closePopup)closePopup();}
void MacroCard::resized() {
  int w = getWidth(), h = getHeight();
  on->setBounds(w - 70, 10, 55, 26);
  variant->setBounds(w - 153, 43, 138, 48);
  amount->setBounds(12, slot == 1 ? 84 : 83, slot == 1 ? w - 110 : w - 24,
                    h - 94);
  if (res) {
    int y = 92, half = (h - y - 10) / 2;
    resOn->setBounds(w - 100, y, 86, 23);
    res->setBounds(w - 107, y + 25, 99, half - 28);
    airOn->setBounds(w - 100, y + half, 86, 23);
    air->setBounds(w - 107, y + half + 25, 99, half - 28);
  }
  if (mode) {
    amount->setBounds(10, 90, w - 155, h - 100);
    mode->setBounds(w - 145, 103, 130, 48);
    rate->setBounds(w - 145, 165, 130, 48);
  }
  if(match)match->setBounds(12,48,103,27);
  trim->setBounds(12,h-81,94,76);
  auto a=amount->getBounds();a.setBottom(h-86);amount->setBounds(a);
  if(slot==4)spaceDetails.setBounds(112,h-51,w-126,34);
  if(duck){amount->setBounds(12,90,w-130,h-180);duck->setBounds(w-115,95,103,h-185);}
  if(colour){auto r=amount->getBounds();r.setBottom(h-174);amount->setBounds(r);colour->setBounds(12,h-169,w-124,81);}
}
ThrowPanel::ThrowPanel(MalleProcessor &p) : Card("THROW DELAY", true), proc(p) {
  type = std::make_unique<ParamChoice>(
      p, "throw.type", "Typ", juce::StringArray{"Mono", "Stereo", "Ping Pong"});
  grid = std::make_unique<ParamChoice>(
      p, "throw.beats", "Raster",
      juce::StringArray{"1/32", "1/16", "1/8", "3/16", "1/4", "1/2", "1/1"},
      std::vector<float>{.125f, .25f, .5f, .75f, 1, 2, 4});
  feel = std::make_unique<ParamChoice>(
      p, "throw.feel", "Feel",
      juce::StringArray{"Gerade", "Punktiert", "Triolisch"});
  verbType =
      std::make_unique<ParamChoice>(p, "throw.verb.type", "Hall-Typ",
                                    juce::StringArray{"Room", "Plate", "Hall"});
  verbOn = std::make_unique<ParamToggle>(p, "throw.verb.on", "Hall ON");
  on = std::make_unique<ParamToggle>(p, "throw.on", "ON");
  sync = std::make_unique<ParamToggle>(p, "throw.sync", "Tempo Sync");
  for (auto *c : std::initializer_list<juce::Component *>{
           &trigger, &kill, sync.get(), type.get(), grid.get(), feel.get(),
           verbType.get(), verbOn.get(), on.get()})
    addAndMakeVisible(c);
  trigger.onStateChange = [this] {
    proc.setValue("throw.trigger", trigger.isDown() ? 1 : 0);
  };
  kill.onStateChange = [this] {
    proc.setValue("throw.kill", kill.isDown() ? 1 : 0);
  };
  for (auto pair : std::initializer_list<std::pair<std::string, juce::String>>{
           {"throw.feedback", "Feedback"},
           {"throw.return", "Throw-Mix"},
           {"throw.verb.amount", "Hall-Menge"}}) {
    auto c = std::make_unique<ValueControl>(p, pair.first, pair.second, true);
    addAndMakeVisible(*c);
    primary.push_back(std::move(c));
  }
  for (auto pair : std::initializer_list<std::pair<std::string, juce::String>>{
           {"throw.low", "Low Cut"},
           {"throw.high", "High Cut"},
           {"throw.width", "Breite"},
           {"throw.window", "Send-Fenster"},
           {"throw.offset", "Startversatz"},
           {"throw.quantize", "Quant. Beats"},
           {"throw.tail", "Max. Tail"},
           {"throw.fade", "Fade-out"},
           {"throw.time", "Freie Zeit"},
           {"throw.send", "Send-Pegel"},
           {"throw.verb.decay", "Hall-Dauer"}}) {
    auto c = std::make_unique<ValueControl>(p, pair.first, pair.second, true);
    addAndMakeVisible(*c);
    detail.push_back(std::move(c));
  }
  trigger.setTooltip("Steigende Flanke oeffnet das Send-Fenster. Loslassen "
                     "schliesst den Send; Echo und Hall klingen weiter.");
}
void ThrowPanel::setAdvanced(bool a) {
  advanced = a;
  for (auto &c : detail)
    c->setVisible(a);
  kill.setVisible(a);
  sync->setVisible(true);
  resized();
}
void ThrowPanel::resized() {
  int w = getWidth();
  on->setBounds(w - 65, 8, 50, 25);
  trigger.setBounds(15, 44, 155, 65);
  int left = 185;
  int col = (w - 200) / 8;
  type->setBounds(left, 43, col - 7, 55);
  grid->setBounds(left + col, 43, col - 7, 55);
  feel->setBounds(left + 2 * col, 43, col - 7, 55);
  primary[0]->setBounds(left + 3 * col, 30, col, 100);
  primary[1]->setBounds(left + 4 * col, 30, col, 100);
  verbOn->setBounds(left + 5 * col, 62, col - 9, 30);
  verbType->setBounds(left + 6 * col, 43, col - 7, 55);
  primary[2]->setBounds(left + 7 * col, 30, col, 100);
  sync->setBounds(15, 111, 155, 24);
  if (advanced) {
    int dw = (w - 155) / int(detail.size());
    for (int i = 0; i < int(detail.size()); ++i)
      detail[i]->setBounds(12 + i * dw, 132, dw - 5, 107);
    kill.setBounds(w - 140, 168, 125, 36);
  }
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
void RoutingView::paint(juce::Graphics &g) {
  g.fillAll(juce::Colour(0xff111719));
  auto order = proc.routing();
  auto base = malle::defaults();
  for (size_t i = 0; i < base.size(); ++i)
    base[i] = proc.getValue(malle::parameters()[i].id);
  auto effective = malle::resolve(base);
  auto heading = [&](juce::String text, int y) {
    g.setColour(juce::Colour(0xff283235));
    g.fillRect(0, y - scroll, getWidth(), 28);
    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(14, juce::Font::bold));
    g.drawText(text, 9, y - scroll, getWidth() - 12, 28,
               juce::Justification::left);
  };
  auto row = [&](juce::String text, std::string id, int y, bool serial) {
    bool on = effective[size_t(malle::index(id))] > .5f;
    g.setColour(on ? juce::Colour(0xffe6e9e9) : juce::Colour(0xff727e81));
    g.setFont(juce::FontOptions(14));
    g.drawText((serial ? ":: " : "> ") + text, 9, y - scroll, getWidth() - 37,
               26, juce::Justification::left);
    g.setColour(on ? (serial ? orange : teal) : juce::Colour(0xff41494b));
    g.drawEllipse(float(getWidth() - 25), float(y - scroll + 8), 11, 11, 2);
  };
  heading("DIREKTKETTE - verschiebbar", 0);
  for (int i = 0; i < 11; ++i)
    row(nodeNames[order[i]], nodeOn[order[i]], 28 + i * 26, true);
  heading("PARALLEL - Abgriff / Return", 314);
  for (int i = 0; i < 8; ++i)
    row(parallelNames[i], parallelOn[i], 342 + i * 26, false);
  heading("STEUERUNG - Quelle / Ziele", 550);
  for (int i = 8; i < 10; ++i)
    row(parallelNames[i], parallelOn[i], 578 + (i - 8) * 26, false);
  heading("OUTPUT - fester Abschluss", 630);
  if (drop >= 0) {
    g.setColour(orange);
    g.fillRect(4, 28 + drop * 26 - scroll, getWidth() - 8, 3);
  }
}
void RoutingView::mouseDown(const juce::MouseEvent &e) {
  int y = e.y + scroll;
  from = -1;
  auto order = proc.routing();
  if (y >= 28 && y < 314) {
    int row = (y - 28) / 26;
    if (e.x > getWidth() - 34) {
      auto id = nodeOn[order[row]];
      proc.undo.beginNewTransaction("Modul-Bypass");
      proc.setValue(id, proc.getValue(id) > .5f ? 0 : 1);
      proc.state.copyState();
      proc.undo.beginNewTransaction();
    } else {
      from = row;
      select(nodeSections[order[row]]);
    }
  } else {
    int row = y >= 342 && y < 550   ? (y - 342) / 26
              : y >= 578 && y < 630 ? 8 + (y - 578) / 26
                                    : -1;
    if (row >= 0 && row < 10) {
      if (e.x > getWidth() - 34) {
        auto id = parallelOn[row];
        proc.undo.beginNewTransaction("Weg-Bypass");
        proc.setValue(id, proc.getValue(id) > .5f ? 0 : 1);
        proc.state.copyState();
        proc.undo.beginNewTransaction();
      } else
        select(parallelSection[row]);
    }
  }
  repaint();
}
void RoutingView::mouseDrag(const juce::MouseEvent &e) {
  if (from >= 0 && e.getDistanceFromDragStart() > 5)
    if (auto *d = juce::DragAndDropContainer::findParentDragContainerFor(this))
      if (!d->isDragAndDropActive())
        d->startDragging(from, this);
}
void RoutingView::mouseWheelMove(const juce::MouseEvent &,
                                 const juce::MouseWheelDetails &w) {
  scroll = juce::jlimit(0, std::max(0, 658 - getHeight()),
                        scroll - int(w.deltaY * 150));
  repaint();
}
bool RoutingView::isInterestedInDragSource(const SourceDetails &s) {
  return s.sourceComponent.get() == this && int(s.description) >= 0 &&
         int(s.description) < 11;
}
void RoutingView::itemDragMove(const SourceDetails &s) {
  int y = s.localPosition.y + scroll;
  drop = y >= 28 && y < 314 ? (y - 28) / 26 : -1;
  repaint();
}
void RoutingView::itemDragExit(const SourceDetails &) {
  drop = -1;
  repaint();
}
void RoutingView::itemDropped(const SourceDetails &s) {
  if (drop >= 0)
    proc.moveModule(int(s.description), drop);
  drop = -1;
  from = -1;
  repaint();
}
MalleEditor::MalleEditor(MalleProcessor &p)
    : AudioProcessorEditor(p), proc(p), throws(p), wave(p), eq(p),
      routing(p, [this](juce::String s) { selectModule(s); }) {
  setLookAndFeel(&look);
  look.setColour(juce::Label::textColourId, juce::Colour(0xffeef0f0));
  look.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
  look.setColour(juce::TextButton::textColourOnId, juce::Colours::black);
  look.setColour(juce::Slider::textBoxBackgroundColourId,
                 juce::Colour(0xff101719));
  look.setColour(juce::Slider::textBoxOutlineColourId,
                 juce::Colour(0xff515c60));
  look.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff111719));
  look.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff586063));
  look.setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff172023));
  setResizable(true, true);
  setResizeLimits(800, 625, 1792, 1400);
  for (auto *c : std::initializer_list<juce::Component *>{
           &simple,      &advancedButton, &undo,    &redo,     &save,
           &load,        &favorite,       &import,  &preview,  &lockKey,
           &lockInput,   &lockRouting,    &preset,  &section,  &search,
           &status,      &sourceInfo,     &throws,  &stadium,  &tuning,
           &io,          &detailCard,     &routing, &viewport, &eq,
           &deessPresets})
    addAndMakeVisible(c);
  for (int i = 0; i < 6; ++i) {
    macros[i] = std::make_unique<MacroCard>(p, i);
    addAndMakeVisible(*macros[i]);
  }
  const juce::StringArray tabNames{"Klang & Dynamik", "Tune & Stimmen",
                                   "Stadium", "Hall & Delay", "Routing"};
  for (int i = 0; i < 5; ++i) {
    tabs[i].setButtonText(tabNames[i]);
    addAndMakeVisible(tabs[i]);
    tabs[i].onClick = [this, i] {
      selectModule(juce::StringArray{"Klang EQ", "Tune", "Stadium", "reverb",
                                     "Routing"}[i]);
    };
  }
  stadiumOn = std::make_unique<ParamToggle>(p, "stadium.on", "ON");
  stadiumAmount = std::make_unique<ValueControl>(p, "stadium.mix", "Menge");
  stadiumPitch = std::make_unique<ValueControl>(p,"stadium.pitch","Crowd Pitch");
  texture = std::make_unique<ParamChoice>(
      p, "stadium.slot", "Crowd-Platz",
      juce::StringArray{"1 - Leer", "2 - Leer", "3 - Leer", "4 - Leer", "5 - Leer", "6 - Leer", "7 - Leer"});
  for (auto *c : std::initializer_list<juce::Component *>{
           stadiumOn.get(), stadiumAmount.get(), stadiumPitch.get(), texture.get(), &wave, &import,
           &preview, &bankSave, &clearSample})
    stadium.addAndMakeVisible(c);
  tuneOn = std::make_unique<ParamToggle>(p, "tune.on", "ON");
  tuneAmount = std::make_unique<ValueControl>(p, "tune.amount", "Korrektur");
  root = std::make_unique<ParamChoice>(p, "tune.root", "Grundton",
                                       juce::StringArray{"C", "C#", "D", "D#",
                                                         "E", "F", "F#", "G",
                                                         "G#", "A", "A#", "B"});
  scale = std::make_unique<ParamChoice>(
      p, "tune.scale", "Skala",
      juce::StringArray{"Chromatisch", "Dur", "Moll", "Custom"});
  for (auto *c : std::initializer_list<juce::Component *>{
           tuneOn.get(), tuneAmount.get(), root.get(), scale.get()})
    tuning.addAndMakeVisible(c);
  input = std::make_unique<ValueControl>(p, "input", "Input");
  output = std::make_unique<ValueControl>(p, "output", "Output");
  fxMix = std::make_unique<ValueControl>(p, "fx.mix", "Gesamt-FX-Mix", true);
  for (auto *c : {input.get(), fxMix.get(), output.get()})
    io.addAndMakeVisible(c);
  viewport.setViewedComponent(&content, false);
  juce::StringArray groups;
  for (auto &d : malle::parameters())
    if (!groups.contains(d.section) && juce::String(d.section) != "Global" &&
        juce::String(d.section) != "throw")
      groups.add(d.section);
  section.addItemList(groups, 1);
  section.setText("Klang EQ", juce::dontSendNotification);
  section.onChange = [this] { rebuild(); };
  deessPresets.addItemList({"S-Laute", "SCH/SH-Laute", "Breitband"}, 1);
  deessPresets.onChange = [this] {
    proc.deesserPreset(deessPresets.getSelectedId() - 1);
  };
  auto changeView = [this](bool a) {
    advanced = a;
    simple.setToggleState(!a, juce::dontSendNotification);
    advancedButton.setToggleState(a, juce::dontSendNotification);
    rebuild();
  };
  simple.onClick = [changeView] { changeView(false); };
  advancedButton.onClick = [changeView] { changeView(true); };
  simple.setToggleState(true, juce::dontSendNotification);
  undo.onClick = [this] {
    proc.state.copyState();
    proc.undo.undo();
  };
  redo.onClick = [this] {
    proc.state.copyState();
    proc.undo.redo();
  };
  save.onClick = [this] { choose(true); };
  load.onClick = [this] { choose(false); };
  favorite.onClick = [this] {
    auto key = "fav." + juce::String(preset.getSelectedId());
    proc.state.state.setProperty(key, !bool(proc.state.state.getProperty(key)),
                                 &proc.undo);
    populatePresets();
  };
  search.setTextToShowWhenEmpty("Suche (* Favoriten)", juce::Colours::grey);
  search.onTextChange = [this] { populatePresets(); };
  populatePresets();
  preset.onChange = [this] {
    int i = preset.getSelectedId() - 1;
    proc.applyPreset(i);
    if (i >= 0 && i < int(malle::presets().size())) {
      auto &p = malle::presets()[size_t(i)];
      preset.setTooltip(p.description + "\n" + p.status + "\n" + p.reference);
    }
  };
  auto bindLock = [this](juce::ToggleButton &b, const char *key) {
    b.onClick = [this, &b, key] {
      proc.undo.beginNewTransaction("Lock");
      proc.state.state.setProperty(key, b.getToggleState(), &proc.undo);
    };
  };
  bindLock(lockKey, "lock.key");
  bindLock(lockInput, "lock.input");
  bindLock(lockRouting, "lock.routing");
  import.onClick = [this] {
    chooser = std::make_unique<juce::FileChooser>(
        "Sample oder Crowd-Bank laden", juce::File{}, "*.wav;*.aif;*.aiff;*.flac;*.mallecrowd");
    auto safe = juce::Component::SafePointer<MalleEditor>(this);
    chooser->launchAsync(juce::FileBrowserComponent::openMode |
                             juce::FileBrowserComponent::canSelectFiles,
                         [safe](const juce::FileChooser &fc) {
                           if (safe && fc.getResult().existsAsFile())
                             safe->filesDropped(
                                 {fc.getResult().getFullPathName()}, 0, 0);
                         });
  };
  import.setButtonText("Sample / Bank laden");
  clearSample.onClick=[this] { proc.clearCrowdSlot(); };
  bankSave.onClick=[this] {
    chooser=std::make_unique<juce::FileChooser>("Crowd-Bank speichern",juce::File{},"*.mallecrowd");
    auto safe=juce::Component::SafePointer<MalleEditor>(this);
    chooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting,
      [safe](const juce::FileChooser& fc) {
        if(safe && fc.getResult()!=juce::File{}) {
          juce::String error;
          if(!safe->proc.saveCrowdBank(fc.getResult().withFileExtension("mallecrowd"),error))
            juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,"Crowd-Bank",error);
        }
      });
  };
  preview.setClickingTogglesState(true);
  preview.onClick = [this] {
    proc.engine.audition.store(preview.getToggleState());
  };
  modernSound=std::make_unique<ParamToggle>(p,"macro.v2.on","Makros 0.5");
  modernSound->setTooltip("0.5: staerkere EQ-/Waerme-/Breite-Makros, Ducking bis 100%, Crowd-Filter und Delay-Zeitueberblendung. Fuer alte Projekte bewusst einschalten. Pegelgleich in den vier Sektionen aktivieren.");
  addAndMakeVisible(*modernSound);
  monitor=std::make_unique<ParamChoice>(p,"monitor.on","Betrieb",juce::StringArray{"Mix / mit Pitch-Latenz","Aufnahme / 0 Samples"});
  // Param values are 1 for recording, 0 for mixing.
  addAndMakeVisible(*monitor);
  monitor->setTooltip("Aufnahme: Tune und kreative Pitch/Formant-Prozessoren werden umgangen. Interface- und DAW-Puffer bleiben. Modus bevorzugt bei gestopptem Transport wechseln.");
  std::vector<juce::Component*> children;
  for(auto* c:getChildren())if(dynamic_cast<juce::ResizableCornerComponent*>(c)==nullptr && dynamic_cast<juce::ResizableBorderComponent*>(c)==nullptr)children.push_back(c);
  addAndMakeVisible(canvas);
  for(auto* c:children)canvas.addAndMakeVisible(c);
  ready = true;
  auto* display=juce::Desktop::getInstance().getDisplays().getPrimaryDisplay();
  auto area=display?display->userArea:juce::Rectangle<int>(0,0,1440,1000);
  int width=std::min(1040,int(area.getWidth()*.88f));
  width=std::min(width,int((area.getHeight()-90)*1.28f));
  width=std::clamp(int(proc.state.state.getProperty("ui.width",width)),800,1792);
  int height=std::clamp(int(proc.state.state.getProperty("ui.height",width/1.28f)),625,1400);
  width=std::min(width,std::max(800,area.getWidth()-80));height=std::min(height,std::max(625,area.getHeight()-90));
  setSize(width,height);
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
  auto q = search.getText();
  bool favorites = q.startsWith("*");
  if (favorites)
    q = q.substring(1).trim();
  int i = 0;
  for (auto &p : malle::presets()) {
    ++i;
    bool fav = bool(proc.state.state.getProperty("fav." + juce::String(i)));
    juce::String text = (fav ? "* " : "") + p.category + " / " + p.name;
    if (text.containsIgnoreCase(q) && (!favorites || fav))
      preset.addItem(text, i);
    if (selected == 0 && juce::String(p.name) == proc.currentPreset)
      selected = i;
  }
  preset.setSelectedId(selected, juce::dontSendNotification);
}
void MalleEditor::selectModule(juce::String s) {
  section.setText(s, juce::dontSendNotification);
  rebuild();
}
void MalleEditor::rebuild() {
  controls.clear();
  sectionVariant.reset();
  juce::String group = section.getText();
  juce::String macro = group == "FET" || group == "Leveling" ||
                               group == "Multiband" || group == "Parallel"
                           ? "druck"
                       : group == "Klang EQ" || group == "Input EQ" ||
                               group == "De-Esser" || group == "Resonanzen" ||
                               group == "Air"
                           ? "klang"
                       : group == "Saturation" ? "waerme"
                       : group == "Doubler"    ? "breite"
                       : group == "room" || group == "short" ||
                               group == "long" || group == "reverb"
                           ? "raum"
                       : group == "Bewegung" ? "bewegung"
                                             : "";
  if (macro.isNotEmpty()) {
    juce::StringArray names;
    for (auto &v : malle::variants())
      names.add(v);
    sectionVariant =
        std::make_unique<ParamChoice>(proc, macro.toStdString() + ".variant",
                                      "Gemeinsame Sektionvariante", names);
    canvas.addAndMakeVisible(*sectionVariant);
    sectionVariant->setVisible(advanced);
  }
  int selectedSlot=std::clamp(int(proc.getValue("stadium.slot")),0,malle::crowdSlots-1);
  auto visibleSlotParameter=[&](const std::string& id) {
    for(int slot=0;slot<malle::crowdSlots;++slot)
      for(auto field:{"root","loop","fade","level"})
        if(id==malle::crowdParameter(slot,field))return slot==selectedSlot;
    return true;
  };
  for (auto &d : malle::parameters())
    if (visibleSlotParameter(d.id) && d.id!="stadium.slot" && d.id!="stadium.mix" && d.id!="stadium.pitch" &&
        (d.id!="stadium.texture" || proc.crowdSlotName(selectedSlot)=="Leer / Testtextur") && (juce::String(d.section) == group || (group=="Klang EQ" && d.id=="klang.colour")) && d.id.rfind("route.", 0) != 0 &&
        (d.id.size() < 3 || d.id.substr(d.id.size() - 3) != ".on") &&
        d.id != "deess.preset" && d.id.find(".variant") == std::string::npos) {
      auto c = std::make_unique<ValueControl>(
          proc, d.id, juce::String{},
          group == "room" || group == "short" || group == "long" ||
              group == "reverb");
      content.addAndMakeVisible(*c);
      controls.push_back(std::move(c));
    }
  for (auto &c : macros)
    c->setVisible(!advanced);
  stadium.setVisible(!advanced || group=="Stadium");
  tuning.setVisible(!advanced);
  for (auto &b : tabs)
    b.setVisible(advanced);
  routing.setVisible(advanced);
  section.setVisible(advanced);
  detailCard.setVisible(advanced);
  viewport.setVisible(advanced);
  eq.setVisible(advanced && group == "Klang EQ");
  deessPresets.setVisible(advanced && group == "De-Esser");
  sourceInfo.setVisible(advanced);
  throws.setAdvanced(advanced);
  detailCard.title = group == "Routing"
                         ? "ROUTING - DIREKT / PARALLEL / STEUERUNG"
                         : group.toUpperCase();
  resized();
}
void MalleEditor::resized() {
  if (!ready)
    return;
  float uiScale=std::min(getWidth()/1280.f,getHeight()/1000.f);
  canvas.setBounds(0,0,1280,1000);canvas.setTransform(juce::AffineTransform::scale(uiScale));
  int w = 1280, h = 1000;
  proc.state.state.setProperty("ui.width",getWidth(),nullptr);
  proc.state.state.setProperty("ui.height",getHeight(),nullptr);
  monitor->setBounds(320,87,305,42);
  modernSound->setBounds(645,98,180,27);
  simple.setBounds(w - 210, 15, 90, 31);
  advancedButton.setBounds(w - 115, 15, 98, 31);
  preset.setBounds(20, 57, w - 690, 34);
  search.setBounds(w - 650, 57, 165, 34);
  favorite.setBounds(w - 480, 57, 35, 34);
  undo.setBounds(w - 440, 57, 65, 34);
  redo.setBounds(w - 370, 57, 65, 34);
  save.setBounds(w - 300, 57, 90, 34);
  load.setBounds(w - 205, 57, 80, 34);
  lockKey.setBounds(20, 96, 95, 25);
  lockInput.setBounds(115, 96, 85, 25);
  lockRouting.setBounds(205, 96, 100, 25);
  int throwH = advanced ? 250 : 144;
  throws.setBounds(18, h - throwH - 41, w - 36, throwH);
  status.setBounds(22, h - 37, w - 44, 32);
  int top = 132, bottom = throws.getY() - 10;
  if (!advanced) {
    int rightW = w / 4 - 12, leftW = w - rightW - 48, gap = 9,
        col = (leftW - 2 * gap) / 3, rowH = (bottom - top - gap) / 2;
    for (int i = 0; i < 6; ++i)
      macros[i]->setBounds(18 + (i % 3) * (col + gap),
                           top + (i / 3) * (rowH + gap), col, rowH);
    int rx = w - rightW - 18;
    stadium.setBounds(rx, top, rightW, int((bottom - top) * .42));
    tuning.setBounds(rx, stadium.getBottom() + 8, rightW,
                     int((bottom - top) * .27));
    io.setBounds(rx, tuning.getBottom() + 8, rightW,
                 bottom - tuning.getBottom() - 8);
  } else {
    int rw = 220;
    for (int i = 0; i < 5; ++i)
      tabs[i].setBounds(18 + rw + 8 + i * (w - rw - 54) / 5, top,
                        (w - rw - 54) / 5 - 3, 30);
    routing.setBounds(18, top, rw, bottom - top);
    detailCard.setBounds(18 + rw + 8, top + 38, w - rw - 44,
                         bottom - top - 190);
    section.setBounds(detailCard.getRight() - 235, detailCard.getY() + 7, 218,
                      30);
    int eqH = eq.isVisible() ? 120 : 0;
    eq.setBounds(detailCard.getX() + 13, detailCard.getY() + 42,
                 detailCard.getWidth() - 26, eqH);
    deessPresets.setBounds(detailCard.getX() + 18, detailCard.getY() + 43, 280,
                           32);
    int extra = deessPresets.isVisible() ? 65 : sectionVariant ? 60 : 0;
    if (sectionVariant)
      sectionVariant->setBounds(detailCard.getRight() - 320,
                                detailCard.getY() + 43 + eqH, 300, 54);
    viewport.setBounds(
        detailCard.getX() + 10, detailCard.getY() + 44 + eqH + extra,
        detailCard.getWidth() - 20, detailCard.getHeight() - 54 - eqH - extra);
    io.setBounds(detailCard.getX(), detailCard.getBottom() + 8,
                 detailCard.getWidth(), 143);
    if(section.getText()=="Stadium") {
      int bankWidth=std::min(350,detailCard.getWidth()/3);
      stadium.setBounds(detailCard.getRight()-bankWidth-10,detailCard.getY()+46,
                        bankWidth,detailCard.getHeight()-56);
      stadium.toFront(false);
      viewport.setSize(detailCard.getWidth()-bankWidth-35,viewport.getHeight());
    }
    sourceInfo.setBounds(detailCard.getX() + 280, io.getY() + 6,
                         detailCard.getWidth() - 295, 22);
  }
  int sw = stadium.getWidth(), sh = stadium.getHeight();
  stadiumOn->setBounds(sw - 68, 9, 53, 25);
  stadiumAmount->setBounds(8, 43, sw / 2 - 10, (sh - 140)/2);
  stadiumPitch->setBounds(8,43+(sh-140)/2,sw/2-10,(sh-140)/2);
  texture->setBounds(sw / 2, 43, sw / 2 - 13, 54);
  wave.setBounds(sw / 2, 106, sw / 2 - 14, std::max(25, sh - 210));
  import.setBounds(12, sh - 78, sw / 2 - 17, 28);
  preview.setBounds(sw / 2 + 2, sh - 78, sw / 2 - 16, 28);
  bankSave.setBounds(12,sh-43,sw/2-17,28);
  clearSample.setBounds(sw/2+2,sh-43,sw/2-16,28);
  tuneOn->setBounds(tuning.getWidth() - 68, 9, 53, 25);
  root->setBounds(12, 42, tuning.getWidth() / 2 - 20, 53);
  scale->setBounds(12, 99, tuning.getWidth() / 2 - 20, 53);
  tuneAmount->setBounds(tuning.getWidth() / 2, 40, tuning.getWidth() / 2 - 10,
                        tuning.getHeight() - 50);
  int iw = io.getWidth() / 3;
  input->setBounds(7, 31, iw - 9, io.getHeight() - 36);
  fxMix->setBounds(iw + 2, 31, iw - 7, io.getHeight() - 36);
  output->setBounds(iw * 2, 31, iw - 8, io.getHeight() - 36);
  int cw = viewport.getWidth() - 20, cols = std::max(3, cw / 170),
      cell = cw / cols;
  for (size_t i = 0; i < controls.size(); ++i)
    controls[i]->setBounds(int(i) % cols * cell, int(i) / cols * 137, cell - 6,
                           133);
  content.setSize(cw, int((controls.size() + cols - 1) / cols) * 137);
}
void MalleEditor::paint(juce::Graphics &g) {
  g.setGradientFill(juce::ColourGradient(
      juce::Colour(0xff303638), 0, 0, juce::Colour(0xff0a0e10),
      float(getWidth()), float(getHeight()), false));
  g.fillAll();
  g.setColour(juce::Colour(0xff737b7e));
  g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(2), 10, 1);
  g.saveState();g.addTransform(juce::AffineTransform::scale(std::min(getWidth()/1280.f,getHeight()/1000.f)));
  g.setFont(juce::FontOptions(28, juce::Font::bold));
  g.setColour(juce::Colour(0xffe0e3e4));
  g.drawText("MALLE VOCALS", 1280 / 2 - 220, 9, 440, 37,
             juce::Justification::centred);
  g.setFont(juce::FontOptions(15));
  g.drawText("R O H S T O F F   A U D I O", 20, 10, 300, 22,
             juce::Justification::left);
  g.setFont(juce::FontOptions(12));
  g.drawText("by Micha Benjamin / 0.4.0 ALPHA", 20, 32, 300, 19,
             juce::Justification::left);
  g.restoreState();
  for (int x : {7, getWidth() - 12})
    for (int y : {7, getHeight() - 13}) {
      g.setColour(juce::Colour(0xff071012));
      g.fillEllipse(float(x), float(y), 6, 6);
      g.setColour(juce::Colour(0xff7b8386));
      g.drawLine(float(x + 1), float(y + 4), float(x + 5), float(y + 2), 1);
    }
}
void MalleEditor::timerCallback() {
  int slot=std::clamp(int(proc.getValue("stadium.slot")),0,malle::crowdSlots-1);
  for(int i=0;i<malle::crowdSlots;++i)
    texture->setChoiceName(i,juce::String(i+1)+" - "+proc.crowdSlotName(i));
  if(displayedCrowdSlot!=slot) {
    displayedCrowdSlot=slot;
    if(advanced && section.getText()=="Stadium")rebuild();
  }
  bool recording=proc.getValue("monitor.on")>.5f;
  tuneOn->setEnabled(!recording);tuneAmount->setEnabled(!recording);
  tuning.title=recording?"TUNING: NUR IM MIX":"TUNING & TONART";tuning.repaint();
  auto &m = proc.engine.meters;
  bool external =
      proc.getValue("duck.source") > .5f || proc.getValue("pump.mode") > .5f;
  juce::String sc =
      external ? (proc.sidechainEnabled.load()
                      ? (m.sidechain.load() > 1e-6f ? "SC Signal"
                                                    : "SC: kein Signal")
                      : "SC Eingang fehlt")
               : "Ducking intern";
  juce::String text =
      "IN " + juce::String(malle::db(m.input.load()), 1) + " dB | OUT " +
      juce::String(malle::db(m.output.load()), 1) + " dB | GR FET " +
      juce::String(m.fet.load(), 1) + " / Level " +
      juce::String(m.level.load(), 1) + " dB | " + sc + " | " + (proc.getMainBusNumOutputChannels()==1?"MONO: fuer Breite Stereo-Ausgang nutzen | ":"") + (proc.getValue("monitor.on")>.5f ? "AUFNAHME: Pitch aus / 0 Samples" : "MIX: "+juce::String(proc.getLatencySamples())+" Samples") + " | " + proc.sampleName;
  juce::MemoryBlock baseline;
  auto encoded = proc.state.state.getProperty("preset.baseline").toString();
  bool modified = false;
  if (baseline.fromBase64Encoding(encoded) &&
      baseline.getSize() == malle::parameters().size() * sizeof(float)) {
    for (size_t i = 0; i < malle::parameters().size(); ++i) {
      float v;
      std::memcpy(
          &v, static_cast<const char *>(baseline.getData()) + i * sizeof(float),
          sizeof(v));
      if (std::abs(proc.getValue(malle::parameters()[i].id) - v) >
          1e-5f * (1 + std::abs(v))) {
        modified = true;
        break;
      }
    }
  }
  if (modified)
    text = "CUSTOM / Geaendert | " + text;
  status.setText(text, juce::dontSendNotification);
  status.setFont(juce::FontOptions(13));
  lockKey.setToggleState(bool(proc.state.state.getProperty("lock.key")),
                         juce::dontSendNotification);
  lockInput.setToggleState(bool(proc.state.state.getProperty("lock.input")),
                           juce::dontSendNotification);
  lockRouting.setToggleState(bool(proc.state.state.getProperty("lock.routing")),
                             juce::dontSendNotification);
  undo.setEnabled(proc.undo.canUndo());
  redo.setEnabled(proc.undo.canRedo());
  eq.repaint();
  wave.repaint();
  auto group = section.getText();
  juce::String context =
      group == "Parallel"
          ? (proc.getValue("parallel.tap") < .5f
                 ? "Abgriff Input (latenzausgerichtet) -> Kompressor -> "
                   "Vocal-Bus"
                 : "Abgriff Direktkette -> Kompressor -> Vocal-Bus")
      : group == "Doubler" || group == "Stadium"
          ? "Abgriff Direktkette -> eigener Weg -> Vocal-Bus"
      : group == "Ducking"
          ? "Quelle: eigene Vocal / externer Lead | Ziele: FX, optional Dry"
      : group == "Bewegung" ? "Tempo-Shaping ODER externe SC-Kompression | "
                              "Ziel: Dry / FX / Gesamt"
      : group == "room" || group == "short" || group == "long" ||
              group == "reverb" || group == "throw"
          ? "Vocal-Bus -> eigener 100% Wet-Weg -> FX-Bus -> Gesamt-FX-Mix"
          : "Direktkette in angezeigter Reihenfolge -> Vocal-Bus -> Output";
  if(recording && (group=="Tune" || group=="Pitch/Formant"))context="AUFNAHME: Dieser Pitch-Prozessor ist umgangen. Fuer Bearbeitung oben Mix waehlen.";
  sourceInfo.setText(context, juce::dontSendNotification);
  sourceInfo.setFont(juce::FontOptions(12));
  routing.repaint();
}
void MalleEditor::filesDropped(const juce::StringArray &files, int, int) {
  if (files.isEmpty())
    return;
  juce::String error;
  auto file=juce::File(files[0]);
  bool ok=file.hasFileExtension("mallecrowd") ? proc.loadCrowdBank(file,error) : proc.importSample(file,error);
  if (!ok)
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

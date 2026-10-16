#pragma once
#include "PluginProcessor.h"
class GraphiteLook final : public juce::LookAndFeel_V4 {
public:
  void drawRotarySlider(juce::Graphics &, int, int, int, int, float, float,
                        float, juce::Slider &) override;
  void drawButtonBackground(juce::Graphics &, juce::Button &,
                            const juce::Colour &, bool, bool) override;
};
class ValueControl : public juce::Component {
public:
  ValueControl(MalleProcessor &, std::string, juce::String title = {},
               bool teal = false);
  void resized() override;
  juce::Slider slider;
  juce::Label label;

private:
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attach;
};
class ParamChoice : public juce::Component {
public:
  ParamChoice(MalleProcessor &, std::string, juce::String, juce::StringArray,
              std::vector<float> values = {});
  void resized() override;
  void setChoiceName(int index, const juce::String& name) { box.changeItemText(index+1,name); }

private:
  juce::Label label;
  juce::ComboBox box;
  std::vector<float> values;
  std::unique_ptr<juce::ParameterAttachment> attach;
};
class ParamToggle : public juce::TextButton {
public:
  ParamToggle(MalleProcessor &, std::string, juce::String);

private:
  std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attach;
};
class Card : public juce::Component {
public:
  explicit Card(juce::String title = {}, bool teal = false)
      : title(title), teal(teal) {}
  void paint(juce::Graphics &) override;
  juce::String title;
  bool teal;
};
class EqView : public juce::Component {
public:
  explicit EqView(MalleProcessor &p) : proc(p) {}
  void paint(juce::Graphics &) override;
  void mouseDown(const juce::MouseEvent &) override;
  void mouseDrag(const juce::MouseEvent &) override;
  void mouseUp(const juce::MouseEvent &) override;
  void mouseWheelMove(const juce::MouseEvent &,
                      const juce::MouseWheelDetails &) override;

private:
  MalleProcessor &proc;
  int selected = -1;
  juce::Point<float> point(int) const;
};
class WaveView : public juce::Component {
public:
  explicit WaveView(MalleProcessor &p) : proc(p) {}
  void paint(juce::Graphics &) override;

private:
  MalleProcessor &proc;
};
class MacroCard : public Card {
public:
  MacroCard(MalleProcessor &, int);
  void resized() override;

private:
  MalleProcessor &proc;
  int slot;
  std::unique_ptr<ValueControl> amount;
  std::unique_ptr<ParamToggle> on, resOn, airOn;
  std::unique_ptr<ParamChoice> variant, mode, rate;
  std::unique_ptr<ValueControl> res, air;
};
class ThrowPanel : public Card {
public:
  explicit ThrowPanel(MalleProcessor &);
  void setAdvanced(bool);
  void resized() override;

private:
  MalleProcessor &proc;
  bool advanced = false;
  juce::TextButton trigger{"THROW\nSend oeffnen"}, kill{"TAIL KILL"};
  std::unique_ptr<ParamChoice> type, grid, feel, verbType;
  std::unique_ptr<ParamToggle> on, verbOn, sync;
  std::vector<std::unique_ptr<ValueControl>> primary, detail;
};
class RoutingView : public juce::Component, public juce::DragAndDropTarget {
public:
  RoutingView(MalleProcessor &p, std::function<void(juce::String)> select)
      : proc(p), select(select) {}
  void paint(juce::Graphics &) override;
  void mouseDown(const juce::MouseEvent &) override;
  void mouseDrag(const juce::MouseEvent &) override;
  void mouseWheelMove(const juce::MouseEvent &,
                      const juce::MouseWheelDetails &) override;
  bool isInterestedInDragSource(const SourceDetails &) override;
  void itemDragMove(const SourceDetails &) override;
  void itemDragExit(const SourceDetails &) override;
  void itemDropped(const SourceDetails &) override;

private:
  MalleProcessor &proc;
  std::function<void(juce::String)> select;
  int from = -1, drop = -1, scroll = 0;
};
class MalleEditor final : public juce::AudioProcessorEditor,
                          private juce::Timer,
                          public juce::FileDragAndDropTarget,
                          public juce::DragAndDropContainer {
public:
  explicit MalleEditor(MalleProcessor &);
  ~MalleEditor() override;
  void paint(juce::Graphics &) override;
  void resized() override;
  bool isInterestedInFileDrag(const juce::StringArray &) override {
    return true;
  }
  void filesDropped(const juce::StringArray &, int, int) override;

private:
  MalleProcessor &proc;
  GraphiteLook look;
  juce::TooltipWindow tips{this, 700};
  bool advanced = false;
  bool ready = false;
  int displayedCrowdSlot = -1;
  juce::TextButton simple{"Simple"}, advancedButton{"Advanced"}, undo{"Undo"},
      redo{"Redo"}, save{"Speichern"}, load{"Laden"}, favorite{"*"},
      import{"Sample laden"}, preview{"Vorhoeren"}, bankSave{"Bank speichern"}, clearSample{"Platz leeren"};
  juce::ToggleButton lockKey{"Tonart"}, lockInput{"Input"},
      lockRouting{"Routing"};
  juce::ComboBox preset, section, deessPresets;
  juce::TextEditor search;
  juce::Label status, sourceInfo;
  std::array<juce::TextButton, 5> tabs;
  std::array<std::unique_ptr<MacroCard>, 6> macros;
  ThrowPanel throws;
  Card stadium{"STADIUM"}, tuning{"TUNING & TONART"},
      io{"INPUT / FX-MIX / OUTPUT", true}, detailCard{"KLANG & DYNAMIK"};
  WaveView wave;
  EqView eq;
  RoutingView routing;
  juce::Viewport viewport;
  juce::Component content;
  std::unique_ptr<ParamToggle> stadiumOn, tuneOn;
  std::unique_ptr<ParamChoice> texture, root, scale, sectionVariant;
  std::unique_ptr<ValueControl> stadiumAmount, tuneAmount, input, output, fxMix;
  std::vector<std::unique_ptr<ValueControl>> controls;
  std::unique_ptr<juce::FileChooser> chooser;
  void populatePresets();
  void rebuild();
  void selectModule(juce::String);
  void choose(bool);
  void timerCallback() override;
};

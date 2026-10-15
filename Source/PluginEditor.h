#pragma once
#include "PluginProcessor.h"
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
class MalleEditor final : public juce::AudioProcessorEditor,
                          private juce::Timer,
                          public juce::FileDragAndDropTarget {
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
  struct Control : juce::Component {
    juce::Label label;
    juce::Slider slider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        attachment;
    Control(MalleProcessor &, const malle::Param &);
    void resized() override;
  };
  MalleProcessor &proc;
  juce::LookAndFeel_V4 look;
  juce::TooltipWindow tips{this, 700};
  juce::TextButton mode{"Advanced"}, undo{"Undo"}, redo{"Redo"},
      import{"Crowd laden / Drop"}, preview{"Vorhoeren"},
      throwButton{"THROW (halten)"}, save{"Preset speichern"},
      load{"Preset laden"}, favorite{"Favorit"};
  juce::ToggleButton lockKey{"Tonart Lock"}, lockInput{"Input Lock"},
      lockRouting{"Routing Lock"};
  juce::TextEditor search;
  juce::ComboBox preset, section;
  juce::Label status, sample;
  juce::Viewport viewport;
  juce::Component content;
  EqView eq;
  WaveView wave;
  std::vector<std::unique_ptr<Control>> controls;
  std::array<juce::ComboBox, 6> variants;
  std::array<
      std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>,
      6>
      variantAttachments;
  std::unique_ptr<juce::FileChooser> chooser;
  bool advanced = false;
  void rebuild();
  void populatePresets();
  void timerCallback() override;
  void choose(bool saving);
};

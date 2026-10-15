#pragma once
#include "dsp/Engine.h"
#include <JuceHeader.h>
class MalleProcessor final : public juce::AudioProcessor, private juce::Timer {
public:
  MalleProcessor();
  ~MalleProcessor() override;
  void prepareToPlay(double, int) override;
  void releaseResources() override {}
  bool isBusesLayoutSupported(const BusesLayout &) const override;
  void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;
  void processBlockBypassed(juce::AudioBuffer<float> &,
                            juce::MidiBuffer &) override;
  juce::AudioProcessorEditor *createEditor() override;
  bool hasEditor() const override { return true; }
  const juce::String getName() const override { return "Malle Vocals"; }
  bool acceptsMidi() const override { return false; }
  bool producesMidi() const override { return false; }
  double getTailLengthSeconds() const override { return 30; }
  int getNumPrograms() override { return 1; }
  int getCurrentProgram() override { return 0; }
  void setCurrentProgram(int) override {}
  const juce::String getProgramName(int) override { return {}; }
  void changeProgramName(int, const juce::String &) override {}
  void getStateInformation(juce::MemoryBlock &) override;
  void setStateInformation(const void *, int) override;
  void applyPreset(int);
  std::array<int, 11> routing() const;
  void moveModule(int from, int to);
  void deesserPreset(int n);
  bool importSample(const juce::File &, juce::String &error);
  void setValue(const std::string &, float);
  float getValue(const std::string &) const;
  void savePreset(const juce::File &);
  bool loadPreset(const juce::File &);
  juce::UndoManager undo;
  juce::AudioProcessorValueTreeState state;
  malle::Engine engine;
  juce::String sampleName = "Synthetic test crowd (original)";
  std::array<float, 512> waveform{};
  juce::String currentPreset = "Init";
  std::atomic<bool> sampleMissing{false}, sidechainEnabled{false};
  static juce::AudioProcessorValueTreeState::ParameterLayout layout();

private:
  void timerCallback() override;
  void process(juce::AudioBuffer<float> &, bool);
  void restoreSample();
  std::vector<std::atomic<float> *> raw;
  malle::Values base, resolved;
  std::vector<std::unique_ptr<malle::Sample>> retainedSamples;
  juce::String loadedSample;
  std::vector<juce::String> sampleKeys;
};

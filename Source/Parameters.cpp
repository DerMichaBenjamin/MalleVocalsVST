#include "Parameters.h"
#include "CrowdSlots.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace malle {
const std::vector<Param> &parameters() {
  static const auto p = [] {
    std::vector<Param> v;
    auto add = [&](std::string id, std::string n, std::string s, float lo,
                   float hi, float d, std::string u = "", bool log = false) {
      v.push_back({id, n, s, u, lo, hi, d, log});
    };
    auto on = [&](std::string id, std::string s, float d = 1) {
      add(id + ".on", "Aktiv", s, 0, 1, d);
    };
    add("input", "Input", "Global", -24, 24, 0, "dB");
    add("output", "Output", "Global", -24, 12, 0, "dB");
    add("mix", "Gesamt Mix", "Global", 0, 1, 1);
    add("fx.mix", "FX Mix", "Global", 0, 1, 1);
    add("bypass", "Bypass", "Global", 0, 1, 0);
    for (auto s : {"druck", "klang", "waerme", "breite", "raum", "bewegung"}) {
      add(std::string(s) + ".amount", "Intensitaet", s, 0, 1, 0);
      add(std::string(s) + ".variant", "Variante", s, 0, 19, 0);
    }
    on("inputEQ", "Input EQ");
    add("hp", "Low Cut", "Input EQ", 20, 400, 80, "Hz", true);
    add("lp", "High Cut", "Input EQ", 2000, 22000, 20000, "Hz", true);
    add("correct.freq", "Korrektur Frequenz", "Input EQ", 80, 4000, 350, "Hz",
        true);
    add("correct.gain", "Korrektur Gain", "Input EQ", -15, 15, 0, "dB");
    add("correct.q", "Korrektur Q", "Input EQ", .2, 10, 1);
    on("tune", "Tune", 0);
    add("tune.root", "Grundton C=0", "Tune", 0, 11, 0);
    add("tune.scale", "Chromatisch / Dur / Moll / Custom", "Tune", 0, 3, 0);
    add("tune.speed", "Retune", "Tune", 0, 300, 50, "ms");
    add("tune.amount", "Korrektur", "Tune", 0, 1, 1);
    add("tune.human", "Humanize", "Tune", 0, 1, .3);
    add("pitch", "Pitch", "Pitch/Formant", -24, 24, 0, "st");
    add("formant", "Formant", "Pitch/Formant", -12, 12, 0, "st");
    add("formant.preserve", "Formant erhalten", "Pitch/Formant", 0, 1, 1);
    for (int i = 0; i < 12; ++i)
      add("note." + std::to_string(i), "Note " + std::to_string(i), "Tune", 0,
          1, 1);
    for (auto s : {"fet", "level", "parallel"}) {
      std::string k = s;
      std::string group = k == "fet"     ? "FET"
                          : k == "level" ? "Leveling"
                                         : "Parallel";
      on(k, group, k == "parallel" ? 0 : 1);
      add(k + ".threshold", "Threshold", group, -60, 0, k == "fet" ? -18 : -24,
          "dB");
      add(k + ".ratio", "Ratio", group, 1, 20, k == "fet" ? 4 : 2);
      add(k + ".attack", "Attack", group, .05, 100, k == "fet" ? .5 : 15, "ms",
          true);
      add(k + ".release", "Release", group, 10, 1500, k == "fet" ? 80 : 300,
          "ms", true);
      add(k + ".knee", "Knee", group, 0, 18, 6, "dB");
      add(k + ".makeup", "Makeup", group, 0, 24, 0, "dB");
      add(k + ".mix", "Mix", group, 0, 1, k == "parallel" ? .25 : 1);
    }
    on("deess", "De-Esser");
    add("deess.low", "Low", "De-Esser", 1500, 10000, 4500, "Hz", true);
    add("deess.high", "High", "De-Esser", 3000, 18000, 11000, "Hz", true);
    add("deess.threshold", "Threshold", "De-Esser", -60, 0, -24, "dB");
    add("deess.amount", "Staerke", "De-Esser", 0, 1, .5);
    add("deess.listen", "Listen", "De-Esser", 0, 1, 0);
    on("eq", "Klang EQ");
    for (int b = 0; b < 4; ++b) {
      std::string k = "eq" + std::to_string(b);
      add(k + ".freq", "Band " + std::to_string(b + 1) + " Hz", "Klang EQ", 30,
          20000, std::array<float, 4>{180, 650, 2800, 11000}[b], "Hz", true);
      add(k + ".gain", "Gain", "Klang EQ", -18, 18, 0, "dB");
      add(k + ".q", "Q", "Klang EQ", .2, 12, 1);
      add(k + ".type", "Bell / LowShelf / HighShelf", "Klang EQ", 0, 2, 0);
      add(k + ".dynamic", "Dynamic Range", "Klang EQ", -12, 0, 0, "dB");
      add(k + ".threshold", "Dynamic Threshold", "Klang EQ", -60, 0, -24, "dB");
    }
    on("mb", "Multiband", 0);
    for (int b = 0; b < 3; ++b)
      add("mb.cross" + std::to_string(b), "Crossover " + std::to_string(b + 1),
          "Multiband", 80, 16000, std::array<float, 3>{200, 1200, 5000}[b],
          "Hz", true);
    for (int b = 0; b < 4; ++b) {
      std::string k = "mb" + std::to_string(b);
      add(k + ".threshold", "Band " + std::to_string(b + 1) + " Threshold",
          "Multiband", -60, 0, -24, "dB");
      add(k + ".ratio", "Ratio", "Multiband", 1, 10, 2);
      add(k + ".attack", "Attack", "Multiband", .2, 100, 10, "ms");
      add(k + ".release", "Release", "Multiband", 10, 1000, 150, "ms");
      add(k + ".gain", "Gain", "Multiband", -12, 12, 0, "dB");
    }
    on("res", "Resonanzen", 0);
    add("res.amount", "Staerke", "Resonanzen", 0, 1, .4);
    add("res.max", "Max Reduction", "Resonanzen", 0, 12, 6, "dB");
    add("res.focus", "Gewichtung Hochton", "Resonanzen", -1, 1, 0);
    add("res.select", "Selektivitaet", "Resonanzen", 1, 6, 2);
    add("res.attack", "Attack", "Resonanzen", 1, 100, 10, "ms");
    add("res.release", "Release", "Resonanzen", 20, 1000, 150, "ms");
    add("res.delta", "Delta Listen", "Resonanzen", 0, 1, 0);
    on("air", "Air", 0);
    add("air.presence", "Presence", "Air", 0, 1, 0);
    add("air.high", "Air", "Air", 0, 1, 0);
    add("air.guard", "Sibilance Guard", "Air", 0, 1, .7);
    on("sat", "Saturation", 1);
    add("sat.drive", "Drive", "Saturation", 0, 24, 6, "dB");
    add("sat.type", "Soft / Tube / Hard", "Saturation", 0, 2, 0);
    add("sat.tone", "Tone", "Saturation", 500, 18000, 8000, "Hz", true);
    add("sat.mix", "Mix", "Saturation", 0, 1, 0);
    add("sat.output", "Output", "Saturation", -24, 12, 0, "dB");
    on("double", "Doubler", 1);
    add("double.mix", "Mix", "Doubler", 0, 1, 0);
    add("double.voices", "Stimmen", "Doubler", 2, 8, 4);
    add("double.time", "Zeitvariation", "Doubler", 5, 50, 18, "ms");
    add("double.pitch", "Pitchvariation", "Doubler", 0, 30, 8, "cent");
    add("double.width", "Breite", "Doubler", 0, 1, 1);
    on("stadium", "Stadium", 0);
    add("stadium.mix", "Menge", "Stadium", 0, 1, .2);
    add("stadium.size", "Groesse", "Stadium", 0, 1, .5);
    add("stadium.rough", "Rauheit", "Stadium", 0, 1, .2);
    add("stadium.width", "Breite", "Stadium", 0, 1, 1);
    add("stadium.attack", "Attack", "Stadium", 1, 300, 20, "ms");
    add("stadium.release", "Release", "Stadium", 20, 1000, 150, "ms");
    add("stadium.vocoder", "Artikulation", "Stadium", 0, 1, .6);
    add("stadium.root", "Sample Grundton", "Stadium", 0, 11, 0);
    add("stadium.loop", "Loop", "Stadium", 0, 1, 1);
    add("stadium.fade", "Sample Fade", "Stadium", 1, 500, 30, "ms");
    add("stadium.level", "Sample Pegel", "Stadium", -24, 12, 0, "dB");
    for (auto s : {"room", "short", "long", "reverb", "throw"}) {
      std::string k = s;
      on(k, k, 1);
      add(k + ".send", "Send", k, 0, 1, k == "throw" ? .5 : 0);
      add(k + ".return", k == "throw" ? "Throw-Mix" : "Return", k, 0, 1,
          k == "throw" ? .25f : 1);
      add(k + ".time", "Zeit", k, 1, 2000,
          k == "room"    ? 35
          : k == "short" ? 100
                         : 375,
          "ms", true);
      add(k + ".sync", "Tempo Sync", k, 0, 1,
          k == "long" || k == "throw" ? 1 : 0);
      add(k + ".beats", "Rhythmus (Viertel)", k, .125, 4,
          k == "short" ? .25 : .75);
      add(k + ".feedback", "Feedback", k, 0, .9, k == "reverb" ? .7 : .3);
      add(k + ".low", "Low Cut", k, 20, 2000, 200, "Hz", true);
      add(k + ".high", "High Cut", k, 1000, 20000, 6500, "Hz", true);
      add(k + ".width", "Breite", k, 0, 1, .8);
      add(k + ".tail", "Ausklingzeit", k, .1, 20, 6, "s");
      add(k + ".fade", "Tail Fade", k, .01, 5, .5, "s");
    }
    add("delay.reverb", "Delay in Reverb", "reverb", 0, 1, 0);
    add("throw.trigger", "Throw", "throw", 0, 1, 0);
    add("throw.window", "Send Fenster", "throw", 10, 2000, 180, "ms");
    add("throw.offset", "Startversatz", "throw", 0, 1000, 0, "ms");
    add("throw.quantize", "Quantisierung (Viertel; 0=aus)", "throw", 0, 4, 0);
    add("duck.source", "Intern / Extern", "Ducking", 0, 1, 0);
    add("duck.amount", "FX Ducking", "Ducking", 0, 1, .5);
    add("duck.dry", "Dry Ducking", "Ducking", 0, 1, 0);
    add("duck.attack", "Attack", "Ducking", .1, 100, 5, "ms");
    add("duck.release", "Release", "Ducking", 20, 1500, 250, "ms");
    on("pump", "Bewegung", 1);
    add("pump.mode", "Volume / Sidechain", "Bewegung", 0, 1, 0);
    add("pump.depth", "Tiefe", "Bewegung", 0, 1, 0);
    add("pump.beats", "Notenwert (Viertel)", "Bewegung", .125, 4, 1);
    add("pump.phase", "Phase", "Bewegung", 0, 1, 0);
    add("pump.shape", "Huellkurve", "Bewegung", .2, 5, 1);
    add("pump.target", "Dry / FX / Gesamt", "Bewegung", 0, 2, 1);
    add("stutter", "Stutter", "Bewegung", 0, 1, 0);
    add("tail", "Gesamt Ausklingzeit", "Global", .1, 30, 15, "s");
    add("fade", "Gesamt Fade", "Global", .01, 5, .5, "s");
    add("stadium.texture", "Testtextur 1–7", "Stadium", 0, 6, 0);
    add("pump.threshold", "SC Threshold", "Bewegung", -60, 0, -24, "dB");
    add("pump.ratio", "SC Ratio", "Bewegung", 1, 20, 4);
    add("pump.attack", "SC Attack", "Bewegung", .1, 100, 2, "ms");
    add("pump.release", "SC Release", "Bewegung", 10, 1500, 120, "ms");
    for (auto section :
         {"druck", "klang", "waerme", "breite", "raum", "bewegung"})
      add(std::string(section) + ".on", "Sektion aktiv", section, 0, 1, 1);
    add("creative.on", "Pitch/Formant aktiv", "Pitch/Formant", 0, 1, 1);
    for (int i = 0; i < 11; ++i)
      add("route." + std::to_string(i),
          "Direktkette Position " + std::to_string(i + 1), "Routing", 0, 10,
          float(i));
    add("parallel.tap", "Abgriff: Input / Post-Direktkette", "Parallel", 0, 1,
        0);
    add("deess.mode", "Split / Wideband", "De-Esser", 0, 1, 0);
    add("deess.preset", "S / SCH-SH / Breitband", "De-Esser", 0, 2, 0);
    add("throw.type", "Mono / Stereo / Ping Pong", "throw", 0, 2, 1);
    add("throw.feel", "Gerade / Punktiert / Triolisch", "throw", 0, 2, 0);
    add("throw.verb.on", "Throw Hall aktiv", "throw", 0, 1, 0);
    add("throw.verb.type", "Room / Plate / Hall", "throw", 0, 2, 1);
    add("throw.verb.amount", "Throw Hall Menge", "throw", 0, 1, .2);
    add("throw.kill", "Tail Kill", "throw", 0, 1, 0);
    add("duck.on", "FX Ducking aktiv", "Ducking", 0, 1, 1);
    add("stadium.slot", "Crowd-Platz 1–7", "Stadium", 0, 6, 0);
    add("stadium.loopSmooth", "Loop Crossfade", "Stadium", 0, 1, 0);
    for (int slot = 1; slot < crowdSlots; ++slot) {
      add(crowdParameter(slot,"root"), "Sample Grundton", "Stadium", 0,11,0);
      add(crowdParameter(slot,"loop"), "Loop", "Stadium", 0,1,1);
      add(crowdParameter(slot,"fade"), "Sample Fade", "Stadium", 1,500,30,"ms");
      add(crowdParameter(slot,"level"), "Sample Pegel", "Stadium", -24,12,0,"dB");
    }
    // Append-only IDs: old projects retain the original engine and latency.
    add("monitor.on", "Aufnahme: 0 Samples / Pitch aus", "Global", 0,1,0);
    add("quality.on", "Neue Raum-Engine", "Global", 0,1,0);
    for(auto section : {"druck","klang","waerme","breite","raum","bewegung"}) {
      add(std::string(section)+".trim", "Sektion Pegel", section,-12,12,0,"dB");
      if(std::string(section)!="raum" && std::string(section)!="bewegung")
        add(std::string(section)+".match.on", "Pegelvergleich", section,0,1,0);
    }
    add("reverb.type", "Room / Plate / Hall", "reverb",0,2,1);
    add("reverb.decay", "Hall Dauer (RT60 Ziel)", "reverb",.2,8,1.6,"s");
    add("room.decay", "Raum Dauer (RT60 Ziel)", "room",.1,2,.45,"s");
    add("throw.verb.decay", "Throw Hall Dauer", "throw",.2,8,1.8,"s");
    add("macro.v2.on", "Makros 0.5", "Global",0,1,0);
    add("klang.colour", "Dunkel / Hell", "klang",-1,1,0);
    add("stadium.pitch", "Crowd Pitch", "Stadium",-12,12,0,"st");
    add("stadium.low", "Crowd Low Cut", "Stadium",60,600,180,"Hz",true);
    add("stadium.high", "Crowd High Cut", "Stadium",1500,14000,6500,"Hz",true);
    add("duck.threshold", "Gesang erkannt ab", "Ducking",-60,-6,-38,"dB");
    return v;
  }();
  return p;
}
int index(std::string_view id) {
  const auto &p = parameters();
  for (size_t i = 0; i < p.size(); ++i)
    if (p[i].id == id)
      return int(i);
  throw std::runtime_error("Unknown parameter: " + std::string(id));
}
bool discrete(std::string_view id) {
  return id == "bypass" || id == "tune.root" || id == "tune.scale" ||
         id == "formant.preserve" || id == "double.voices" ||
         id == "stadium.root" || id == "stadium.loop" ||
         id == "stadium.texture" || id == "stadium.slot" || id == "stadium.loopSmooth" ||
         (id.substr(0,8) == "stadium." && (id.find(".root") != std::string_view::npos || id.find(".loop") != std::string_view::npos)) || id == "duck.source" || id == "pump.mode" ||
         id == "pump.target" || id == "throw.type" || id == "throw.feel" ||
         id == "throw.kill" || id == "throw.verb.type" || id == "deess.mode" ||
         id == "deess.preset" || id == "parallel.tap" ||
         id.substr(0, 6) == "route." || id == "stutter" ||
         id == "throw.trigger" || id.substr(0, 5) == "note." ||
         id.find(".on") != std::string_view::npos ||
         id.find(".type") != std::string_view::npos ||
         id.find(".sync") != std::string_view::npos ||
         id.find(".variant") != std::string_view::npos ||
         id.find(".listen") != std::string_view::npos || id == "res.delta";
}
Values defaults() {
  Values v;
  for (auto &p : parameters())
    v.push_back(p.def);
  return v;
}
const std::array<VariantProfile, 20> &profiles() {
  static const std::array<VariantProfile, 20> v{
      {{"Main Direkt",
        "Main",
        "Fuehrende Einzelstimme: Spitzenkontrolle, deutliche Mitten, kleiner "
        "Raum.",
        12,
        4,
        .5f,
        80,
        -1,
        3,
        1,
        5,
        .2f,
        .15f,
        16,
        3,
        .55f,
        .25f,
        .2f,
        .35f,
        1},
       {"Main Warm",
        "Main",
        "Weichere Stimme, weniger Schaerfe; kein Ersatz fuer eine warme "
        "Aufnahme.",
        8,
        2,
        2,
        180,
        1,
        1,
        -1,
        7,
        .45f,
        .1f,
        22,
        2,
        .4f,
        .3f,
        .15f,
        .2f,
        1.4f},
       {"Main Air kontrolliert",
        "Main",
        "Helle Praesenz mit De-Esser-Reserve. Sibilanz vor mehr Air pruefen.",
        10,
        3,
        .8f,
        110,
        -2,
        4,
        3,
        3,
        .1f,
        .18f,
        18,
        3,
        .65f,
        .35f,
        .25f,
        .3f,
        1},
       {"Main Radio dicht",
        "Main",
        "Gleichmaessige dichte Leadstimme; Eingangspegel kalibrieren.",
        17,
        6,
        .3f,
        70,
        -1,
        2,
        1,
        6,
        .3f,
        .2f,
        12,
        4,
        .6f,
        .2f,
        .3f,
        .5f,
        .8f},
       {"Main Shout rau",
        "Main",
        "Raues Shouting mit kraeftiger Verdichtung; braucht energische "
        "Performance.",
        15,
        8,
        .2f,
        55,
        -3,
        3,
        0,
        12,
        .6f,
        .25f,
        15,
        4,
        .7f,
        .25f,
        .3f,
        .55f,
        .6f},
       {"Backing weich",
        "Harmony/Backing",
        "Unter der Main bleiben; echte zusaetzliche Aufnahme oder Harmonie "
        "einspeisen.",
        8,
        2,
        8,
        220,
        -2,
        -2,
        -1,
        3,
        .15f,
        .35f,
        24,
        4,
        .8f,
        .45f,
        .25f,
        .25f,
        1.6f},
       {"Backing breit",
        "Harmony/Backing",
        "Unisono-Doubles verbreitern. Der Doubler erfindet keine gesungenen "
        "Harmonien.",
        10,
        3,
        4,
        160,
        -2,
        -1,
        1,
        4,
        .2f,
        .55f,
        30,
        6,
        1,
        .5f,
        .35f,
        .4f,
        1.3f},
       {"Harmony klar",
        "Harmony/Backing",
        "Bereits eingesungene mehrstimmige Harmonien, klare gemeinsame "
        "Artikulation.",
        9,
        2,
        6,
        180,
        -3,
        1,
        1,
        2,
        .1f,
        .4f,
        22,
        4,
        .9f,
        .55f,
        .3f,
        .3f,
        1.2f},
       {"Harmony warm",
        "Harmony/Backing",
        "Weicher Hintergrund fuer reale Harmonien; wenig harte Hoehen.",
        7,
        2,
        10,
        240,
        -1,
        -2,
        -2,
        6,
        .4f,
        .4f,
        28,
        5,
        .9f,
        .6f,
        .25f,
        .3f,
        1.8f},
       {"Backing tight",
        "Harmony/Backing",
        "Kurze eng gefuehrte Unterstuetzung mit wenig Raum.",
        13,
        4,
        1,
        90,
        -3,
        1,
        0,
        4,
        .2f,
        .3f,
        10,
        3,
        .65f,
        .15f,
        .1f,
        .35f,
        .8f},
       {"Chor Unisono dicht",
        "Chor/Crowd",
        "Mehrere Unisono-Takes bevorzugt; synthetische Dopplung ergaenzt "
        "Dichte.",
        14,
        5,
        1,
        130,
        -2,
        2,
        0,
        7,
        .4f,
        .55f,
        21,
        7,
        .95f,
        .5f,
        .25f,
        .4f,
        1},
       {"Chor Stadion weit",
        "Chor/Crowd",
        "Echte Gruppenrufe plus breiter Doubler; Stadium nur als Textur.",
        15,
        5,
        2,
        200,
        -3,
        1,
        1,
        6,
        .3f,
        .65f,
        34,
        8,
        1,
        .9f,
        .4f,
        .5f,
        1.4f},
       {"Chor Gang rau",
        "Chor/Crowd",
        "Raue dicht eingesungene Gang-Shouts; Saettigung unter Kontrolle "
        "halten.",
        18,
        8,
        .2f,
        80,
        -3,
        3,
        -1,
        13,
        .7f,
        .55f,
        18,
        8,
        1,
        .45f,
        .25f,
        .6f,
        .7f},
       {"Chor Antwort tight",
        "Chor/Crowd",
        "Kurze Antwort-Rufe; wenig Dauerhall, Throw bei Bedarf separat.",
        12,
        4,
        1,
        100,
        -3,
        2,
        0,
        5,
        .25f,
        .45f,
        13,
        5,
        .9f,
        .2f,
        .15f,
        .45f,
        .9f},
       {"Chor Finale gross",
        "Chor/Crowd",
        "Finalrefrain mit echten mehreren Takes, breiten Antworten und "
        "geduckten Tails.",
        17,
        6,
        .7f,
        180,
        -2,
        2,
        2,
        9,
        .5f,
        .7f,
        38,
        8,
        1,
        1,
        .5f,
        .65f,
        1.2f},
       {"Legacy Chor Hell",
        "Chor/Crowd",
        "Kompatibilitaetsprofil aus der ersten Alpha.",
        12,
        4,
        1,
        140,
        -2,
        3,
        3,
        5,
        .2f,
        .55f,
        25,
        6,
        1,
        .6f,
        .3f,
        .4f,
        1},
       {"Legacy Chor Dunkel",
        "Chor/Crowd",
        "Kompatibilitaetsprofil aus der ersten Alpha.",
        12,
        4,
        3,
        200,
        0,
        -2,
        -3,
        9,
        .5f,
        .55f,
        28,
        6,
        1,
        .6f,
        .3f,
        .4f,
        1.6f},
       {"Legacy Chor Gang",
        "Chor/Crowd",
        "Kompatibilitaetsprofil aus der ersten Alpha.",
        18,
        8,
        .3f,
        90,
        -3,
        3,
        0,
        12,
        .7f,
        .6f,
        18,
        8,
        1,
        .45f,
        .25f,
        .6f,
        .8f},
       {"Legacy Chor Antwort",
        "Chor/Crowd",
        "Kompatibilitaetsprofil aus der ersten Alpha.",
        12,
        4,
        1,
        100,
        -3,
        2,
        0,
        5,
        .25f,
        .4f,
        13,
        5,
        .85f,
        .2f,
        .15f,
        .4f,
        .9f},
       {"Legacy Chor Finale",
        "Chor/Crowd",
        "Kompatibilitaetsprofil aus der ersten Alpha.",
        17,
        6,
        .7f,
        180,
        -2,
        2,
        2,
        9,
        .5f,
        .7f,
        38,
        8,
        1,
        1,
        .5f,
        .65f,
        1.2f}}};
  return v;
}
const std::array<std::string, 20> &variants() {
  static const auto names = [] {
    std::array<std::string, 20> n;
    for (size_t i = 0; i < n.size(); ++i)
      n[i] = profiles()[i].name;
    return n;
  }();
  return names;
}
void resolveInto(const Values &b, Values &v) {
  v = b;
  auto get = [&](std::string_view id) { return b[size_t(index(id))]; };
  auto add = [&](std::string_view id, float d) {
    auto i = size_t(index(id));
    v[i] = std::clamp(b[i] + d, parameters()[i].min, parameters()[i].max);
  };
  auto enabled = [&](std::string_view id) { return get(id) > .5f; };
  auto off = [&](std::string_view id) { v[size_t(index(id))] = 0; };
  if (!enabled("druck.on")) {
    for (auto id : {"fet.on", "level.on", "mb.on", "parallel.on"})
      off(id);
  }
  if (!enabled("klang.on")) {
    for (auto id : {"inputEQ.on", "eq.on", "res.on", "air.on"})
      off(id);
  }
  if (!enabled("waerme.on"))
    off("sat.on");
  if (!enabled("breite.on"))
    off("double.on");
  if (!enabled("raum.on")) {
    for (auto id :
         {"room.on", "short.on", "long.on", "reverb.on", "room.return",
          "short.return", "long.return", "reverb.return"})
      off(id);
  }
  if (!enabled("bewegung.on"))
    off("pump.on");
  auto profile = [&](std::string_view id) -> const VariantProfile & {
    return profiles()[size_t(std::clamp(int(get(id)), 0, 19))];
  };
  const bool modern=enabled("quality.on");
  const bool character=enabled("macro.v2.on");
  auto &d = profile("druck.variant");
  float a = get("druck.amount");
  add("fet.threshold", -d.compression * a * (modern?1.3f:1.f));
  add("level.threshold", -d.compression * .55f * a * (modern?1.2f:1.f));
  if(!(modern && enabled("druck.match.on"))) {
    add("fet.makeup", d.compression * .25f * a);
    add("level.makeup", d.compression * .15f * a);
  }
  add("fet.ratio", (d.ratio - 4) * a);
  add("fet.attack", (d.attack - .5f) * a);
  add("fet.release", (d.release - 80) * a);
  auto &k = profile("klang.variant");
  a = get("klang.amount");
  add("eq0.gain", k.body * a);
  add("eq1.gain", -.5f * k.presence * a);
  add("eq2.gain", k.presence * a * (modern?1.35f:1.f));
  add("eq3.gain", k.air * a);
  add("air.high", std::max(0.f, k.air) * .06f * a);
  add("deess.threshold", -std::max(0.f, k.air) * a);
  if(character) {
    // Additional broad shelves preserve all manually edited band bases.
    // Colour is bipolar; intensity still scales only the chosen profile.
    add("eq0.gain", k.body*a*.8f);
    add("eq2.gain", k.presence*a*.75f);
    add("eq3.gain", k.air*a*1.5f);
    // Sibilance remains independent; colour must not silently change it.
    v[size_t(index("deess.threshold"))]=get("deess.threshold");
  }
  auto &w = profile("waerme.variant");
  a = get("waerme.amount");
  add("sat.mix", w.warmth * a * (modern?2.f:1.f));
  add("sat.drive", w.drive * a * (modern?1.5f:1.f));
  if(character) {
    add("sat.drive", (12+w.drive)*a);
    add("sat.mix", .65f*a);
    add("sat.tone", -std::max(0.f,get("sat.tone")-3800.f)*a);
  }
  auto &bwidth = profile("breite.variant");
  a = get("breite.amount");
  add("double.mix", bwidth.width * a * (modern?1.5f:1.f));
  add("double.time", (bwidth.time - 18) * a);
  add("double.voices", (bwidth.voices - 4) * a);
  add("double.width", (bwidth.stereo - 1) * a);
  if(character){add("double.mix",.8f*a);add("double.pitch",12*a);}
  auto &r = profile("raum.variant");
  a = get("raum.amount") * (modern?2.5f:1.f);
  add("room.send", r.space * .12f * a);
  add("short.send", r.space * .10f * a);
  add("reverb.send", r.space * .38f * a);
  add("long.send", r.space * .20f * a);
  add("long.feedback", r.feedback * .25f * a);
  auto &m = profile("bewegung.variant");
  a = get("bewegung.amount");
  add("pump.depth", m.pump * a);
  add("pump.shape", (m.shape - 1) * a);
}
Values resolve(const Values &b) {
  Values v = b;
  resolveInto(b, v);
  return v;
}
const std::vector<Preset> &presets() {
  static auto ps = [] {
    std::vector<Preset> p;
    for (auto c : {"Main", "Chor", "Backing/Harmony", "Effects"}) {
      std::string cat = c;
      int count = cat == "Chor" ? 20 : cat == "Effects" ? 15 : 10;
      for (int j = 0; j < count; ++j) {
        auto v = defaults();
        auto s = [&](std::string id, float x) { v[index(id)] = x; };
        int variant = cat == "Main"   ? j % 5
                      : cat == "Chor" ? 10 + j % 5
                                      : 5 + j % 5;
        for (auto m :
             {"druck", "klang", "waerme", "breite", "raum", "bewegung"})
          s(std::string(m) + ".variant", float(variant));
        s("druck.amount", .12f + .055f * (j % 9));
        s("klang.amount", .1f + .07f * (j % 6));
        s("waerme.amount", .04f * (j % 5));
        s("breite.amount",
          cat == "Main" ? .04f * (j % 3) : .3f + .04f * (j % 7));
        s("raum.amount", .08f + .05f * (j % 8));
        s("res.on", 1);
        s("air.on", 1);
        s("air.high", .02f * (j % 6));
        if (cat == "Chor") {
          s("double.voices", float(4 + j % 5));
          s("stadium.on", 1);
          s("stadium.mix", .1f + .025f * (j % 8));
        }
        if (cat == "Effects") {
          s("tune.on", 1);
          s("pitch", std::array<float, 5>{-12, 7, 12, 0, 0}[j % 5]);
          s("formant", std::array<float, 5>{-5, 4, 6, 0, 0}[j % 5]);
          s("tune.speed", j % 5 == 4 ? 0 : 50);
          s("stutter", j % 5 == 3 ? 1 : 0);
        }
        std::string n =
            cat == "Effects"
                ? std::array<std::string, 5>{"Monster Ansager", "Party Hoch",
                                             "Comic Kind", "Stutter",
                                             "Hard Tune"}[j % 5]
                : variants()[variant];
        Preset preset{n + " " + std::to_string(j + 1), cat, v};
        preset.description = profiles()[size_t(variant)].purpose +
                             " Globaler technischer Ausgangspunkt, noch kein "
                             "nachgehoertes Kuenstlerpreset.";
        p.push_back(std::move(preset));
      }
    }
#include "ReferencePresets.inc"
    for(auto& preset : p) {
      auto& v=preset.values;
      v[index("quality.on")]=1;
      v[index("macro.v2.on")]=1;
      if(preset.category.find("Studie/")!=0){v[index("druck.amount")]=std::max(.4f,v[index("druck.amount")]);v[index("klang.amount")]=std::max(.45f,v[index("klang.amount")]);}
      for(auto section : {"druck","klang","waerme","breite"})
        v[index(std::string(section)+".match.on")]=1;
      // More explicit front/back separation; dry centre, filtered rhythmic returns.
      v[index("reverb.time")]=45;
      v[index("reverb.decay")]=preset.category=="Chor" ? 2.2f : 1.5f;
      v[index("reverb.tail")]=8;
      v[index("throw.type")]=2;
      v[index("throw.beats")]=.5f;
      v[index("throw.high")]=5000;
      v[index("throw.low")]=250;
      v[index("throw.feedback")]=.38f;
    }
    return p;
  }();
  return ps;
}
} // namespace malle

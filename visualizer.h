// ╔══════════════════════════════════════════════════════════════╗
// ║                     visualizer.h                             ║
// ║  Easter egg WS2812B VU meter. Samples DAC_L audio from       ║
// ║  PIN_AUDIO, drives LED strip on PIN_LED. Only active when    ║
// ║  strip and audio wires are physically connected.             ║
// ╚══════════════════════════════════════════════════════════════╝

#pragma once
#include "config.h"
#include "state.h"

// Drives the WS2812B LED strip as a rainbow VU meter with a peak-hold dot.
// Called every loop iteration. If nothing is physically connected to
// PIN_AUDIO and PIN_LED the strip simply stays off — no configuration needed.
//
// Tuning constants (adjust in config.h):
//   VIS_SMOOTHING   — smooths the bar rise/fall (higher = slower)
//   VIS_PEAK_DECAY  — controls how fast the peak dot falls (higher = slower)
//   VIS_SENSITIVITY — peak ADC deviation that maps to a full strip (lower = more sensitive)
void handleVisualizer() {
  if (!playing) {
    strip.clear();
    strip.show();
    return;
  }

  static int   dcOffset    = 2048;  // slow-moving DC bias of the audio signal
  static float smoothLevel = 0;     // exponentially smoothed bar level
  static float peak        = 0;     // peak hold level for the white dot

  // Sample the DC offset using a 64-sample exponential moving average,
  // then take the peak of 32 rapid samples to catch transients accurately.
  int audioVal = analogRead(PIN_AUDIO);
  dcOffset = (dcOffset * 63 + audioVal) / 64;

  int peakRaw = 0;
  for (int s = 0; s < 32; s++) {
    int sample = abs(analogRead(PIN_AUDIO) - dcOffset);
    if (sample > peakRaw) peakRaw = sample;
  }

  // Map peak deviation to LED count and apply smoothing
  int level   = constrain(map(peakRaw, 0, VIS_SENSITIVITY, 0, NUM_LEDS), 0, NUM_LEDS);
  smoothLevel = smoothLevel * VIS_SMOOTHING + level * (1.0f - VIS_SMOOTHING);

  // Peak hold: rises instantly, decays slowly
  if (smoothLevel > peak) peak = smoothLevel;
  else                    peak *= VIS_PEAK_DECAY;

  int displayLevel = (int)smoothLevel;
  int peakPos      = (int)peak;

  // Draw rainbow bar + white peak dot
  for (int i = 0; i < NUM_LEDS; i++) {
    if (i < displayLevel) {
      uint8_t hue = map(i, 0, NUM_LEDS, 0, 255);
      strip.setPixelColor(i, strip.ColorHSV(hue * 256, 255, 200));
    } else if (i == peakPos) {
      strip.setPixelColor(i, strip.Color(255, 255, 255));
    } else {
      strip.setPixelColor(i, 0);
    }
  }
  strip.show();
}
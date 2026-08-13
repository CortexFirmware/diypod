// ╔══════════════════════════════════════════════════════════════╗
// ║                      battery.h                               ║
// ║  Battery voltage sampling via ADC voltage divider on         ║
// ║  PIN_VBAT. Returns 0–100% or -1 if no divider connected.     ║
// ╚══════════════════════════════════════════════════════════════╝

#pragma once
#include "config.h"

// Returns battery charge as 0–100 (percent).
// Returns -1 if no voltage divider is connected (floating pin).
//
// Implementation details:
//   - 8-sample rolling average reduces ADC noise
//   - raw < 2000 indicates a floating pin — no divider present
//   - Voltage is calculated assuming a 100kΩ+100kΩ divider on a 3.3V ADC
//   - LiPo range mapped from 3.3V (0%) to 4.2V (100%)
//   - 2% deadband prevents jitter from causing unnecessary redraws
int getBatteryPct() {
#if VBAT_SIMULATE
  return VBAT_SIM_PCT;
#else
  static int     samples[8] = {0};
  static uint8_t idx        = 0;

  samples[idx] = analogRead(PIN_VBAT);
  idx = (idx + 1) % 8;

  long sum = 0;
  for (int i = 0; i < 8; i++) sum += samples[i];
  int raw = sum / 8;

  if (raw < 2000) return -1;  // Floating pin — no divider connected

  float v   = (raw / 4095.0f) * 3.3f * 2.0f;
  int   pct = (int)((v - 3.3f) / (4.2f - 3.3f) * 100.0f);
  pct = constrain(pct, 0, 100);

  // 2% deadband — ignore small changes to prevent display jitter
  static int lastPct = -1;
  if (lastPct >= 0 && abs(pct - lastPct) < 2) return lastPct;
  lastPct = pct;
  return pct;
#endif
}
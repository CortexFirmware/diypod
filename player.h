// ╔══════════════════════════════════════════════════════════════╗
// ║                       player.h                               ║
// ║  Playback control (playTrack, onFinished), settings          ║
// ║  persistence (saveSettings, loadSettings) via Preferences.   ║
// ╚══════════════════════════════════════════════════════════════╝

#pragma once
#include "config.h"
#include "state.h"

// Plays the given track number, clamped to the valid range.
// Resets elapsed time tracking and tells the DFPlayer to play.
void playTrack(int t) {
  track       = constrain(t, 1, trackTotal > 0 ? trackTotal : 9999);
  playing     = true;
  trackStart  = millis();
  pausedAccum = 0;
  dirty       = true;
  if (dfReady) df.play(track);
}

// Saves user settings to flash. Called on menu exit from
// Volume, Equalizer, and Settings screens.
void saveSettings() {
  prefs.begin("diypod", false);
  prefs.putInt("vol",          vol);
  prefs.putInt("eqIdx",        eqIdx);
  prefs.putBool("shuffle",     shuffle);
  prefs.putInt("repeatMd",     (int)repeatMd);
  prefs.putBool("autoPlay",    autoPlay);
  prefs.putInt("sleepTimeout", sleepTimeout);
  prefs.end();
}

// Loads user settings from flash on boot.
// Falls back to sensible defaults if no saved settings exist.
void loadSettings() {
  prefs.begin("diypod", true);
  vol          = prefs.getInt("vol",          20);
  eqIdx        = prefs.getInt("eqIdx",        0);
  shuffle      = prefs.getBool("shuffle",     false);
  repeatMd     = (RepeatMode)prefs.getInt("repeatMd", 0);
  autoPlay     = prefs.getBool("autoPlay",    true);
  sleepTimeout = prefs.getInt("sleepTimeout", 0);
  prefs.end();
}

// Called when the DFPlayer signals a track has finished.
// Handles repeat, auto-play, shuffle, and end-of-library behaviour.
void onFinished() {
  // Repeat single track
  if (repeatMd == REP_ON) {
    if (dfReady) df.play(track);
    trackStart = millis(); pausedAccum = 0;
    return;
  }

  // Auto-play off — stop after each track
  if (!autoPlay && repeatMd == REP_OFF) {
    playing = false; pausedAt = millis(); dirty = true;
    return;
  }

  // Advance to next track (shuffle or sequential)
  int next;
  if (shuffle) {
    do { next = random(1, trackTotal + 1); } while (next == track && trackTotal > 1);
  } else {
    next = track + 1;
    // End of library — stop playback
    if (trackTotal > 0 && next > trackTotal) {
      playing = false; pausedAt = millis(); dirty = true;
      return;
    }
  }
  playTrack(next);
}
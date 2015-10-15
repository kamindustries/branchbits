#pragma once

#include <map>
#include <math.h>

#include "allocore/io/al_App.hpp"
#include "allocore/sound/al_reverb.hpp"
#include <Gamma/Noise.h>
#include <Gamma/Oscillator.h>
#include <Gamma/Filter.h>
#include <Gamma/Delay.h>
#include <Gamma/Envelope.h>
#include "allocore/graphics/al_Mesh.hpp"

#define NUM_SINE 20         // number of sine waves
const int numSoundSources =  4;    // number of sound sources

gam::Sine<> sine[NUM_SINE];
gam::Sine<> leafWave[NUM_SINE];
gam::LFO<> dtLFO;
gam::Delay<> echo[numSoundSources];
gam::OnePole<> highPassFilter;
gam::AD<> leafAD;
gam::AD<> branchAD;
gam::Accum<> tmr; // Timer for resetting envelope

Reverb<> reverb[numSoundSources];
float carrier[numSoundSources];
// sound source to represent a sound in space
SoundSource tap[numSoundSources];
SoundSource tapOut;
float f[NUM_SINE];
map<int,int> majorScale;

float soundTime = 0;

Mesh m_tap;

void audioSetup() {
// init audio and ambisonic spatialization
  // cout << "test: " << AlloSphereAudioSpatializer::audioIO().fps() << endl;
  // cout<< "check" <<endl;

  // set up major scale map
  majorScale[0] = 0;
  majorScale[2] = 4;
  majorScale[5] = 7;
  majorScale[9] = 11;
  // modFreq.freq(1.f/20.f);

  highPassFilter.freq(1100);
  tmr.period(1.3);
  branchAD.attack(0.02);
  branchAD.decay(1.2);

  // add our sound source to the audio scene
  for (int i = 0; i<numSoundSources; i++) {
    tap[i].pose(Pose(Vec3f( 4 * cos(M_PI*(i+1)/(numSoundSources)), 0, 
                            4 * sin(M_PI*(i+1)/(numSoundSources)) ), Quatf()));
    carrier[i] = 0.f;
    m_tap.vertex(Vec3f( 4 * cos( 2.f * 3.1415 * ((float)i/numSoundSources) ), 0, 
                        4 * sin( 2.f * 3.1415 * ((float)i/numSoundSources) ) ));
    m_tap.color(RGB(1,0,0));
    echo[i].maxDelay(.7);
    echo[i].delay(.7);
  }

  for (int i=0; i<NUM_SINE; i++) {
    sine[i].freq(440);
    leafWave[i].freq(0);
  }
}

void setLeafFreq(int idx) {
  leafWave[idx % NUM_SINE].freq(220 * (idx % NUM_SINE + 1.f * 0.5));

  // reset leaf frequencies to 0 if too many are pinging
  int leafWaveCheck = 0;
  for (int i=0; i<NUM_SINE; i++){
    if (leafWave[i].freq() > 0) leafWaveCheck++;
  }
  if (leafWaveCheck >= NUM_SINE / 2) {
    for (int i=0; i<NUM_SINE; i++) leafWave[i].freq(0.0);
  }
}

void updateAudio(double gain){
  if (tmr()) {
    soundTime += 0.01; if(soundTime>1) soundTime = 0;
    branchAD.lengths(soundTime, 1-soundTime);
    leafAD.lengths(soundTime*2.f, 1-soundTime*2.f);
    branchAD.reset();
    leafAD.reset();
  }

  for (int i=0; i<NUM_SINE; i++) {
    // carrier[i%numSoundSources] += ((sine[i]() * branchAD()) + (leafWave[i]() * leafAD()) * 0.4) / NUM_SINE;
    carrier[i%numSoundSources] += ((sine[i]() * branchAD())) / numSoundSources;
  }

  for (int i=0; i<numSoundSources; i++) {
    float s = carrier[i] ;
    float s_reverb; // is not initialized?????? 0??????
    reverb[i].damping(.2);
    reverb[i].mix(s, s_reverb, .8);
    // tap[i].writeSample(highPassFilter( s + 0.5 * echo[i](s + echo[i]() * .05 ) / SOUND_SOURCES ));
    tap[i].writeSample(highPassFilter( s / numSoundSources ) * gain );
    carrier[i] = 0.0;
  }
}
#pragma once

#include "ofMain.h"

#include "ofxAudioUnit.h"
#include "ofxUI.h"
#include "ofxXmlSettings.h"

#include "scrollingGraph.h"
#include "smoother.h"

#include "pitchDetector.h"

struct marker {
    float start;
    float end;
};


struct audioNote {
    
    bool bPlaying;
    vector < float > samples;
    int playhead;
};


class testApp : public ofBaseApp{

public:
    void setup();
    void update();
    void draw();
    void exit();
    
    void setupGUI();
    void guiEvent(ofxUIEventArgs &e);

    void keyPressed(int key);
    void keyReleased(int key);
    void mouseMoved(int x, int y );
    void mouseDragged(int x, int y, int button);
    void mousePressed(int x, int y, int button);
    void mouseReleased(int x, int y, int button);
    void windowResized(int w, int h);
    void dragEvent(ofDragInfo dragInfo);
    void gotMessage(ofMessage msg);

    ofSoundStream ss;
    void audioIn(float * input, int bufferSize, int nChannels);
    void audioOut(float * output, int bufferSize, int nChannels);
    
    int numPDs;
    vector<char_t *> methods;
    vector<pitchDetector> pitchDetectors;
    vector<smoother> smoothers;
    vector<scrollingGraph> pitchGraphs, medianGraphs, velGraphs;
    vector<bool> drawPitch, drawMedian;
    vector<ofColor> graphColors;
    
    
    // zach notes:
    audioNote currentNote;
    vector < audioNote > notes;

    
    float graphWidth;
    float graphMax;
    float graphHeight;
    
    bool startFound;
    int currentStart;
    vector<marker> markers;
    bool drawMarkers;
    
    scrollingGraph runs;
    
    // recording
    bool bAmRecording;
    bool bGoodNoteFound;
    bool bWasRecording;
    //AUs
    ofxAudioUnitFilePlayer player;
    ofxAudioUnit lpf;
    ofxAudioUnitTap tap;
    ofxAudioUnitOutput output;
    ofxAudioUnitMixer mixer;
    
    vector<float> samples;
    
    float threshold;
    float minDuration, maxDuration;
    float noteRun;
    
    //UI
    ofxUICanvas * gui;
    
    int notePlayed;
    bool notePlaying;
    
    bool belowThresh;
    
    int PDMethod;
    
    uint_t samplerate;
    uint_t win_s; // window size
    uint_t hop_s;  // hop size
    
    // create some vectors
    fvec_t * in; // input buffer
    
};

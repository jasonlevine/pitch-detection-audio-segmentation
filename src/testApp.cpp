#include "testApp.h"
#include "aubio.h"

//transient/steady-state separation

uint_t samplerate = 44100;
uint_t win_s = 2048; // window size
uint_t hop_s = 1024;  // hop size

// create some vectors
fvec_t * in       = new_fvec (hop_s); // input buffer
cvec_t * fftgrain = new_cvec (win_s); // fft norm and phase
cvec_t * cstead   = new_cvec (win_s); // fft norm and phase
cvec_t * ctrans   = new_cvec (win_s); // fft norm and phase
fvec_t * stead    = new_fvec (hop_s); // output buffer
fvec_t * trans    = new_fvec (hop_s); // output buffer

// create phase vocoder for analysis of input signal
aubio_pvoc_t * pv = new_aubio_pvoc (win_s,hop_s);
// create transient/steady-state separation object
aubio_tss_t *  tss = new_aubio_tss(win_s,hop_s);
// create phase vocoder objects for synthesis of output signals
aubio_pvoc_t * pvt = new_aubio_pvoc(win_s,hop_s);
aubio_pvoc_t * pvs = new_aubio_pvoc(win_s,hop_s);



void
process_tss() {
    
    /* execute stft */
    
    // fftgrain = pv(in)
    aubio_pvoc_do (pv, in, fftgrain);
    // ctrans, cstead = tss (fftgrain)
    aubio_tss_do (tss, fftgrain, ctrans, cstead);
    // stead = pvt_inverse (cstead)
    // trans = pvt_inverse (ctrans)
    aubio_pvoc_rdo (pvt, cstead, stead);
    aubio_pvoc_rdo (pvs, ctrans, trans);
    
}



////pitch detection
//aubio_pitch_t *o;
//aubio_wavetable_t *wavetable;
//fvec_t *pitch;
////fvec_t *ibuf = new_fvec (hop_s);
////fvec_t *obuf = new_fvec (hop_s);
////uint_t hop_size = 1024;
////uint_t buffer_size = 512;
//uint_t samplerate = 44100;
//float pitch_tolerance = 0;
//float silence_threshold = -90.;
//char_t * pitch_unit = "yinfft";
//char_t * pitch_method = "midi";
//int blocks = 0;
//smpl_t freqIn;
//smpl_t freqStead;
//
//void
//process_pitch() {
//    
//    aubio_pitch_do (o, in, pitch);
//    freqIn = fvec_get_sample(pitch, 0);
//    
//    aubio_pitch_do (o, stead, pitch);
//    freqStead = fvec_get_sample(pitch, 0);
//    
//}




void
process_print (void) {
//    smpl_t pitch_found = fvec_get_sample(pitch, 0);
//    printf("%f %f\n",(blocks)
//           *hop_size/(float)samplerate, pitch_found);
}




//--------------------------------------------------------------
void testApp::setup(){

    
    numPDs = 6;
    
    pitchDetector tempPD;
    for (int i = 0; i < numPDs; i++) {
        pitchDetectors.push_back(tempPD);
    }
    
//    for (int i = 0; i < numPDs; i++) {
//        pitchDetectors.push_back(new pitchDetector);
//    }
    
    methods.push_back("yin");
    methods.push_back("yinfft");
    methods.push_back("specacf");
    methods.push_back("schmitt");
    methods.push_back("mcomb");
    methods.push_back("fcomb");
    
    for (int i = 0; i < numPDs; i++) {
        pitchDetectors[i].setup("midi", methods[i]);
    }
    
    smoother tempSmoother;
    tempSmoother.setNumPValues(11);
    for (int i = 0; i < numPDs; i++) {
        smoothers.push_back(tempSmoother);
    }
    
    
    graphWidth = ofGetWidth()/2;
    graphMax = 120;
    
    scrollingGraph tempGraph;
    tempGraph.setup(graphWidth, 0, 0, graphMax);
    for (int i = 0; i < numPDs; i++) {
        pitchGraphs.push_back(tempGraph);
        medianGraphs.push_back(tempGraph);
        velGraphs.push_back(tempGraph);
    }

    for (int i = 0; i < numPDs; i++) {
        ofColor tempColor;
        tempColor.setHsb(i*40, 255, 180, 200);
        graphColors.push_back(tempColor);
    }

    drawPitch.assign(numPDs, false);
    drawPitch[0] = true;
    drawMedian.assign(numPDs, false);

    startFound = false;
    currentStart = graphWidth - 1;
    
    //AU
    
    lpf = ofxAudioUnit(kAudioUnitType_Effect, kAudioUnitSubType_LowPassFilter);
    lpf.setParameter(kLowPassParam_CutoffFrequency, kAudioUnitScope_Global, 20000);

    player.setFile(ofToDataPath("Marc Terenzi - Love To Be Loved By You [692].mp3")); //Marc Terenzi - Love To Be Loved By You [692].mp3
    
    tap.setBufferLength(1024);
    
    mixer.setInputBusCount(1);
    player.connectTo(lpf).connectTo(tap).connectTo(mixer, 0).connectTo(output);

    
    mixer.setOutputVolume(0.00);
    output.start();
    
    player.loop();
    
    samples.assign(hop_s, 0.0);
    
    threshold = 0.1;
    minDuration = 25;
    maxDuration = 50;
    drawMarkers = true;
    
    bWasRecording = false;
    bAmRecording = false;
    bGoodNoteFound = false;
    
    
    setupGUI();
    ss.setup(this, 1, 1, samplerate, hop_s, 4);
}



//--------------------------------------------------------------
void testApp::update(){
    
    
    
    for (int i = 0; i < numPDs; i++) {
        pitchGraphs[i].addValue(pitchDetectors[i].getPitch());
        smoothers[i].addValue(pitchDetectors[i].getPitch());
        medianGraphs[i].addValue(smoothers[i].getMedian());
        
        
        float lastVal = medianGraphs[i].valHistory[medianGraphs[i].valHistory.size() - 1];
        float secondLastVal = medianGraphs[i].valHistory[medianGraphs[i].valHistory.size() - 2];
        float vel = abs(lastVal - secondLastVal);
        velGraphs[i].addValue(vel);
        
        if (vel < threshold * graphMax )  belowThresh = true;
        else belowThresh = false;
        
        cout << belowThresh << endl;
        
        //startFound
        if ( velGraphs[i].valHistory[velGraphs[i].valHistory.size()-2] > threshold * graphMax
            && velGraphs[i].valHistory[velGraphs[i].valHistory.size()-1] <= threshold * graphMax
            && !startFound) {
            
//            cout << "startFound" << endl;
            
            startFound = true;
            bAmRecording = true;
            currentStart = graphWidth - 1;
        }
        //endfound
        else if ( velGraphs[i].valHistory[velGraphs[i].valHistory.size()-2] <= threshold * graphMax
                 && velGraphs[i].valHistory[velGraphs[i].valHistory.size()-1] > threshold * graphMax
                 && startFound) {
            
            startFound = false;
            bAmRecording = false;
            
//            cout << "endFound" << endl;
            
            
            if ( (graphWidth - 1) - currentStart > minDuration ) {
//                cout << "start : " << currentStart << " end : " << graphWidth - 1 << endl;
                marker segment;
                segment.start = currentStart;
                segment.end = graphWidth - 1;
                markers.push_back(segment);
                
                bGoodNoteFound = true;
            }

        }
        
    }
    
    if (startFound) currentStart--;
    if (markers.size() > 0) {
        for (int i = 0; i < markers.size(); i++) {
            markers[i].start--;
            markers[i].end--;
        }
    }

    
}

//--------------------------------------------------------------
void testApp::draw(){

    for (int i = 0; i < numPDs; i++) {
        if ( drawPitch[i] ) {
            ofSetColor(graphColors[i]);
            ofDrawBitmapString(methods[i], ofGetWidth() / 7 * (i+1), 50);
//            ofPushMatrix();
//            ofTranslate(0, 100);
//            pitchGraphs[i].draw(ofGetHeight()/3);
//            ofPopMatrix();
            
            ofPushMatrix();
            ofTranslate(0, 100);
            medianGraphs[i].draw(ofGetHeight()/3);
            if (drawMarkers) {
                
                for (int i = 0; i < markers.size(); i++) {
                    ofSetColor(255,255,255,127);
//                    ofRect(markers[i].start * 2, ofGetHeight()/3, markers[i].end * 2, 0);
                    ofRect(markers[i].start * 2, 0, markers[i].end * 2 - markers[i].start * 2, ofGetHeight()/3);
                    
//                    ofSetColor(0,255,0);
//                    ofLine(markers[i].start * 2, ofGetHeight()/3, markers[i].start * 2, 0);
//                    
//                    ofSetColor(255,0,0);
//                    ofLine(markers[i].end * 2, ofGetHeight()/3, markers[i].end * 2, 0);
                }
            }
            ofPopMatrix();
            
            ofPushMatrix();
            ofTranslate(0, ofGetHeight()/3 + 100);
            ofSetColor(255,0,0);
            velGraphs[i].draw(ofGetHeight()/3);
            
            if (drawMarkers) {
                
                ofSetColor(0);
                float height = (1.0 - threshold) * ofGetHeight()/3;
                ofLine(0, height, ofGetWidth(), height);
                
                ofSetColor(0,0,0,127);
                for (int j = 0; j < velGraphs[i].valHistory.size(); j++) {
                    if ( velGraphs[i].valHistory[j] > threshold * graphMax ) {
                        ofLine(j * 2, 0, j * 2, height);
                    }
                }
                
            }


            ofPopMatrix();
        }
    }

}

void testApp::exit(){
    del_aubio_pvoc(pv);
    del_aubio_pvoc(pvt);
    del_aubio_pvoc(pvs);
    del_aubio_tss(tss);
    
    del_fvec(in);
    del_cvec(fftgrain);
    del_cvec(cstead);
    del_cvec(ctrans);
    del_fvec(stead);
    del_fvec(trans);
    
    for (int i = 0; i < numPDs; i++) {
        pitchDetectors[i].cleanup();
    }
    
    aubio_cleanup();
    
    ss.stop();
}



//--------------------------------------------------------------
void testApp::audioIn(float * input, int bufferSize, int nChannels){
    
    tap.getSamples(samples);
    
   // cout << samples.size() << endl;
    
    if (samples.size() > 0) {
        for (int i = 0; i < bufferSize; i++){
            //        in->data[i] = input[i*nChannels];
            in->data[i] = samples[i];
            
        }
        
        process_tss();
        for (int i = 0; i < numPDs; i++) {
            pitchDetectors[i].process_pitch(in);
        }

        process_print();
    }
    
    
//    bool bAmRecording = /*have we found the beginning?*/
    
    if (bAmRecording){
        for (int i = 0; i < samples.size(); i++){
            currentNote.samples.push_back(samples[i]);
        }
    } else  { //if (bWasRecording)
        
        if ( bGoodNoteFound ){ // is this a good note / ending ?
            currentNote.playhead = 0;
            currentNote.bPlaying = true;
            notes.push_back(currentNote);
            bGoodNoteFound = false;
        }
        currentNote.samples.clear();
    }
    
}

void testApp::audioOut(float * output, int bufferSize, int nChannels){
    
    
    for (int j = 0; j < bufferSize; j++){
        output[j] = 0;
    }
    
    for (int i = 0; i < notes.size(); i++){
        if (notes[i].bPlaying == true && (notes[i].playhead + bufferSize) < notes[i].samples.size()){
            
            int playhead = notes[i].playhead;
            for (int j = 0; j < bufferSize; j++){
                output[j] += notes[i].samples[playhead + j] * 0.2;
            }
            notes[i].playhead += bufferSize ;
            
        } else {
            notes[i].bPlaying = false;
        }
        
    }
}



void testApp::setupGUI(){
    //init params
    
    //init gui dims
    float dim = 16;
	float xInit = OFX_UI_GLOBAL_WIDGET_SPACING;
    float length = 255-xInit;
    
    //gui!
    gui = new ofxUICanvas(0, 0, length+xInit, ofGetHeight());
    
    gui->addFPSSlider("FPS SLIDER", length-xInit, dim*.25, 60);
    
    gui->addSpacer(length-xInit, 1);
    gui->addSlider("LPF cutoff", 1000, 20000, 20000, length-xInit, dim);
    gui->addSlider("LPF resonance", -20.0, 40.0, 0.0, length-xInit, dim);
    gui->addIntSlider("MF numPValues", 3, 33, 11, length-xInit, dim);
    gui->addSpacer(length-xInit, 1);
    gui->addSlider("Threshold", 0.0, 1.0, &threshold, length-xInit, dim);
    gui->addSlider("Min duration", 1, 60, &minDuration, length-xInit, dim);
    
    ofAddListener(gui->newGUIEvent,this,&testApp::guiEvent);
}

void testApp::guiEvent(ofxUIEventArgs &e){
    string name = e.widget->getName();
	int kind = e.widget->getKind();
    
    if(name == "LPF cutoff") {
        ofxUISlider *slider = (ofxUISlider *) e.widget;
        lpf.setParameter(kLowPassParam_CutoffFrequency, kAudioUnitScope_Global, slider->getScaledValue());
    }
    else if (name == "LPF resonance") {
     // Global, dB, -20->40, 0
        ofxUISlider *slider = (ofxUISlider *) e.widget;
        lpf.setParameter(kLowPassParam_Resonance, kAudioUnitScope_Global, slider->getScaledValue());
    }
    else if (name == "MF numPValues") {
        ofxUIIntSlider *slider = (ofxUIIntSlider *) e.widget;
        for (int i = 0; i < numPDs; i++) {
            smoothers[i].setNumPValues(slider->getValue());
        }
    }
}

//--------------------------------------------------------------
void testApp::keyPressed(int key){
//    if (key == ' ') {
//        cout << "start" << endl;
//        for (int i = 0; i < hop_s; i++) {
//            cout << in->data[i] << endl;
//        }
//        cout << "end" << endl;
//    }
    
    switch (key) {
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        {
            drawPitch.assign(numPDs, false);
            int index = key - 49;
            drawPitch[index] = !drawPitch[index];
            break;
        }
            
        case 'm':
            drawMarkers = !drawMarkers;
            break;
    }
    
//    int index = key - 49;
//    cout << index << endl;

}

//--------------------------------------------------------------
void testApp::keyReleased(int key){

}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void testApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void testApp::dragEvent(ofDragInfo dragInfo){ 

}

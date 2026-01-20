/*
  ==============================================================================

    Visualizer.h
    Created: 22 May 2016 2:45:34pm
    Author:  Johannes Neumann

  ==============================================================================
*/

#ifndef VISUALIZER_H_INCLUDED
#define VISUALIZER_H_INCLUDED

#include "VCOTuner.h"

class Visualizer: public Component,
                  public VCOTuner::Listener
{
public:
    Visualizer(VCOTuner* t);
    ~Visualizer();
    
    void paintWithFixedScaling(Graphics& g, int width, int height, double min, double max);
    void paint(Graphics& g, int width, int height);
    virtual void paint(Graphics& g);
    
    virtual void newMeasurementReady(const VCOTuner::measurement_t& m);
    
    void clearCache() { measurements.clear(); }

private:
    void drawTopInfoPanel(Graphics& g, int width, int panelHeight);
    void drawTunerDial(Graphics& g, Rectangle<float> bounds, float cents, int midiNote, float frequency);
    void drawTunerArc(Graphics& g, float centerX, float centerY, float radius, float cents);

    /** holds the list of completed measurements */
    Array<VCOTuner::measurement_t> measurements;
    
    float heightForFlipping;
    float yFlip(float y);
    
    VCOTuner* tuner;
};


#endif  // VISUALIZER_H_INCLUDED

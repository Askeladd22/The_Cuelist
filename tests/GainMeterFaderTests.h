#pragma once
#include "GainMeterFader.h"
#include "MixerControls.h"

void gainMeterFaderTests() {
    test("Fader wheel steps are calibrated, precise and respect silence",[] {
        GainMeterFader fader;
        near(GainMeterFader::wheelTargetDb(-12.0f,1,false),-11.5);
        near(GainMeterFader::wheelTargetDb(-12.0f,-1,false),-12.5);
        near(GainMeterFader::wheelTargetDb(-12.0f,1,true),-11.9);
        near(GainMeterFader::wheelTargetDb(-12.0f,0,false),-12.0);
        near(GainMeterFader::wheelTargetDb(0.0f,1,false,0.0f),0.0);
        near(GainMeterFader::wheelTargetDb(GainMeterFader::silenceDb,1,false),-59.5);
        check(GainMeterFader::wheelTargetDb(-60.0f,-1,false)==
                  GainMeterFader::silenceDb &&
              GainMeterFader::wheelTargetDb(GainMeterFader::silenceDb,-1,true)==
                  GainMeterFader::silenceDb,
              "Wheel edits cannot reach and preserve digital silence");
        fader.setGainDb(-8.0f,true);
        near(fader.getGainDb(),-8.0);
    });
    test("Fader range geometry round-trips at -inf/+6 and -inf/0 ceilings", [] {
        const float top=12,bottom=124;
        for (const float ceiling:{6.0f,0.0f}) {
            for (const float db:{-48.0f,-36.0f,-24.0f,-18.0f,-12.0f,-6.0f,0.0f}) {
                const auto y=GainMeterFader::yForDb(db,top,bottom,ceiling);
                near(GainMeterFader::dbForY(y,top,bottom,ceiling),db);
                check(y>=top && y<=bottom,"Tick fell outside the selected range");
            }
            near(GainMeterFader::dbForY(top,top,bottom,ceiling),ceiling);
            check(GainMeterFader::dbForY(bottom,top,bottom,ceiling)==GainMeterFader::silenceDb,
                  "Lowest fader position did not become digital silence");
        }
        near(GainMeterFader::yForDb(0,top,bottom,0),top);
        near(GainMeterFader::toLinear(GainMeterFader::silenceDb),0);
        near(GainMeterFader::toLinear(0),1);
        check(std::abs(GainMeterFader::toLinear(6)-std::pow(10.0,6.0/20.0))<0.0001,
              "6 dB ceiling has incorrect linear gain");
        check(GainMeterFader::yForDb(6,top,bottom)<GainMeterFader::yForDb(0,top,bottom),
              "Positive gain drawn below unity on the +6 dB scale");
    });
    test("GainMeterFader owns one mouse/keyboard control and preserves saved dB range", [] {
        GainMeterFader component;
        component.setSize(92,148);
        check(component.getNumChildComponents()==0,"Gain/meter is implemented as stacked child widgets");
        int callbacks=0; float delivered=0;
        component.onGainChanged=[&](float db) { ++callbacks; delivered=db; };
        component.setGainDb(-6.3f,true);
        near(component.getGainDb(),-6.3);
        near(delivered,-6.3);
        check(callbacks==1,"Drag/edit callback did not fire exactly once");
        component.setGainDb(-6.3f,true);
        check(callbacks==1,"Identical value produced unnecessary render state rebuild");
        component.setGainDb(0);
        near(component.getGainDb(),0);
        check(callbacks==1,"UI synchronisation must not notify the engine");
        component.setGainDb(900,true);
        near(component.getGainDb(),6);
        component.setGainDb(-900,true);
        near(component.getGainDb(),-60);
        check(component.getGainDb()>=GainMeterFader::minDb,"Gain fell below floor");
        component.setGainDb(5,false);
        const int priorCallbacks=callbacks;
        component.setMaximumDb(0);
        near(component.getGainDb(),5);
        check(callbacks==priorCallbacks,"Switching preference changed saved gain");
        component.setGainDb(900,true);
        near(component.getGainDb(),0);
        component.setGainDb(GainMeterFader::silenceDb,true);
        check(component.getGainDb()==GainMeterFader::silenceDb &&
              GainMeterFader::toLinear(component.getGainDb())==0.0f,
              "Minimum fader does not fully mute linear audio");
        near(juce::Decibels::decibelsToGain(-60.0f,-60.0f),0);
    });
    test("stereo meter displays independent L/R dBFS, silence and decay/hold", [] {
        GainMeterFader component;
        component.setSize(92,148);
        component.setStereoPeaks(1.0f,0.1f);
        near(component.getDisplayedPeakDb(0),0.0);
        near(component.getDisplayedPeakDb(1),-20.0);
        const float held=component.getHeldPeakDb(0);
        for (int i=0;i<5;++i) component.setStereoPeaks(0,0);
        check(component.getDisplayedPeakDb(0)<0 && component.getDisplayedPeakDb(1)<-20,
              "Stereo meters must decay independently when silent");
        near(component.getHeldPeakDb(0),held);
        for (int i=0;i<100;++i) component.setStereoPeaks(0,0);
        near(component.getDisplayedPeakDb(0),-60);
        near(component.getDisplayedPeakDb(1),-60);
        near(component.getHeldPeakDb(0),-60);
        component.setStereoPeaks(0.00000001f,0.95f);
        near(component.getDisplayedPeakDb(0),-60);
        check(component.getDisplayedPeakDb(1)>-1.0f,"Near-clipping peak not displayed");
    });
    test("N5.3 column dB scale always labels unity and digital silence",[] {
        using namespace cuelist;
        constexpr int ticks[]={6,0,-6,-12,-18,-24,-36,-48,-60};
        for(const int height:{135,148,163,195,260}) {
            const float top=12.f,bottom=(float)(height-48);
            for(const float ceiling:{6.f,0.f}) {
                const auto labels=GainMeterFader::columnTickLabelMask(
                    top,bottom,ceiling);
                check(labels[1] && labels[8],
                      "N5.3 a middle tick displaced 0 dB or -infinity");
                check((ceiling!=0.f || !labels[0]),
                      "N5.3 a +6 dB caption was drawn above a zero ceiling");
                for(int i=0;i<9;++i) {
                    if(!labels[(size_t)i]) continue;
                    check((float)ticks[i]<=ceiling &&
                          GainMeterFader::yForDb((float)ticks[i],top,bottom,ceiling)
                              >=top &&
                          GainMeterFader::yForDb((float)ticks[i],top,bottom,ceiling)
                              <=bottom,
                          "N5.3 visible label is out of calibrated scale");
                    for(int j=i+1;j<9;++j) {
                        if(!labels[(size_t)j]) continue;
                        const float delta=std::abs(
                            GainMeterFader::yForDb((float)ticks[i],
                                                      top,bottom,ceiling)-
                            GainMeterFader::yForDb((float)ticks[j],
                                                      top,bottom,ceiling));
                        check(delta>=13.49f,
                              "N5.3 two compact scale captions overlap");
                    }
                }
            }
        }
        // The default shared component is still the exact pinned Master
        // presentation. A column-only style can be enabled and switched
        // off without changing gain, telemetry, callbacks or base pixels.
        GainMeterFader master;
        master.setSize(92,163);
        check(!master.isColumnReadableScale() &&
              master.getGainDb()==0.0f && master.getNumChildComponents()==0,
              "N5.3 default/master fader unexpectedly changed presentation");
        const auto original=master.createComponentSnapshot(master.getLocalBounds());
        int callbacks=0;
        master.onGainChanged=[&](float){++callbacks;};
        master.setColumnReadableScale(true);
        const auto column=master.createComponentSnapshot(master.getLocalBounds());
        check(master.isColumnReadableScale() && column.isValid() &&
              original.isValid() && callbacks==0 &&
              master.getGainDb()==0.0f,
              "N5.3 presentation switch touched transport/gain state");
        bool styledDifferently=false;
        if(original.isValid() && column.isValid())
            for(int y=0;y<original.getHeight()&&!styledDifferently;++y)
                for(int x=0;x<original.getWidth();++x)
                    if(original.getPixelAt(x,y)!=column.getPixelAt(x,y)){
                        styledDifferently=true;break;
                    }
        check(styledDifferently,
              "N5.3 native Audio strip readability mode is visually inert");
        master.setColumnReadableScale(false);
        const auto restored=master.createComponentSnapshot(master.getLocalBounds());
        bool unchanged=restored.isValid() && original.isValid();
        if(unchanged)
            for(int y=0;y<original.getHeight()&&unchanged;++y)
                for(int x=0;x<original.getWidth();++x)
                    if(original.getPixelAt(x,y)!=restored.getPixelAt(x,y)){
                        unchanged=false;break;
                    }
        check(unchanged && callbacks==0,
              "N5.3 switching back changed the original Master pixels");
    });
    test("N5.2 flat meter rails retain live stereo signal and readable fader cap",[] {
        using namespace cuelist;
        auto different=[](const juce::Image& a,const juce::Image& b) {
            if (!a.isValid() || !b.isValid() ||
                a.getWidth()!=b.getWidth() || a.getHeight()!=b.getHeight())
                return false;
            for(int y=0;y<a.getHeight();y+=2)
                for(int x=0;x<a.getWidth();x+=2)
                    if(a.getPixelAt(x,y)!=b.getPixelAt(x,y)) return true;
            return false;
        };
        GainMeterFader fader;
        fader.setSize(92,195);
        const auto idle=fader.createComponentSnapshot(fader.getLocalBounds());
        check(idle.isValid() && idle.getPixelAt(18,90)==palette::input,
              "Expanded L/R meter still has a raised decorative chassis");
        fader.setStereoPeaks(1.f,.1f);
        const auto live=fader.createComponentSnapshot(fader.getLocalBounds());
        check(different(idle,live) && live.getPixelAt(18,90)!=
              idle.getPixelAt(18,90) &&
              fader.getDisplayedPeakDb(0)>fader.getDisplayedPeakDb(1),
              "Live left/right peaks are invisible in the calibrated meter");
        const auto beforeModel=fader.getGainDb();
        fader.setGainDb(-12.f,false);
        check(different(live,fader.createComponentSnapshot(fader.getLocalBounds()))
              && fader.getGainDb()!=beforeModel,
              "Fader cap does not follow its actual dB gain");
        fader.setCompact(true);
        fader.setSize(34,195);
        for(int i=0;i<120;++i) fader.setStereoPeaks(0.f,0.f);
        const auto compact=fader.createComponentSnapshot(fader.getLocalBounds());
        check(compact.isValid() && compact.getWidth()==34,
              "Collapsed mixer cannot render its compact meter/fader");
        fader.setStereoPeaks(1.f,.02f);
        check(different(compact,fader.createComponentSnapshot(fader.getLocalBounds())),
              "Compact L/R meter lost its real audio telemetry");
    });
    test("N5.2 physical PAN shows centre calibration, actual deflection and shared style",[] {
        using namespace cuelist;
        juce::Slider pan;
        configureMixerPan(pan);
        pan.setSize(49,49);
        pan.setValue(0.0,juce::dontSendNotification);
        const auto center=pan.createComponentSnapshot(pan.getLocalBounds());
        pan.setValue(.78,juce::dontSendNotification);
        const auto right=pan.createComponentSnapshot(pan.getLocalBounds());
        bool changed=false;
        if(center.isValid() && right.isValid())
            for(int y=0;y<center.getHeight();y+=2)
                for(int x=0;x<center.getWidth();x+=2)
                    changed=changed ||
                        center.getPixelAt(x,y)!=right.getPixelAt(x,y);
        check(changed && pan.getSliderStyle()==juce::Slider::RotaryVerticalDrag &&
              &pan.getLookAndFeel()==&sharedMixerPanLookAndFeel(),
              "PAN centre/detent is unreadable or Master/Column style diverged");
        check(std::abs(pan.getValue()-.78)<.001 &&
              pan.getTextValueSuffix().isEmpty(),
              "Pan graphic unexpectedly modified the underlying control");
    });
    test("pre-master stereo telemetry measures actual summed columns after gain/pan", [] {
        auto show=makeShow();
        addRow(show,"Layer");
        auto source=std::make_shared<AudioClip>();
        source->sampleRate=4;
        source->samples.setSize(2,8);
        for (int i=0;i<8;++i) {
            source->samples.setSample(0,i,.20f);
            source->samples.setSample(1,i,.40f);
        }
        ClipMap clips;
        clips[show.rows[0].cells[0].id]=source;
        clips[show.rows[1].cells[0].id]=source;
        show.masterGain=.1f;
        show.columns[0].gain=.5f;
        show.columns[0].pan=0.5f; // side gains: left x0.5, right x1.0
        auto state=makeRenderState(show,clips);
        for (const auto& row:show.rows)
            state->trigger(row.cells[0].id,false);
        juce::AudioBuffer<float> output(2,1);
        state->render(output.getArrayOfWritePointers(),2,1,4);
        // two clips: left 2*.2*.5*.5=.1; right 2*.4*.5=.4
        near(state->columnMeters[0].second->left.load(),.1);
        near(state->columnMeters[0].second->right.load(),.4);
        near(state->columnMeters[0].second->peak.load(),.4);
        near(output.getSample(0,0),.01); // Master is post-meter
        near(output.getSample(1,0),.04);
        show.columns[0].muted=true;
        auto muted=makeRenderState(show,clips);
        muted->trigger(show.rows[0].cells[0].id,false);
        muted->render(output.getArrayOfWritePointers(),2,1,4);
        near(muted->columnMeters[0].second->left.load(),0);
        near(muted->columnMeters[0].second->right.load(),0);
    });
    test("simultaneous opposite polarity voices cancel in the actual column meter", [] {
        auto show=makeShow(); addRow(show,"Opposite");
        auto a=std::make_shared<AudioClip>(); a->sampleRate=4; a->samples.setSize(2,8);
        auto b=std::make_shared<AudioClip>(); b->sampleRate=4; b->samples.setSize(2,8);
        for (int i=0;i<8;++i) {
            a->samples.setSample(0,i,.6f); a->samples.setSample(1,i,.3f);
            b->samples.setSample(0,i,-.6f); b->samples.setSample(1,i,-.3f);
        }
        ClipMap clips; clips[show.rows[0].cells[0].id]=a; clips[show.rows[1].cells[0].id]=b;
        auto state=makeRenderState(show,clips);
        for (const auto& row:show.rows) state->trigger(row.cells[0].id,false);
        juce::AudioBuffer<float> audio(2,1);
        state->render(audio.getArrayOfWritePointers(),2,1,4);
        near(audio.getSample(0,0),0);
        near(audio.getSample(1,0),0);
        near(state->columnMeters[0].second->left.load(),0);
        near(state->columnMeters[0].second->right.load(),0);
    });
    test("AudioEngine publishes separate peak channels to UI without master affecting taps", [] {
        auto show=makeShow();
        auto source=std::make_shared<AudioClip>();
        source->sampleRate=4; source->samples.setSize(2,8);
        for (int i=0;i<8;++i) {
            source->samples.setSample(0,i,.8f);
            source->samples.setSample(1,i,.1f);
        }
        ClipMap clips; clips[show.rows[0].cells[0].id]=source;
        const int id=show.columns[0].id;
        show.masterGain=0;
        AudioEngine engine;
        engine.publish(makeRenderState(show,clips));
        check(engine.send(show.rows[0].cells[0].id,false),"Failed to enqueue test voice");
        juce::AudioBuffer<float> audio(2,1);
        engine.processBlock(audio.getArrayOfWritePointers(),2,1,4);
        auto peaks=engine.getColumnStereoPeak(id);
        near(peaks[0],.8);
        near(peaks[1],.1);
        near(engine.getColumnPeak(id),.8);
        near(audio.getSample(0,0),0);
        near(audio.getSample(1,0),0);
        auto missing=engine.getColumnStereoPeak(-12);
        near(missing[0],0); near(missing[1],0);
    });
}

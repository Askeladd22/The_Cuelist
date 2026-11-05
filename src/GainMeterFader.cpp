#include "GainMeterFader.h"

namespace cuelist {
void GainMeterFader::paint(juce::Graphics& g) {
    const float top=topY(),bottom=bottomY();
    const float length=bottom-top;
    if(compact) {
        const float mid=getWidth()*.5f;
        const auto rail=juce::Rectangle<float>(mid-10.0f,top,20.0f,length);
        // Rack-meter rails are precisely bounded instrument surfaces, not
        // nested rounded cards. No change to the calibrated dB mapping.
        g.setColour(palette::input);g.fillRect(rail);
        g.setColour(palette::line.withAlpha(.65f));g.drawRect(rail,.75f);
        for(int ch=0;ch<2;++ch) {
            const float value=peaks[(size_t)ch].current;
            if(value>minDb+.01f) {
                const float y=yForDb(value,top,bottom,ceilingDb);
                g.setColour(value>=0 ? palette::red : value>=-9 ? palette::standby : palette::playing);
                g.fillRect(mid-7.0f+ch*8.0f,y,6.0f,bottom-y);
            }
        }
        const float y=yForDb(gainDb,top,bottom,ceilingDb);
        // Slim horizontal cap with one calibrated centre notch.
        g.setColour(palette::background);
        g.fillRect(mid-15.0f,y-5.0f,30.0f,10.0f);
        g.setColour(palette::muted);
        g.fillRect(mid-13.0f,y-3.0f,26.0f,6.0f);
        g.setColour(palette::background);
        g.fillRect(mid-9.0f,y-.5f,18.0f,1.0f);
        if(hasKeyboardFocus(false)) {
            g.setColour(palette::teal);
            g.drawRect(juce::Rectangle<float>(mid-15.f,y-5.f,30.f,10.f),1.f);
        }
        g.setColour(palette::muted);
        const auto silenceText=juce::String::charToString((juce::juce_wchar)0x2212)
                              +juce::String::charToString((juce::juce_wchar)0x221e);
        // Math symbols always use the bundled Noto Sans Math directly.
        g.setFont(gainDb==silenceDb ? mathFont(10.0f) : font(10.0f,true));
        g.drawText(gainDb==silenceDb ? silenceText : juce::String((int)std::round(gainDb)),
                   0,getHeight()-18,getWidth(),17,juce::Justification::centred,false);
        return;
    }
    const juce::Rectangle<float> backdrop(12.0f,top,28.0f,length);
    // Flat, twin-channel dBFS rails. Peak hold and operating levels remain
    // live; only the non-informational rounded container is removed.
    g.setColour(palette::input); g.fillRect(backdrop);
    g.setColour(palette::line.withAlpha(.65f)); g.drawRect(backdrop,.75f);
    g.setColour(palette::line.withAlpha(.4f));
    g.fillRect(26.0f,top+1.0f,1.0f,std::max(0.f,length-2.f));

    // Gain and both dBFS readings use exactly the same vertical coordinate
    // transform; labels describe different quantities rather than gain=signal.
    const float xs[2]={15.0f,29.0f};
    for (int ch=0;ch<2;++ch) {
        const auto db=peaks[(size_t)ch].current;
        if (db>minDb+0.01f) {
            const float y=yForDb(db,top,bottom,ceilingDb);
            const auto fill=juce::Rectangle<float>(xs[ch],y,10.0f,bottom-y);
            g.setColour(db>=0 ? palette::red.withAlpha(.80f)
                : db>=-9 ? palette::standby.withAlpha(.85f) : palette::playing.withAlpha(.85f));
            g.fillRect(fill);
        }
        if (peaks[(size_t)ch].held>minDb+0.01f) {
            const float mark=yForDb(peaks[(size_t)ch].held,top,bottom,ceilingDb);
            g.setColour(palette::text);
            g.fillRect(xs[ch],mark,10.0f,1.8f);
        }
    }

    constexpr int ticks[]={6,0,-6,-12,-18,-24,-36,-48,-60};
    float previousLabel=-10000.0f;
    for (const int tick:ticks) {
        if (tick>ceilingDb) continue;
        const float y=yForDb((float)tick,top,bottom,ceilingDb);
        const bool major=tick==(int)ceilingDb || tick==0 || tick==-12 || tick==-24
            || tick==-36 || tick==-48 || tick==-60;
        // Unity gain is a readable reference, not another signal status
        // colour. Reserve teal/amber/red for real meter states and focus.
        g.setColour(tick==0 ? palette::text.withAlpha(.85f)
                            : palette::muted.withAlpha(.66f));
        g.drawHorizontalLine((int)std::round(y),42.0f,major ? 50.0f : 46.0f);
        // Retain all tick marks but skip conflicting labels in short strips.
        if (y-previousLabel>=13.0f || previousLabel<0) {
            g.setFont(tick==-60 ? mathFont(10.0f) : font(10.0f));
            const auto minusInfinity=juce::String::charToString((juce::juce_wchar)0x2212)
                                    +juce::String::charToString((juce::juce_wchar)0x221e);
            g.drawText(tick==-60 ? minusInfinity :
                       tick>0 ? "+"+juce::String(tick) : juce::String(tick),
                       52,(int)std::round(y-7),27,14,juce::Justification::centredLeft,false);
            previousLabel=y;
        }
    }

    // A single, visible gain thumb overlaps both L/R bars. No child slider.
    const float thumbY=yForDb(gainDb,top,bottom,ceilingDb);
    const juce::Rectangle<float> thumb(8.0f,thumbY-5.0f,37.0f,10.0f);
    // Mechanically legible fader cap: low-profile metal, dark centre groove.
    g.setColour(palette::background); g.fillRect(thumb.expanded(1.f));
    g.setColour(palette::muted); g.fillRect(thumb);
    g.setColour(palette::background); g.fillRect(14.0f,thumbY-.65f,25.0f,1.3f);
    if (hasKeyboardFocus(false)) {
        g.setColour(palette::teal);
        g.drawRect(thumb.expanded(2.f),1.f);
    }

    g.setColour(palette::muted);
    g.setFont(font(11.0f));
    g.drawText("L",14,(int)bottom+3,12,14,juce::Justification::centred,false);
    g.drawText("R",29,(int)bottom+3,12,14,juce::Justification::centred,false);
    g.setColour(palette::text);
    const int valueY=getHeight()-22;
    if (gainDb==silenceDb) {
        // Draw only the mathematical part in Noto Sans Math. Exo 2 still
        // renders the unit, so instrument typography stays consistent.
        const auto value=juce::String::charToString((juce::juce_wchar)0x2212)
                         +juce::String::charToString((juce::juce_wchar)0x221e);
        const auto math=mathFont(12.0f);
        const int symbolWidth=(int)std::ceil(juce::GlyphArrangement::getStringWidth(math,value));
        g.setFont(math);
        g.drawText(value,4,valueY,symbolWidth+2,20,juce::Justification::centredLeft,false);
        g.setFont(font(12.0f,true));
        g.drawText(" dB",4+symbolWidth,valueY,std::max(25,getWidth()-8-symbolWidth),
                   20,juce::Justification::centredLeft,false);
    } else {
        g.setFont(font(12.0f,true));
        g.drawText(juce::String(gainDb,1)+" dB",4,valueY,
                   std::max(70,getWidth()-8),20,juce::Justification::centredLeft,false);
    }
}
void GainMeterFader::mouseDown(const juce::MouseEvent& e) {
    grabKeyboardFocus();
    setFromMouse(e.position.y);
}
void GainMeterFader::mouseDrag(const juce::MouseEvent& e) { setFromMouse(e.position.y); }
void GainMeterFader::mouseWheelMove(const juce::MouseEvent& e,
                                    const juce::MouseWheelDetails& wheel) {
    const float delta=wheel.isReversed ? -wheel.deltaY : wheel.deltaY;
    if (!std::isfinite(delta) || delta==0.0f) return;
    // JUCE reports mouse-wheel notches as either ~0.1 or ~1.0 depending
    // on device/platform. Trackpads deliver smaller fractions; accumulate
    // those without amplifying a single physical notch tenfold.
    wheelAccumulator+=std::abs(delta)>=0.5f ? std::copysign(0.1f,delta) : delta;
    const int ticks=(int)std::trunc(wheelAccumulator/0.099f);
    if (ticks==0) return;
    wheelAccumulator-=ticks*0.1f;
    setGainDb(wheelTargetDb(gainDb,ticks,e.mods.isShiftDown(),ceilingDb),true);
}
bool GainMeterFader::keyPressed(const juce::KeyPress& key) {
    const int k=key.getKeyCode();
    if (k==juce::KeyPress::upKey || k==juce::KeyPress::rightKey) {
        setGainDb((gainDb==silenceDb ? minDb : gainDb)+(key.getModifiers().isShiftDown() ? 1.0f : 0.1f),true); return true;
    }
    if (k==juce::KeyPress::downKey || k==juce::KeyPress::leftKey) {
        setGainDb(gainDb-(key.getModifiers().isShiftDown() ? 1.0f : 0.1f),true); return true;
    }
    if (k==juce::KeyPress::homeKey) { setGainDb(0,true); return true; }
    return false;
}
}

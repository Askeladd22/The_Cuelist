#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <cmath>

namespace cuelist {
// One shared geometry contract for telemetry-only painting,
// responsive operator menus and the Cuelist tab overflow.
// The entire console remains within the original 72px global header.
struct HeaderTelemetryLayout final {
    // Canonical operator header geometry shared with native tests.
    static constexpr int top=7;
    static constexpr int panelHeight=58;
    static constexpr int groupGap=10;
    juce::Rectangle<int> clock,usage,activity,audio;
    juce::Rectangle<int> timeText,meridiemText,cpuLabel,cpuValue,ramLabel,ramValue;
    juce::Rectangle<float> cpuMeter,ramMeter;
    juce::Rectangle<int> activityHeading,audioHeading,audioStatus;
    juce::Rectangle<int> midiLearn,oscLearn;
    std::array<juce::Rectangle<int>,4> activityLabels{};
    std::array<juce::Rectangle<int>,4> activityDots{};

    static HeaderTelemetryLayout atWidth(int windowWidth) {
        // Grow horizontally, not vertically. Three profiles keep at least
        // File + More accessible down to the existing 1024px test width.
        const bool wide=windowWidth>=1550;
        const bool medium=windowWidth>=1250 && !wide;
        const int cw=wide?168:medium?154:134;
        const int uw=wide?156:medium?144:130;
        const int iw=wide?258:medium?238:218;
        const int aw=wide?184:medium?164:150;
        const int x=windowWidth-10-(cw+uw+iw+aw+groupGap*3);
        HeaderTelemetryLayout t;
        t.clock={x,top,cw,panelHeight};
        t.usage={t.clock.getRight()+groupGap,top,uw,panelHeight};
        t.activity={t.usage.getRight()+groupGap,top,iw,panelHeight};
        t.audio={t.activity.getRight()+groupGap,top,aw,panelHeight};

        t.timeText={x+4,10,cw-8,38};
        t.meridiemText={x+cw-49,49,45,13};
        t.cpuLabel={t.usage.getX()+9,16,35,19};
        t.cpuValue={t.usage.getX()+44,16,45,19};
        t.ramLabel={t.usage.getX()+9,39,35,19};
        t.ramValue={t.usage.getX()+44,39,45,19};
        // Meters grow with their section. Even the compact 130px profile
        // preserves clearance after the numeric values.
        const int meterWidth=wide?48:medium?40:30;
        const float meterX=(float)(t.usage.getRight()-10-meterWidth);
        t.cpuMeter={meterX,22.f,(float)meterWidth,7.f};
        t.ramMeter={meterX,45.f,(float)meterWidth,7.f};

        // A narrow, dedicated action rail on the right stacks both original
        // Learn buttons. The RX/TX lamps remain in two readable columns on
        // the left, without any overlapping hitboxes at 1024px or ultrawide.
        t.activityHeading={t.activity.getX()+9,10,iw-84,18};
        t.midiLearn={t.activity.getRight()-66,8,60,21};
        t.oscLearn ={t.activity.getRight()-66,36,60,21};
        const int leftX=t.activity.getX()+8;
        const int lampColumnWidth=(iw-76)/2;
        const int rightX=leftX+lampColumnWidth;
        const int labelWidth=lampColumnWidth-17;
        t.activityDots={juce::Rectangle<int>{leftX,33,10,10},
                        juce::Rectangle<int>{rightX,33,10,10},
                        juce::Rectangle<int>{leftX,50,10,10},
                        juce::Rectangle<int>{rightX,50,10,10}};
        t.activityLabels={juce::Rectangle<int>{leftX+14,29,labelWidth,17},
                          juce::Rectangle<int>{rightX+14,29,labelWidth,17},
                          juce::Rectangle<int>{leftX+14,47,labelWidth,17},
                          juce::Rectangle<int>{rightX+14,47,labelWidth,17}};
        t.audioHeading={t.audio.getX()+12,11,aw-22,20};
        t.audioStatus={t.audio.getX()+30,31,aw-40,26};
        return t;
    }

    static juce::String percentage(float value,bool cpu) {
        if (!std::isfinite(value) || value<0) return "N/A";
        return juce::String(value,cpu ? 1 : 0)+"%";
    }
};
} // namespace cuelist

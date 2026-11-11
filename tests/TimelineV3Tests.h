#pragma once
#include "PianoRoll.h"

void timelineV3Tests() {
    using namespace cueViewFixtures;
    test("Timeline uses neutral toolbar and visible type-keyed media fills",[] {
        MainComponent ui(false);
        addColumn(ui.show,ColumnType::control,"Stage control");
        auto audio=std::make_shared<AudioClip>();
        audio->samples.setSize(1,48000);
        audio->samples.clear();
        ui.clips[ui.show.rows[0].cells[0].id]=audio;
        auto* timeline=cueViewFixtures::find<Timeline>(ui);
        check(timeline!=nullptr,"Timeline missing for N6 visual review");
        timeline->setSize(800,250);
        const auto initial=encodeShow(ui.show);
        const auto preview=timeline->createComponentSnapshot(
            timeline->getLocalBounds());
        check(preview.isValid() && preview.getWidth()==800 &&
              preview.getHeight()==250,
              "N6 Timeline did not render at standard operator height");
        // No decorative teal tab at the leading edge of the 45px toolbar.
        check(preview.getPixelAt(10,21)==preview.getPixelAt(14,21),
              "Timeline still has the old ornamental teal header stripe");
        // First row: loaded Audio clip, 1s on a 10s ruler (x=140).
        // Industrial Orange has a type-tinted body; the dedicated 3px type
        // key is still distinct. The selected column keeps its type fill.
        const auto audioFill=palette::input.interpolatedWith(
            palette::type((int)ColumnType::audio),.46f);
        check(preview.getPixelAt(141,130)!=preview.getPixelAt(152,130) &&
              preview.getPixelAt(152,130)==audioFill &&
              audioFill!=palette::input,
              "Loaded Audio clip lost its visible type-coloured body or type key");
        // Legacy must preserve the previous genuinely neutral body.
        ui.setAppearanceTheme(palette::Appearance::legacy);
        const auto previous=timeline->createComponentSnapshot(
            timeline->getLocalBounds());
        check(previous.getPixelAt(152,130)==palette::input,
              "Legacy Timeline no longer renders its historical neutral clip body");
        ui.setAppearanceTheme(palette::Appearance::industrialOrange);
        // Next row: Control configuration should no longer be one big
        // translucent pastel card, but remain an identifiable Control lane.
        check(preview.getPixelAt(141,174)!=preview.getPixelAt(400,174) &&
              preview.getPixelAt(400,174)==palette::input,
              "Control lane lost its technical surface or signal key");
        // Resolve children using the Timeline's own public component API.
        // This test must not rely on helpers defined in ControlsV3Tests.h,
        // which is deliberately included later by Tests.cpp.
        juce::Component* snap=nullptr;
        juce::Component* grid=nullptr;
        for (auto* child:timeline->getChildren()) {
            if (child->getName()=="timelineSnapToggle") snap=child;
            else if (child->getName()=="timelineGridVisibility") grid=child;
        }
        check(snap && grid && snap->isVisible() && grid->isVisible(),
              "Simplified Timeline removed real magnetic Snap/Grid controls");
        timeline->setSize(800,115);
        const auto compact=timeline->createComponentSnapshot(
            timeline->getLocalBounds());
        check(compact.isValid() && !snap->isVisible() && !grid->isVisible(),
              "Compact Timeline exposes controls over the ruler/tracks");
        check(encodeShow(ui.show)==initial,
              "A Timeline-only visual update mutated persisted show data");
    });
    test("timeline zoom controls pan and fit keep ruler coordinates consistent", [] {
        MainComponent ui(false);
        auto* timeline=find<Timeline>(ui); check(timeline!=nullptr,"Timeline absent"); timeline->setSize(800,250);
        auto* scroll=find<juce::ScrollBar>(*timeline); check(scroll!=nullptr,"Timeline scrollbar absent");
        click(*timeline,708,22);
        near(scroll->getCurrentRangeSize(),5); near(scroll->getCurrentRangeStart(),2.5);
        const auto seek=[&](float x) {
            click(*timeline,x,60);
            float samples[16]{}; float* outputs[]{samples}; ui.engine.processBlock(outputs,1,16,48000);
            return ui.engine.playheadSeconds.load();
        };
        near(seek(300),3.75);
        scroll->setCurrentRangeStart(5,juce::sendNotificationSync);
        near(seek(300),6.25);
        click(*timeline,674,22); near(scroll->getCurrentRangeSize(),10); near(scroll->getCurrentRangeStart(),0);
        click(*timeline,708,22); click(*timeline,754,22);
        near(scroll->getCurrentRangeSize(),10); near(seek(460),5);
    });
    test("timeline zoomed clip drag uses frozen scale through resize and fit includes offsets", [] {
        MainComponent ui(false);
        auto clip=std::make_shared<AudioClip>(); clip->samples.setSize(1,48000); clip->samples.clear();
        ui.clips[ui.show.rows[0].cells[0].id]=clip;
        ui.setClipStart(0,3);
        auto* timeline=find<Timeline>(ui); timeline->setSize(800,250);
        click(*timeline,708,22);
        const juce::Point<float> from{236,118}, to{300,118};
        timeline->mouseDown(event(*timeline,from,from)); timeline->setSize(1000,250);
        timeline->mouseDrag(event(*timeline,to,from,1,true)); timeline->mouseUp(event(*timeline,to,from,1,true));
        near(ui.show.rows[0].cells[0].startSeconds,3.5);
        ui.setClipStart(0,20); click(*timeline,954,22);
        auto* scroll=find<juce::ScrollBar>(*timeline);
        near(scroll->getCurrentRangeSize(),23.1); near(scroll->getCurrentRangeStart(),0);
    });
    test("timeline magnet snaps audio clips by musical tempo and can be disabled",[] {
        MainComponent ui(false);
        auto clip=std::make_shared<AudioClip>(); clip->samples.setSize(1,48000); clip->samples.clear();
        ui.clips[ui.show.rows[0].cells[0].id]=clip;
        auto* timeline=find<Timeline>(ui); timeline->setSize(800,250);
        const juce::Point<float> from{160,118},to{195,118};
        timeline->mouseDown(event(*timeline,from,from));
        timeline->mouseDrag(event(*timeline,to,from,1,true));
        timeline->mouseUp(event(*timeline,to,from,1,true));
        near(ui.show.rows[0].cells[0].startSeconds,.5);
        for (auto* component:timeline->getChildren())
            if (component->getName()=="timelineSnapToggle") dynamic_cast<juce::TextButton*>(component)->onClick();
        ui.setClipStart(0,0);
        timeline->mouseDown(event(*timeline,from,from));
        timeline->mouseDrag(event(*timeline,to,from,1,true));
        timeline->mouseUp(event(*timeline,to,from,1,true));
        near(ui.show.rows[0].cells[0].startSeconds,.55); // model rounds offsets to 0.01 s
    });
    test("timeline grid visibility and musical division are independent of magnet",[] {
        MainComponent ui(false);
        auto* timeline=find<Timeline>(ui); timeline->setSize(800,250);
        juce::TextButton *visibility=nullptr,*magnet=nullptr;
        juce::ComboBox *gridResolution=nullptr,*snapResolution=nullptr;
        for (auto* component : timeline->getChildren()) {
            if (component->getName()=="timelineGridVisibility") visibility=dynamic_cast<juce::TextButton*>(component);
            if (component->getName()=="timelineGridDivision") gridResolution=dynamic_cast<juce::ComboBox*>(component);
            if (component->getName()=="timelineSnapToggle") magnet=dynamic_cast<juce::TextButton*>(component);
            if (component->getName()=="timelineSnapDivision") snapResolution=dynamic_cast<juce::ComboBox*>(component);
        }
        check(visibility && magnet && gridResolution && snapResolution,"Grid or snap controls absent");
        check(visibility->getToggleState() && magnet->getToggleState(),"Initial controls incorrect");
        gridResolution->setSelectedId(1,juce::sendNotificationSync); // quarter notes
        const auto pixelAt=[&](int x) {
            auto screenshot=timeline->createComponentSnapshot(timeline->getLocalBounds());
            return screenshot.getPixelAt(x,190).getARGB();
        };
        const auto withGrid=pixelAt(172); // beat 1 at 0.5 s, 120 BPM and 640px/10s
        visibility->onClick();
        check(!visibility->getToggleState() &&
              visibility->getTooltip().contains("Grid OFF"),
              "Grid icon did not reflect the hidden state");
        check(pixelAt(172)!=withGrid,"Grid still drawn when hidden");
        check(magnet->getToggleState(),"Hiding grid disabled magnet");
        visibility->onClick();
        const auto withoutHalfBeat=pixelAt(156);
        gridResolution->setSelectedId(2,juce::sendNotificationSync); // eighth notes
        check(pixelAt(156)!=withoutHalfBeat,"Grid resolution does not affect rendered lines");
        check(snapResolution->getSelectedId()==3,"Changing grid resolution changed snap");
        magnet->onClick();
        check(!magnet->getToggleState() && visibility->getToggleState(),
              "Turning magnet off also hid grid");
    });
    test("timeline snap division works with hidden grid and independent tempo map",[] {
        MainComponent ui(false);
        auto clip=std::make_shared<AudioClip>(); clip->samples.setSize(1,48000); clip->samples.clear();
        ui.clips[ui.show.rows[0].cells[0].id]=clip;
        auto* timeline=find<Timeline>(ui); timeline->setSize(800,250);
        juce::TextButton *visibility=nullptr,*magnet=nullptr;
        juce::ComboBox* division=nullptr;
        for (auto* component : timeline->getChildren()) {
            if (component->getName()=="timelineGridVisibility") visibility=dynamic_cast<juce::TextButton*>(component);
            if (component->getName()=="timelineSnapToggle") magnet=dynamic_cast<juce::TextButton*>(component);
            if (component->getName()=="timelineSnapDivision") division=dynamic_cast<juce::ComboBox*>(component);
        }
        check(visibility && division && magnet,"Snap controls absent");
        visibility->onClick(); // grid OFF, snap ON
        division->setSelectedId(1,juce::sendNotificationSync); // 1/4, 0.5 s at 120 BPM
        const juce::Point<float> from{160,118},to{198,118};
        const auto drag=[&] {
            timeline->mouseDown(event(*timeline,from,from));
            timeline->mouseDrag(event(*timeline,to,from,1,true));
            timeline->mouseUp(event(*timeline,to,from,1,true));
            return ui.show.rows[0].cells[0].startSeconds;
        };
        near(drag(),.5);
        ui.setClipStart(0,0);
        division->setSelectedId(3,juce::sendNotificationSync); // 1/16, 0.125 s at 120 BPM
        near(drag(),.63); // stored to centisecond
        ui.setClipStart(0,0);
        magnet->onClick();
        near(drag(),.59); // free movement independent of hidden grid
        ui.setClipStart(0,0);
        magnet->onClick();
        ui.show.rows[0].tempo={{0,120,4,4},{1,60,4,4}};
        division->setSelectedId(1,juce::sendNotificationSync);
        // .594 s falls in 60 BPM section (after .5 s); quantizes to beat 1 = 0.5 s
        near(drag(),.5);
    });
    test("timeline BPM grid follows tempo changes and is not just a pixel grid",[] {
        MainComponent ui(false);
        auto* timeline=find<Timeline>(ui); timeline->setSize(800,250);
        juce::ComboBox* grid=nullptr;
        for (auto* component:timeline->getChildren())
            if (component->getName()=="timelineGridDivision") grid=dynamic_cast<juce::ComboBox*>(component);
        check(grid!=nullptr,"Grid division control missing");
        grid->setSelectedId(1,juce::sendNotificationSync);
        const auto pixel=[&](int x) { return timeline->createComponentSnapshot(timeline->getLocalBounds()).getPixelAt(x,190).getARGB(); };
        // At 120 BPM, 3.5 s is beat 7 (a visible musical line).
        // After beat 4 switches to 60 BPM, 3.5 s lies between beats 5 and 6.
        const auto before=pixel(364);
        ui.show.rows[0].tempo={{0,120,4,4},{4,60,4,4}};
        check(pixel(364)!=before,"BPM change did not move musical lines away from 3.5 seconds");
    });
    test("timeline MIDI single click opens shared docked clip editor",[] {
        MainComponent ui(false); addColumn(ui.show,ColumnType::midi,{});
        ui.show.rows[0].cells[1].midi=MidiClip{4,{{0,1,60,100,1}}};
        auto* timeline=find<Timeline>(ui); timeline->setSize(800,250);
        const juce::Point<float> p{160,160};
        timeline->mouseDown(event(*timeline,p,p));
        check(cueViewFixtures::find<PianoRollEditor>(ui)!=nullptr,"Timeline click did not dock editor");
        check(ui.activeClipEditor()!=nullptr && !ui.isClipEditorFloating(),"Timeline editor not integrated");
        ui.closeMidiClip();
    });
    test("timeline Ctrl wheel anchors time and changing row cancels a seek", [] {
        MainComponent ui(false); addRow(ui.show,{});
        auto* timeline=find<Timeline>(ui); timeline->setSize(800,250);
        const auto time=juce::Time::getCurrentTime();
        const juce::MouseEvent e{juce::Desktop::getInstance().getMainMouseSource(),{300,60},
            juce::ModifierKeys(juce::ModifierKeys::ctrlModifier),1,0,0,0,0,
            timeline,timeline,time,{300,60},time,1,false};
        juce::MouseWheelDetails wheel{}; wheel.deltaY=1.0f/3;
        timeline->mouseWheelMove(e,wheel);
        auto* scroll=find<juce::ScrollBar>(*timeline);
        check(std::abs(scroll->getCurrentRangeSize()-5)<.001,"Ctrl wheel did not zoom");
        check(std::abs(scroll->getCurrentRangeStart()-1.25)<.001,"Ctrl wheel anchor moved");
        timeline->mouseDown(event(*timeline,{300,60},{300,60}));
        ui.select(1,0);
        timeline->mouseDrag(event(*timeline,{460,60},{300,60},1,true));
        timeline->mouseUp(event(*timeline,{460,60},{300,60},1,true));
        near(scroll->getCurrentRangeSize(),10); near(scroll->getCurrentRangeStart(),0);
    });
}

#pragma once
#include "ControlDispatchPolicy.h"
#include <cmath>

void controlDispatchPolicyTests() {
    test("physical control edges use device/channel/number rather than mapping identity",[] {
        PhysicalControlEdges edges;
        auto press=ControlEvent{"pedal",ControlKind::note,1,64,127,false};
        auto release=ControlEvent{"pedal",ControlKind::note,1,64,0,true};
        check(!edges.recordAndWasDown(press),"First GO note must be a rising edge");
        check(edges.recordAndWasDown(press),"Held GO must not generate a second edge");
        check(edges.recordAndWasDown(release),"Release must observe previous held state");
        check(!edges.recordAndWasDown(press),"New GO press after release not detected");
        auto otherDevice=press; otherDevice.deviceId="another pedal";
        auto otherChannel=press; otherChannel.channel=2;
        auto otherNumber=press; otherNumber.number=65;
        auto cc=press;cc.kind=ControlKind::cc;
        check(!edges.recordAndWasDown(otherDevice) &&
              !edges.recordAndWasDown(otherChannel) &&
              !edges.recordAndWasDown(otherNumber) &&
              !edges.recordAndWasDown(cc),
              "Independent physical inputs shared a debounce state");
        edges.clear();
        check(!edges.recordAndWasDown(press),"Disconnected device kept stale held state");
    });

    test("unmatched releases clear hardware state across cue and binding changes",[] {
        PhysicalControlEdges edges;
        const auto press=ControlEvent{"control surface",ControlKind::note,4,90,127,false};
        const auto release=ControlEvent{"control surface",ControlKind::note,4,90,0,true};
        check(!edges.recordAndWasDown(press),"Initial GO input was already down");
        // The current cue has switched mapping sets. No binding matches
        // this release, but it must still be recorded.
        check(edges.recordAndWasDown(release),"Unmatched release did not see held input");
        check(!edges.recordAndWasDown(press),
              "A re-armed cue failed to accept the next legitimate GO edge");
        auto cc=ControlEvent{"fader",ControlKind::cc,2,7,80,false};
        check(!edges.recordAndWasDown(cc) && edges.recordAndWasDown(cc),
              "Above-midpoint CC threshold did not latch");
        cc.value=63;
        check(edges.recordAndWasDown(cc),"Crossing below CC press threshold lost held state");
        cc.value=127;
        check(!edges.recordAndWasDown(cc),"Rearmed CC did not generate an edge");
    });

    test("OSC address flood cannot re-arm GO or exhaust physical edge identities",[] {
        PhysicalControlEdges edges;
        const ControlEvent midiGo{"footswitch",ControlKind::note,1,64,127,false};
        const ControlEvent oscGo{"/show/go",ControlKind::osc,1,0,127,false};
        check(!edges.recordAndWasDown(midiGo),"First MIDI GO press was already held");
        check(!edges.recordAndWasDown(oscGo),"First OSC GO press was already held");

        // Exercises the actual dispatch policy on the UI thread, not UDP timing.
        // The listener bounds packet size, but previously not unique address state.
        int newlyAccepted=0;
        for (int n=0;n<100000;++n) {
            const ControlEvent unexpected{"/noise/"+juce::String(n),
                ControlKind::osc,1,0,127,false};
            if (!edges.recordAndWasDown(unexpected)) ++newlyAccepted;
        }
        check(newlyAccepted<=4095,
            "Unique OSC presses bypassed bounded physical-edge tracking");
        check(edges.oscTrackingSaturated(),
            "Capacity exhaustion was not latched for a safe manual re-arm");
        check(edges.recordAndWasDown(midiGo),
            "OSC flood lost the held MIDI GO debounce state");
        check(edges.recordAndWasDown(oscGo),
            "OSC flood re-armed an already-held mapped GO");

        const ControlEvent unknown{"/new/command",ControlKind::osc,1,0,127,false};
        check(edges.recordAndWasDown(unknown),
            "A new OSC transport press was accepted while tracking was saturated");
        auto release=oscGo; release.value=0; release.isNoteOff=true;
        check(edges.recordAndWasDown(release),"Held OSC GO release was forgotten");
        // A rejected still-held source must NOT become a fresh GO just because
        // some different tracked OSC source released a slot.
        check(edges.recordAndWasDown(unknown),
            "Rejected held OSC became a rising edge when capacity became available");
        auto ignoredRelease=unknown; ignoredRelease.value=0; ignoredRelease.isNoteOff=true;
        check(!edges.recordAndWasDown(ignoredRelease),
            "Untracked OSC release invented a held button or a new state slot");
        check(edges.recordAndWasDown(unknown),
            "An untracked OSC source bypassed the sticky overload latch");

        edges.clearMidi(); // MIDI reconnect must not re-arm held network GO.
        check(edges.oscTrackingSaturated() && edges.recordAndWasDown(unknown),
            "MIDI-only reset incorrectly re-armed blocked OSC GO");
        check(!edges.recordAndWasDown(midiGo),
            "MIDI-only reset left a stale held MIDI button");
        edges.resetOsc(); // Explicit successful Apply in OSC Settings.
        check(!edges.oscTrackingSaturated() && !edges.recordAndWasDown(unknown),
            "Operator re-arm did not restore a fresh OSC rising edge");
        check(edges.recordAndWasDown(unknown),
            "Repeated OSC press after re-arm generated another rising edge");
        edges.clear();
        check(!edges.recordAndWasDown(oscGo) && !edges.recordAndWasDown(midiGo),
            "Explicit full input reset did not clear both held-state sets");
    });

    test("pickup current-target normalization honors curve/invert and stable ID",[] {
        auto show=makeShow();
        addColumn(show,ColumnType::audio,"Other audio");
        const int lastAudio=show.columns[1].id;
        show.masterGain=0.6f;
        show.masterPan=-0.5f;
        show.columns[1].gain=0.8f;
        show.columns[1].pan=-0.5f;
        ControlBinding binding;
        binding.minimum=0.2f; binding.maximum=1.0f;
        binding.target=ControlTarget::masterGain;
        auto val=mappedPickupTarget(show,binding);
        check(val && std::abs(*val-0.5f)<0.00001f,
              "Master gain pickup normalization is wrong");
        binding.invert=true;
        binding.curve=ControlCurve::easeIn;
        val=mappedPickupTarget(show,binding);
        check(val && std::abs(*val-(1.0f-std::sqrt(0.5f)))<0.00001f,
              "Response curve/inversion was not reversed before pickup");
        binding.invert=false;
        binding.curve=ControlCurve::linear;
        binding.target=ControlTarget::columnGain;
        binding.targetColumnId=lastAudio;
        val=mappedPickupTarget(show,binding);
        check(val && std::abs(*val-0.75f)<0.00001f,
              "Column pickup did not use the stable destination ID");
        binding.target=ControlTarget::columnPan;
        binding.minimum=-1;binding.maximum=1;
        val=mappedPickupTarget(show,binding);
        check(val && std::abs(*val-0.25f)<0.00001f,
              "Column pan pickup normalization is incorrect");
        binding.target=ControlTarget::masterPan;
        val=mappedPickupTarget(show,binding);
        check(val && std::abs(*val-0.25f)<0.00001f,
              "Master pan pickup normalization is incorrect");
        binding.target=ControlTarget::columnGain;
        binding.targetColumnId=999999;
        check(!mappedPickupTarget(show,binding),
              "Missing column produced an invented pickup target");
        binding.target=ControlTarget::go;
        check(!mappedPickupTarget(show,binding),
              "GO must not require a slider pickup value");
        binding.target=ControlTarget::masterGain;
        binding.minimum=1;binding.maximum=1;
        check(!mappedPickupTarget(show,binding),
              "Zero-span pickup incorrectly produced a normalization");
    });

    test("routing selectors reject wrong types and preserve destinations after column reorder",[] {
        auto show=makeShow();
        addColumn(show,ColumnType::midi,"MIDI output");
        addColumn(show,ColumnType::osc,"OSC output");
        addColumn(show,ColumnType::control,"Controller");
        const int midiId=show.columns[1].id;
        const int oscId=show.columns[2].id;
        const int audioId=show.columns[0].id;
        check(mappedTargetColumn(show,midiId,ColumnType::midi)==1 &&
              mappedTargetColumn(show,oscId,ColumnType::osc)==2 &&
              mappedTargetColumn(show,audioId,ColumnType::audio)==0,
              "Audio/MIDI/OSC mapping did not find correct targets");
        check(mappedTargetColumn(show,midiId,ColumnType::osc)==-1 &&
              mappedTargetColumn(show,oscId,ColumnType::audio)==-1 &&
              mappedTargetColumn(show,999999,ColumnType::midi)==-1,
              "Dispatch policy accepted wrong or deleted destination");
        check(swapCueColumns(show,1,2).wasOk(),"Could not reorder MIDI and OSC outputs");
        check(mappedTargetColumn(show,midiId,ColumnType::midi)==2 &&
              mappedTargetColumn(show,oscId,ColumnType::osc)==1,
              "Mapping followed a visual index instead of the stable column ID");
    });

    test("mapped gain deadband and MIDI CC quantization match the original dispatch",[] {
        check(!shouldApplyMappedValue(0.5f,0.503f) &&
              shouldApplyMappedValue(0.5f,0.506f),
              "Mapped fader deadband changed");
        check(mappedMidiCcValue(-20)==0 &&
              mappedMidiCcValue(0)==0 &&
              mappedMidiCcValue(63.49f)==63 &&
              mappedMidiCcValue(63.51f)==64 &&
              mappedMidiCcValue(127)==127 &&
              mappedMidiCcValue(500)==127,
              "Mapped MIDI CC rounding or clamping changed");
    });
}

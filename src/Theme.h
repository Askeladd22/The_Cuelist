#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "BinaryData.h"
#include <cmath>
#include <cstdint>
#include <algorithm>
namespace cuelist {
namespace palette {
// One mutable token table, two immutable source presets. The Legacy numbers
// below are copied from the last pre-Industrial Orange Theme.h (v0.9.15,
// commit e6f1c384). They are NOT approximate reconstructions.
enum class Appearance { legacy, industrialOrange };
struct ThemeTokens {
    uint32_t bgMain,bgPanel,bgElevated,bgHover,bgSelected,borderDefault;
    uint32_t accentPrimary,accentHover,accentPressed;
    uint32_t typeAudio,typeMidi,typeOsc,typeControl;
    uint32_t statusGo,statusPanic,statusWarning,statusInfo;
    uint32_t textPrimary,textSecondary,textMuted,textDisabled;
};
inline constexpr ThemeTokens legacyTokens{
    0xff121416,0xff24272a,0xff2b2f32,0xff2b2f32,0xff34393c,0xff474b4f,
    0xffcf8b62,0xffe4a77e,0xff5c3b2d,
    0xffe4a77e,0xffdebd88,0xffb7a1bf,0xff8ea7b8,
    0xff8cbd9f,0xffab5146,0xffdebd88,0xff8ea7b8,
    0xfff2ede5,0xffc2b8aa,0xffc2b8aa,0xff918a82
};
inline constexpr ThemeTokens industrialTokens{
    0xff101619,0xff182227,0xff202c32,0xff29373d,0xff303b40,0xff35434a,
    0xffc66b42,0xffd47a4f,0xffa95534,
    0xff8fa8b2,0xffb79a72,0xffa18bb9,0xff7186a6,
    0xff55b875,0xffe05a46,0xffe5a52e,0xff55afc5,
    0xffe5e7e3,0xffa5b0ae,0xff718087,0xff536168
};
inline Appearance activeAppearance=Appearance::industrialOrange;
inline const ThemeTokens& tokens(Appearance id) {
    return id==Appearance::legacy ? legacyTokens : industrialTokens;
}
inline const char* displayName(Appearance id) {
    return id==Appearance::legacy ? "Legacy — Graphite / Copper" :
                                    "Industrial Orange";
}
namespace swatch {
// Changing the theme reassigns these tokens once on the message/UI thread.
// Existing native JUCE drawing code reads the SAME token names as before.
inline juce::Colour bgMain{industrialTokens.bgMain},
    bgPanel{industrialTokens.bgPanel},bgElevated{industrialTokens.bgElevated},
    bgHover{industrialTokens.bgHover},bgSelected{industrialTokens.bgSelected},
    borderDefault{industrialTokens.borderDefault};
inline juce::Colour accentPrimary{industrialTokens.accentPrimary},
    accentHover{industrialTokens.accentHover},accentPressed{industrialTokens.accentPressed};
inline juce::Colour typeAudio{industrialTokens.typeAudio},
    typeMidi{industrialTokens.typeMidi},typeOsc{industrialTokens.typeOsc},
    typeControl{industrialTokens.typeControl};
inline juce::Colour statusGo{industrialTokens.statusGo},
    statusPanic{industrialTokens.statusPanic},statusWarning{industrialTokens.statusWarning},
    statusInfo{industrialTokens.statusInfo};
inline juce::Colour textPrimary{industrialTokens.textPrimary},
    textSecondary{industrialTokens.textSecondary},textMuted{industrialTokens.textMuted},
    textDisabled{industrialTokens.textDisabled};
// These seven user-assignable cue values are intentionally NOT themed,
// because saved Show rows contain their IDs (1–7).
inline const juce::Colour cue1{0xffe4a77e},cue2{0xffdebd88},
    cue3{0xffe48c7f},cue4{0xffb7a1bf},cue5{0xff8ea7b8},
    cue6{0xffa1b5a4},cue7{0xffd9b577};
// Compatibility aliases for earlier native components.
inline juce::Colour graphiteBlack=bgMain,graphiteInput=bgPanel,
    graphitePanel=bgPanel,graphiteToolbar=bgElevated,graphiteRaised=bgSelected,
    graphiteLine=borderDefault,warmWhite=textPrimary,warmGrey=textSecondary,
    copper=accentPrimary,copperLight=accentHover,copperDark=accentPressed,
    brass=cue7,amber=typeMidi,violet=typeOsc,slateBlue=cue5,
    slateSage=cue6,success=statusGo,error=statusPanic,danger=statusPanic,
    petrol=cue5,sage=cue6,leaf=cue7;
}
inline juce::Colour background=swatch::bgMain,input=swatch::bgPanel,
    panel=swatch::bgPanel,toolbar=swatch::bgElevated,raised=swatch::bgSelected,
    line=swatch::borderDefault,hover=swatch::bgHover;
inline juce::Colour copper=swatch::accentPrimary,
    accent=swatch::accentPrimary,accentHover=swatch::accentHover,
    accentPressed=swatch::accentPressed,text=swatch::textPrimary,
    muted=swatch::textSecondary,dimmed=swatch::textMuted,
    disabled=swatch::textDisabled,
    teal=swatch::accentPrimary.interpolatedWith(swatch::textPrimary,.45f),
    amber=swatch::typeMidi,purple=swatch::typeOsc,
    red=swatch::statusPanic.interpolatedWith(swatch::textPrimary,.35f),
    go=swatch::statusGo,panic=swatch::statusPanic,
    warning=swatch::statusWarning,selectedTab=swatch::bgSelected,
    selection=swatch::accentPrimary,playing=swatch::statusGo,
    standby=swatch::statusWarning,technicalInfo=swatch::statusInfo;
inline juce::Colour type(int i) {
    return i==0 ? swatch::typeAudio : i==1 ? swatch::typeMidi :
           i==2 ? swatch::typeOsc : swatch::typeControl;
}
inline void activate(Appearance chosen) {
    activeAppearance=chosen;
    const auto& t=tokens(chosen);
    swatch::bgMain=juce::Colour(t.bgMain);
    swatch::bgPanel=juce::Colour(t.bgPanel);
    swatch::bgElevated=juce::Colour(t.bgElevated);
    swatch::bgHover=juce::Colour(t.bgHover);
    swatch::bgSelected=juce::Colour(t.bgSelected);
    swatch::borderDefault=juce::Colour(t.borderDefault);
    swatch::accentPrimary=juce::Colour(t.accentPrimary);
    swatch::accentHover=juce::Colour(t.accentHover);
    swatch::accentPressed=juce::Colour(t.accentPressed);
    swatch::typeAudio=juce::Colour(t.typeAudio);
    swatch::typeMidi=juce::Colour(t.typeMidi);
    swatch::typeOsc=juce::Colour(t.typeOsc);
    swatch::typeControl=juce::Colour(t.typeControl);
    swatch::statusGo=juce::Colour(t.statusGo);
    swatch::statusPanic=juce::Colour(t.statusPanic);
    swatch::statusWarning=juce::Colour(t.statusWarning);
    swatch::statusInfo=juce::Colour(t.statusInfo);
    swatch::textPrimary=juce::Colour(t.textPrimary);
    swatch::textSecondary=juce::Colour(t.textSecondary);
    swatch::textMuted=juce::Colour(t.textMuted);
    swatch::textDisabled=juce::Colour(t.textDisabled);
    swatch::graphiteBlack=swatch::bgMain;
    // Legacy used #1B1E20 for the input, independently of its panel.
    swatch::graphiteInput=chosen==Appearance::legacy ?
                          juce::Colour(0xff1b1e20) : swatch::bgPanel;
    swatch::graphitePanel=swatch::bgPanel;
    swatch::graphiteToolbar=swatch::bgElevated;
    swatch::graphiteRaised=swatch::bgSelected;
    swatch::graphiteLine=swatch::borderDefault;
    swatch::warmWhite=swatch::textPrimary;
    swatch::warmGrey=swatch::textSecondary;
    swatch::copper=swatch::accentPrimary;
    swatch::copperLight=swatch::accentHover;
    swatch::copperDark=swatch::accentPressed;
    swatch::amber=swatch::typeMidi;
    swatch::violet=swatch::typeOsc;
    swatch::success=swatch::statusGo;
    swatch::error=chosen==Appearance::legacy ?
        juce::Colour(0xffe48c7f) : swatch::statusPanic;
    swatch::danger=swatch::statusPanic;
    background=swatch::bgMain;
    input=swatch::graphiteInput;
    panel=swatch::bgPanel;
    toolbar=swatch::bgElevated;
    raised=swatch::bgSelected;
    line=swatch::borderDefault;
    hover=swatch::bgHover;
    accent=swatch::accentPrimary;
    accentHover=swatch::accentHover;
    accentPressed=swatch::accentPressed;
    text=swatch::textPrimary;
    muted=swatch::textSecondary;
    dimmed=swatch::textMuted;
    disabled=swatch::textDisabled;
    // Pre-Industrial UI drew with CopperLight, not the darker raw copper.
    copper=chosen==Appearance::legacy ? swatch::copperLight : accent;
    teal=chosen==Appearance::legacy ? swatch::copperLight :
         accent.interpolatedWith(text,.45f);
    amber=swatch::typeMidi;
    purple=swatch::typeOsc;
    red=chosen==Appearance::legacy ? juce::Colour(0xffe48c7f) :
        swatch::statusPanic.interpolatedWith(text,.35f);
    go=swatch::statusGo;
    panic=swatch::statusPanic;
    warning=swatch::statusWarning;
    selectedTab=chosen==Appearance::legacy ? swatch::copperDark : swatch::bgSelected;
    selection=chosen==Appearance::legacy ? teal : accent;
    playing=go;
    standby=warning;
    technicalInfo=swatch::statusInfo;
}
inline float relativeLuminance(juce::Colour c) {
    const auto component=[](float s) {
        return s<=.04045f ? s/12.92f : std::pow((s+.055f)/1.055f,2.4f);
    };
    return .2126f*component(c.getFloatRed())+
           .7152f*component(c.getFloatGreen())+
           .0722f*component(c.getFloatBlue());
}
inline juce::Colour readableInk(juce::Colour surface) {
    const auto y=relativeLuminance(surface);
    const auto light=relativeLuminance(text),dark=relativeLuminance(background);
    const auto lightRatio=(std::max(y,light)+.05f)/(std::min(y,light)+.05f);
    const auto darkRatio=(std::max(y,dark)+.05f)/(std::min(y,dark)+.05f);
    return lightRatio>=darkRatio ? text : background;
}
inline juce::Colour cueSurface(juce::Colour base,juce::Colour custom,
                               bool selected,bool hovered) {
    if(activeAppearance==Appearance::legacy || custom.isTransparent())return base;
    // Draw custom colours on the cue body, leaving the selection outline
    // independent of the saved (unchanged) cue colour.
    return base.interpolatedWith(custom,selected ? .62f : hovered ? .59f : .55f);
}
}
// Small icon buttons share one restrained surface, but retain their own
// glyphs, colours, geometry and event handling. Ordinary text buttons are
// intentionally not forced into this visual treatment.
struct IconButtonState {
    bool enabled=true;
    bool hovered=false;
    bool pressed=false;
    bool active=false;
    bool keyboardFocus=false;
};
inline juce::Colour iconButtonInk(juce::Colour accent,const IconButtonState& state) {
    if (!state.enabled) return palette::muted.withAlpha(.44f);
    return accent.withAlpha(state.active || state.hovered || state.pressed ? 1.0f : .90f);
}
inline void drawIconButtonSurface(juce::Graphics& g,juce::Rectangle<float> bounds,
                                  juce::Colour accent,const IconButtonState& state,
                                  float radius=4.5f) {
    if (bounds.isEmpty()) return;
    const auto rect=bounds.reduced(.65f);
    const auto fill=state.active ? accent.withAlpha(state.pressed ? .18f : .12f) :
                    state.pressed ? palette::raised.withAlpha(.82f) :
                    palette::raised.withAlpha(state.hovered ? .68f : .47f);
    g.setColour(state.enabled ? fill : fill.withMultipliedAlpha(.40f));
    g.fillRoundedRectangle(rect,radius);
    const auto stroke=state.keyboardFocus ? palette::teal :
        state.active ? accent.withAlpha(.58f) :
        state.hovered ? palette::line.withAlpha(.48f) :
                        palette::line.withAlpha(.27f);
    g.setColour(state.enabled ? stroke : stroke.withMultipliedAlpha(.45f));
    g.drawRoundedRectangle(rect,radius,state.keyboardFocus ? 1.65f :
                                         state.active ? .95f : .8f);
}
// Shared vector Undo/Redo icons: both editors use the same hit target and
// local edit-history callbacks, without depending on font glyph coverage.
class HistoryIconButton final : public juce::TextButton {
public:
    explicit HistoryIconButton(bool forward) :
        juce::TextButton(forward ? "Redo" : "Undo"),redoDirection(forward) {}
    void paintButton(juce::Graphics& g,bool hovered,bool pressed) override {
        const IconButtonState state{isEnabled(),hovered,pressed,false,
                                    hasKeyboardFocus(false)};
        drawIconButtonSurface(g,getLocalBounds().toFloat(),palette::teal,state);
        const auto bounds=getLocalBounds().toFloat();
        const float cx=bounds.getCentreX(), cy=bounds.getCentreY();
        juce::Path curve;
        curve.startNewSubPath(cx-6.f,cy+4.f);
        curve.cubicTo(cx-7.f,cy-4.f,cx+1.f,cy-7.f,cx+5.f,cy-1.f);
        curve.startNewSubPath(cx+1.f,cy-3.5f);
        curve.lineTo(cx+5.f,cy-1.f);
        curve.lineTo(cx+6.5f,cy-5.f);
        if (!redoDirection)
            curve.applyTransform(juce::AffineTransform::scale(-1.f,1.f,cx,cy));
        g.setColour(iconButtonInk(palette::text,state));
        g.strokePath(curve,juce::PathStrokeType(1.65f,
            juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
    }
private:
    bool redoDirection=false;
};
// Dock edge-grip follows the Inspector's vector/button styling.
// The icon changes direction while the existing Collapse/Expand action remains.
class DockCollapseButton final : public juce::TextButton {
public:
    DockCollapseButton() : juce::TextButton("Collapse") {}
    void setCollapsed(bool value) {
        if (collapsed!=value) {collapsed=value;repaint();}
    }
    void paintButton(juce::Graphics& g,bool hovered,bool pressed) override {
        const auto r=getLocalBounds().toFloat();
        const IconButtonState state{isEnabled(),hovered,pressed,false,
                                    hasKeyboardFocus(false)};
        drawIconButtonSurface(g,r,palette::teal,state,4.f);
        const float cx=r.getCentreX(),cy=r.getCentreY();
        g.setColour(palette::muted.withAlpha(.75f));
        g.drawLine(cx-6.f,cy+5.f,cx+6.f,cy+5.f,1.1f);
        juce::Path chevron;
        if (collapsed) {
            chevron.startNewSubPath(cx-4.f,cy-3.f);
            chevron.lineTo(cx,cy+1.f);
            chevron.lineTo(cx+4.f,cy-3.f);
        } else {
            chevron.startNewSubPath(cx-4.f,cy+1.f);
            chevron.lineTo(cx,cy-3.f);
            chevron.lineTo(cx+4.f,cy+1.f);
        }
        g.setColour(hovered || pressed ? palette::text : palette::teal);
        g.strokePath(chevron,juce::PathStrokeType(1.8f,
            juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
    }
private:
    bool collapsed=false;
};
// Compact clip-editor window actions. Both buttons use the same 28×26
// geometry as DockCollapseButton, with vector paths (no font glyph fallback).
class EditorCloseIconButton final : public juce::TextButton {
public:
    EditorCloseIconButton():juce::TextButton("Close") {}
    void paintButton(juce::Graphics& g,bool hovered,bool pressed) override {
        const IconButtonState state{isEnabled(),hovered,pressed,false,
                                   hasKeyboardFocus(false)};
        drawIconButtonSurface(g,getLocalBounds().toFloat(),palette::teal,state,4.f);
        const auto r=getLocalBounds().toFloat();
        const float x=r.getCentreX(),y=r.getCentreY();
        g.setColour(hovered || pressed ? palette::text : palette::muted);
        g.drawLine(x-4.5f,y-4.5f,x+4.5f,y+4.5f,1.9f);
        g.drawLine(x+4.5f,y-4.5f,x-4.5f,y+4.5f,1.9f);
    }
};
class EditorDetachIconButton final : public juce::TextButton {
public:
    EditorDetachIconButton():juce::TextButton("Detach") {}
    void setFloating(bool value) {
        if(floating!=value){floating=value;repaint();}
    }
    void paintButton(juce::Graphics& g,bool hovered,bool pressed) override {
        const IconButtonState state{isEnabled(),hovered,pressed,false,
                                   hasKeyboardFocus(false)};
        drawIconButtonSurface(g,getLocalBounds().toFloat(),palette::teal,state,4.f);
        const auto r=getLocalBounds().toFloat();
        const float x=r.getCentreX(),y=r.getCentreY();
        g.setColour(hovered || pressed ? palette::text : palette::teal);
        g.drawRoundedRectangle(x-7.f,y-5.5f,10.f,10.f,1.f,1.2f);
        if(!floating) {
            // Window-opening arrow points out to a separate window.
            g.drawLine(x-1.f,y+1.f,x+6.f,y-6.f,1.5f);
            g.drawLine(x+1.5f,y-6.f,x+6.f,y-6.f,1.5f);
            g.drawLine(x+6.f,y-6.f,x+6.f,y-1.5f,1.5f);
        } else {
            // Return arrow points back into the dock.
            g.drawLine(x+6.f,y-6.f,x-1.f,y+1.f,1.5f);
            g.drawLine(x-1.f,y-3.5f,x-1.f,y+1.f,1.5f);
            g.drawLine(x-1.f,y+1.f,x+3.5f,y+1.f,1.5f);
        }
    }
private:
    bool floating=false;
};
// Embedded OFL typefaces are registered once and kept alive for the app's
// lifetime. No user-installed fonts, startup network or Windows font state.
struct AppTypefaces {
    juce::Typeface::Ptr exo2=juce::Typeface::createSystemTypefaceFor(
        BinaryData::Exo2_ttf,BinaryData::Exo2_ttfSize);
    juce::Typeface::Ptr hind=juce::Typeface::createSystemTypefaceFor(
        BinaryData::HindRegular_ttf,BinaryData::HindRegular_ttfSize);
    juce::Typeface::Ptr hindSemibold=juce::Typeface::createSystemTypefaceFor(
        BinaryData::HindSemiBold_ttf,BinaryData::HindSemiBold_ttfSize);
    // Noto families are embedded for explicit math/symbol rendering.
    // Do not rely on finding system-installed typefaces.
    juce::Typeface::Ptr notoMath=juce::Typeface::createSystemTypefaceFor(
        BinaryData::NotoSansMathRegular_ttf,BinaryData::NotoSansMathRegular_ttfSize);
    juce::Typeface::Ptr notoSymbols2=juce::Typeface::createSystemTypefaceFor(
        BinaryData::NotoSansSymbols2Regular_ttf,BinaryData::NotoSansSymbols2Regular_ttfSize);
};
inline const AppTypefaces& appTypefaces() {
    static const AppTypefaces faces;
    return faces;
}

// Historical design sizes are JUCE logical pixels, not typographic points.
// 1 pt = 4/3 logical px at the Windows 96-DPI reference scale. Preserve the
// intended layout while enforcing Exo 2 at >=10 actual points for small text.
inline constexpr float smallTextCutoff=14.5f;
inline juce::Font font(float size,bool bold=false) {
    const auto& faces=appTypefaces();
    const bool small=size<=smallTextCutoff;
    const auto face=small ? faces.exo2 : (bold ? faces.hindSemibold : faces.hind);
    const auto pt=juce::jmax(10.0f,size*0.75f);
    if (face!=nullptr) {
        const juce::Font embedded(juce::FontOptions(face)
            .withPointHeight(pt)
            // Noto fonts are selected explicitly for special glyph runs.
            // Ordinary UI text always retains Exo 2 or Hind.
            .withFallbacks({"Segoe UI Symbol"}));
        // JUCE 8 FontOptions has no withStyleFlags() member. Bold the
        // embedded Exo 2 face using the supported juce::Font API.
        return small && bold ? embedded.boldened() : embedded;
    }
    // Last resort only (e.g. a corrupt embedded font), never default behavior.
    return juce::Font(juce::FontOptions("Segoe UI",size,
        bold ? juce::Font::bold : juce::Font::plain).withPointHeight(pt)
        .withFallbacks({"Segoe UI Symbol"}));
}
// Explicit fonts for special glyphs. Never use these for ordinary headings,
 // labels or transport iconography; those retain Hind/Exo 2 or vector paths.
inline juce::Font mathFont(float size) {
    const auto& face=appTypefaces().notoMath;
    const auto pt=juce::jmax(10.0f,size*0.75f);
    return face!=nullptr
        ? juce::Font(juce::FontOptions(face).withPointHeight(pt))
        : font(size);
}
inline juce::Font symbolsFont(float size) {
    const auto& face=appTypefaces().notoSymbols2;
    const auto pt=juce::jmax(10.0f,size*0.75f);
    return face!=nullptr
        ? juce::Font(juce::FontOptions(face).withPointHeight(pt))
        : font(size);
}
// Draw a symbol only when it actually exists in the embedded Symbols 2 font.
// Do not ask JUCE to substitute an OS font or an emoji. Callers must supply a
// small vector fallback if this glyph is absent from the pinned font.
inline bool drawNotoSymbol(juce::Graphics& g,juce::juce_wchar codepoint,
                           juce::Rectangle<int> area,float size,juce::Colour ink) {
    const auto& symbols=appTypefaces().notoSymbols2;
    if (symbols==nullptr) return false;
    const auto glyph=symbols->getNominalGlyphForCodepoint(codepoint);
    if (!glyph || static_cast<int>(*glyph)==0) return false;
    juce::Path outline;
    symbols->getOutlineForGlyph(juce::TypefaceMetricsKind::portable,
                                static_cast<int>(*glyph),outline);
    // A nominal glyph lookup is not enough: some characters map to a
    // missing/notdef glyph, or JUCE has no drawable path for that glyph.
    // Reporting success here used to leave empty Play/Stop icons instead of
    // running the caller's well-tested vector fallback.
    const auto inkBounds=outline.getBounds();
    if (outline.isEmpty() || inkBounds.getWidth()<=0.0f ||
        inkBounds.getHeight()<=0.0f)
        return false;
    g.saveState();
    g.setColour(ink);
    {
        // Do not centre by font advance/baseline: optical (ink) bounds can
        // differ substantially, especially for ▶, ⏹ and directional arrows.
        // Preserve proportions and leave 2px of breathing room on each side.
        const float extent=juce::jmax(1.0f,juce::jmin(
            juce::jmin((float)area.getWidth()-4.0f,
                       (float)area.getHeight()-4.0f),size*.82f));
        const auto target=juce::Rectangle<float>(extent,extent)
            .withCentre(area.toFloat().getCentre());
        g.fillPath(outline,outline.getTransformToScaleToFit(
            target,true,juce::Justification::centred));
    }
    g.restoreState();
    return true;
}
// The Fader Range menu mixes two scripts: draw only the mathematical
// minus/infinity pair with Noto Sans Math; the rest remains Exo 2.
inline juce::String faderMinusInfinity() {
    return juce::String::charToString((juce::juce_wchar)0x2212)
         + juce::String::charToString((juce::juce_wchar)0x221e);
}
inline juce::String faderRangeMenuTitle(int maximumDb) {
    return faderMinusInfinity()+" dB / "+(maximumDb>0?"+":"")
           +juce::String(maximumDb)+" dB";
}
inline void label(juce::Graphics& g,const juce::String& s,juce::Rectangle<int> r,float size=14,juce::Colour colour=palette::text,bool bold=false,int align=juce::Justification::centredLeft) {
    g.setColour(colour); g.setFont(font(size,bold)); g.drawText(s,r,align,true);
}
class Theme final : public juce::LookAndFeel_V4 {
public:
    Theme() {applyColourTokens();}
    // LookAndFeel colour IDs are stored, unlike the dynamic palette tokens
    // used by paint(). Reapply on every theme switch, including popups.
    void applyColourTokens() {
        setColour(juce::TextButton::buttonColourId,palette::raised);
        setColour(juce::TextButton::textColourOffId,palette::text);
        setColour(juce::ComboBox::backgroundColourId,palette::input);
        setColour(juce::ComboBox::textColourId,palette::text);
        setColour(juce::ComboBox::outlineColourId,palette::line);
        setColour(juce::PopupMenu::backgroundColourId,palette::panel);
        setColour(juce::PopupMenu::textColourId,palette::text);
        setColour(juce::PopupMenu::headerTextColourId,palette::teal);
        setColour(juce::PopupMenu::highlightedBackgroundColourId,palette::raised);
        setColour(juce::PopupMenu::highlightedTextColourId,palette::text);
        setColour(juce::TextEditor::backgroundColourId,palette::input);
        setColour(juce::TextEditor::textColourId,palette::text);
        setColour(juce::TextEditor::outlineColourId,palette::line);
        setColour(juce::TextEditor::focusedOutlineColourId,palette::teal);
        setColour(juce::Label::textColourId,palette::text);
        setColour(juce::ToggleButton::textColourId,palette::text);
        setColour(juce::ToggleButton::tickColourId,palette::teal);
        setColour(juce::Slider::thumbColourId,palette::teal);
        setColour(juce::Slider::trackColourId,palette::teal);
        setColour(juce::Slider::backgroundColourId,palette::line);
        setColour(juce::Slider::textBoxTextColourId,palette::text);
        setColour(juce::Slider::textBoxOutlineColourId,palette::line);
        setColour(juce::ResizableWindow::backgroundColourId,palette::background);
        // JUCE DocumentWindow exposes textColourId only; its titlebar
        // background comes from ResizableWindow and the LookAndFeel.
        setColour(juce::DocumentWindow::textColourId,palette::text);
        setColour(juce::ScrollBar::backgroundColourId,palette::raised);
        setColour(juce::AlertWindow::backgroundColourId,palette::panel);
        setColour(juce::AlertWindow::textColourId,palette::text);
        // Quiet left-edge scrollbar: readable on hover, but not a glowing
        // white line competing with cues and the cyan BPM boundary.
        setColour(juce::ScrollBar::thumbColourId,
                  palette::muted.withMultipliedBrightness(0.72f));
    }
    juce::Font getTextButtonFont(juce::TextButton&,int height) override {
        return font(height<26 ? 12.5f : 15.0f,true);
    }
    juce::Font getComboBoxFont(juce::ComboBox&) override { return font(14.5f); }
    juce::Font getLabelFont(juce::Label& l) override {
        return font(l.getFont().getHeight(),l.getFont().isBold());
    }
    juce::Font getAlertWindowTitleFont() override { return font(22.0f,true); }
    juce::Font getAlertWindowMessageFont() override { return font(15.0f); }
    juce::Font getAlertWindowFont() override { return font(14.0f); }
    juce::Font getSliderPopupFont(juce::Slider&) override { return font(14.0f); }
    void drawButtonBackground(juce::Graphics& g,juce::Button& b,const juce::Colour& colour,bool over,bool down) override {
        const auto caption=b.getButtonText();
        // Only icon-only JUCE TextButtons. Keep full-text controls, the
        // emergency PANIC action and the GO button visually unchanged.
        if (caption=="+" || caption=="x" || caption=="..." || caption=="⋯") {
            const IconButtonState state{b.isEnabled(),over,down,
                b.getToggleState(),b.hasKeyboardFocus(false)};
            drawIconButtonSurface(g,b.getLocalBounds().toFloat(),
                                  caption=="x" ? palette::red : palette::teal,state);
            return;
        }
        const auto r=b.getLocalBounds().toFloat().reduced(0.8f);
        const auto fill=colour.withMultipliedBrightness(down ? 0.87f : over ? 1.14f : 1.0f);
        g.setColour(b.isEnabled() ? fill : fill.withAlpha(0.46f));
        g.fillRoundedRectangle(r,6.0f);
        g.setColour(b.hasKeyboardFocus(false) ? palette::teal :
                    over && b.isEnabled() ? palette::muted : palette::line);
        g.drawRoundedRectangle(r,6.0f,b.hasKeyboardFocus(false) ? 1.6f : 1.0f);
    }

    // JUCE PopupMenu is a separate desktop window. Explicitly attach this
    // LookAndFeel when constructing a menu (not only to its target button),
    // otherwise Windows can render it using the default grey theme.
    juce::Font getPopupMenuFont() override { return font(13.0f); }

    void drawPopupMenuBackground(juce::Graphics& g,int width,int height) override {
        g.fillAll(palette::panel);
        g.setColour(palette::line.withAlpha(.9f));
        g.drawRect(0,0,width,height,1);
    }

    void drawPopupMenuItem(juce::Graphics& g,const juce::Rectangle<int>& area,
                           bool separator,bool active,bool highlighted,bool ticked,
                           bool hasSubMenu,const juce::String& title,
                           const juce::String& shortcut,const juce::Drawable* icon,
                           const juce::Colour* customTextColour) override {
        if (separator) {
            g.setColour(palette::line.withAlpha(.65f));
            g.fillRect(area.getX()+12,area.getCentreY(),
                       juce::jmax(1,area.getWidth()-24),1);
            return;
        }

        const auto hover=highlighted && active;
        if (hover) {
            const auto surface=area.reduced(3,1).toFloat();
            g.setColour(palette::raised);
            g.fillRoundedRectangle(surface,4.0f);
            g.setColour(palette::accent);
            g.fillRoundedRectangle((float)area.getX()+3.0f,
                                   (float)area.getY()+4.0f,
                                   2.5f,(float)juce::jmax(1,area.getHeight()-8),1.0f);
        }

        const auto ink=customTextColour!=nullptr ? *customTextColour : palette::text;
        g.setColour(active ? (hover ? palette::text : ink)
                           : palette::muted.withAlpha(.48f));
        g.setFont(font(13.0f));

        auto content=area.reduced(10,0);
        const auto iconArea=content.removeFromLeft(20).toFloat();
        if (icon!=nullptr)
            icon->drawWithin(g,iconArea,
                juce::RectanglePlacement::centred |
                juce::RectanglePlacement::onlyReduceInSize,active ? 1.0f : .48f);
        else if (ticked) {
            g.setColour(active ? palette::teal : palette::muted.withAlpha(.48f));
            juce::Path tick;
            tick.startNewSubPath(iconArea.getX()+3.0f,iconArea.getCentreY());
            tick.lineTo(iconArea.getX()+8.0f,iconArea.getCentreY()+4.0f);
            tick.lineTo(iconArea.getX()+16.0f,iconArea.getCentreY()-5.0f);
            g.strokePath(tick,juce::PathStrokeType(1.9f,
                juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
        }
        content.removeFromLeft(4);
        if (hasSubMenu) {
            const float cx=(float)content.getRight()-7.0f;
            const float cy=(float)content.getCentreY();
            juce::Path chevron;
            chevron.startNewSubPath(cx-3.0f,cy-4.0f);
            chevron.lineTo(cx+1.0f,cy);
            chevron.lineTo(cx-3.0f,cy+4.0f);
            g.setColour(active ? palette::teal : palette::muted.withAlpha(.48f));
            g.strokePath(chevron,juce::PathStrokeType(1.6f,
                juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
            content.removeFromRight(19);
        }
        if (shortcut.isNotEmpty()) content.removeFromRight(70);
        g.setColour(active ? (hover ? palette::text : ink)
                           : palette::muted.withAlpha(.48f));
        if (title.startsWith(faderMinusInfinity())) {
            const auto prefix=faderMinusInfinity();
            const auto mathematical=mathFont(13.0f);
            const int prefixWidth=(int)std::ceil(
                juce::GlyphArrangement::getStringWidth(mathematical,prefix))+2;
            const auto prefixArea=content.removeFromLeft(prefixWidth);
            g.setFont(mathematical);
            g.drawText(prefix,prefixArea,juce::Justification::centredLeft,false);
            g.setFont(font(13.0f));
            g.drawFittedText(title.substring(prefix.length()),content,
                             juce::Justification::centredLeft,1);
        } else {
            g.setFont(font(13.0f));
            g.drawFittedText(title,content,juce::Justification::centredLeft,1);
        }
        if (shortcut.isNotEmpty()) {
            g.setColour(active ? palette::muted : palette::muted.withAlpha(.48f));
            g.setFont(font(10.5f));
            const auto shortcutArea=area.reduced(12,0).removeFromRight(69);
            g.drawFittedText(shortcut,shortcutArea,
                             juce::Justification::centredRight,1);
        }
    }

    void drawPopupMenuSectionHeader(juce::Graphics& g,
                                    const juce::Rectangle<int>& area,
                                    const juce::String& section) override {
        g.setColour(palette::teal);
        g.setFont(font(11.0f,true));
        g.drawFittedText(section,area.reduced(12,2),
                         juce::Justification::centredLeft,1);
    }

    void getIdealPopupMenuItemSize(const juce::String& title,bool separator,
                                    int standardHeight,int& width,int& height) override {
        if (separator) {
            width=28;
            height=9;
            return;
        }
        height=juce::jmax(34,standardHeight);
        if (title.startsWith(faderMinusInfinity())) {
            const auto prefix=faderMinusInfinity();
            width=juce::jmax(142,(int)std::ceil(
                juce::GlyphArrangement::getStringWidth(mathFont(13.0f),prefix)
              + juce::GlyphArrangement::getStringWidth(
                    font(13.0f),title.substring(prefix.length())))+73);
        } else {
            width=juce::jmax(110,
                (int)std::ceil(juce::GlyphArrangement::getStringWidth(
                    font(13.0f),title))+65);
        }
    }

    void drawComboBox(juce::Graphics& g,int width,int height,bool isButtonDown,
                      int buttonX,int buttonY,int buttonW,int buttonH,
                      juce::ComboBox& combo) override {
        const auto r=juce::Rectangle<float>(0.5f,0.5f,
                          (float)width-1.0f,(float)height-1.0f);
        g.setColour(combo.isEnabled() ? palette::input
                                      : palette::input.withAlpha(.5f));
        g.fillRoundedRectangle(r,5.0f);
        g.setColour(combo.hasKeyboardFocus(true) ? palette::teal :
                    isButtonDown ? palette::muted : palette::line);
        g.drawRoundedRectangle(r,5.0f,
                               combo.hasKeyboardFocus(true) ? 1.4f : 1.0f);
        const auto cx=(float)buttonX+(float)buttonW*.5f;
        const auto cy=(float)buttonY+(float)buttonH*.5f;
        juce::Path arrow;
        arrow.startNewSubPath(cx-4.0f,cy-2.0f);
        arrow.lineTo(cx,cy+2.0f);
        arrow.lineTo(cx+4.0f,cy-2.0f);
        g.setColour(combo.isEnabled() ? palette::teal : palette::muted.withAlpha(.5f));
        g.strokePath(arrow,juce::PathStrokeType(1.6f,
            juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
    }
};
}

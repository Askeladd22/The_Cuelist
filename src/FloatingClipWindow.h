#pragma once
#include "Theme.h"
#include <functional>
#include <utility>

namespace cuelist {

// One JUCE-owned native shell for the detachable MIDI and OSC clip editors.
// The editor, dimensions and safe asynchronous dock callback remain owned
// by each protocol's MainComponent functions.
class FloatingClipWindow final : public juce::DocumentWindow {
public:
    FloatingClipWindow(const juce::String& title,std::function<void()> onClose)
        : juce::DocumentWindow(title,palette::panel,
              juce::DocumentWindow::closeButton,true),
          close(std::move(onClose)) {}
    void closeButtonPressed() override { if (close) close(); }
private:
    std::function<void()> close;
};

} // namespace cuelist

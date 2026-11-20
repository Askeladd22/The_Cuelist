#include "ControlOutputRouter.h"
#include <cmath>
#include <cstring>
#include <utility>

namespace cuelist {
ControlOutputRouter::ControlOutputRouter(std::function<void()> probe)
    : juce::Thread("Control OSC output"), beforeWriteProbe(std::move(probe)) {startThread();}
ControlOutputRouter::~ControlOutputRouter() {
    signalThreadShouldExit(); notify(); stopThread(-1);
}
bool ControlOutputRouter::sendOscFloat(const juce::String& host,int port,
                                      const juce::String& address,float value) {
    // For exactly one OSC float: padded address (UTF-8 + NUL), four-byte
    // ",f" typetag and four-byte float. With a 1024-byte output cap, 1015
    // UTF-8 address bytes is the largest encodable value (1016 padded bytes).
    constexpr std::size_t maxAddressUtf8Bytes=1024u-8u-1u;
    if (host.isEmpty() || port<1 || port>65535 || !address.startsWithChar('/') ||
        !std::isfinite(value) ||
        std::strlen(address.toRawUTF8())>maxAddressUtf8Bytes) return false;
    const juce::ScopedLock lock(queueLock);
    if (queue.size()>=maxQueue) {
        dropped.fetch_add(1,std::memory_order_relaxed); return false;
    }
    queue.push_back({host,address,port,value});
    notify(); return true;
}
void ControlOutputRouter::clear() {
    const juce::ScopedLock guard(queueLock); queue.clear();
}
void ControlOutputRouter::run() {
    juce::DatagramSocket udp(false);
    while (!threadShouldExit()) {
        Packet p;
        {
            const juce::ScopedLock lock(queueLock);
            if (!queue.empty()) {p=std::move(queue.front());queue.pop_front();}
        }
        if (p.host.isEmpty()) {wait(25);continue;}
        OscEvent event; event.address=p.address;
        OscValue argument; argument.type=OscValueType::decimal; argument.decimal=p.decimal;
        event.arguments.push_back(std::move(argument));
        auto bytes=encodeOscMessage(event);
        // Test-only staging hook: create a deterministic pause after dequeue
        // and encoding, before the irrevocable OS UDP write.
        if (beforeWriteProbe) beforeWriteProbe();
        if (!bytes.empty() && bytes.size()<=1024 &&
            udp.write(p.host,p.port,bytes.data(),(int)bytes.size())==(int)bytes.size())
            sent.fetch_add(1,std::memory_order_relaxed);
        else
            dropped.fetch_add(1,std::memory_order_relaxed);
    }
}
}

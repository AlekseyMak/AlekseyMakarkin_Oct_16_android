//
// Created by Andrey on 7/10/18.
//

#include "Logger.h"
#include "AlanBase.h"
#include "AsioHttp.h"
#include "FFMPEGHeaders.h"

#include <mutex>
#include <boost/chrono.hpp>

using namespace std;
using namespace chrono;

namespace sky {

static const string _VERSION = "0.2.2";

AlanBase::Config::Config() {
    send.codec = "opus";
    send.sampleRate = 48000;

    recv.codec = "pcm_s16le";
    recv.sampleRate = 16000;

    sampleRate = 44100;
};

AlanBase::PlayItem::PlayItem(json js) : isAudio(false), event(move(js)) {
}

static volatile bool _ffmpegInitialized = false;

ostream & operator << (ostream & out, AlanBase::ConnectState state) {
    if (state == AlanBase::ConnectState::Idle) {
        return out<<"Idle";
    }
    if (state == AlanBase::ConnectState::Connecting) {
        return out<<"Connecting";
    }
    if (state == AlanBase::ConnectState::Authorizing) {
        return out<<"Authorizing";
    }
    if (state == AlanBase::ConnectState::Connected) {
        return out<<"Connected";
    }
    if (state == AlanBase::ConnectState::Closed) {
        return out<<"Closed";
    }
    return out<<"#"<<state;
}

ostream & operator << (ostream & out, AlanBase::DialogState state) {
    if (state == AlanBase::DialogState::Idle) {
        return out<<"Idle";
    }
    if (state == AlanBase::DialogState::Listen) {
        return out<<"Listen";
    }
    if (state == AlanBase::DialogState::Process) {
        return out<<"Process";
    }
    if (state == AlanBase::DialogState::Reply) {
        return out<<"Reply";
    }
    return out<<"#"<<state;
}

AlanBase::~AlanBase() {
    stop();
}

AlanBase::AlanBase(json js, const Config & cfg) : _auth(move(js)), _timeZone(cfg.timeZone) {
    if (!_ffmpegInitialized) {
        _ffmpegInitialized = true;
        av_register_all();
    }
    _url = cfg.server + "/ws_project/" + cfg.projectId;
    _encoder = AudioEncoder::create(cfg.send.codec, cfg.sampleRate, cfg.send.sampleRate);
    _decoder = AudioDecoder::create(cfg.recv.codec, cfg.recv.sampleRate, cfg.sampleRate);
    _thread.reset(new thread([=] { run(); }));
}

string AlanBase::getVersion() const {
    return _VERSION;
}

void AlanBase::run() {
    Log<<"AlanBase main thread started"<<eol;
    try {
        aio::deadline_timer timer(_ios);
        bool timerActive = false;
        while (!_interrupted || _websocket) {
            if (!timerActive) {
                timerActive = true;
                timer.expires_from_now(aio::posix_time::milliseconds(20));
                timer.async_wait([&](const error_code & e) {
                    if (e) {
                        Err<<"timer failed: "<<e.message()<<eol;
                    } else {
                        timerActive = false;
                    }
                });
            }
            reconnect();
            sendFrames();
            _ios.reset();
            _ios.run_one();
        }
    } catch (const std::exception & e) {
        Err<<"exception in event thread: "<<toStr(e)<<eol;
    }
    Log<<"exit AlanBase thread"<<eol;
    lock_guard<decltype(_mutex)> lock(_mutex);
    setState(ConnectState::Closed, lock);

}

bool AlanBase::isConnected() const {
    return _websocket && _websocket->state() == WebSocket::State::Connected;
}

void AlanBase::setState(ConnectState state, lock_guard<mutex> &) {
    if (_connectState != state) {
        _connectState = state;
        _ios.dispatch([this] {
            if (onConnectState) {
                onConnectState(_connectState);
            }
        });
    }
}

void AlanBase::setState(DialogState state, lock_guard<mutex> &) {
    if (_dialogState != state) {
        _dialogState = state;
        _ios.dispatch([this]{
            if (onDialogState) {
                onDialogState(_dialogState);
            }
        });
    }
}

void AlanBase::sendAuth() {
    assert(isConnected());
    {
        lock_guard<decltype(_mutex)> lock(_mutex);
        setState(ConnectState::Authorizing, lock);
    }
    auto p = _auth;
    if (_dialogId.empty()) {
        p["dialogId"] = _dialogId;
    }
    p["timeZone"] = _timeZone;
    Call cid{_callIds++, "_auth_", p, [=](string err, json js) {
        if (err.size()) {
            if (onError) {
                onError("auth-failed");
            }
            stop();
        } else {
            if (js["dialogId"].is_string()) {
                _dialogId = js["dialogId"];
                Trx<<"authorized with dialogId: "<<_dialogId<<eol;
            }
            _formatSent = false;
            for (auto & d : _deferred) {
                sendCall(move(d));
            }
            _deferred.clear();
            lock_guard<decltype(_mutex)> lock(_mutex);
            setState(ConnectState::Connected, lock);
        }
    }};
    sendCall(cid);
}

void AlanBase::reconnect() {
    if (_websocket || _interrupted) {
        return;
    }
    {
        lock_guard<decltype(_mutex)> lock(_mutex);
        setState(ConnectState::Connecting, lock);
    }
    auto interval = chrono::milliseconds(min(uint32_t(100 * _disconnects * _disconnects), 7000U));
    if (now() - _lastDisconnectTs < interval) {
        return;
    }
    Log<<"reconnecting: "<<_url<<eol;
    _websocket.reset(new WebSocket(_ios, _url));
    _websocket->onError = [=](const string & err) {
        Err<<"connection error: "<<err<<eol;
        if (onError) {
            onError("networking-error");
        }
    };
    _websocket->onState = [=](WebSocket::State state) {
        if (Trx) {
            Trx<<"connection state: "<<toStr(state)<<eol;
        }
        if (state == WebSocket::State::Closed) {
            Log<<"connection closed"<<eol;
            _websocket.reset();
            _disconnects += 1;
            _lastDisconnectTs = now();
        } else if (state == WebSocket::State::Connected) {
            _disconnects = 0;
            sendAuth();
        }
    };
    _websocket->onText = [=](string s) {
        auto js = json::parse(s);
        if (js["i"].is_number()) {
            auto it = _calls.find(js["i"].get<int64_t>());
            if (it != _calls.end()) {
                if (it->second.callback) {
                    if (!js["e"].is_null()) {
                        it->second.callback(js["e"], json());
                    } else {
                        it->second.callback("", js["r"]);
                    }
                }
                _calls.erase(it);
            }
        } else if (js["e"].is_string()) {
            handleEvent(move(js));
        } else {
            Err<<"invalid json message: "<<s<<eol;
        }
    };
    _websocket->onBinary = [=](vector<uint8_t> binary) {
        AlanFrame frame;
        frame.parse(binary);
        if (frame.data.isSet()) {
            auto & b = frame.data.get();
            _decoder->decode(&b[0], b.size());
            auto p = _decoder->read();
            auto s = _decoder->size();
            if (!s) {
                return;
            }
            lock_guard<decltype(_mutex)> lock(_mutex);
            if (!_enabled) {
                return;
            }
            setState(DialogState::Reply, lock);
            if (_voiceQueueRecv.empty() || !_voiceQueueRecv.back().isAudio) {
                _voiceQueueRecv.emplace_back();
            }
            auto & v = _voiceQueueRecv.back().data;
            v.write(p, s);
            _decoder->consume(s);
        }
    };
    _websocket->connect();
}

void AlanBase::handleEvent(json js) {
    auto & event = js["e"];
    if (event == "options") {
        if (onEvent) {
            onEvent(event, js["p"]);
        }
        return;
    }
    {
        lock_guard<decltype(_mutex)> lock(_mutex);
        if (!_enabled) {
            return;
        }
    }
    if (event == "vievent") {
        lock_guard<decltype(_mutex)> lock(_mutex);
        setState(DialogState::Reply, lock);
        _voiceQueueRecv.emplace_back(js);
        return;
    }
    if (event == "inactivity") {
        turn(false);
        return;
    }
    if (event == "recognized") {
        if (js["p"]["final"].get<bool>()) {
            lock_guard<decltype(_mutex)> lock(_mutex);
            _lastRecvVoiceSTs = -1;
            _lastRecvEventSTs = -1;
            setState(DialogState::Process, lock);
        }
    }
    if (onEvent) {
        onEvent(event, js["p"]);
    }
}

void AlanBase::turn(bool enabled) {
    {
        lock_guard<mutex> lock(_mutex);
        _enabled = enabled;
        _voiceQueueSend.clear();
        _voiceQueueRecv.clear();
    }
    _ios.dispatch([this, enabled]{
        AlanFrame frame;
        auto js = json{{"signal", enabled ? "listen" : "stopListen"}};
        frame.json.set(js.dump());
        sendFrame(frame);
        lock_guard<mutex> lock(_mutex);
        setState(DialogState::Idle, lock);
    });
}

void AlanBase::stop() {
    if (_thread) {
        _ios.dispatch([this]{
            Trx<<"stopping AlanBase"<<eol;
            _interrupted = true;
            if (_websocket) {
                _websocket->stop();
            }
        });
        _thread->join();
        _thread.reset();
    }
}

void AlanBase::writeFrame(const float * frame, size_t size) {
    lock_guard<mutex> lock(_mutex);
    if (_enabled && (_dialogState == DialogState::Listen || _dialogState == DialogState::Idle)) {
        _voiceQueueSend.insert(_voiceQueueSend.end(), frame, frame + size);
    }
}

void AlanBase::deferEvent(json js) {
    _ios.dispatch([this, js]{
        if (onEvent) {
            onEvent(js["e"], js["p"]);
        }
    });
}

bool AlanBase::readFrame(float * frame, size_t size) {
    lock_guard<mutex> lock(_mutex);
    if (!_enabled) {
        return false;
    }
    if (_voiceQueueRecv.empty()) {
        if (_dialogState == DialogState::Reply) {
            if (_lastRecvVoiceSTs >= 0) { _lastRecvVoiceSTs += size; }
            if (_lastRecvEventSTs >= 0) { _lastRecvEventSTs += size; }
            if (_lastRecvVoiceSTs / 44 > 75 || _lastRecvEventSTs / 44 > 5000) {
                setState(DialogState::Listen, lock);
            }
        }
        return false;
    }
    size_t idx = 0;
    while (idx < size && !_voiceQueueRecv.empty()) {
        if (!_voiceQueueRecv.front().isAudio) {
            _lastRecvEventSTs = 0;
            auto & js = _voiceQueueRecv.front().event;
            deferEvent(move(js));
            _voiceQueueRecv.pop_front();
            continue;
        }
        _lastRecvVoiceSTs = 0;
        auto & v = _voiceQueueRecv.front().data;
        auto s = min(size - idx, v.size());
        auto r = v.read();
        copy(r, r + s, frame + idx);
        idx += s;
        v.consume(s);
        if (!v.size()) {
            _voiceQueueRecv.pop_front();
            continue;
        }
    }
    if (idx < size) {
        fill(frame + idx, frame + size, 0.);
    }
    return true;
}

void AlanBase::sendFrames() {
    {
        lock_guard<decltype(_mutex)> lock(_mutex);
        if (_connectState != ConnectState::Connected) {
            return;
        }
        if (_voiceQueueSend.empty()) {
            return;
        }
        _encodeBuffer.assign(_voiceQueueSend.begin(), _voiceQueueSend.end());
        _voiceQueueSend.clear();
        if (_dialogState == DialogState::Idle) {
            setState(DialogState::Listen, lock);
        }
    }
    _encoder->encode(&_encodeBuffer[0], _encodeBuffer.size());
    vector<uint8_t> packet;
    while (_encoder->getPacket(packet)) {
        if (!_formatSent) {
            _formatSent = true;
            AlanFrame frame;
            auto send = json{{"codec", _encoder->codec()}, {"sampleRate", _encoder->outputSampleRate()}};
            auto recv = json{{"codec", _decoder->codec()}, {"sampleRate", _decoder->inputSampleRate()}};
            auto js   = json{{"format", {{"send", send}, {"recv", recv}}}};
            frame.json.set(js.dump());
            sendFrame(frame);
        }
        AlanFrame frame;
        frame.data.set(move(packet));
        sendFrame(frame);
    }
}

void AlanBase::sendFrame(AlanFrame & frame) {
    frame.sentTs = nowMs();
    _websocket->write(frame.write());
}

void AlanBase::sendCall(Call cid) {
    json js{{"i", cid.idx}, {"m", cid.method}, {"p", cid.param}};
    _websocket->write(js.dump());
    _calls[cid.idx] = move(cid);
}

void AlanBase::call(string method, json param, function<void(string, json)> callback) {
    _ios.dispatch([=] {
        Call cid{_callIds++, method, param, move(callback)};
        if (_connectState != ConnectState::Connected) {
            _deferred.push_back(move(cid));
        } else {
            sendCall(cid);
        }
    });
}

}

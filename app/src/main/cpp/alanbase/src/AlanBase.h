//
// Created by Andrey on 7/10/18.
//

#pragma once

#include <deque>
#include <string>
#include <thread>
#include <functional>
#include "AsioHttp.h"
#include "AudioEncoder.h"
#include "AudioDecoder.h"
#include "AlanFrame.h"
#include "json.hpp"

namespace sky {

using namespace nlohmann;

class AlanBase {
  public:
    class Config {
      public:
        struct {
            std::string codec;
            int sampleRate;
        } send;
        struct {
            std::string codec;
            int sampleRate;
        } recv;
        int sampleRate;
        std::string timeZone;
        std::string server;
        std::string projectId;
        Config();
    };

    enum class ConnectState {Idle = 0, Connecting = 1, Authorizing = 2, Connected = 3, Closed = 4};
    enum class DialogState  {Idle = 0, Listen = 1, Process = 2, Reply = 3};

    friend std::ostream & operator << (std::ostream &, ConnectState);
    friend std::ostream & operator << (std::ostream &, DialogState);

    std::function<void(ConnectState)>      onConnectState;
    std::function<void(DialogState)>       onDialogState;
    std::function<void(std::string)>       onError;
    std::function<void(std::string, json)> onEvent;

    std::string getVersion() const;
    void writeFrame(const float *, size_t samples);
    bool readFrame (float *, size_t samples);
    void call(std::string method, json param, std::function<void(std::string, json)> callback=std::function<void(std::string, json)>());

    void turn(bool enabled);
    void stop();

    ~AlanBase();
    AlanBase(json auth, const Config & cfg);
  private:
    struct Call {
        int64_t idx;
        std::string method;
        json param;
        std::function<void(std::string, json)> callback;
    };

    struct PlayItem {
        bool isAudio{true};
        Buffer<float> data;
        json event;

        PlayItem() = default;
        explicit PlayItem(json);
    };

    void run();
    void reconnect();
    void sendAuth();
    bool isConnected() const;
    void setState(DialogState,  std::lock_guard<std::mutex> &);
    void setState(ConnectState, std::lock_guard<std::mutex> &);
    void sendFrames();
    void sendFrame(AlanFrame &);
    void sendCall(Call cid);
    void handleEvent(json);
    void deferEvent(json);

    ConnectState                  _connectState{ConnectState::Idle};
    DialogState                   _dialogState{DialogState::Idle};
    int64_t                       _callIds{1};
    std::map<int64_t, Call>       _calls;
    std::string                   _dialogId;
    std::string                   _timeZone;
    std::vector<Call>             _deferred;
    std::unique_ptr<WebSocket>    _websocket;
    std::unique_ptr<std::thread>  _thread;
    aio::io_service               _ios;
    std::string                   _url;
    json                          _auth;
    bool                          _enabled{false};
    bool                          _interrupted{false};
    int                           _disconnects{0};
    Timestamp                     _lastDisconnectTs;
    std::vector<float>            _encodeBuffer;
    std::unique_ptr<AudioEncoder> _encoder;
    std::unique_ptr<AudioDecoder> _decoder;
    bool                          _formatSent{false};
    std::mutex                    _mutex;
    std::vector<float>            _voiceQueueSend;
    std::deque<PlayItem>          _voiceQueueRecv;
    int64_t                       _lastRecvVoiceSTs{-1};
    int64_t                       _lastRecvEventSTs{-1};
};

}



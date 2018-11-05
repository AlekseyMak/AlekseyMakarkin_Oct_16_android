//
// Created by Andrey Ryabov on 12/4/17.
//

#include "AsioHttp.h"
#include "Logger.h"
#include <regex>

namespace sky {

using namespace std;
using namespace aio;
using namespace std::placeholders;

ResolvePromise::ResolvePromise(io_service & ios) : AsioPromise(ios), _resolver(ios) {
}

ResolvePromise & ResolvePromise::resolve(const std::string & host, int port) {
    _pending++;
    tcp::resolver::query q(host, toStr(port));
    _resolver.async_resolve(q, [=](const aio::error_code & err, tcp::resolver::iterator it) {
        _pending--;
        if (err) {
            Err<<"failed to resolve: "<<host<<", "<<err<<eol;
        } else {
            if (it != tcp::resolver::iterator()) {
                Trx<<"name resolved: "<<host<<":"<<port<<" - "<<tcp::endpoint(*it)<<eol;
                AsioPromise::resolve(*it);
            }
        }
    });
    return * this;
}

void ResolvePromise::cancel() {
    _resolver.cancel();
}

ConnectPromise::ConnectPromise(io_service & c, tcp::socket & s) : AsioPromise<>(c), _socket(s) {
}

ConnectPromise & ConnectPromise::connect(const tcp::endpoint & e) {
    _pending++;
    _socket.async_connect(e, [=](const error_code & ec) {
        _pending--;
        if (ec) {
            Err<<"failed to connect to: "<<ec<<" - "<<e<<eol;
            reject(ec.message());
        } else {
            resolve();
        }
    });
    return * this;
}

SSLHandshakePromise::SSLHandshakePromise(io_service & io, ssl::stream<tcp::socket> & s) : AsioPromise(io), _stream(s) {
}

SSLHandshakePromise & SSLHandshakePromise::handshake() {
    _pending++;
    _stream.async_handshake(ssl::stream_base::client, [=](const error_code & ec) {
        _pending--;
        if (ec) {
            Err<<"failed to handshake: "<<ec<<eol;
            reject(ec.message());
        } else {
            resolve();
        }
    });
    return * this;
}


WSSHandshakePromise::WSSHandshakePromise(aio::io_service & i, WebSocket & s) : AsioPromise(i), _stream(s) {
}

WSSHandshakePromise & WSSHandshakePromise::handshake(string host, string url, string bearer) {
    _pending++;
    _stream.async_handshake_ex(host, url,
        [=](request_type & m) {
            if (bearer.size()) {
                m.insert("Authorization", "Bearer " + bearer);
            }
        },
        [=](const error_code & ec) {
            _pending--;
            if (ec) {
                Err<<"failed to handshake: "<<host<<url<<" - "<<ec<<eol;
                reject(ec.message());
            } else {
                resolve();
            }
        });
    return * this;
}

void HttpSession::execute() {
    if (!SSL_set_tlsext_host_name(_stream.native_handle(), request.host.c_str())) {
        Err<<"SSL_set_tlsext_host_name failed"<<eol;
        return;
    }
    _self = shared_from_this();
    async_connect(_stream.next_layer(),
            &endpoint, &endpoint + 1,
            std::bind(&HttpSession::onConnected, this, _1));
}

HttpSession::HttpSession(io_service & ios, ssl::context & ssl)
    : AsioPromise(ios), _ssl(ssl), _stream(ios, _ssl) {
}

void HttpSession::onConnected(const error_code & ec) {
    if (ec) {
        Err<<"failed to connect: "<<ec.message()<<eol;
        _self.reset();
        return;
    }
    _stream.async_handshake(stream_base::client, std::bind(&HttpSession::onHandshake, this, _1));
}

void HttpSession::onHandshake(const aio::error_code & ec) {
    if (ec) {
        Err<<"handshake failed: "<<ec.message()<<eol;
        _self.reset();
        return;
    }

    _req.version(11);
    if (request.method == "GET") {
        _req.method(http::verb::get);
    } else if (request.method == "PUT") {
        _req.method(http::verb::put);
    } else if (request.method == "POST") {
        _req.method(http::verb::post);
    } else {
        throw Exception EX("invalid method: " + toStr(request.method));
    }
    _req.target(request.query);
    _req.set(field::host, request.host);
    _req.set(field::user_agent, "Synqq Browser");
    _req.set(field::content_length, request.body.size());
    for (auto & p : request.headers) {
        _req.set(p.first, p.second);
    }
    _req.body() = request.body;

    async_write(_stream, _req, std::bind(&HttpSession::onWrite, this, _1, _2));
}

void HttpSession::onWrite(const aio::error_code & ec, size_t written) {
    ignore_unused(written);
    if (ec) {
        Err<<"write failed: "<<ec.message()<<eol;
        _self.reset();
        return;
    }
    async_read(_stream, _buffer, _res, std::bind(&HttpSession::onRead, this, _1, _2));
}

void HttpSession::onRead(const aio::error_code & ec, size_t read) {
    boost::ignore_unused(read);
    if (ec) {
        Err<<"read failed: "<<ec.message()<<eol;
        _self.reset();
        return;
    }
    response.code    = _res.result_int();
    response.message = string(_res.reason());
    response.body    = _res.body();
    resolve(this);
    _stream.async_shutdown(std::bind(&HttpSession::onShutdown, this, _1));
}

void HttpSession::onShutdown(const aio::error_code & ec) {
    _self.reset();
    if (ec == boost::asio::error::eof) {
        // Rationale:
        // http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
    }
    if (ec) {
        Trx<<"shutdown failed: "<<ec.message()<<eol;
    }
}

Ptr<HttpSession> HttpService::execute(const HttpRequest & request) {
    if (request.host.empty()) {
        throw Exception EX("invalid host: " + toStr(request.host));
    }
    auto session = ptr(new HttpSession(_ios, _ssl));
    session->request  = request;
    resolve(request.host, request.port).then([=](tcp::endpoint ep) {
        session->endpoint = ep;
        session->execute();
    });
    return session;
}

HttpService::HttpService(aio::io_service & ios) : _ios(ios), _ssl(aio::context::sslv23_client) {
}

ResolvePromise & HttpService::resolve(const std::string & host, int port) {
    auto key = host + ":" + toStr(port);
    auto it = _resolvers.find(key);
    if (it == _resolvers.end()) {
        auto p = uptr(new ResolvePromise(_ios));
        p->resolve(host, port);
        _resolvers[key] = std::move(p);
    }
    return *_resolvers[key];
}

BingSecretToken::BingSecretToken(aio::io_service & io) : _http(io), _apiSecret(false), _timer(io) {
    _host = "api.cognitive.microsoft.com";
}

Promise<string> & BingSecretToken::get() {
    return _apiSecret;
}

void BingSecretToken::setApiKey(string apiKey) {
    _apiKey = apiKey;
}

void BingSecretToken::setHost(string host) {
    _host = host;
}

void BingSecretToken::start() {
    assert(_host.size());
    assert(_apiKey.size());
    setTimer(posix_time::seconds(0));
}

void BingSecretToken::setTimer(posix_time::time_duration after) {
    if (_timerActive) {
        return;
    }
    _timerActive = true;
    _timer.expires_from_now(after);
    _timer.async_wait([=](const error_code & e) {
        _timerActive = false;
        if (e) {
            Err<<"timer failed: "<<e.message()<<eol;
        } else {
            HttpRequest request;
            request.method = "POST";
            request.host   = _host;
            request.port   = 443;
            request.query  = "/sts/v1.0/issueToken";
            request.headers["Content-Type"] = "application/x-www-form-urlencoded";
            request.headers["Ocp-Apim-Subscription-Key"] = _apiKey;
            _http.execute(request)->then([=](const HttpSession * s) {
                if (s->response.code == 200) {
                    Trx<<"Bing API secret token updated: "<<s->response.body<<eol;
                    _apiSecret.resolve(s->response.body);
                } else {
                    Err<<"Error getting Bing secret token: "<<s->response.code<<" "<<s->response.message<<eol;
                }
            });
        }
        setTimer(posix_time::minutes(1));
    });
}

//----- class: WebSocket ---------------------------------------------------------------
static const std::regex WSS_URL_REGEX("ws(s)?://([^:/]*)(?::(\\d+))?(/.*)?", std::regex::ECMAScript);

WebSocket::~WebSocket() {
    assert(_state == State::Idle || _state == State::Closed);
}

WebSocket::WebSocket(aio::io_service & ios, string url)
    : _ios(ios), _url(url), _ssl(aio::ssl::context::sslv23),
      _resolver(_ios), _wss(_ios, _ssl), _connect(_ios, _wss.next_layer().next_layer()),
      _sslHandshake(_ios, _wss.next_layer()),
      _wssHandshake(_ios, _wss) {

    auto rescuer = [=](const string & err) {
        handleError(err);
    };
    _connect.rescue(rescuer);
    _resolver.rescue(rescuer);
    _sslHandshake.rescue(rescuer);
    _wssHandshake.rescue(rescuer);

    _resolver.then([=](const aio::tcp::endpoint & e) {
        _connect.connect(e);
    });
    _connect.then([=]() {
        _sslHandshake.handshake();
    });
    _sslHandshake.then([=]() {
        _wssHandshake.handshake(_host, _path);
    });
    _wssHandshake.then([=]() {
        setState(State::Connected);
        doRead();
    });
    _readBuffer.prepare(10 * 1024);
}

void WebSocket::doRead() {
    if (_state != State::Connected) {
        return;
    }
    _reading = true;
    _wss.async_read(_readBuffer, [=](aio::error_code ec, size_t transfered) {
        _reading = false;
        if (ec) {
            handleError(ec.message());
            return;
        }
        if (_wss.got_text()) {
            auto text = buffers_to_string(_readBuffer.data());
            if (onText) {
                onText(std::move(text));
            }
        } else if (_wss.got_binary()) {
            vector<uint8_t> data(_readBuffer.size());
            auto dataBuffer = buffer(data);
            buffer_copy(dataBuffer, _readBuffer.data());
            if (onBinary) {
                onBinary(std::move(data));
            }
        } else {
            Err<<"invalid message type"<<eol;
        }
        _readBuffer.consume(_readBuffer.size());
        doRead();
    });
}

void WebSocket::setState(State state) {
    if (_state != state) {
        _state = state;
        if (onState) {
            _ios.post([=] {
                onState(state);
            });
        }
    }
}

void WebSocket::handleError(const string & err) {
    if (_state == State::Closing) {
        if (!isBusy()) {
            setState(State::Closed);
        }
        return;
    }
    if (onError) {
        onError(err);
    }
    stop();
}

WebSocket::State WebSocket::state() const {
    return _state;
}

void WebSocket::connect() {
    if (_state != State::Idle) {
        Err<<"connect in wrong state: "<<_state<<eol;
        return;
    }
    std::smatch sm;
    if (!std::regex_match(_url, sm, WSS_URL_REGEX)) {
        handleError("invalid url: " + _url);
        return;
    }
    _host  = sm[2].str();
    _path  = sm[4].str();
    auto s = sm[1].str();
    auto p = 443;
    if (sm[3].length()) {
        p = stoi(sm[3].str());
    }
    setState(State::Connecting);
    _resolver.resolve(_host, p);
}

bool WebSocket::isBusy() {
    return _reading || _writting
        || _resolver.isPending()
        || _connect.isPending()
        || _sslHandshake.isPending()
        || _wssHandshake.isPending();
}

void WebSocket::stop() {
    if (_state == State::Closing || _state == State::Closed) {
        Err<<"already stopping socket"<<eol;
        return;
    }
    Trx<<"stopping websocket"<<eol;
    setState(State::Closing);
    _resolver.cancel();
    auto & sock = _wss.next_layer().next_layer();
    if (sock.is_open()) {
        sock.close();
    }
    if (!isBusy()) {
        setState(State::Closed);
    }
}

void WebSocket::write(std::string text) {
    _writeQueue.push(Payload(std::move(text)));
    doWrite();
}

void WebSocket::write(std::vector<uint8_t> data) {
    _writeQueue.push(Payload(std::move(data)));
    doWrite();
}

void WebSocket::doWrite() {
    if (_writting) {
        return;
    }
    if (_writeQueue.empty() || _state != State::Connected) {
        return;
    }
    auto cb = [=](const error_code & e, size_t size) {
        _writting = false;
        _writeQueue.pop();
        if (e) {
            handleError(e.message());
        } else {
            doWrite();
        }
    };
    _writting = true;
    auto & f = _writeQueue.front();
    if (f.binary) {
        _wss.binary(true);
        _wss.async_write(buffer(f.data), cb);
    } else {
        _wss.text(true);
        _wss.async_write(buffer(f.text), cb);
    }
}

std::ostream & operator << (std::ostream & out, WebSocket::State state) {
    if (state == WebSocket::State::Idle) {
        return out<<"Idle";
    }
    if (state == WebSocket::State::Connecting) {
        return out<<"Connecting";
    }
    if (state == WebSocket::State::Connected) {
        return out<<"Connected";
    }
    if (state == WebSocket::State::Closing) {
        return out<<"Closing";
    }

    if (state == WebSocket::State::Closed) {
        return out<<"Closed";
    }
    return out<<"#"<<int(state);
}


}

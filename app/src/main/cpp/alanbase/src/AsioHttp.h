//
// Created by Andrey Ryabov on 12/4/17.
//

#ifndef SKYBASE_ASIOHTTP_H
#define SKYBASE_ASIOHTTP_H

#include "Exception.h"
#include "Utils.h"

#include <queue>
#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/beast.hpp>
#include <boost/beast/websocket/ssl.hpp>

namespace sky {

namespace aio {
    using namespace boost;
    using namespace beast;
    using namespace asio;
    using namespace http;
    using namespace websocket;
    using namespace ip;
    using namespace ssl;
    using error_code = system::error_code;
}

template<typename... P_>
class Promise {
  public:
    template<typename F_>
    void then(F_ && f) {
        if (_complete) {
            _invoke(f);
        } else {
            _resolve.emplace_back(std::move(f));
        }
    }

    template<typename F_>
    void rescue(F_ && f) {
        if (isRejected()) {
            f(_error);
        } else {
            _rescue.emplace_back(std::move(f));
        }
    }

    void reject(const std::string & err) {
        checkFinalState();
        _error = err;
        _ts = now();
        for (auto & f : _rescue) {
            f(err);
        }
        _rescue.clear();
        _resolve.clear();
    }

    void resolve(P_...  p) {
        checkFinalState();
        _complete = true;
        _ts = now();
        _invoke = [=](std::function<void(P_... args)> f) { f(p...); };
        for (auto & f : _resolve) {
            _invoke(f);
        }
        _rescue.clear();
        _resolve.clear();
    }

    Timestamp ts() const {
        return _ts;
    }

    bool isRejected() const {
        return !_error.empty();
    }

    bool isComplete() const {
        return _complete;
    }

    Promise(bool final = true) : _final(final) {
    }
  private:
    void checkFinalState() {
        if (_final) {
            if (_error.size()) {
                throw Exception EX("promise already rejected");
            }
            if (_complete) {
                throw Exception EX("promise already complete");
            }
        } else {
            _complete = false;
            _error.clear();
        }
    }

    Timestamp                                             _ts{now()};
    bool                                                  _final;
    bool                                                  _complete{false};
    std::string                                           _error;
    std::function<void(std::function<void(P_...)>)>       _invoke;
    std::vector<std::function<void(P_...)>>               _resolve;
    std::vector<std::function<void(const std::string &)>> _rescue;

};

template<typename... P_>
class AsioPromise : public Promise<P_...> {
  public:
    bool isPending() const {
        return _pending > 0;
    }

    AsioPromise(aio::io_service & ios) : Promise<P_...>(false), _ios(ios) {
    }
  protected:
    int _pending{0};
    boost::asio::io_service & _ios;
};

class ResolvePromise : public AsioPromise<aio::tcp::endpoint> {
  public:
    ResolvePromise & resolve(const std::string & host, int port);

    void cancel();

    ResolvePromise(aio::io_service &);
  private:
    aio::tcp::resolver _resolver;
};

class ConnectPromise : public AsioPromise<> {
  public:
    ConnectPromise & connect(const aio::tcp::endpoint &);

    ConnectPromise(aio::io_service &, aio::tcp::socket &);
  private:
    aio::tcp::socket & _socket;
};

class SSLHandshakePromise : public AsioPromise<> {
  public:
    SSLHandshakePromise & handshake();

    SSLHandshakePromise(aio::io_service &, aio::ssl::stream<aio::tcp::socket> &);
  private:
    aio::ssl::stream<aio::tcp::socket> & _stream;
};

class WSSHandshakePromise : public AsioPromise<> {
  public:
    typedef aio::websocket::stream<aio::ssl::stream<aio::tcp::socket>> WebSocket;

    WSSHandshakePromise & handshake(std::string host, std::string url, std::string bearer="");

    WSSHandshakePromise(aio::io_service &, WebSocket &);
  private:
    WebSocket & _stream;
};

class HttpRequest {
  public:
    std::string method{"GET"};
    std::string host;
    int         port{443};
    std::string query;
    std::map<std::string, std::string> headers;
    std::string body;
};

class HttpResponse {
  public:
    int code;
    std::string message;
    std::map<std::string, std::string> headers;
    std::string body;
};

class HttpSession :
     public AsioPromise<const HttpSession *>,
     public std::enable_shared_from_this<HttpSession> {
  public:
    aio::tcp::endpoint endpoint;
    HttpRequest        request;
    HttpResponse       response;

    void execute();

    HttpSession(aio::io_service &, aio::context &);
  private:
    void onConnected(const aio::error_code &);
    void onHandshake(const aio::error_code &);
    void onWrite    (const aio::error_code &, size_t);
    void onRead     (const aio::error_code &, size_t);
    void onShutdown (const aio::error_code &);

    Ptr<HttpSession>                   _self;
    aio::context &                     _ssl;
    aio::request<aio::string_body>     _req;
    aio::response<aio::string_body>    _res;
    aio::flat_buffer                   _buffer;
    aio::ssl::stream<aio::tcp::socket> _stream;
};

class HttpService {
  public:
    Ptr<HttpSession> execute(const HttpRequest &);

    HttpService(aio::io_service &);
  private:
    ResolvePromise  & resolve(const std::string & host, int port);
    aio::io_service & _ios;
    aio::context      _ssl;
    std::map<std::string, UPtr<ResolvePromise>> _resolvers;
};

class BingSecretToken {
  public:
    Promise<std::string> & get();
    void setHost(std::string);
    void setApiKey(std::string);
    void start();

    BingSecretToken(aio::io_service &);
  private:
    void setTimer(aio::posix_time::time_duration);

    std::string          _apiKey;
    std::string          _host;
    HttpService          _http;
    Promise<std::string> _apiSecret;
    aio::deadline_timer  _timer;
    bool                 _timerActive{false};
};

class WebSocket {
  public:
    enum class State {Idle = 0, Connecting = 1, Connected = 2, Closing = 3, Closed = 4};

    std::function<void(State)>                   onState;
    std::function<void(const std::string &)>     onError;
    std::function<void(std::string &&)>          onText;
    std::function<void(std::vector<uint8_t> &&)> onBinary;

    State state() const;
    void  write(std::string);
    void  write(std::vector<uint8_t>);
    void  connect();
    void  stop();

    ~WebSocket();
    WebSocket(aio::io_service &ios, std::string url);
  private:
    typedef aio::ssl::stream<aio::tcp::socket> SSLStream;
    typedef aio::websocket::stream<SSLStream> AsioWebSocket;

    void handleError(const std::string &);
    void setState(State);
    void doRead();
    void doWrite();
    bool isBusy();

    struct Payload {
        bool binary;
        std::string text;
        std::vector<uint8_t> data;

        Payload(std::string s) : binary(false), text(move(s)) {}
        Payload(std::vector<uint8_t> s) : binary(true), data(move(s)) {}
    };

    State                   _state{State::Idle};
    aio::io_service       & _ios;
    std::string             _url;
    std::string             _host;
    std::string             _path;
    aio::ssl::context       _ssl;
    ResolvePromise          _resolver;
    AsioWebSocket           _wss;
    ConnectPromise          _connect;
    SSLHandshakePromise     _sslHandshake;
    WSSHandshakePromise     _wssHandshake;
    aio::multi_buffer       _readBuffer;
    std::queue<Payload>     _writeQueue;
    bool                    _reading{false};
    bool                    _writting{false};
};

std::ostream & operator << (std::ostream &, WebSocket::State);

}

#endif //SKYBASE_ASIOHTTP_H

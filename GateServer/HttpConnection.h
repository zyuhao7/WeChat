#pragma once
#include "const.h"

class HttpConnection : public std::enable_shared_from_this<HttpConnection>
{
    friend class LogicSystem;

public:
    HttpConnection(boost::asio::io_context& ioc);
    void Start();
    tcp::socket& GetSocket()
    {
        return _socket;
    }
    std::string RequestBody() const
    {
        return beast::buffers_to_string(_request.body().data());
    }
    void SetContentType(const std::string& type)
    {
        _response.set(http::field::content_type, type);
    }
    void AppendBody(const std::string& data)
    {
        beast::ostream(_response.body()) << data;
    }

private:
    void CheckDeadLine();
    void WriteResponse();
    void HandleReq();
    void Dispatch();
    void WriteStatus(http::status status, const std::string& text);
    tcp::socket _socket;
    beast::flat_buffer _buffer{8192};
    http::request<http::dynamic_body> _request;
    http::response<http::dynamic_body> _response;
    net::steady_timer deadline_{
        _socket.get_executor(), std::chrono::seconds(60)};

    std::string _get_url;
    std::unordered_map<std::string, std::string> _get_params;
};

#include "HttpConnection.h"
#include "LogicSystem.h"
#include "HttpParse.h"

HttpConnection::HttpConnection(boost::asio::io_context& ioc)
    : _socket(ioc)
{
}
void HttpConnection::Start()
{
    auto self = shared_from_this(); // ensure the lifetime
    // armed before the read so a client that never finishes its request is dropped
    CheckDeadLine();
    http::async_read(_socket, _buffer, _request, [self](beast::error_code ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);
        if (ec)
        {
            std::cout << " http read  err is " << ec.message() << std::endl;
            self->deadline_.cancel();
            return;
        }
        self->HandleReq();
    });
}

void HttpConnection::HandleReq()
{
    // set the version
    _response.version(_request.version());
    _response.keep_alive(false);
    _response.set(http::field::server, "GateServer");
    try
    {
        Dispatch();
    }
    catch (std::exception& ep)
    {
        std::cout << "handle request exception is " << ep.what() << std::endl;
        _response.body().consume(_response.body().size());
        _response.set(http::field::content_type, "text/json");
        _response.result(http::status::internal_server_error);
        Json::Value root;
        root["error"] = ErrorCodes::Error_Json;
        beast::ostream(_response.body()) << root.toStyledString();
    }
    WriteResponse();
}

void HttpConnection::Dispatch()
{
    auto method = _request.method();
    if (method != http::verb::get && method != http::verb::post)
    {
        _response.set(http::field::allow, "GET, POST");
        WriteStatus(http::status::method_not_allowed, "method not allowed\r\n");
        return;
    }

    ParseTarget(std::string(_request.target()), _get_url, _get_params);
    bool success = method == http::verb::get
                       ? LogicSystem::GetInstance()->HandleGet(_get_url, shared_from_this())
                       : LogicSystem::GetInstance()->HandlePost(_get_url, shared_from_this());
    if (!success)
    {
        WriteStatus(http::status::not_found, "url not found\r\n");
        return;
    }
    _response.result(http::status::ok);
}

void HttpConnection::WriteStatus(http::status status, const std::string& text)
{
    _response.result(status);
    _response.set(http::field::content_type, "text/plain");
    beast::ostream(_response.body()) << text;
}

void HttpConnection::CheckDeadLine()
{
    auto self = shared_from_this();
    deadline_.async_wait([self](beast::error_code ec) {
        if (!ec)
        {
            self->_socket.close(ec);
        }
    });
}
void HttpConnection::WriteResponse()
{
    auto self = shared_from_this();
    _response.content_length(_response.body().size());
    http::async_write(_socket, _response, [self](beast::error_code ec, std::size_t bytes_transferred) {
        self->_socket.shutdown(tcp::socket::shutdown_send, ec);
        self->deadline_.cancel();
    });
}
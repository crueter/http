module;

#include <string>

export module http;
export import :socket;

export namespace http
{

struct RequestOptions {
    Version version = Version::HTTP_1_1;
    Header headers;
    std::string body;
    UserCallback callback;
    ConnectOptions connect;
};



} // namespace http
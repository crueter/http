module;

#include <format>
#include <string>
#include <string_view>

export module http:enums;

export namespace http
{

// Basic API
enum class Status {
    NoStatusCode = 0,

    Continue = 100,
    SwitchingProtocols = 101,
    Processing = 102,
    EarlyHints = 103,

    Ok = 200,
    Created = 201,
    Accepted = 202,
    NonAuthoritativeInformation = 203,
    NoContent = 204,
    ResetContent = 205,
    PartialContent = 206,
    MultiStatus = 207,
    AlreadyReported = 208,
    ImUsed = 226,
    MultipleChoices = 300,
    MovedPermanently = 301,
    Found = 302,
    SeeOther = 303,
    NotModified = 304,
    UseProxy = 305,
    TemporaryRedirect = 307,
    PermanentRedirect = 308,

    BadRequest = 400,
    Unauthorized = 401,
    PaymentRequired = 402,
    Forbidden = 403,
    NotFound = 404,
    MethodNotAllowed = 405,
    NotAcceptable = 406,
    ProxyAuthenticationRequired = 407,
    RequestTimeout = 408,
    Conflict = 409,
    Gone = 410,
    LengthRequired = 411,
    PreconditionFailed = 412,
    PayloadTooLarge = 413,
    UriTooLong = 414,
    UnsupportedMediaType = 415,
    RangeNotSatisfiable = 416,
    ExpectationFailed = 417,
    ImATeapot = 418,
    MisdirectedRequest = 421,
    UnprocessableEntity = 422,
    Locked = 423,
    FailedDependency = 424,
    TooEarly = 425,
    UpgradeRequired = 426,
    PreconditionRequired = 428,
    TooManyRequests = 429,
    RequestHeaderFieldsTooLarge = 431,
    UnavailableForLegalReasons = 451,

    InternalServerError = 500,
    NotImplemented = 501,
    BadGateway = 502,
    ServiceUnavailable = 503,
    GatewayTimeout = 504,
    HTTPVersionNotSupported = 505,
    VariantAlsoNegotiates = 506,
    InsufficientStorage = 507,
    LoopDetected = 508,
    NotExtended = 510,
    NetworkAuthenticationRequired = 511
};

enum class Protocol { NoProtocol = 0, HTTP, HTTPS };

enum class Method { GET, POST, PUT, HEAD, OPTIONS };

enum class TransferMode {
    NoData = 0,
    Sized = 1,
    Chunked = 2,
};

enum class Version { HTTP_1_0 = -1, HTTP_1_1 = 0 };

// Connection errors.
enum class Error {
    // No error
    Success,

    // Failed DNS lookup
    DNSFailure,

    // Failed to create socket
    SocketCreate,

    // Connection to IP failed
    ConnectFailed,

    // Invalid TLS context
    TLSCtx,

    // Failed to initialize TLS
    TLSInit,

    // Failed to grab certificate
    TLSCert,

    // Failed to verify certificate
    TLSVerify,

    // Invalid certificate passed in
    InvalidCert,

    // Failed to send data
    SendFailed,

    // Failed to read data
    ReadFailed,

    // Headers are way too large to handle
    HeaderTooBig,

    // Malformed or otherwise invalid chunk data
    InvalidChunk,
};

// Connection options
struct ConnectOptions {
    // Whether or not to verify the server certificate.
    bool verify = true;

    // A custom certificate to load.
    std::string cert = "";

    // Disable SSL entirely.
    bool disableSsl = false;
};

} // namespace http

// formatter specs
export {

    template <>
    struct std::formatter<http::Error> : std::formatter<std::string_view> {
        auto format(const http::Error e, std::format_context &ctx) const
        {
            std::string_view name;
            switch (e) {
            case http::Error::Success:
                name = "No error";
                break;
            case http::Error::DNSFailure:
                name = "DNS lookup failure";
                break;
            case http::Error::SocketCreate:
                name = "OS failed to create socket";
                break;
            case http::Error::ConnectFailed:
                name = "Failed to connect to host";
                break;
            case http::Error::TLSCtx:
                name = "Failed to initialize TLS context";
                break;
            case http::Error::TLSInit:
                name = "Failed to initialize TLS state";
                break;
            case http::Error::TLSCert:
                name = "Server returned no or invalid certificate";
                break;
            case http::Error::TLSVerify:
                name = "Failed to verify server certificate";
                break;
            case http::Error::InvalidCert:
                name = "Invalid custom certificate passed";
                break;
            case http::Error::SendFailed:
                name = "Connection closed or failed during send";
                break;
            case http::Error::ReadFailed:
                name = "Connection closed or failed during read";
                break;
            case http::Error::InvalidChunk:
                name = "Invalid or malformed chunk data received";
                break;
            case http::Error::HeaderTooBig:
                name = "Response header is too large";
                break;
            }

            return std::formatter<std::string_view>::format(name, ctx);
        }
    };
}

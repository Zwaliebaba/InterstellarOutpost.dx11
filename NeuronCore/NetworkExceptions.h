#pragma once

class PeerNotFoundException : public BaseException
{
  public:
    PeerNotFoundException(const std::string& _s)
      : BaseException(_s) {}
};

class ConnectionException : public BaseException
{
  public:
    ConnectionException(const std::string& _s)
      : BaseException(_s) {}
};

class ConnectionBindFailed : public BaseException
{
  public:
    ConnectionBindFailed(const std::string& _s)
      : BaseException(_s) {}
};

class InvalidIncomingDataException : public BaseException
{
  public:
    InvalidIncomingDataException(const std::string& _s)
      : BaseException(_s) {}
};

class SocketException : public BaseException
{
  public:
    SocketException(const std::string& _s)
      : BaseException(_s) {}
};

class ResolveError : public BaseException
{
  public:
    ResolveError(const std::string& _s)
      : BaseException(_s) {}
};

class SendFailedException : public BaseException
{
  public:
    SendFailedException(const std::string& _s)
      : BaseException(_s) {}
};

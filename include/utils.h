#pragma once

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace ba = boost::asio;
using tcp = ba::ip::tcp;
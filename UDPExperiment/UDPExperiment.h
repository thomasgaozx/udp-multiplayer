// UDPExperiment.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <iostream>
#include <boost/system/error_code.hpp>
#include <boost/asio.hpp>

using boost::asio::ip::udp;

class UDPServer {
public:
	UDPServer(boost::asio::io_service& io_service) : _socket(io_service, udp::endpoint(udp::v4(), 1111)) {
	
	}



private:
	udp::socket _socket;
	udp::endpoint _remoteEndpoint;
	std::array<char, 1024> _recvBuffer;
};

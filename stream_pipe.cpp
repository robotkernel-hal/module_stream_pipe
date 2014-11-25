//! robotkernel module for stream_pipe streams
/*!
 * author: Florian Schmidt
 *
 * $Id$
 */

/*
 * This file is part of robotkernel.
 *
 * robotkernel is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * robotkernel is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with robotkernel.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "stream_pipe.h"
#include "robotkernel/helpers.h"
#include "robotkernel/kernel.h"
#include "robotkernel/exceptions.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/select.h>
#include <errno.h>
#include <sys/stat.h>

#ifdef __VXWORKS__
#include <ioLib.h>
#include <sioLib.h>
#include <strings.h>
#endif

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <netdb.h>

		  
#include <string_util/string_util.h>

using namespace std;
using namespace robotkernel;
using namespace module_stream_pipe;

#define str_exception ::str_exception

stream_pipe::stream_pipe(const char *name, const YAML::Node& node) {
	this->name = name;

	// init state
	state = module_state_init;
	server_fd = -1;
	fd = -1;

	const YAML::Node *value;

	mode = node["mode"].to<std::string>();
	if(mode == "server") {
		listening_port = node["listening_port"].to<unsigned>();
		mode_desc = format_string("server on port %d", listening_port);
	}
	else if(mode == "client") {
		if((value = node.FindValue("peer"))) {
			string peer = (*value).to<string>();
			string::size_type p = peer.find(":");
			if(p == string::npos)
				throw str_exception_tb("peer spec %s does not include port! needs to look like: hostname:portnumber", repr(peer).c_str());
			peer_hostname = peer.substr(0, p);
			peer_port = atoi(peer.substr(p+1).c_str());
		} else {
			peer_hostname = node["peer_hostname"].to<string>();
			peer_port = node["peer_port"].to<unsigned>();
		}
		mode_desc = format_string("client to peer %s:%d", peer_hostname.c_str(), peer_port);
	} else
		throw str_exception_tb("invalid/unknown mode: %s. needs to be either server or client!", repr(mode).c_str());
	
	if((value = node.FindValue("read_timeout")))
		read_timeout = (*value).to<double>();
	else
		read_timeout = -1; // blocking no timeout
}

stream_pipe::~stream_pipe() {
	set_state(module_state_init);
}

void set_timeout(struct timeval* tv, double timeout) {
	tv->tv_sec = (unsigned long)timeout;
	tv->tv_usec = (unsigned long)((timeout - tv->tv_sec) * 1e6);
}


ssize_t stream_pipe::read(char *data, size_t data_len) {
	if (state < module_state_safeop) // invalid state
		return -1;

	if(fd == -1) {
		if(mode == "client") {
			log(warning, "read: there is no connection to server!");
			set_state(module_state_init);
			return -1;
		}
		// server has to wait for incoming client connection
		while(read_timeout != -1) {
			fd_set readset;
			FD_ZERO(&readset);
			FD_SET(server_fd, &readset);
			timeval timeout;
			set_timeout(&timeout, read_timeout);			
			int rc = select(server_fd + 1, &readset, NULL, NULL, &timeout);
			if (rc == -1) {
				if (errno == EINTR)
					continue;
				log(warning, "select returned %s\n", strerror(errno));
				return -1;
			} else if (rc == 0) {
				log(info, "reading from stream_pipe timed out\n");
				return 0;
			}
			break;
		}
		// accept client connection!
		struct sockaddr_in client;
		socklen_t client_len = sizeof(client);
		fd = accept(server_fd, (struct sockaddr*)&client, &client_len);
		if(fd == -1)
			throw str_exception_tb("stream_pipe(%s)::read(%d) failed to accept new client!", name.c_str(), data_len);
	}
	
	while(read_timeout != -1) {
		fd_set readset;
		FD_ZERO(&readset);
		FD_SET(fd, &readset);
		timeval timeout;
		set_timeout(&timeout, read_timeout);			
		int rc = select(fd + 1, &readset, NULL, NULL, &timeout);
		if (rc == -1) {
			if (errno == EINTR)
				continue;
			log(warning, "select returned %s\n", strerror(errno));
			return -1;
		} else if (rc == 0) {
			log(info, "reading from stream_pipe stream timed out\n");
			return 0;
		}
		break;
	}
	
	int ret = recv(fd, data, data_len, 0);
	if(ret == -1)
		log(warning, "recv: %d %s", errno, strerror(errno));
	if(ret == 0)
		log(warning, "recv: eof from stream!");
	if(ret <= 0) {
		if(mode == "client") {
			set_state(module_state_init);
		} else {
			log(warning, "closing connection to client.");
			close(fd);
			fd = -1;
		}
		return ret;
	}
	log(info, "read %d bytes\n", ret);
	return ret;		
}

ssize_t stream_pipe::write(char *data, size_t data_len) {
	if(state < module_state_op)
		// invalid state
		return 0;
	if(fd == -1)
		return -1; // no connection!
	
	int ret = send(fd, data, data_len, 0);
	if(ret == -1) {
		log(warning, "send: %d %s", errno, strerror(errno));
		set_state(module_state_init);
		return -1;
	}
	return ret;		
}

void resolve_hostname(const char* hostname, struct sockaddr_in* sa) {
	// resolve hostname
	if(!hostname || hostname[0] == 0)
		throw str_exception_tb("empty hostname!");

	if(isdigit(hostname[0])) {
		// printf("hostname[0] is digit: '%s'\n", hostname);
		// assume dotted decimal notation
		int ret = inet_aton(hostname, &sa->sin_addr);
#ifndef __VXWORKS__
		if(ret) {
#else
		if(ret == 0) {
#endif
			// printf("it seems to be a dotted decimal ip: %s, ret: %d\n", hostname, ret);
			sa->sin_family = AF_INET;
			return; // otherwise try to resolve...
		}
	}		
#ifdef __WIN32__
	struct addrinfo *result = NULL;

	int ret = getaddrinfo(hostname, NULL, NULL, &result);
	if(ret)
		throw str_exception_tb("resolve_hostname: gai_strerror: %s\n", gai_strerror(ret));

	struct addrinfo *ptr = NULL;
	int found = 0;
	for(ptr = result; ptr != NULL; ptr = ptr->ai_next) {
		if (ptr->ai_family == AF_INET) {
			//memcpy(&sa->sin_addr, ptr->ai_addr, sizeof(sa->sin_addr));
			memcpy(&sa->sin_addr, &((struct sockaddr_in*)ptr->ai_addr)->sin_addr, sizeof(sa->sin_addr));
			sa->sin_family = AF_INET;
			found = 1;
			break;
		}
	}
	freeaddrinfo(result);
	if(!found)
		throw str_exception_tb("unknown hostname: %s", repr(hostname).c_str());
#else
	// struct hostent* he = gethostbyname(hostname);
	struct addrinfo hints;
	struct addrinfo* result = NULL;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	int ret;
	if((ret = getaddrinfo(hostname, NULL, &hints, &result)))
		throw str_exception_tb("unknown hostname: %s", repr(hostname).c_str());

	struct addrinfo *ptr = NULL;
	int found = 0;
	for(ptr = result; ptr != NULL; ptr = ptr->ai_next) {
		if (ptr->ai_family == AF_INET) {
			memcpy(&sa->sin_addr, &((struct sockaddr_in*)ptr->ai_addr)->sin_addr, sizeof(sa->sin_addr));
			sa->sin_family = AF_INET;
			found = 1;
			break;
		}
	}
	freeaddrinfo(result);
	if(!found)
		throw str_exception_tb("unknown hostname: %s", repr(hostname).c_str());
#endif
	return;
}

int stream_pipe::set_state(module_state_t state) {
	switch (state) {
        case module_state_init:
		if(fd > 0) {
			close(fd);
			fd = -1;
		}
		if(server_fd > 0) {
			close(server_fd);
			server_fd = -1;
		}
		break;
        case module_state_preop: {
		if(mode == "client") {
			fd = socket(AF_INET, SOCK_STREAM, 0);
			if(fd == -1)
				throw errno_exception_tb("socket");
		} else if(mode == "server") {
			server_fd = socket(AF_INET, SOCK_STREAM, 0);
			if(server_fd == -1)
				throw errno_exception_tb("socket");

			struct sockaddr_in peer_addr;
			peer_addr.sin_family = AF_INET;
			peer_addr.sin_port = htons(listening_port);
			peer_addr.sin_addr.s_addr = INADDR_ANY;
			
#ifdef __VXWORKS__
#elif __WIN32__
			char on = 1;
			if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
				throw errno_exception("setsockopt(SO_REUSEADDR)");
#else
			int on = 1;
			if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
				throw errno_exception("setsockopt(SO_REUSEADDR)");
#endif
			
			if(bind(server_fd, (struct sockaddr*) &peer_addr, sizeof(peer_addr)))
				throw errno_exception("bind");
			
			if(listen(server_fd, 1))
				throw errno_exception("listen");
		}
		break;
        }
        case module_state_safeop: {
		log(info, "opening stream_pipe stream %s ...\n", mode_desc.c_str());
		
		if(mode == "client") {
			// try to connect to server!			
			struct sockaddr_in peer_addr;
			peer_addr.sin_family = AF_INET;
			peer_addr.sin_port = htons(peer_port);
			resolve_hostname(peer_hostname.c_str(), &peer_addr);			

			int ret = connect(fd, (struct sockaddr*)&peer_addr, sizeof(peer_addr));
			if(ret == -1)
				throw errno_exception_tb("connect %s:%d", repr(peer_hostname).c_str(), peer_port);
		} else {
			
		}
		break;
	}
	case module_state_op:
        case module_state_boot:
		break;
        default:
		// invalid state
		return -1;
	}

	// assign new state
	this->state = state;

	return state;
}

int stream_pipe::request(int reqcode, void* ptr) {
	int ret = 0;

	switch (reqcode) {
        case MOD_REQUEST_GET_MODULE_FEAT: {
		int *mod_feat = (int *)ptr;
		*mod_feat = MODULE_FEAT_READ | MODULE_FEAT_WRITE;
		break;
        }
        default:
		log(verbose, "not implemented request %d\n", 
                    reqcode);
		ret = -1;
		break;
	}

	return ret;
}
        

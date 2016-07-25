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

stream_pipe::piper::piper(stream_pipe* parent) {
	this->parent = parent;
	m1 = NULL;
	m2 = NULL;
}
stream_pipe::piper::~piper() {
	stop();
}
void stream_pipe::piper::init() {
	stop();
	this->m1 = NULL;
	this->m2 = NULL;
}

void stream_pipe::piper::op(std::string mod1, robotkernel::module* m1, std::string mod2, robotkernel::module* m2) {
	this->mod1 = mod1;
	this->mod2 = mod2;
	this->m1 = m1;
	this->m2 = m2;
	start();
}

void stream_pipe::piper::run() {
	vector<char> buffer(parent->buffer_size);
	parent->log(verbose, "stream pipe reading from %s writing to %s running!\n", mod1.c_str(), mod2.c_str());
	while (running()) {
		ssize_t ret = m1->read(&buffer[0], parent->buffer_size);
		if(parent->debug) parent->log(info, "got %d bytes from %s\n", ret, mod1.c_str());

		if(ret <= 0) {
			parent->log(error, "read from %p returned %d!\n", m1, ret);
			usleep(100000);
			continue;
		}
		unsigned int to_write = (unsigned int)ret;
		unsigned int written = 0;

		if(mod2 == "STDOUT") {
			stringstream ss;
			unsigned int N = 16;
			while(written < to_write) {
				for(unsigned int i = written; i < written + N && i < to_write; ++i) {
					ss << format_string("%02x ", (unsigned int)buffer[i]);
				}
				for(unsigned int i = 0; i < (written + N) - to_write; ++i)
					ss << "   ";
				ss << " | ";
				for(unsigned int i = written; i < written + N && i < to_write; ++i) {
					char cp = buffer[i];
					if(isprint(cp))
						ss << format_string("%c", cp);
					else
						ss << ".";
				}
				ss << "\n";
				written += N;				
			}
			parent->log(info, "write to stdout:\n%s", ss.str().c_str());
			continue;
		}
		while(to_write > written) {
			if(parent->debug) parent->log(info, "write %d bytes to %s\n", to_write - written, mod2.c_str());
			ret = m2->write(&buffer[written], to_write - written);
			if(ret <= 0) {
				parent->log(warning, "write %d bytes to %p returned %d\n", to_write - written, m2, ret);
				usleep(100000);
				continue;
			}
			written += ret;
		}
	}
}

stream_pipe::stream_pipe(const std::string& name, const YAML::Node& node)
	: module_base("module_stream_pipe", name, node) {
	module1       = get_as<std::string>(node, "module1");
	module2       = get_as<std::string>(node, "module2");

	bidirectional = get_as<bool>(node, "bidirectional", true);
	buffer_size   = get_as<unsigned int>(node, "buffer_size", 1024);
	debug         = get_as<bool>(node, "debug", false);
	
	pipers.push_back(new piper(this));
	if(bidirectional)
		pipers.push_back(new piper(this));
}

stream_pipe::~stream_pipe() {
	set_state(module_state_init);
	for(unsigned int i = 0; i < 2; ++i)
		delete pipers[i];
}


int stream_pipe::set_state(module_state_t state) {
	switch (state) {
        case module_state_init:
		for(unsigned int i = 0; i < 2; ++i)
			pipers[i]->init();
		break;
	case module_state_preop:
		m1 = kernel::get_instance()->get_module(module1.c_str());
		if(!m1)
			throw str_exception_tb("failed to find module1: %s", repr(module1).c_str());
		if(module2 == "STDOUT")
			m2 = NULL;
		else {
			m2 = kernel::get_instance()->get_module(module2.c_str());
			if(!m2)
				throw str_exception_tb("failed to find module2: %s", repr(module2).c_str());
		}
		
		pipers[0]->op(module1, m1, module2, m2);
		if(bidirectional && module2 != "STDOUT")
			pipers[1]->op(module2, m2, module1, m1);
		break;
        case module_state_safeop:
		break;
	case module_state_op:
		break;
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
		*mod_feat = 0;
		break;
        }
        default:
		log(verbose, "not implemented request %d\n", reqcode);
		ret = -1;
		break;
	}

	return ret;
}
        

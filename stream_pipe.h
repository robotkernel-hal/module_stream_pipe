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

#ifndef __STREAM_PIPE_H__
#define __STREAM_PIPE_H__

#include "robotkernel/kernel.h"
#include "module_stream_pipe.h"
#include <string>
#include "yaml-cpp/yaml.h"

namespace module_stream_pipe {
#ifdef EMACS_IS_CLEVER
}
#endif

class stream_pipe {
public:
	std::string name;

	std::string mode; // server | client
	unsigned int listening_port; // for server
	std::string peer_hostname; // for client
	unsigned int peer_port; // for client
	double read_timeout;

	// state
	std::string mode_desc;
	module_state_t state;
	int server_fd; // for server
	int fd; // connection fd;

	stream_pipe(const char *name, const YAML::Node& node);
	~stream_pipe();

	int set_state(module_state_t state);
	ssize_t read(char *data, size_t data_len);
	ssize_t write(char *data, size_t data_len);
	int request(int reqcode, void* ptr);
        
	//! log to kernel logging facility
	void log(robotkernel::loglevel lvl, const char *format, ...) {
		char buf[1024];

		// format argument list
		va_list args;
		va_start(args, format);
		vsnprintf(buf, 1024, format, args);
		klog(lvl, "[%s|%s] %s", MODNAME, name.c_str(), buf);
	}
};

}; // namespace module_stream_pipe

#endif /* __STREAM_PIPE_H__ */


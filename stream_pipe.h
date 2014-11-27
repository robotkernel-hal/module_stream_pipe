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
#include "yaml-cpp/yaml.h"

#include <string>
#include <vector>

namespace module_stream_pipe {
#ifdef EMACS_IS_CLEVER
}
#endif

class stream_pipe {
public:
	std::string name;

	std::string module1;
	std::string module2;
	bool bidirectional;
	unsigned int buffer_size;
	bool debug;
	
	// state
	module_state_t state;
	robotkernel::module* m1;
	robotkernel::module* m2;

	stream_pipe(const char *name, const YAML::Node& node);
	~stream_pipe();

	class piper : public robotkernel::runnable {
		stream_pipe* parent;

		std::string mod1;
		std::string mod2;
		
		robotkernel::module* m1;
		robotkernel::module* m2;
	public:
		piper(stream_pipe* parent);
		~piper();
		void init();
		void op(std::string mod1, robotkernel::module* m1, std::string mod2, robotkernel::module* m2);

		void run();
	};
	typedef std::vector<piper*> pipers_t;
	pipers_t pipers;

	int set_state(module_state_t state);
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


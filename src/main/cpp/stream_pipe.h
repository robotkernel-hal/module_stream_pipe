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
#include "robotkernel/module_base.h"
#include "yaml-cpp/yaml.h"

#include <string>
#include <vector>

namespace module_stream_pipe {
#ifdef EMACS
}
#endif

class stream_pipe : 
    robotkernel::module_base 
{
    public:
        std::string stream_dev_name_1;
        std::string stream_dev_name_2;
        bool bidirectional;
        unsigned int buffer_size;
        bool debug;

        // state
        robotkernel::sp_stream_t stream_1;
        robotkernel::sp_stream_t stream_2;

    public:
        //! construction
        /*!
         * \param[in] name      Module instance name.
         * \param[in] node      Module configuration node.
         */
        stream_pipe(const std::string& name, const YAML::Node& node);
        ~stream_pipe();

        class piper : 
            public robotkernel::runnable 
        {
            private:
                stream_pipe* parent;

                robotkernel::sp_stream_t stream_1;
                robotkernel::sp_stream_t stream_2;

            public:
                piper(stream_pipe* parent);
                ~piper();

                void init();
                void op(robotkernel::sp_stream_t str_1, robotkernel::sp_stream_t str_2);

                void run();
        };

        typedef std::vector<piper*> pipers_t;
        pipers_t pipers;

        int set_state(module_state_t state);
        int request(int reqcode, void* ptr);
};

#ifdef EMACS
{
#endif
}; // namespace module_stream_pipe

#endif /* __STREAM_PIPE_H__ */


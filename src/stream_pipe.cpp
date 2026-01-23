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
#include "robotkernel/robotkernel.h"
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

using namespace std;
using namespace robotkernel;
using namespace robotkernel::helpers;
using namespace module_stream_pipe;

stream_pipe::piper::piper(stream_pipe* parent) {
    this->parent = parent;
    stream_1 = nullptr;
    stream_2 = nullptr;
}

stream_pipe::piper::~piper() {
    stop();
}

void stream_pipe::piper::init() {
    stop();
    stream_1 = nullptr;
    stream_2 = nullptr;
}

void stream_pipe::piper::op(robotkernel::sp_stream_t str_1, robotkernel::sp_stream_t str_2) {
    stream_1 = str_1;
    stream_2 = str_2;
    start();
}

void stream_pipe::piper::run() {
    vector<char> buffer(parent->buffer_size);
    parent->log(verbose, "stream pipe reading from %s writing to %s running!\n", stream_1->id().c_str(), stream_2->id().c_str());

    while (running()) {
        ssize_t ret = stream_1->read(&buffer[0], parent->buffer_size);
        if (parent->debug) parent->log(info, "got %d bytes from %s\n", ret, stream_1->id().c_str());

        if(ret <= 0) {
            parent->log(error, "read from %s returned %d!\n", stream_1->id().c_str(), ret);
            usleep(100000);
            continue;
        }

        unsigned int to_write = (unsigned int)ret;
        unsigned int written = 0;

#if 0 // move this to a new stream stdin/out module
        if (mod2 == "STDOUT") {
            stringstream ss;
            unsigned int N = 16;
            while(written < to_write) {
                for(unsigned int i = written; i < written + N && i < to_write; ++i) {
                    ss << string_printf("%02x ", (unsigned int)buffer[i]);
                }
                for(unsigned int i = 0; i < (written + N) - to_write; ++i)
                    ss << "   ";
                ss << " | ";
                for(unsigned int i = written; i < written + N && i < to_write; ++i) {
                    char cp = buffer[i];
                    if(isprint(cp))
                        ss << string_printf("%c", cp);
                    else
                        ss << ".";
                }
                ss << "\n";
                written += N;				
            }

            parent->log(info, "write to stdout:\n%s", ss.str().c_str());
            continue;
        }
#endif

        while (to_write > written) {
            if (parent->debug) parent->log(info, "write %d bytes to %s\n", to_write - written, stream_2->id().c_str());

            ret = stream_2->write(&buffer[written], to_write - written);
            if(ret <= 0) {
                parent->log(warning, "write %d bytes to %s returned %d\n", to_write - written, stream_2->id().c_str(), ret);
                usleep(100000);
                continue;
            }

            written += ret;
        }
    }
}

stream_pipe::stream_pipe(const std::string& name, const YAML::Node& node) :
    module_base("module_stream_pipe", name, node) 
{
    stream_dev_name_1 = get_as<std::string>(node, "stream_1");
    stream_dev_name_2 = get_as<std::string>(node, "stream_2");

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
            stream_1 = get_device<stream>(stream_dev_name_1);
            if (!stream_1)
                throw runtime_error(string_printf("failed to find stream_1: %s", stream_dev_name_1.c_str()));
#if 0 // move this to a new stream stdin/out module
            if (module2 == "STDOUT")
                m2 = NULL;
            else {
#else 
            {
#endif
                stream_2 = get_device<stream>(stream_dev_name_2);
                if (!stream_2)
                    throw runtime_error(string_printf("failed to find stream_2: %s", stream_dev_name_2.c_str()));
            }

            pipers[0]->op(stream_1, stream_2);
            
            if (bidirectional) // && module2 != "STDOUT")
                pipers[1]->op(stream_2, stream_2);
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


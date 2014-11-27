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

// module includes
#include "module_stream_pipe.h"
#include "stream_pipe.h"
#include "config.h"

// robotkernel includes
#include "robotkernel/kernel.h"
#include "robotkernel/module.h"
#include "robotkernel/trigger_base.h"
#include "robotkernel/runnable.h"

// config parser
#include "yaml-cpp/yaml.h"

using namespace std;
using namespace robotkernel;
using namespace module_stream_pipe;

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

ssize_t mod_read(MODULE_HANDLE hdl, void* buf, size_t bufsize) {
	return -1;
}
ssize_t mod_write(MODULE_HANDLE hdl, void* buf, size_t bufsize) {
	return -1;
}

//! configures module
/*!
  \param name module name
  \param config configure string
  \return handle on success, NULL otherwise
*/
MODULE_HANDLE mod_configure(const char* name, const char* config) {
    stream_pipe* dev = NULL;

    // read module configuration
    stringstream stream(config);
    YAML::Parser parser(stream);
    YAML::Node doc;

    klog(info, "[%s|%s] build by: " BUILD_USER "@" BUILD_HOST " date: " BUILD_DATE "\n", MODNAME, name);

    if (!parser.GetNextDocument(doc)) {
        klog(error, "[%s|%s] error parsing config file\n", MODNAME, name);
        return (MODULE_HANDLE)NULL;
    }

    // parsing stream_pipe configuration
    try {
        dev = new stream_pipe(name, doc);
        if (!dev) {
            klog(error, "[%s|%s] cannot allocate memory: %s\n",
                 MODNAME, name, strerror(errno));
            return (MODULE_HANDLE)NULL;
        }
    } catch (YAML::Exception& e) {
        klog(error, "[%s|%s] exception creating: %s\n", MODNAME, name, e.what());
        klog(error, "[%s|%s] got config string: \n====\n%s\n====\n", MODNAME, name, config);
        return (MODULE_HANDLE)NULL;
    }

    return (MODULE_HANDLE)dev;
}

//! unconfigure module
/*!
  \param hdl module handle
  \return success or failure
  */
int mod_unconfigure(MODULE_HANDLE hdl) {
    // cast struct
    stream_pipe* dev = (stream_pipe*)hdl;
    if (dev)
        delete dev;

    return 0;
}

//! set module state machine to defined state
/*!
  \param hdl module handle
  \param state requested state
  \return success or failure
  */
int mod_set_state(MODULE_HANDLE hdl, module_state_t state) {
    // cast struct and set new state
    stream_pipe* dev = (stream_pipe*)hdl;
    return dev->set_state(state);
}

//! get module state machine state
/*!
  \param hdl module handle
  \return current state
  */
module_state_t mod_get_state(MODULE_HANDLE hdl) {
    // cast struct and return state
    stream_pipe* dev = (stream_pipe*)hdl;
    return dev->state;
}

//! send a request to module
/*!
  \param hdl module handle
  \param reqcode request code
  \param ptr pointer to request structure
  \return success or failure
 */
int mod_request(MODULE_HANDLE hdl, int reqcode, void* ptr) {
    // cast struct
    stream_pipe* dev = (stream_pipe*)hdl;
    return dev->request(reqcode, ptr);
}

#if 0
{
#endif
#ifdef __cplusplus
}
#endif


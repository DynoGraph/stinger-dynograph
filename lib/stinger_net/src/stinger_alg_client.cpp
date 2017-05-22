#include "stinger_alg_client.h"
#include "stinger_core/stinger_error.h"
#include <vector>

void
gt::stinger::run_alg_as_client(IDynamicGraphAlgorithm &alg)
{
	LOG_V_A("Starting %s...", alg.getName().c_str());
	// FIXME Allow uncommon params to be set somehow
    std::string alg_name = alg.getName();
    int64_t data_per_vertex = alg.getDataPerVertex();
    std::string data_description = alg.getDataDescription();
    stinger_register_alg_params params = {
        alg_name.c_str(),
        "localhost",    // host
        0,              // port
        0,              // is_remote
        0,				// map_private
        data_per_vertex,
        data_description.c_str(),
        NULL,			// dependencies
        0,				// num_dependencies
    };

	stinger_registered_alg * alg_handle = stinger_register_alg_impl(params);

    if(!alg_handle) {
        LOG_E("Registering algorithm failed.  Exiting");
        return;
    }

	stinger_alg_begin_init(alg_handle);
	alg.onInit(alg_handle);
	stinger_alg_end_init(alg_handle);

	while(alg_handle->enabled)
	{
		if (stinger_alg_begin_pre(alg_handle))
		{
			alg.onPre(alg_handle);
			stinger_alg_end_pre(alg_handle);
		}

		if (stinger_alg_begin_post(alg_handle))
		{
			alg.onPost(alg_handle);
			stinger_alg_end_post(alg_handle);
		}
	}
	LOG_I("Algorithm complete... shutting down");
	free(alg_handle);
}
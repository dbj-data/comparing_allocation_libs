
#include "common.h"
#include "comparisons.h"

#ifdef DBJ_PLAYGROUND
#include "dbj_pool_allocator/pool_allocator_sampling.h"
#include "dbj_concept/is_it_feasible.h"
#endif // DBJ_PLAYGROUND
/// ---------------------------------------------------------------------

/// ---------------------------------------------------------------------
int main(int, char**)
{
	//  test_dbj_debug();

	/// bellow might be executed easy with std::async
	/// but that would add yet another unknown in the
	/// already complex benchmarking equation
	return dbj::tu::testing_system::execute();
}
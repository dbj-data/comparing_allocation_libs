
#include "common.h"
#include "comparisons.h"

#include "dbj_pool_allocator/pool_allocator_sampling.h"
#include "dbj_concept/is_it_feasible.h"
/// ---------------------------------------------------------------------
#include "dbj--nanolib/dbj++debug.h"
#include "dbj--nanolib/utf/dbj_utf_cpp.h"

// WIN c++ char32_t string literal
#define HIRAGANA_SL U"平仮名"

static void test_dbj_debug()
{
	// 1. convert 32 to utf 16
	dbj::utf::utf16_string utf16_(
		dbj::utf::utf32_string(HIRAGANA_SL)
	);
	wprintf(L"\n%s\n", utf16_.get() );

	dbj::utf::utf8_string utf8_(
		dbj::utf::utf32_string(HIRAGANA_SL)
	);
	wprintf(L"\n%S\n", utf8_.get() );

	constexpr auto arb = dbj::make_arr_buffer(U"平仮名") ;

	DBJ_SXT( dbj::make_arr_buffer(U"平仮名"));
	DBJ_SX(  dbj::make_arr_buffer(U"平仮名").data());

	auto buf = dbj::nanolib::v_buffer::make(dbj::make_arr_buffer(U"平仮名").data());

}
/// ---------------------------------------------------------------------
int main(int, char**)
{
	test_dbj_debug();
	/// this might be executed easy with std::async
	/// but that would add yet another unknown in the
	/// already complex benchmarking equation
	// return dbj::tu::testing_system::execute();
}
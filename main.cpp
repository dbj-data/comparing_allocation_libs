
#include "common.h"
#include "comparisons.h"

#include "dbj_pool_allocator/pool_allocator_sampling.h"
#include "dbj_concept/is_it_feasible.h"
/// ---------------------------------------------------------------------
#include "dbj--nanolib/dbj++debug.h"
#include "dbj--nanolib/utf/dbj_utf_cpp.h"

// WIN10 cosnole setup required
// codepage: 65001
// font: NSimSun
// 
// WIN c++ char32_t string literal
#define HIRAGANA_SL U"平仮名"

static void test_dbj_debug()
{
	dbj::utf::utf16_string utf16_(
		dbj::utf::utf32_string(HIRAGANA_SL)
	);
	wprintf(L"\n%s\n", utf16_.get());

	dbj::utf::utf8_string utf8_(
		dbj::utf::utf32_string(HIRAGANA_SL)
	);
	wprintf(L"\n%S\n", utf8_.get());

	constexpr auto arb = dbj::nanolib::make_arr_buffer(U"平仮名");

	(void)arb;

	DBJ_SXT(dbj::nanolib::make_arr_buffer(U"平仮名"));
	DBJ_SX(
		dbj::utf::utf8_string(
			dbj::utf::utf32_string(
				dbj::nanolib::make_arr_buffer(U"平仮名").data()
			)
		).get()
	);

	using bufy = typename dbj::nanolib::v_buffer::type ;

	DBJ_SX("make char32_t string literal into std array buffer, then into dbj buffer then print it");

	DBJ_SX( bufy::make(dbj::nanolib::make_arr_buffer(U"平仮名").data()).data() );

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
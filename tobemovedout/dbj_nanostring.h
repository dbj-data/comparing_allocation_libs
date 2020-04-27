
#ifndef DBJ_NANO_STRNG_INC
#define DBJ_NANO_STRNG_INC

#include "common.h"
#include "../dbj--nanolib/dbj_heap_alloc.h"


namespace dbj::nanolib {
	///
	/// nano but fully functional strng
	/// (name is deliberate; not to clash with 'string'
	/// optimized for sizes up to strng::small_size_
	/// 
	class strng final {
	public:
		/// no heap alloc for up to small size
		constexpr static auto small_size_ = 1024;
		constexpr static auto max_size_ = 0xFFFF;
	private:

		// NOTE! this is size that user has used
		// for constructing
		mutable size_t size{};

		enum class kind_tag { small, large };
		mutable kind_tag data_kind_;

		mutable union {
			char* large;
			char small[strng::small_size_]{ 0 };
		} data_;

		/// ------------------------------------------
		/// Notice we do not touch this->size here
		char* make_data(size_t size_arg_) const noexcept {

			if (size_arg_ < small_size_)
			{
				this->data_kind_ = kind_tag::small;
				return data_.small;
			}

			// on greedy size we return nullptr
			if (size_arg_ > max_size_)
			{
#ifndef NDEBUG
				errno = ENOMEM;
				perror("dbj nanolib strng size too large");
#endif
				return nullptr;
			}

			data_.large = DBJ_NANO_CALLOC(char, size_arg_);
			// on greedy size we return nullptr
			if (data_.large == nullptr)
			{
#ifndef NDEBUG
				errno = ENOMEM;
				perror("dbj nanolib calloc() failed");
#endif
				return nullptr;
			}
			this->data_kind_ = kind_tag::large;
			return data_.large;
		}
		/// ------------------------------------------
		/// Notice we do not touch this->size here
		void free_data() {
			if (data_kind_ == kind_tag::large)
			{
				if (data_.large) {
					DBJ_NANO_FREE(data_.large);
					data_.large = nullptr;
				}
			}
#ifndef NDEBUG
			else {
				memset(data_.small, 0, sizeof( data_.small ));
			}
#endif // !NDEBUG
		}

		explicit strng(size_t s_)
			: size(size_t(s_))
		{
			make_data(size);
		}

	public:

		volatile bool is_large() volatile const noexcept {
			return this->data_kind_ == kind_tag::large;
		};

		volatile bool is_small() volatile const noexcept {
			return this->data_kind_ == kind_tag::small;
		};

		char* data() const {
			if (data_kind_ == kind_tag::large)
				return data_.large;
			return data_.small;
		}

		// no default ctor
		strng() = delete;
		// no copy
		strng(strng const&) = delete;
		strng& operator = (strng const&) = delete;

		// yes move
		strng(strng&& other_) noexcept
			:
			size(other_.size),
			data_kind_(other_.data_kind_)
		{
			if (this->is_large()) {
				this->data_.large = other_.data_.large;
				other_.data_.large = nullptr;
			}
			else {
				errno_t memrez_ =
					memcpy_s(this->data_.small, strng::small_size_,
						other_.data_.small, strng::small_size_);
				if (memrez_ != 0) {
					perror(__FILE__ "\n\nmemcpy_s() failed?");
					exit(errno);
				}
			}
			other_.size = 0;
		}

		strng& operator = (strng&& other_) noexcept
		{
			if (this != &other_)
			{
				free_data();

				this->size = other_.size;
				this->data_kind_ = other_.data_kind_;

				if (this->is_large()) {
					this->data_.large = other_.data_.large;
					other_.data_.large = nullptr;
				}
				else {
					errno_t memrez_ =
						memcpy_s(this->data_.small, strng::small_size_,
							other_.data_.small, strng::small_size_);
					if (memrez_ != 0) {
						perror(__FILE__ "\n\nmemcpy_s() failed?");
						exit(errno);
					}
				}

				other_.size = 0;
			}
			return *this;
		}

		~strng() {
			free_data();
		}

		/// strng can be made only here
		/// use callback if provided
		/// fill with filer char, if provided
		/// rule no 1: one function should do one thing. yes I know...
		static strng make
		(size_t size_arg_, void (*cback)(const char*) = nullptr, const signed char filler_ = 0)
		{
			strng buf{ size_arg_ };
			if (filler_) memset(buf.data(), filler_, sizeof(buf.size));

			if (cback) cback(buf.data());

			return buf;
		}
	};
} // dbj::nanolib


#ifdef DBJ_NANOSTRING_TEST

extern "C" {
	__declspec(dllimport) void __stdcall Sleep(_In_ unsigned long /*dwMilliseconds*/);
} // "C"

/*
this delivers:
Stack cookie instrumentation code detected a stack-based buffer overrun.

info here:
https://kallanreed.wordpress.com/2015/02/14/disabling-the-stack-cookie-generation-in-visual-studio-2013/
*/
TU_REGISTER(
	[] {
		using dbj::nanolib::strng;

		constexpr auto loop_count = 0xFFFF;

		DBJ_PRINT("%s", " ");
		DBJ_PRINT("%s", "testing dbj strng ");
		DBJ_PRINT("Loop count 0x%X", loop_count);
		/// ---------------------------------------------------------------------
		auto driver = [&](auto prompt_, auto specimen)
		{
			volatile clock_t time_point_ = clock();
			specimen();
			float rez = (float)(clock() - time_point_) / CLOCKS_PER_SEC;
			DBJ_PRINT("%-20s %.3f sec,  %.0f dbj's", prompt_, rez, 1000 * rez);
		};

		// make number of small ones ie size < 1024
		driver("small dbj strng",
			[&] {
				DBJ_REPEAT(loop_count) {
					auto hammer = []() {
						return strng::make(0xFF, nullptr, ' ');
					};
					volatile auto s1 = hammer();
					DBJ_ASSERT(s1.is_small());
					// Sleep(1);
				}
			});

		driver("LARGE dbj strng",
			[&] {
				DBJ_REPEAT(loop_count) {
					auto hammer = []() {
						return strng::make(strng::small_size_ * 2, nullptr, ' ');
					};
					volatile auto s1 = hammer();
					DBJ_ASSERT(s1.is_large());
					// Sleep(1);
				}
			});
	}
);

#endif // DBJ_NANOSTRING_TEST

#endif // !DBJ_NANO_STRNG_INC

#pragma once

#include <assert.h>
#include <malloc.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// NOTE: NDEBUG is standard !
#ifndef _DEBUG
#ifndef DEBUG
#ifndef NDEBUG
#define NDEBUG
#endif // !NDEBUG
#endif // !DEBUG
#endif // !_DEBUG

namespace dbj::nanolib {
	///
	/// nano but fully functional strng
	/// (name is deliberate; not to clash with 'string'
	/// optimized for sizes up to strng::small_size_
	/// 
	class strng final {

		/// no heap alloc for up to small size
		constexpr static auto small_size_ = 1024;
		constexpr static auto max_size_ = 0xFFFF;

		mutable size_t size{};
		enum kind_tag { small, large } data_kind_;

		mutable union {
			char* large;
			char small[small_size_]{ 0 };
		} data_;

		char* make_data(size_t size_arg_) const noexcept {
			if (size_arg_ < small_size_)
				return data_.small;

			data_.large = (char*)calloc(size + 1, sizeof(char));
			assert(data_.large);
			return data_.large;
		}

		void free_data() {
			if (data_kind_ == kind_tag::large)
			{
				if (data_.large) {
					free(data_.large);
					data_.large = nullptr;
				}
			}

#ifndef NDEBUG
			else {
				memset( data_.small, 0, small_size_);
			}
#endif // !NDEBUG

		}

		explicit strng(size_t s_)
			: size(size_t(s_))
		{
			make_data( size );
		}

	public:

		char* data() const {
			if (data_kind_ == kind_tag::large)
				return data_.large;
			return data_.small;
		}

		strng() = delete;
		strng(strng const&) = delete;


		strng(strng&& other_) noexcept
			: size(other_.size)
		{
			make_data( size );

			if ( data_kind_ ==  large  ) 
				other_.data_.large = nullptr;
			other_.size = 0;
		}

		~strng() {
			free_data();
		}

		/// can be made only here
		/// use callback if provided
		/// fill with filer char, if provided
		static strng make
		(float size_arg_, void (*cback)( const char *) = nullptr, const signed char filler_ = 0 )
		{
			strng buf{ size_t(size_arg_) % strng::max_size_ };

			if ( filler_ )	memset(buf.data(), filler_, buf.size);

			if (cback) cback( buf.data() );

			return buf;
		}

	

	};
} // dbj::nanolib
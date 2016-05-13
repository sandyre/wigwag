#ifndef WIGWAG_POLICIES_CREATION_LAZY_HPP
#define WIGWAG_POLICIES_CREATION_LAZY_HPP

// Copyright (c) 2016, Dmitry Koplyarov <koplyarov.da@gmail.com>
//
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby granted,
// provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.
// IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
// WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.


#include <utility>


namespace wigwag {
namespace creation
{

#include <wigwag/detail/disable_warnings.hpp>

	struct lazy
	{
		template < typename OwningPtr_, typename DefaultType_ >
		class storage
		{
		private:
			mutable OwningPtr_		_ptr;

		public:
			template < typename T_, bool enable = std::is_same<T_, DefaultType_>::value && std::is_constructible<T_>::value, typename = typename std::enable_if<enable>::type >
			void create()
			{ }

			template < typename T_, typename... Args_ >
			void create(Args_&&... args)
			{ _ptr.reset(new T_(std::forward<Args_>(args)...)); }

			const OwningPtr_& get_ptr() const
			{
				ensure_created();
				return _ptr;
			}

			bool constructed() const
			{ return (bool)_ptr; }

		private:
			template < bool has_default_ctor = std::is_constructible<DefaultType_>::value>
			void ensure_created(typename std::enable_if<has_default_ctor, detail::enabler>::type = detail::enabler()) const
			{
				if (!_ptr)
					_ptr.reset(new DefaultType_());
			}

			template < bool has_default_ctor = std::is_constructible<DefaultType_>::value>
			void ensure_created(typename std::enable_if<!has_default_ctor, detail::enabler>::type = detail::enabler()) const
			{ WIGWAG_ASSERT(_ptr, "Internal wigwag error, _ptr must have been initialized before!"); }
		};
	};

#include <wigwag/detail/enable_warnings.hpp>

}}

#endif

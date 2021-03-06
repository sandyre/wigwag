#ifndef WIGWAG_DETAIL_INTRUSIVE_REF_COUNTER_HPP
#define WIGWAG_DETAIL_INTRUSIVE_REF_COUNTER_HPP

// Copyright (c) 2016, Dmitry Koplyarov <koplyarov.da@gmail.com>
//
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby granted,
// provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.
// IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
// WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.


#include <wigwag/detail/annotations.hpp>


namespace wigwag {
namespace detail
{

#include <wigwag/detail/disable_warnings.hpp>

    template < typename RefCounterPolicy_, typename Derived_ >
    class intrusive_ref_counter : private RefCounterPolicy_
    {
    public:
        intrusive_ref_counter()
            : RefCounterPolicy_(1)
        { }

        intrusive_ref_counter(const intrusive_ref_counter&) = delete;
        intrusive_ref_counter& operator = (const intrusive_ref_counter&) = delete;

        void add_ref() const
        { RefCounterPolicy_::add_ref(); }

        void release() const
        {
            if (RefCounterPolicy_::release() == 0)
                delete static_cast<const Derived_*>(this);
        }
    };

#include <wigwag/detail/enable_warnings.hpp>

}}

#endif

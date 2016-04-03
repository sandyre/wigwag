#ifndef WIGWAG_POLICIES_EXCEPTION_HANDLING_POLICY_CONCEPT_HPP
#define WIGWAG_POLICIES_EXCEPTION_HANDLING_POLICY_CONCEPT_HPP

// Copyright (c) 2016, Dmitry Koplyarov <koplyarov.da@gmail.com>
//
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby granted,
// provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.
// IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
// WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.


#include <wigwag/detail/policy_version_detector.hpp>
#include <wigwag/detail/type_expression_check.hpp>

#include <functional>


namespace wigwag {
namespace exception_handling
{

#include <wigwag/detail/disable_warnings.hpp>

	namespace detail
	{
		WIGWAG_DECLARE_TYPE_EXPRESSION_CHECK(has_handle_exceptions, std::declval<T_>().handle_exceptions(std::function<void(int, double)>(), 42, 3.14));

		template < typename T_ >
		struct check_policy_v1_0
		{ using adapted_policy = typename std::conditional<has_handle_exceptions<T_>::value, T_, void>::type; };
	}


	template < typename T_ >
	struct policy_concept
	{
		using adapted_policy = typename wigwag::detail::policy_version_detector<detail::check_policy_v1_0<T_>>::adapted_policy;
	};

#include <wigwag/detail/enable_warnings.hpp>

}}

#endif
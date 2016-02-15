#ifndef WIGWAG_SIGNAL_HPP
#define WIGWAG_SIGNAL_HPP

// Copyright (c) 2016, Dmitry Koplyarov <koplyarov.da@gmail.com>
//
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby granted,
// provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.
// IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
// WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.


#include <wigwag/detail/at_scope_exit.hpp>
#include <wigwag/detail/signal_impl.hpp>
#include <wigwag/signal_connector.hpp>
#include <wigwag/signal_policies.hpp>
#include <wigwag/token.hpp>


namespace wigwag
{

	template <
			typename Signature_,
			typename ExceptionHandlingPolicy_ = exception_handling::default_,
			typename ThreadingPolicy_ = threading::default_,
			typename StatePopulatingPolicy_ = state_populating::default_,
			typename HandlersStackContainerPolicy_ = handlers_stack_container::default_,
			typename LifeAssurancePolicy_ = life_assurance::default_
		>
	class signal
	{
	private:
		using handler_type = std::function<Signature_>;

		using impl_type = detail::signal_impl<Signature_, ExceptionHandlingPolicy_, ThreadingPolicy_, StatePopulatingPolicy_, HandlersStackContainerPolicy_, LifeAssurancePolicy_>;
		using impl_type_ptr = detail::intrusive_ptr<impl_type>;

		using handlers_stack_container = typename HandlersStackContainerPolicy_::template handlers_stack_container<typename impl_type::handler_info>;

		using execution_guard = typename LifeAssurancePolicy_::execution_guard;

	private:
		impl_type_ptr		_impl;

	public:
		template < typename... Args_ >
		signal(Args_&&... args)
			: _impl(new impl_type(std::forward<Args_>(args)...))
		{ }

		signal(const signal&) = delete;
		signal& operator = (const signal&) = delete;

		auto lock_primitive() const -> decltype(_impl->get_lock_primitive().get_primitive())
		{ return _impl->get_lock_primitive().get_primitive(); }

		signal_connector<Signature_> connector() const
		{ return signal_connector<Signature_>(_impl); }

		token connect(const handler_type& handler)
		{ return _impl->connect(handler); }

		template < typename... Args_ >
		void operator() (Args_&&... args) const
		{
			handlers_stack_container handlers_copy;

			{
				_impl->get_lock_primitive().lock_invoke();
				auto sg = detail::at_scope_exit([&] { _impl->get_lock_primitive().unlock_invoke(); } );

				handlers_copy.assign(_impl->get_handlers_container().begin(), _impl->get_handlers_container().end());
			}

			for (const auto& h : handlers_copy)
			{
				execution_guard g(h.get_life_checker());
				if (g.is_alive())
					_impl->get_exception_handler().handle_exceptions(h.get_handler(), std::forward<Args_>(args)...);
			}
		}
	};


}

#endif

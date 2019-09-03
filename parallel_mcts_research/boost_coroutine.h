#pragma once

#include <experimental/coroutine>

#define BOOST_THREAD_PROVIDES_FUTURE
#define BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION
#include <boost/thread/future.hpp>

template <typename... Args>
struct std::experimental::coroutine_traits<boost::future<void>, Args...> {
	struct promise_type {
		boost::promise<void> p;

		auto get_return_object() {
			return p.get_future();
		}

		std::experimental::suspend_never initial_suspend() {
			return {}; 
		}

		std::experimental::suspend_never final_suspend() {
			return {}; 
		}

		void set_exception(std::exception_ptr e) {
			p.set_exception(std::move(e)); 
		}

		void return_void() {
			p.set_value();
		}
	};
};

template <typename R, typename... Args>
struct std::experimental::coroutine_traits<boost::future<R>, Args...> {
	struct promise_type {
		boost::promise<R> p;

		auto get_return_object() {
			return p.get_future();
		}

		std::experimental::suspend_never initial_suspend() {
			return {};
		}

		std::experimental::suspend_never final_suspend() {
			return {}; 
		}

		void set_exception(std::exception_ptr e) {
			p.set_exception(std::move(e)); 
		}

		void return_void() {
			p.set_value();
		}
	};
};

template <typename R>
auto operator co_await(boost::future<R>&& f) {
	struct Awaiter {
		boost::future<R>&& input;
		boost::future<R> output;

		bool await_ready() { 
			return false; 
		}

		auto await_resume() {
			return output.get(); 
		}

		void await_suspend(std::experimental::coroutine_handle<> coro) {
			input.then([this, coro](auto result_future) {
				this->output = std::move(result_future);
				coro.resume();
				});
		}
	};

	return Awaiter{ 
		static_cast<boost::future<R>&&>(f)
	};
}
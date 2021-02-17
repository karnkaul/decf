#pragma once
#include <mutex>
#include <tuple>

namespace decf::detail {
template <typename Mutex = std::mutex>
struct lockable_t final {
	Mutex mutex;

	template <template <typename...> typename Lock = std::scoped_lock>
	Lock<Mutex> lock() {
		return Lock<Mutex>(mutex);
	}
};

template <template <typename...> typename L, typename M, typename... T>
struct locked_t {
	using lock_t = L<M>;

	lock_t lock;
	std::tuple<T...>& tuple;

	locked_t(lockable_t<M>& mutex, std::tuple<T...>& t) : lock(mutex.template lock<L>()), tuple(t) {
	}

	template <typename U>
	U& get() const noexcept {
		return std::get<U>(tuple);
	}

	template <std::size_t I>
	decltype(std::get<I>(tuple)) get() const noexcept {
		return std::get<I>(tuple);
	}
};

template <template <typename...> typename L, typename M, typename... T>
struct locked_t<L, M, T const...> {
	using lock_t = L<M>;

	lock_t lock;
	std::tuple<T...> const& tuple;

	locked_t(lockable_t<M>& mutex, std::tuple<T...> const& t) : lock(mutex.template lock<L>()), tuple(t) {
	}

	template <typename U>
	U const& get() const noexcept {
		return std::get<U>(tuple);
	}

	template <std::size_t I>
	decltype(std::get<I>(tuple)) get() const noexcept {
		return std::get<I>(tuple);
	}
};

template <template <typename...> typename L, typename M, typename T>
struct locked_t<L, M, T> {
	using lock_t = L<M>;

	lock_t lock;
	T& t;

	locked_t(lockable_t<M>& mutex, std::tuple<T>& t) : lock(mutex.template lock<L>()), t(std::get<T>(t)) {
	}

	T& get() const noexcept {
		return t;
	}
};

template <template <typename...> typename L, typename M, typename T>
struct locked_t<L, M, T const> {
	using lock_t = L<M>;

	lock_t lock;
	T const& t;

	locked_t(lockable_t<M>& mutex, std::tuple<T> const& t) : lock(mutex.template lock<L>()), t(std::get<T>(t)) {
	}

	T const& get() const noexcept {
		return t;
	}
};

template <typename M, typename... T>
struct locker_t {
	mutable lockable_t<M> mutex;
	std::tuple<T...> tuple;

	locker_t() = default;
	locker_t(T&&... t) : tuple(std::make_tuple(std::move(t)...)) {
	}
	locker_t(T const&... t) : tuple(std::make_tuple(t...)) {
	}

	template <template <typename...> typename L = std::scoped_lock>
	locked_t<L, M, T const...> lock() const {
		return locked_t<L, M, T const...>(mutex, tuple);
	}

	template <template <typename...> typename L = std::scoped_lock>
	locked_t<L, M, T...> lock() {
		return locked_t<L, M, T...>(mutex, tuple);
	}
};
} // namespace decf::detail

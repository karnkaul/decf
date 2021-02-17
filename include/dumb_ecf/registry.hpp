#pragma once
#include <memory>
#include <optional>
#include <typeindex>
#include <typeinfo>
#include <dumb_ecf/detail/locker.hpp>
#include <dumb_ecf/detail/storage.hpp>
#include <kt/enum_flags/enum_flags.hpp>

namespace decf {
///
/// \brief Desired flags can be combined with a mask as per-entity_t filters for `view()`
///
enum class flag_t { disabled, debug };
using flags_t = kt::enum_flags<flag_t, 2>;

class registry_t final {
  public:
	///
	/// \brief entity_t metadata
	///
	struct info_t final {
		std::string name;
		flags_t flags;
	};

  public:
	registry_t();
	registry_t(registry_t&&) = delete;
	registry_t(registry_t const&) = delete;
	registry_t& operator=(registry_t&&) = delete;
	registry_t& operator=(registry_t const&) = delete;
	virtual ~registry_t() noexcept;

  public:
	///
	/// \brief Obtain signature of `T`
	///
	template <typename T>
	static sign_t sign();

	///
	/// \brief Obtain signatures of `T...`
	///
	template <typename... T>
	static std::array<sign_t, sizeof...(T)> signs();

  public:
	///
	/// \brief Make new entity_t with `T(Args&&...)` attached
	///
	template <typename T, typename... Args>
	spawn_t<T> spawn(std::string name, Args&&... args);
	///
	/// \brief Make new entity_t with `T...` attached
	///
	template <typename... T>
	spawn_t<T...> spawn(std::string name);
	///
	/// \brief Destroy entity_t
	///
	bool destroy(entity_t entity);
	///
	/// \brief Toggle Enabled flag
	///
	bool enable(entity_t entity, bool enabled);
	///
	/// \brief Obtain whether Enabled flag is set
	///
	bool enabled(entity_t entity) const;
	///
	/// \brief Obtain whether entity_t exists in database
	///
	bool contains(entity_t entity) const;
	///
	/// \brief Obtain entity_t name
	///
	std::string_view name(entity_t entity) const;
	///
	/// \brief Obtain info for entity
	///
	info_t* info(entity_t entity);
	///
	/// \brief Obtain info for entity
	///
	info_t const* info(entity_t entity) const;

	///
	/// \brief Add T{args...} to entity if exists
	///
	template <typename T, typename... Args>
	T* attach(entity_t entity, Args&&... args);
	///
	/// \brief Remove T if attached to entity_t
	///
	template <typename T>
	bool detach(entity_t entity);

	///
	/// \brief Obtain const pointer to T if attached to entity
	///
	template <typename T>
	T const* find(entity_t entity) const;
	///
	/// \brief Obtain pointer to T if attached to entity
	///
	template <typename T>
	T* find(entity_t entity);
	///
	/// \brief Obtain reference to T (assumes attached to entity)
	///
	template <typename T>
	T const& get(entity_t entity) const;
	///
	/// \brief Obtain const reference to T (assumes attached to entity)
	///
	template <typename T>
	T& get(entity_t entity);

	///
	/// \brief Obtain View of T const...
	///
	template <typename... T>
	spawn_list_t<T const...> view(flags_t mask = flag_t::disabled, flags_t pattern = {}) const;
	///
	/// \brief Obtain View of T...
	///
	template <typename... T>
	spawn_list_t<T...> view(flags_t mask = flag_t::disabled, flags_t pattern = {});

	///
	/// \brief Destroy everything
	///
	void clear();
	///
	/// \brief Obtain entity_t count
	///
	std::size_t size() const;

  private:
	template <typename T>
	static sign_t sign_impl();

	bool exists_impl(sign_t sign, entity_t entity) const;

	template <typename T>
	detail::storage_t<T>& get_impl();

	template <typename T>
	detail::storage_t<std::decay_t<T>>* cast_impl() const;

	template <typename Th, std::size_t N>
	static detail::erased_storage_t* min_store_impl(Th self, std::array<sign_t, N> const& list);

	entity_t spawn_impl(std::string name);

	template <typename T>
	bool exists_impl(entity_t entity) const;

	template <typename... T, typename Th>
	static spawn_list_t<T...> view_impl(Th self, flags_t mask, flags_t pattern);

  protected:
	// Thread-safe member mutex
	mutable detail::lockable_t<std::mutex> m_mutex;

  private:
	using storage_t = std::unordered_map<sign_t, std::unique_ptr<detail::erased_storage_t>>;

	inline static std::unordered_map<std::type_index, sign_t> s_signs;
	inline static std::unordered_map<sign_t, std::string> s_names;
	inline static detail::lockable_t<std::mutex> s_mutex;
	inline static detail::locker_t<std::mutex, id_t> s_next_reg_id{null_id};

	storage_t m_db;
	detail::locker_t<std::mutex, id_t> m_next_id{null_id};
	id_t m_reg_id = null_id;
};

template <typename T>
sign_t registry_t::sign() {
	auto lock = s_mutex.lock();
	return sign_impl<T>();
}

template <typename... T>
std::array<sign_t, sizeof...(T)> registry_t::signs() {
	auto lock = s_mutex.lock();
	return {sign_impl<T>()...};
}

template <typename T, typename... Args>
spawn_t<T> registry_t::spawn(std::string name, Args&&... args) {
	auto lock = m_mutex.lock();
	auto entity = spawn_impl(std::move(name));
	auto& comp = get_impl<T>().attach(entity, std::forward<Args>(args)...);
	return {entity, comp};
}

template <typename... T>
spawn_t<T...> registry_t::spawn(std::string name) {
	auto lock = m_mutex.lock();
	auto entity = spawn_impl(std::move(name));
	if constexpr (sizeof...(T) > 0) {
		auto comps = std::tie(get_impl<T>().attach(entity)...);
		return {entity, std::move(comps)};
	} else {
		return {entity};
	}
}

template <typename T, typename... Args>
T* registry_t::attach(entity_t entity, Args&&... args) {
	static_assert(std::is_constructible_v<T, Args...>, "Cannot construct T with given Args...");
	auto lock = m_mutex.lock();
	if (get_impl<info_t>().contains(entity)) {
		return &get_impl<T>().attach(entity, std::forward<Args>(args)...);
	}
	return nullptr;
}

template <typename T>
bool registry_t::detach(entity_t entity) {
	static_assert((!std::is_same_v<info_t, std::decay_t<T>>), "Cannot destroy Info!");
	auto lock = m_mutex.lock();
	if (auto info = get_impl<info_t>().find(entity)) {
		return get_impl<T>().detach(entity);
	}
	return false;
}

template <typename T>
T const* registry_t::find(entity_t entity) const {
	auto lock = m_mutex.lock();
	if (auto t = cast_impl<T>()) {
		return t->find(entity);
	}
	return nullptr;
}

template <typename T>
T* registry_t::find(entity_t entity) {
	auto lock = m_mutex.lock();
	if (auto t = cast_impl<T>()) {
		return t->find(entity);
	}
	return get_impl<T>().find(entity);
}

template <typename T>
T const& registry_t::get(entity_t entity) const {
	return *find<T>(entity);
}

template <typename T>
T& registry_t::get(entity_t entity) {
	return *find<T>(entity);
}

template <typename... T>
spawn_list_t<T const...> registry_t::view(flags_t mask, flags_t pattern) const {
	static_assert(sizeof...(T) > 0, "Must pass at least one T!");
	auto lock = m_mutex.lock();
	return view_impl<T const...>(this, mask, pattern);
}

template <typename... T>
spawn_list_t<T...> registry_t::view(flags_t mask, flags_t pattern) {
	static_assert(sizeof...(T) > 0, "Must pass at least one T!");
	auto lock = m_mutex.lock();
	return view_impl<T...>(this, mask, pattern);
}

template <typename T>
sign_t registry_t::sign_impl() {
	auto const& t = typeid(std::decay_t<T>);
	auto const index = std::type_index(t);
	auto it = s_signs.find(index);
	if (it == s_signs.end()) {
		auto result = s_signs.emplace(index, static_cast<sign_t>(t.hash_code()));
		it = result.first;
	}
	return it->second;
}

template <typename T>
detail::storage_t<T>& registry_t::get_impl() {
	static auto const s = sign<T>();
	auto& uT = m_db[s];
	if (!uT) {
		uT = std::make_unique<detail::storage_t<T>>();
		uT->sign = s;
	}
	return static_cast<detail::storage_t<T>&>(*uT);
}

template <typename T>
detail::storage_t<std::decay_t<T>>* registry_t::cast_impl() const {
	static auto const s = sign<std::decay_t<T>>();
	if (auto it = m_db.find(s); it != m_db.end()) {
		return static_cast<detail::storage_t<std::decay_t<T>>*>(it->second.get());
	}
	return nullptr;
}

template <typename Th, std::size_t N>
detail::erased_storage_t* registry_t::min_store_impl(Th self, std::array<sign_t, N> const& list) {
	detail::erased_storage_t* ret = nullptr;
	for (auto& s : list) {
		if (auto it = self->m_db.find(s); it != self->m_db.end()) {
			auto& concept = it->second;
			if (!ret || ret->size() > concept->size()) {
				ret = concept.get();
			}
		}
	}
	return ret;
}

template <typename T>
bool registry_t::exists_impl(entity_t entity) const {
	if (auto pT = cast_impl<T>()) {
		return pT->find(entity) != nullptr;
	}
	return false;
}

template <typename... T, typename Th>
spawn_list_t<T...> registry_t::view_impl(Th self, flags_t mask, flags_t pattern) {
	spawn_list_t<T...> ret;
	if constexpr (sizeof...(T) == 1) {
		static auto const s = sign<T...>();
		if (auto it = self->m_db.find(s); it != self->m_db.end()) {
			auto& storage = static_cast<detail::storage_t<std::decay_t<T>...>&>(*it->second);
			for (auto& [e, t] : storage.map) {
				auto info_storage = self->template cast_impl<info_t>();
				auto const flags = info_storage->get(e).flags;
				if ((flags & mask) == (pattern & mask)) {
					ret.push_back({e, std::tie(t)});
				}
			}
		}
	} else {
		static auto const s = signs<T...>();
		auto min_store = min_store_impl(self, s);
		if (min_store) {
			for (auto& e : min_store->entities()) {
				auto storage = self->template cast_impl<info_t>();
				auto const flags = storage->get(e).flags;
				auto check = [e, self](sign_t s) -> bool { return self->exists_impl(s, e); };
				if ((flags & mask) == (pattern & mask) && std::all_of(s.begin(), s.end(), check)) {
					ret.push_back({e, std::tie(*self->template cast_impl<T>()->find(e)...)});
				}
			}
		}
	}
	return ret;
}
} // namespace decf

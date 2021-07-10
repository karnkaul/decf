#pragma once
#include <memory>
#include <optional>
#include <typeindex>
#include <typeinfo>
#include <dumb_ecf/detail/storage.hpp>
#include <kt/enum_flags/enum_flags.hpp>

namespace decf {
///
/// \brief Desired flags can be combined with a mask as per-entity filters for `view()`
///
enum class flag_t { disabled, debug, eCOUNT_ };
using flags_t = kt::enum_flags<flag_t>;

class registry final {
  public:
	///
	/// \brief entity metadata
	///
	struct info_t final {
		std::string name;
		flags_t flags;
	};

  public:
	registry();
	registry(registry&&) noexcept;
	registry(registry const&) = delete;
	registry& operator=(registry&&) noexcept;
	registry& operator=(registry const&) = delete;
	virtual ~registry() noexcept;

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
	/// \brief Make new entity with `T(Args&&...)` attached
	///
	template <typename T, typename... Args>
	spawn_t<T> spawn(std::string name, Args&&... args);
	///
	/// \brief Make new entity with `T...` attached
	///
	template <typename... T>
	spawn_t<T...> spawn(std::string name);
	///
	/// \brief Destroy entity
	///
	bool destroy(entity e);
	///
	/// \brief Toggle Enabled flag
	///
	bool enable(entity e, bool enabled);
	///
	/// \brief Obtain whether Enabled flag is set
	///
	bool enabled(entity e) const;
	///
	/// \brief Obtain whether entity exists in database
	///
	bool contains(entity e) const;
	///
	/// \brief Obtain entity name
	///
	std::string_view name(entity e) const;
	///
	/// \brief Obtain info for entity
	///
	info_t* info(entity e);
	///
	/// \brief Obtain info for entity
	///
	info_t const* info(entity e) const;

	///
	/// \brief Add T{args...} to entity (assumed to exist in registry)
	///
	template <typename T, typename... Args>
	T& attach(entity e, Args&&... args);
	///
	/// \brief Remove T if attached to entity
	///
	template <typename T>
	bool detach(entity e);

	///
	/// \brief Check if entity has T attached
	///
	template <typename T>
	bool attached(entity e) const;
	///
	/// \brief Obtain const pointer to T if attached to entity
	///
	template <typename T>
	T const* find(entity e) const;
	///
	/// \brief Obtain pointer to T if attached to entity
	///
	template <typename T>
	T* find(entity e);
	///
	/// \brief Obtain reference to T (assumes attached to entity)
	///
	template <typename T>
	T const& get(entity e) const;
	///
	/// \brief Obtain const reference to T (assumes attached to entity)
	///
	template <typename T>
	T& get(entity e);

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
	void clear() noexcept;
	///
	/// \brief Obtain entity count
	///
	std::size_t size() const noexcept;
	///
	/// \brief Check if any entities are present
	///
	bool empty() const noexcept;

  private:
	using storage_t = std::unordered_map<sign_t, std::unique_ptr<detail::erased_storage_t>>;

	template <typename T>
	detail::storage_t<T>& storage();

	template <typename T>
	detail::storage_t<std::decay_t<T>>* cast() const;

	template <typename Th, std::size_t N>
	static detail::erased_storage_t* min_store(Th self, std::array<sign_t, N> const& list);

	bool contains(sign_t sign, entity e) const;

	template <typename... T, typename Th>
	static spawn_list_t<T...> view_impl(Th self, flags_t mask, flags_t pattern);

	inline static std::unordered_map<std::type_index, sign_t> s_signs;
	inline static std::unordered_map<sign_t, std::string> s_names;
	inline static id_t s_next_reg_id = null_id;

	storage_t m_db;
	id_t m_next_id = null_id;
	id_t m_reg_id = null_id;
};

template <typename T>
sign_t registry::sign() {
	auto const& t = typeid(std::decay_t<T>);
	auto const index = std::type_index(t);
	auto it = s_signs.find(index);
	if (it == s_signs.end()) {
		auto result = s_signs.emplace(index, static_cast<sign_t>(t.hash_code()));
		it = result.first;
	}
	return it->second;
}

template <typename... T>
std::array<sign_t, sizeof...(T)> registry::signs() {
	return {sign<T>()...};
}

template <typename T, typename... Args>
spawn_t<T> registry::spawn(std::string name, Args&&... args) {
	auto [entity] = spawn(std::move(name));
	auto& comp = storage<T>().attach(entity, std::forward<Args>(args)...);
	return {entity, comp};
}

template <typename... T>
spawn_t<T...> registry::spawn(std::string name) {
	auto const id = ++m_next_id;
	entity e{id, m_reg_id};
	storage<info_t>().attach(e).name = std::move(name);
	if constexpr (sizeof...(T) > 0) {
		auto comps = std::tie(storage<T>().attach(e)...);
		return {e, std::move(comps)};
	} else {
		return {e};
	}
}

template <typename T, typename... Args>
T& registry::attach(entity e, Args&&... args) {
	static_assert(std::is_constructible_v<T, Args...>, "Cannot construct T with given Args...");
	return storage<T>().attach(e, std::forward<Args>(args)...);
}

template <typename T>
bool registry::detach(entity e) {
	static_assert((!std::is_same_v<info_t, std::decay_t<T>>), "Cannot destroy Info!");
	if (auto info = storage<info_t>().find(e)) { return storage<T>().detach(e); }
	return false;
}

template <typename T>
bool registry::attached(entity e) const {
	return find<T>(e) != nullptr;
}

template <typename T>
T const* registry::find(entity e) const {
	if (auto t = cast<T>()) { return t->find(e); }
	return nullptr;
}

template <typename T>
T* registry::find(entity e) {
	if (auto t = cast<T>()) { return t->find(e); }
	return nullptr;
}

template <typename T>
T const& registry::get(entity e) const {
	return *find<T>(e);
}

template <typename T>
T& registry::get(entity e) {
	return *find<T>(e);
}

template <typename... T>
spawn_list_t<T const...> registry::view(flags_t mask, flags_t pattern) const {
	static_assert(sizeof...(T) > 0, "Must pass at least one T!");
	return view_impl<T const...>(this, mask, pattern);
}

template <typename... T>
spawn_list_t<T...> registry::view(flags_t mask, flags_t pattern) {
	static_assert(sizeof...(T) > 0, "Must pass at least one T!");
	return view_impl<T...>(this, mask, pattern);
}

template <typename T>
detail::storage_t<T>& registry::storage() {
	static auto const s = sign<T>();
	auto& t = m_db[s];
	if (!t) {
		t = std::make_unique<detail::storage_t<T>>();
		t->sign = s;
	}
	return static_cast<detail::storage_t<T>&>(*t);
}

template <typename T>
detail::storage_t<std::decay_t<T>>* registry::cast() const {
	static auto const s = sign<std::decay_t<T>>();
	if (auto it = m_db.find(s); it != m_db.end()) { return static_cast<detail::storage_t<std::decay_t<T>>*>(it->second.get()); }
	return nullptr;
}

template <typename Th, std::size_t N>
detail::erased_storage_t* registry::min_store(Th self, std::array<sign_t, N> const& list) {
	detail::erased_storage_t* ret = nullptr;
	for (auto& s : list) {
		if (auto it = self->m_db.find(s); it != self->m_db.end()) {
			auto& c = it->second;
			if (!ret || ret->size() > c->size()) { ret = c.get(); }
		}
	}
	return ret;
}

template <typename... T, typename Th>
spawn_list_t<T...> registry::view_impl(Th self, flags_t mask, flags_t pattern) {
	spawn_list_t<T...> ret;
	if constexpr (sizeof...(T) == 1) {
		static auto const s = sign<T...>();
		if (auto it = self->m_db.find(s); it != self->m_db.end()) {
			auto& st = static_cast<detail::storage_t<std::decay_t<T>...>&>(*it->second);
			for (auto& [e, t] : st.map) {
				auto info_storage = self->template cast<info_t>();
				auto const flags = info_storage->get(e).flags;
				if ((flags & mask) == (pattern & mask)) { ret.push_back({e, std::tie(t)}); }
			}
		}
	} else {
		static auto const s = signs<T...>();
		if (auto min_st = min_store(self, s)) {
			for (auto& e : min_st->entities()) {
				auto st = self->template cast<info_t>();
				auto const flags = st->get(e).flags;
				auto check = [e, self](sign_t s) -> bool { return self->contains(s, e); };
				if ((flags & mask) == (pattern & mask) && std::all_of(s.begin(), s.end(), check)) {
					ret.push_back({e, std::tie(*self->template cast<T>()->find(e)...)});
				}
			}
		}
	}
	return ret;
}
} // namespace decf

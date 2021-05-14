#pragma once
#include <cstdint>
#include <tuple>
#include <vector>

namespace decf {
///
/// \brief Alias for identifying entities (and registries)
///
using id_t = std::uint64_t;
///
/// \brief Datum ID
///
constexpr id_t null_id = 0;

///
/// \brief Entity is a glorified, type-safe combination of its ID and the owning registry's ID
///
class entity_t final {
  public:
	constexpr entity_t() = default;
	constexpr entity_t(id_t id, id_t reg_id) noexcept;
	constexpr entity_t(entity_t&& rhs) noexcept;
	constexpr entity_t(entity_t const& rhs) = default;
	constexpr entity_t& operator=(entity_t&& rhs) noexcept;
	constexpr entity_t& operator=(entity_t const& rhs) = default;

	constexpr explicit operator bool() const noexcept;
	constexpr bool valid() const noexcept;
	constexpr id_t id() const noexcept;
	constexpr id_t reg_id() const noexcept;

  private:
	id_t m_id = null_id;
	id_t m_reg_id = null_id;
};

///
/// \brief Check if two entities are equivalent
///
constexpr bool operator==(entity_t const& lhs, entity_t const& rhs) noexcept;
///
/// \brief Check if two entities are not equivalent
///
constexpr bool operator!=(entity_t const& lhs, entity_t const& rhs) noexcept;

///
/// \brief Return type wrapper for registry_t::spawn<T...>()
///
template <typename... T>
struct spawn_t final {
	entity_t entity;
	std::tuple<T&...> components;

	constexpr operator entity_t() const noexcept;

	template <typename U>
	constexpr U& get() const noexcept;
};

///
/// \brief Return type wrapper for registry_t::spawn<>()
///
template <>
struct spawn_t<> {
	entity_t entity;

	constexpr operator entity_t() const noexcept;
};

///
/// \brief Return type wrapper for registry_t::view<T...>()
///
template <typename... T>
using spawn_list_t = std::vector<spawn_t<T...>>;

///
/// \brief Hash signature of component types
///
using sign_t = std::size_t;

// impl

namespace detail {
template <bool... B>
using require = std::enable_if_t<(B && ...)>;

template <typename T, typename U = T>
constexpr T exchange(T& out_t, U&& val) {
	T old = std::move(out_t);
	out_t = std::forward<U>(val);
	return old;
}
} // namespace detail

constexpr entity_t::entity_t(id_t id, id_t reg_id) noexcept : m_id(id), m_reg_id(reg_id) {}
constexpr entity_t::entity_t(entity_t&& rhs) noexcept : m_id(detail::exchange(rhs.m_id, null_id)), m_reg_id(detail::exchange(rhs.m_reg_id, null_id)) {}
constexpr entity_t& entity_t::operator=(entity_t&& rhs) noexcept {
	if (&rhs != this) {
		m_id = detail::exchange(rhs.m_id, null_id);
		m_reg_id = detail::exchange(rhs.m_reg_id, null_id);
	}
	return *this;
}
constexpr id_t entity_t::id() const noexcept { return m_id; }
constexpr id_t entity_t::reg_id() const noexcept { return m_reg_id; }
constexpr entity_t::operator bool() const noexcept { return valid(); }
constexpr bool entity_t::valid() const noexcept { return m_id != null_id && m_reg_id != null_id; }

constexpr bool operator==(entity_t const& lhs, entity_t const& rhs) noexcept { return lhs.id() == rhs.id() && lhs.reg_id() == rhs.reg_id(); }
constexpr bool operator!=(entity_t const& lhs, entity_t const& rhs) noexcept { return !(lhs == rhs); }

template <typename... T>
constexpr spawn_t<T...>::operator entity_t() const noexcept {
	return entity;
}
template <typename... T>
template <typename U>
constexpr U& spawn_t<T...>::get() const noexcept {
	return std::get<U&>(components);
}
constexpr spawn_t<>::operator entity_t() const noexcept { return entity; }
} // namespace decf

namespace std {
template <>
struct hash<decf::entity_t> {
	size_t operator()(decf::entity_t const& entity) const { return std::hash<decf::id_t>()(entity.id()) ^ (std::hash<decf::id_t>()(entity.reg_id()) << 1); }
};
} // namespace std

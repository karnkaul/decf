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
class entity final {
  public:
	constexpr entity() = default;
	constexpr entity(id_t id, id_t reg_id) noexcept : m_id(id), m_reg_id(reg_id) {}

	constexpr explicit operator bool() const noexcept { return valid(); }
	constexpr bool valid() const noexcept { return m_id != null_id && m_reg_id != null_id; }
	constexpr id_t id() const noexcept { return m_id; }
	constexpr id_t reg_id() const noexcept { return m_reg_id; }

	friend constexpr bool operator==(entity const& lhs, entity const& rhs) noexcept { return lhs.id() == rhs.id() && lhs.reg_id() == rhs.reg_id(); }
	friend constexpr bool operator!=(entity const& lhs, entity const& rhs) noexcept { return !(lhs == rhs); }

  private:
	id_t m_id = null_id;
	id_t m_reg_id = null_id;
};

///
/// \brief Return type wrapper for registry::spawn<T...>()
///
template <typename... T>
struct spawn_t final {
	entity entity_;
	std::tuple<T&...> components;

	constexpr operator entity() const noexcept { return entity_; }

	template <typename U>
	constexpr U& get() const noexcept {
		return std::get<U&>(components);
	}
};

///
/// \brief Return type wrapper for registry::spawn<>()
///
template <>
struct spawn_t<> {
	entity entity_;

	constexpr operator entity() const noexcept { return entity_; }
};

///
/// \brief Return type wrapper for registry::view<T...>()
///
template <typename... T>
using spawn_list_t = std::vector<spawn_t<T...>>;

///
/// \brief Hash signature of component types
///
using sign_t = std::size_t;

// impl

namespace detail {
template <typename T, typename U = T>
constexpr T exchange(T& out_t, U&& val) {
	T old = std::move(out_t);
	out_t = std::forward<U>(val);
	return old;
}
} // namespace detail
} // namespace decf

namespace std {
template <>
struct hash<decf::entity> {
	size_t operator()(decf::entity const& entity) const { return std::hash<decf::id_t>()(entity.id()) ^ (std::hash<decf::id_t>()(entity.reg_id()) << 1); }
};
} // namespace std

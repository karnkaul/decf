#include <dumb_ecf/registry.hpp>

namespace decf {
registry::registry() : m_reg_id(++s_next_reg_id) {}

registry::registry(registry&& rhs) noexcept
	: m_db(std::move(rhs.m_db)), m_next_id(std::exchange(rhs.m_next_id, null_id)), m_reg_id(std::exchange(rhs.m_reg_id, ++s_next_reg_id)) {}

registry& registry::operator=(registry&& rhs) noexcept {
	if (&rhs != this) {
		m_db = std::move(rhs.m_db);
		m_next_id = std::exchange(rhs.m_next_id, null_id);
		m_reg_id = std::exchange(rhs.m_reg_id, ++s_next_reg_id);
	}
	return *this;
}

registry::~registry() noexcept { clear(); }

bool registry::destroy(entity entity) {
	bool ret = false;
	for (auto& [_, concept] : m_db) { ret |= concept->detach(entity); }
	return ret;
}

bool registry::enable(entity entity, bool enabled) {
	if (auto info = storage<info_t>().find(entity)) {
		info->flags[flag_t::disabled] = !enabled;
		return true;
	}
	return false;
}

bool registry::enabled(entity entity) const {
	if (auto storage = cast<info_t>(); auto info = storage->find(entity)) { return info && !info->flags.test(flag_t::disabled); }
	return false;
}

bool registry::contains(entity entity) const {
	if (auto storage = cast<info_t>()) { return storage->contains(entity); }
	return false;
}

std::string_view registry::name(entity entity) const {
	if (auto storage = cast<info_t>(); auto info = storage->find(entity)) { return info->name; }
	return {};
}

registry::info_t* registry::info(entity entity) {
	auto& st = storage<info_t>();
	if (auto info = st.find(entity)) { return info; }
	return nullptr;
}

registry::info_t const* registry::info(entity entity) const {
	if (auto storage = cast<info_t>()) { return storage->find(entity); }
	return nullptr;
}

void registry::clear() noexcept { m_db.clear(); }

std::size_t registry::size() const noexcept {
	if (auto storage = cast<info_t>()) { return storage->size(); }
	return 0;
}

bool registry::empty() const noexcept { return size() == 0; }

bool registry::contains(sign_t sign, entity entity) const {
	if (auto it = m_db.find(sign); it != m_db.end()) { return it->second->contains(entity); }
	return false;
}
} // namespace decf

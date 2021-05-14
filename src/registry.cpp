#include <dumb_ecf/registry.hpp>

namespace decf {
registry_t::registry_t() : m_reg_id(++s_next_reg_id) {}

registry_t::registry_t(registry_t&& rhs) noexcept
	: m_db(std::move(rhs.m_db)), m_next_id(std::exchange(rhs.m_next_id, null_id)), m_reg_id(std::exchange(rhs.m_reg_id, ++s_next_reg_id)) {}

registry_t& registry_t::operator=(registry_t&& rhs) noexcept {
	if (&rhs != this) {
		m_db = std::move(rhs.m_db);
		m_next_id = std::exchange(rhs.m_next_id, null_id);
		m_reg_id = std::exchange(rhs.m_reg_id, ++s_next_reg_id);
	}
	return *this;
}

registry_t::~registry_t() noexcept { clear(); }

bool registry_t::destroy(entity_t entity) {
	bool ret = false;
	for (auto& [_, concept] : m_db) { ret |= concept->detach(entity); }
	return ret;
}

bool registry_t::enable(entity_t entity, bool enabled) {
	if (auto info = storage<info_t>().find(entity)) {
		info->flags[flag_t::disabled] = !enabled;
		return true;
	}
	return false;
}

bool registry_t::enabled(entity_t entity) const {
	if (auto storage = cast<info_t>(); auto info = storage->find(entity)) { return info && !info->flags.test(flag_t::disabled); }
	return false;
}

bool registry_t::contains(entity_t entity) const {
	if (auto storage = cast<info_t>()) { return storage->contains(entity); }
	return false;
}

std::string_view registry_t::name(entity_t entity) const {
	if (auto storage = cast<info_t>(); auto info = storage->find(entity)) { return info->name; }
	return {};
}

registry_t::info_t* registry_t::info(entity_t entity) {
	auto& st = storage<info_t>();
	if (auto info = st.find(entity)) { return info; }
	return nullptr;
}

registry_t::info_t const* registry_t::info(entity_t entity) const {
	if (auto storage = cast<info_t>()) { return storage->find(entity); }
	return nullptr;
}

void registry_t::clear() noexcept { m_db.clear(); }

std::size_t registry_t::size() const noexcept {
	if (auto storage = cast<info_t>()) { return storage->size(); }
	return 0;
}

bool registry_t::empty() const noexcept { return size() == 0; }

bool registry_t::contains(sign_t sign, entity_t entity) const {
	if (auto it = m_db.find(sign); it != m_db.end()) { return it->second->contains(entity); }
	return false;
}
} // namespace decf

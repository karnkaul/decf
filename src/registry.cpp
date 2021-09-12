#include <dumb_ecf/registry.hpp>

namespace decf {
void registry::exchg(registry& lhs, registry& rhs) noexcept {
	std::swap(lhs.m_db, rhs.m_db);
	std::swap(lhs.m_next_id, rhs.m_next_id);
	std::swap(lhs.m_reg_id, rhs.m_reg_id);
}

bool registry::destroy(entity entity) noexcept {
	bool ret = false;
	for (auto& [_, concept] : m_db) { ret |= concept->detach(entity); }
	return ret;
}

bool registry::enable(entity entity, bool enabled) noexcept {
	if (auto info = storage<info_t>().find(entity)) {
		info->flags.assign(flag_t::disabled, !enabled);
		return true;
	}
	return false;
}

bool registry::enabled(entity entity) const noexcept {
	if (auto storage = cast<info_t>(); auto info = storage->find(entity)) { return info && !info->flags.test(flag_t::disabled); }
	return false;
}

bool registry::contains(entity entity) const noexcept {
	if (auto storage = cast<info_t>()) { return storage->contains(entity); }
	return false;
}

std::string_view registry::name(entity entity) const noexcept {
	if (auto storage = cast<info_t>(); auto info = storage->find(entity)) { return info->name; }
	return {};
}

registry::info_t* registry::info(entity entity) noexcept {
	auto& st = storage<info_t>();
	if (auto info = st.find(entity)) { return info; }
	return nullptr;
}

registry::info_t const* registry::info(entity entity) const noexcept {
	if (auto storage = cast<info_t>()) { return storage->find(entity); }
	return nullptr;
}

void registry::clear() noexcept { m_db.clear(); }

std::size_t registry::size() const noexcept {
	if (auto storage = cast<info_t>()) { return storage->size(); }
	return 0;
}

bool registry::empty() const noexcept { return size() == 0; }

bool registry::contains(sign_t sign, entity entity) const noexcept {
	if (auto it = m_db.find(sign); it != m_db.end()) { return it->second->contains(entity); }
	return false;
}
} // namespace decf

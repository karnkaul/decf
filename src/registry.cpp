#include <dumb_ecf/registry.hpp>

namespace decf {
registry_t::registry_t() {
	m_reg_id = ++s_next_reg_id.lock().get();
}

registry_t::~registry_t() noexcept {
	clear();
}

bool registry_t::destroy(entity_t entity) {
	auto lock = m_mutex.lock();
	bool ret = false;
	for (auto& [_, concept] : m_db) {
		ret |= concept->detach(entity);
	}
	return ret;
}

bool registry_t::enable(entity_t entity, bool enabled) {
	auto lock = m_mutex.lock();
	if (auto info = get_impl<info_t>().find(entity)) {
		info->flags[flag_t::disabled] = !enabled;
		return true;
	}
	return false;
}

bool registry_t::enabled(entity_t entity) const {
	auto lock = m_mutex.lock();
	if (auto storage = cast_impl<info_t>(); auto info = storage->find(entity)) {
		return info && !info->flags.test(flag_t::disabled);
	}
	return false;
}

bool registry_t::contains(entity_t entity) const {
	auto lock = m_mutex.lock();
	if (auto storage = cast_impl<info_t>()) {
		return storage->contains(entity);
	}
	return false;
}

std::string_view registry_t::name(entity_t entity) const {
	auto lock = m_mutex.lock();
	if (auto storage = cast_impl<info_t>(); auto info = storage->find(entity)) {
		return info->name;
	}
	return {};
}

registry_t::info_t* registry_t::info(entity_t entity) {
	auto lock = m_mutex.lock();
	auto& storage = get_impl<info_t>();
	if (auto info = storage.find(entity)) {
		return info;
	}
	return nullptr;
}

registry_t::info_t const* registry_t::info(entity_t entity) const {
	auto lock = m_mutex.lock();
	if (auto storage = cast_impl<info_t>()) {
		return storage->find(entity);
	}
	return nullptr;
}

void registry_t::clear() {
	auto lock = m_mutex.lock();
	m_db.clear();
}

std::size_t registry_t::size() const {
	auto lock = m_mutex.lock();
	if (auto storage = cast_impl<info_t>()) {
		return storage->size();
	}
	return 0;
}

bool registry_t::exists_impl(sign_t sign, entity_t entity) const {
	if (auto it = m_db.find(sign); it != m_db.end()) {
		return it->second->contains(entity);
	}
	return false;
}

entity_t registry_t::spawn_impl(std::string name) {
	auto const id = ++m_next_id.lock().get();
	entity_t ret{id, m_reg_id};
	auto& info = get_impl<info_t>().attach(ret);
	info.name = std::move(name);
	return ret;
}
} // namespace decf

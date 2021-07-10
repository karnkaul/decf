#pragma once
#include <unordered_map>
#include <vector>
#include <dumb_ecf/types.hpp>

namespace decf::detail {
struct erased_storage_t {
	sign_t sign = 0;

	virtual ~erased_storage_t() = default;

	virtual bool detach(entity e) = 0;
	virtual std::vector<entity> entities() const noexcept = 0;
	virtual bool contains(entity) const noexcept = 0;
	virtual std::size_t size() const noexcept = 0;
};

template <typename T>
struct storage_t final : erased_storage_t {
	using value_type = T;

	static_assert(std::is_move_constructible_v<T>, "T must be move constructible!");

	std::unordered_map<entity, T, std::hash<entity>> map;

	template <typename... Args>
	value_type& attach(entity e, Args&&... args) {
		if (auto search = map.find(e); search != map.end()) {
			value_type& ret = search->second;
			ret = value_type{std::forward<Args>(args)...};
			return ret;
		}
		auto [ret, bResult] = map.emplace(e, T{std::forward<Args>(args)...});
		return ret->second;
	}

	bool detach(entity e) override {
		if (auto search = map.find(e); search != map.end()) {
			map.erase(search);
			return true;
		}
		return false;
	}

	value_type* find(entity e) {
		if (auto search = map.find(e); search != map.end()) { return std::addressof(search->second); }
		return nullptr;
	}

	value_type const* find(entity e) const {
		if (auto search = map.find(e); search != map.end()) { return std::addressof(search->second); }
		return nullptr;
	}

	value_type& get(entity e) { return *find(e); }

	value_type const& get(entity e) const { return *find(e); }

	std::size_t clear() noexcept {
		auto const ret = map.size();
		map.clear();
		return ret;
	}

	std::vector<entity> entities() const noexcept override {
		std::vector<entity> ret;
		ret.reserve(map.size());
		for (auto const& [e, _] : map) { ret.push_back(e); }
		return ret;
	}

	bool contains(entity e) const noexcept override { return map.find(e) != map.end(); }

	std::size_t size() const noexcept override { return map.size(); }
};
} // namespace decf::detail

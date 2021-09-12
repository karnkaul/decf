#pragma once
#include <unordered_map>
#include <vector>
#include <dumb_ecf/types.hpp>

namespace decf::detail {
struct erased_storage_t {
	sign_t sign = 0;

	virtual ~erased_storage_t() = default;

	virtual bool detach(entity e) noexcept = 0;
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
	T& attach(entity e, Args&&... args) {
		if (auto search = map.find(e); search != map.end()) {
			T& ret = search->second;
			ret = T{std::forward<Args>(args)...};
			return ret;
		}
		auto [ret, _] = map.emplace(e, T{std::forward<Args>(args)...});
		return ret->second;
	}

	bool detach(entity e) noexcept override {
		if (auto search = map.find(e); search != map.end()) {
			map.erase(search);
			return true;
		}
		return false;
	}

	T* find(entity e) noexcept {
		if (auto search = map.find(e); search != map.end()) { return std::addressof(search->second); }
		return nullptr;
	}

	T const* find(entity e) const noexcept {
		if (auto search = map.find(e); search != map.end()) { return std::addressof(search->second); }
		return nullptr;
	}

	T& get(entity e) { return *find(e); }

	T const& get(entity e) const { return *find(e); }

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

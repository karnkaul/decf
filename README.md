## Dumb Entity-Component (Framework)

[![Build status](https://ci.appveyor.com/api/projects/status/aa5hi5i22dyeskv7?svg=true)](https://ci.appveyor.com/project/karnkaul/decf)

This is a "dumb simple" Entity-Component framework.

### Features

- Lightweight strongly-typed 64-bit unsigned int wrapper as `entity`
- Type-erased storage of any moveable type `T` as components
- Supports one instance of any `T` attached to any `entity`
- Performant `attach<T>` / `detach<T>`
- Highly performant `O(1)` lookup for `find<T>` / `get<T>`
- Optimised, performant `view<T...>()`

**Public KT submodules**

- [`enum_flags`](https://github.com/karnkaul/enum-flags.git): `enum class` wrapper over `std::bitset`

### Usage

**Requirements**

- CMake
- C++17 compiler (and stdlib)

**Steps**

1. Clone repo to appropriate subdirectory, say `dumb_ec`
1. Add library to project via: `add_subdirectory(dumb_ec)` and `target_link_libraries(foo decf::dec)`
1. Use via: `#include <dumb_ecf/registry.hpp>`

**Example**

```cpp
// ...
#include <dumb_ecf/registry.hpp>

struct foo {
  int i;
  char c;

  foo(int i = 0, char c = ' ') : i(i), c(c) {
  }
};

// ...

std::unordered_set<entity> spawned;
decf::registry registry;   // Multiple instances can coexist

auto [e0, c0] = registry.spawn<std::string>("ent0");
auto& [name] = c0;  // tuple<T&> => structured binding
name = "ent0";
spawned.insert(e0); // std::hash<Entity> specialized

auto [e1, _] = registry.spawn<foo>("ent1", 42, 'x'); // Args forwarded as T{args...}
spawned.insert(e1);

auto [e2, c2] = registry.spawn<int, char>("ent2");  // Multiple attachments (no args possible)
auto& [i, c] = c2;  // tuple<T&, U&> => structured binding
i = registry.get<foo>(e1).i;  // assumes foo is attached to e1
if (auto f = registry.find<foo>(e1)) {  // returns pointer (nullptr if not present)
  c = f->c;
}

if (registry.contains(e0)) {
  std::cout << registry.name(e0) << " destroyed\n";
  ensure(registry.destroy(e0));   // assuming ensure() to behave like assert(), regardless of _DEBUG
}

if (auto f = registry.attach<foo>(e1)) {  // Returns nullptr if entity doesn't exist
  *f = {-1, 'z'};
}
ensure(registry.detach<char>(e2));

auto [e3, _] = registry.spawn<foo, int>("ent3");
spawned.insert(e3);
registry.enable(e3, false);
auto view = registry.view<foo>(decf::flag_t::disabled, {});  // enabled entities with foo attached (default)
view = registry.view<foo>(decf::flag_t::disabled, decf::flag_t::disabled);  // disabled entities with foo attached
view = registry.view<foo>({}, {});  // all entities with foo attached

auto view2 = registry.view<foo, int>(); // enabled entities with foo and int attached
for (auto& [e, c] : view2) {
  auto& [f, i] = c;
  // ...
}

for (auto const& e : spawned) {
  if (registry.contains(e)) {
    std::cout << registry.name(e) << " destroyed\n";
    ensure(registry.destroy(e));
  }
}
```

### Contributing

Pull/merge requests are welcome.

**[Original Repository](https://github.com/karnkaul/decf)**

# Inline partial application for monadic operations (IPA)

## Introduction

This paper aims to extend monadic operations of `std::expected` and `std::optional` by allowing taking extra arguments. These arguments will be forwarded to the given function, passed after the first argument with the contained value. When applicable, the position of the contained value can be controlled using a placeholder mechanism.

## Motivation and Scope

With the existing API users must always use a lambda expression when it is required to use extra variables from the outer scope. For example, the following code needs two extra arguments to create a `Component` within some `Context`.

```c++
struct Context;
struct Path;
struct MetaData;
struct Component;
struct Error;


std::expected<Context, Error> extractContext();
std::expected<Component, Error> createComponent(Context, Path, MetaData);
```

In this case, the code composition will look as follows:

<table>
<thead>
<tr>
<th>Before</th>
<th>After</th>
</tr>
</thead>
<tbody>
<tr>
<td>

```c++
Path path{/* ... */};
MetaData metaData{/* ... */};

auto componentResult = extractContext()
    .and_then([&path, &metaData](Context &&context) {
        return createComponent(std::move(context), path, metaData);
    });
```

</td>
<td>

```c++
Path path{/* ... */};
MetaData metaData{/* ... */};

auto componentResult = extractContext()
    .and_then(&createComponent, path, metaData);
```

</td>
</tr>
</tbody>
</table>

The code above is written to be as much equal as possible. This highlights another interesting aspect. In the left-hand side example, a user needs to remember about a value category to write efficient code. With the proposed changes, it comes automatically.

Another important point is that the shortest and preferred style can contradict a project's code style and best practices. In an ideal scenario, lambda functions can be used as follows:

```c++
[&](Context &&ctx) { return createComponent(std::move(ctx), path, metaData); }
```

In the real code, it will not always work this way. For example, there might be requirements to capture everything explicitly by name (to be consistent with multithreaded callbacks), type names can be long, and one-line lambda functions may not be allowed. All this makes using lambda functions less attractive.

### The major use cases

1. Some functionality already exists as a free function with more than a single parameter and is used by other code.
2. Some functionality is represented as a class method.
3. Encourage using more properly named free functions with monadic operations.

### The major concerns

1. Why not just use a lambda function?
    1. "Ideal lambda" may conflict with the project code style.
        1. Long captures.
        2. Long type name.
        3. Multi-line body. 
    2. Many named lambda functions can clutter a function and reduce readability as well.
    3. A user has to write more (repeated) code.
    4. Requires keeping the value category in mind in some cases.
2. How does it handle function overloading?
    1. This paper doesn't propose any mechanisms. Ideally, don't have overloads. If you have to, use lambdas as usual.
3. Why not use `bind_back`/`bind_front`?
    1. Looks more noisy.
    2. Can be non-zero cost: overhead[^1], restrictions[^2] on arguments, and create intermediate objects[^3].
4. What to do about the position of a value contained inside `expected`/`optional` that is passed to a function?
    1. Pass as the first argument by default.
    2. Use `std::unwrapped`[^4] (or a similar abstraction) if a different position is desired. See [Argument Placeholder](#argument-placeholder) for more details.
5. Any API/ABI breakage?
    1. No API/ABI breakage.
    2. Full backward compatibility with the old code.
6. Any overhead?
    1. Zero
    2. No intermediate objects, tuples, etc.

The reference implementation for `expected::and_then` lives [here](https://github.com/vt4a2h/ipa/blob/main/include/ipa/ipa.hpp#L144). The tests are [here](https://github.com/vt4a2h/ipa/blob/main/test/ipa/tst_ipa_and_then.cpp#L34). The code contains the implementation for `expected<T>` and `expected<void>`. All other monadic operations of `expected`/`optional` can be implemented the same manner.

## Argument Placeholder

This paper proposes to introduce an abstraction like `std::unwrapped` to use as a value placeholder for the forwarded argument as needed. While `std::unwrapped` serves the conceptual role of a positional placeholder, it is deliberately excluded from the `std::placeholders` namespace. That namespace is intrinsically coupled to `std::bind` and the `std::is_placeholder` trait. Including `std::unwrapped` there would either risk unintended interactions with legacy `std::bind` expressions or create a contradiction where an entity in `std::placeholders` is not recognized by `std::is_placeholder`. Therefore, `std::unwrapped` is proposed as a distinct, standalone utility.

From a type-theory perspective, `std::expected<T, E>` is a sum type. In that model, _unwrapping_ uniformly means "extracting the active payload of the tagged union". In light of this, a single universal placeholder like `std::unwrapped` is conceptually sufficient, and having a separate `std::unwrapped_error` would introduce unnecessary complexity and redundancy.

The usage of `std::unwrapped` is very restricted. It has a clear meaning and purpose: "it'll be replaced with a contained object while keeping the value category". See [Proposed changes](#proposed-changes) for more details.

## Proposed changes

### Introduce a new type
Introduce a type `std::unwrapped` and an object for a forwarded value [placeholder](https://github.com/vt4a2h/ipa/blob/main/include/ipa/ipa.hpp#L38). Comments and suggestions are welcome. The name may be different. We might want to use an existing abstraction or do it completely differently.

```c++
namespace detail
{
    struct unwrapped_t
    {
        // Implementation defined
    
        constexpr unwrapped_t(/* Implementation defined */)
        {
        }
    };
}

inline constexpr detail::unwrapped_t unwrapped(/* Implementation defined */);
```

### Change signatures
Change the signatures of monadic operations for `std::expected` and `std::optional`. Here is an example of `expected<T, E>::and_then`:

```C++
template<class F, class ...Args> 
constexpr auto and_then(F&& f, Args&& ...args);
```

### Add new constraints (expected<T, E>::and_then)
Let `T` be the type of the contained value. Let `P` be a type of the forwarded value placeholder. Let `F` be a type the function to invoke. Let `Args` be a type of the trailing argument pack.
1. `!same_as<T, void>`:
   1. If `sizeof(Args) == 1`, then the pack must contain zero `P` ([ref](https://github.com/vt4a2h/ipa/blob/main/include/ipa/ipa.hpp#L150)).
   2. If `sizeof(Args) > 1`, then the pack must contain zero or one `P` ([ref](https://github.com/vt4a2h/ipa/blob/main/include/ipa/ipa.hpp#L153)).
   3. `F` must be invocable with:
      1. `Args` where `P` is replaced by `T` if `P` is present in `Args` ([ref](https://github.com/vt4a2h/ipa/blob/main/include/ipa/ipa.hpp#L155)).
      2. `T` and `Args` in that order if `P` is not present in `Args`.
   4. The second constraint must be taken into account in all other related code. I.e., we always replace `P` with `T` whenever `P` is encountered ([ref](https://github.com/vt4a2h/ipa/blob/main/include/ipa/ipa.hpp#L70)). The same for objects, not just for the types ([ref](https://github.com/vt4a2h/ipa/blob/main/include/ipa/ipa.hpp#L111)).
2. `same_as<T, void>`:
   1. `Args` must contain zero `P` ([ref](https://github.com/vt4a2h/ipa/blob/main/include/ipa/ipa.hpp#L206)).
   2. `F` must be invocable with `Args` ([ref](https://github.com/vt4a2h/ipa/blob/main/include/ipa/ipa.hpp#L209)).

All other constraints remain the same. But we should take 1.2 and 1.3 into account.

## Examples

### Invoke a function with extra arguments

As it was in the motivation example, the `createComponent` function needs two extra arguments to create a `Component` within some `Context`.

```c++
std::expected<Context, Error> extractContext();
std::expected<Component, Error> createComponent(Context, Path, MetaData);

// ... 

Path path{/* ... */};
MetaData metaData{/* ... */};

auto componentResult = extractContext()
    .and_then(&createComponent, path, metaData);
```

Since `Context` is the first argument, we don't need to change its position. Hence, we can simply forward `path` and `metaData` to the function in the defined order. The code above will invoke `createComponent` as follows:

```c++
std::invoke(&createComponent, std::forward_like<Self>(data).value(), path, metaData);
```

Compare the final solutions:

<table>
<thead>
<tr>
<th>Before</th>
<th>After</th>
</tr>
</thead>
<tbody>
<tr>
<td>

```c++
Path path{/* ... */};
MetaData metaData{/* ... */};

auto componentResult = extractContext()
    .and_then([&path, &metaData](Context &&context) {
        return createComponent(std::move(context), path, metaData);
    });
```

</td>
<td>

```c++
Path path{/* ... */};
MetaData metaData{/* ... */};

auto componentResult = extractContext()
    .and_then(&createComponent, path, metaData);
```

</td>
</tr>
</tbody>
</table>

### Invoke a function with extra arguments and a value placeholder

Assume that `createComponent` function has a different signature:

```c++
std::expected<Component, Error> createComponent(Path, MetaData, Context);
```

In this case, `Context` is the very last parameter, but we still want to use this function with `and_then`, so we can use a placeholder:

```c++
Path path{/* ... */};
MetaData metaData{/* ... */};

auto componentResult = extractContext()
    .and_then(&createComponent, path, metaData, std::unwrapped);
```

In this case, the value of `Context` will be passed to the function as the last argument.

```c++
std::invoke(&createComponent, path, metaData, std::forward_like<Self>(data).value());
```

Compare the final solutions:

<table>
<thead>
<tr>
<th>Before</th>
<th>After</th>
</tr>
</thead>
<tbody>
<tr>
<td>

```c++
Path path{/* ... */};
MetaData metaData{/* ... */};

auto componentResult = extractContext()
    .and_then([&path, &metaData](Context&& context) {
        return createComponent(path, metaData, std::move(context));
    });
```

</td>
<td>

```c++
Path path{/* ... */};
MetaData metaData{/* ... */};

auto componentResult = extractContext()
    .and_then(&createComponent, path, metaData, std::unwrapped);
```

</td>
</tr>
</tbody>
</table>

### Invoke a class method with extra arguments

Assume there is a `Database` class that holds `Component`s. This class already has an `and_then`-compatible API.

```c++
struct Database
{
    constexpr std::expected<void, Error> store(Component /*component*/) const
    {
        // Store component
    }
};
```

Normally, an object of this type already exists. We can simply re-use it in a monadic interface.

```c++
Database db{/* ... */};
Context ctx{/* ... */};
Path path{/* ... */};
MetaData metaData{/* ... */};

const auto result = createComponent(ctx, path, metaData)
    .and_then(&Database::store, db, std::unwrapped);
```

The code above will invoke `Database::store` as follows:

```c++
std::invoke(&Database::store, db, std::forward_like<Self>(data).value());
```

Another good application of this case is to chain something inside a class method using other methods of this class. In this case, we use `this` as a second object.

Compare the final solutions (lambda):

<table>
<thead>
<tr>
<th>Before</th>
<th>After</th>
</tr>
</thead>
<tbody>
<tr>
<td>

```c++
Database db{/* ... */};
Context ctx{/* ... */};
Path path{/* ... */};
MetaData metaData{/* ... */};

const auto result = createComponent(ctx, path, metaData)
    .and_then([&db](Component&& component) {
        return db.store(std::move(component));
    });
```

</td>
<td>

```c++
Database db{/* ... */};
Context ctx{/* ... */};
Path path{/* ... */};
MetaData metaData{/* ... */};

const auto result = createComponent(ctx, path, metaData)
    .and_then(&Database::store, db, std::unwrapped);
```

</td>
</tr>
</tbody>
</table>

Compare the final solutions (`std::bind_front`):

<table>
<thead>
<tr>
<th>Before</th>
<th>After</th>
</tr>
</thead>
<tbody>
<tr>
<td>

```c++
Database db{/* ... */};
Context ctx{/* ... */};
Path path{/* ... */};
MetaData metaData{/* ... */};

const auto result = createComponent(ctx, path, metaData)
    .and_then(std::bind_front(&Database::store, std::ref(db)));
```

</td>
<td>

```c++
Database db{/* ... */};
Context ctx{/* ... */};
Path path{/* ... */};
MetaData metaData{/* ... */};

const auto result = createComponent(ctx, path, metaData)
    .and_then(&Database::store, db, std::unwrapped);
```

</td>
</tr>
</tbody>
</table>

[^1]: We are talking about a **potential** overhead here, for example, when passing large objects as arguments. A user can always use `std::ref` or similar, but it requires writing this code and being aware of it.

[^2]: Both `f` and `args` must be `MoveConstructible`.

[^3]: Also **potentially**. The code certainly returns another function object, but the influence should be negligible.

[^4]: Doesn't exist yet. May or may not be introduced as part of this proposal.
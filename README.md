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
    .and_then([&path, &metaData](Context context) {
        return createComponent(context, path, metaData);
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

The major use cases are:

1. Some functionality already exists as a free function with more than a single parameter and is used by other code.
2. Some functionality is represented as a class method.
3. Encourage using more properly named free functions with monadic operations.

Discussion of the major implementation concerns:

1. Why not just use a lambda function?
    1. Can significantly reduce code readability because of the long captures.
    2. Can have a long body.
    3. Many named lambda functions can clutter a function and reduce readability as well.
    4. A user has to write more (repeated) code.
2. Why not use `bind_back`/`bind_front`?
    1. Looks more noisy.
    2. Can be non-zero cost: overhead[^1], restrictions[^2] on arguments, and create intermediate objects[^3].
3. What to do about the position of a value contained inside `expected`/`optional` that is passed to a function?
    1. Pass as the first argument by default.
    2. Use `placeholders::value` if a different position is desired.
4. Any API/ABI breakage?
    1. No API/ABI breakage.
    2. Full backward compatibility with the old code.
5. Any overhead?
    1. Zero
    2. No intermediate objects, tuples, etc.

The reference implementation for `expected::and_then` lives [here](https://github.com/vt4a2h/ipa/blob/main/include/ipa/ipa.hpp#L144). The tests are [here](https://github.com/vt4a2h/ipa/blob/main/test/ipa/tst_ipa_and_then.cpp#L34). The code contains the implementation for `expected<T>` and `expected<void>`. All other monadic operations of `expected`/`optional` can be implemented the same manner.

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
std::invoke(&createComponent, ctx.value(), path, metaData);
```

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
    .and_then(&createComponent, path, metaData, std::placeholders::value);
```

In this case, the value of `Context` will be passed to the function as the last argument.

```c++
std::invoke(&createComponent, path, metaData, ctx.value());
```

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
    .and_then(&Database::store, db, std::placeholders::value);
```

The code above will invoke `Database::store` as follows:

```c++
std::invoke(&Database::store, db, component.value());
```

Another good application of this case is to chain something inside a class method using other methods of this class. In this case, we use `this` as a second object.

## Proposed changes

### Introduce a new type
Introduce a type and an object for a value [placeholder](https://github.com/vt4a2h/ipa/blob/main/include/ipa/ipa.hpp#L40). Comments and suggestions are welcome. The name may be different. We might want to use an existing abstraction or do it completely differently.

```c++
namespace placeholders
{
    namespace detail
    {
        struct value_t
        {
            // Implementation defined
          
            constexpr value_t(/* Implementation defined */)
            {
            }
        };
    }

    inline constexpr detail::value_t value(/* Implementation defined */);
}
```

### Change signatures
Change the signatures of monadic operations for `std::expected` and `std::optional`. Here is an example of `expected<T, E>::and_then`:

```C++
template<class F, class ...Args> 
constexpr auto and_then(F&& f, Args&& ...args);
```

### Add new constraints
Let `T` the type of the contained value. Let `P` be a type of the value placeholder. Let `F` be a type the function to invoke. Let `Args` be a type of the trailing argument pack.
1. `!same_as<T, void>`:
    1. `Args` must contain zero or one `P` ([ref](https://github.com/vt4a2h/ipa/blob/main/include/ipa/ipa.hpp#L148)).
    2. `F` must be invocable with `Args` where `P` is replaced by `T` ([ref](https://github.com/vt4a2h/ipa/blob/main/include/ipa/ipa.hpp#L150)).
    3. The second constraint must be taken into account in all other related code. I.e., we always replace `P` with `T` whenever `P` is encountered ([ref](https://github.com/vt4a2h/ipa/blob/main/include/ipa/ipa.hpp#L72)). The same for objects, not just for the types ([ref](https://github.com/vt4a2h/ipa/blob/main/include/ipa/ipa.hpp#L116)).
2. `same_as<T, void>`:
    1. `Args` must contain zero `P` ([ref](https://github.com/vt4a2h/ipa/blob/main/include/ipa/ipa.hpp#L201)).
    2. `F` must be invocable with `Args` ([ref](https://github.com/vt4a2h/ipa/blob/main/include/ipa/ipa.hpp#L203)).

All other constraints remain the same. But we should take 1.2 and 1.3 into account.

[^1]: We are talking about a **potential** overhead here, for example, when passing large objects as arguments. A user can always use `std::ref` or similar, but it requires writing this code and being aware of it.

[^2]: Both `f` and `args` must be `MoveConstructible`.

[^3]: Also **potentially**. The code certainly returns another function object, but the influence should be negligible.
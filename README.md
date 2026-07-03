# Inline partial application for monadic operations (IPA)

# Introduction

This paper is about extending monadic operations of `std::expected` and `std::optional` to allow taking extra arguments. These arguments will be forwarded to the function passed as a first argument later along with contained value. When applicable, the position of contained value can be controlled using a placeholder mechanism.

# Motivation and Scope

With the existing API a user must always use lambda expression when this is required to use extra variables from the outer scope. For example, the following code needs two extra arguments to create a `Component` within some `Context`.

```c++
struct Context;
struct Path;
struct MetaData;
struct Component;
struct Error;


std::expected<Context, Error> extractContext();
std::expected<Component, Error> createComponent(Context, Path, MetaData);
```

In this case, the code composition will look as follows.

```c++
Path path{/* ... */};
MetaData metaData{/* ... */};

auto componentResult = extractContext()
    .and_then([&path, &metaData](Context context) { return createComponent(context, path, metaData); });
```

A user should be able to write it like this.

```c++
auto componentResult = extractContext()
    .and_then(&createComponent, path, metaData);
```

The major use cases:

1. Some functionality already exists as a free function with more than a single parameter and is used by other code.
2. Some functionality is represented as a class method.
3. Encourage using more properly named free functions with monadic operations.

The major concerns:

1. Why not just a lambda function?
    1. Can significantly reduce code readability because of the long captures.
    2. Can have a long body.
    3. Many named lambda functions can clutter a function and reduce readability as well.
    4. A user has to write more code.
2. Why not `bind_back`/`bind_front`?
    1. Overhead.
    2. Restrictions on arguments.
    3. Can create intermediate objects.
3. What to do with the position of a value contained inside `expected`/`optional` passed to a function?
    1. Pass as a first argument by default.
    2. Use `std::placeholders::value` if a different position is required.
4. Any API/ABI breakage?
    1. No ABI breakage.
    2. Full backward compatibility with the old code.
5. Any overhead?
    1. Zero
    2. No intermediate objects, tuples, etc.

The reference implementation for `std::expected::and_then` lives [here](https://github.com/vt4a2h/ipa). The tests are [here](https://github.com/vt4a2h/ipa/blob/main/test/ipa/tst_ipa_and_then.cpp). The code contains the implementation for `std::expected<T>` and `std::expected<void>`. All other monadic operations of `std::expected`/`std::optional` can be implemented the same manner.
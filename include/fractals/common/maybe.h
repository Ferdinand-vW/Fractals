#pragma once

#include <bencode/bencode.h>
#include <bencode/bstring.h>
#include <neither/maybe.hpp>
#include <neither/neither.hpp>
#include <variant>

namespace fractals::common
{

template <class A> static neither::Maybe<A> to_maybe(const std::optional<A> &opt)
{
    if (opt.has_value())
    {
        return opt.value();
    }
    else
    {
        return {};
    }
}

template <class A> static neither::Maybe<A> to_maybe(A *opt)
{
    if (opt)
    {
        return *opt;
    }
    else
    {
        return {};
    }
}

static neither::Maybe<bencode::bstring> to_bstring(const bencode::bdata &bd)
{
    if (std::holds_alternative<bencode::bstring>(bd.value()))
    {
        return std::get<bencode::bstring>(bd.value());
    }

    return neither::maybe();
}

static neither::Maybe<bencode::bint> to_bint(const bencode::bdata &bd)
{
    if (std::holds_alternative<bencode::bint>(bd.value()))
    {
        return std::get<bencode::bint>(bd.value());
    }

    return neither::maybe();
}

static neither::Maybe<bencode::blist> to_blist(const bencode::bdata &bd)
{
    if (std::holds_alternative<bencode::blist>(bd.value()))
    {
        return std::get<bencode::blist>(bd.value());
    }

    return neither::maybe();
}

static neither::Maybe<bencode::bdict> to_bdict(const bencode::bdata &bd)
{
    if (std::holds_alternative<bencode::bdict>(bd.value()))
    {
        return std::get<bencode::bdict>(bd.value());
    }

    return neither::maybe();
}

/**
Apply function to each member of @vec and return results in new vector.
All function applications must be successful for a result.
@param vec vector of elements of type A
@param f function of form 'A -> Maybe B'
@return Empty if one application of @f to a member of @vec failed, otherwise returns vector<B>
*/
template <class A, class B>
static neither::Maybe<std::vector<B>> mmap_vector(const std::vector<A> &vec, std::function<neither::Maybe<B>(A)> f)
{
    std::vector<B> vec_out;
    for (auto &a : vec)
    {
        auto mb = f(a);
        if (!mb.hasValue)
        {
            return {};
        }
        else
        {
            vec_out.push_back(mb.value);
        }
    }
    return vec_out;
}

/**
Apply function to each member of @vec and return results in new vector.
All function applications must be successful for a result.
@param vec vector of elements of type A
@param f function of form 'A -> Either B C'
@return Left<B> if one application of @f to a member of @vec failed, otherwise Right<vector<C>>
*/
template <class A, class B, class C>
static neither::Either<B, std::vector<C>> mmap_vector(const std::vector<A> &vec,
                                                      std::function<neither::Either<B, C>(A)> f)
{
    std::vector<C> vec_out;
    for (auto &a : vec)
    {
        auto eb = f(a);
        if (eb.isLeft)
        {
            return neither::left<B>(eb.leftValue);
        }
        else
        {
            vec_out.push_back(eb.rightValue);
        }
    }
    return neither::right<std::vector<C>>(vec_out);
}

/**
Returns first success.
*/
template <class A> static neither::Maybe<A> choice(neither::Maybe<A> a1, neither::Maybe<A> a2)
{
    if (a1.hasValue)
    {
        return a1;
    }
    else
    {
        return a2;
    }
}

/**
Returns first success
*/
template <class A, class B> static neither::Either<A, B> choice(neither::Either<A, B> a1, neither::Either<A, B> a2)
{
    if (a1.isLeft)
    {
        return a2;
    }
    else
    {
        return a1;
    }
}

/**
Converts Maybe A to Either<String,A>.
@return returns Left<String> on Empty, otherwise Right<A>
*/
template <class A>
static neither::Either<std::string, A> maybe_to_either(const neither::Maybe<A> &m, const std::string &s)
{
    if (!m.hasValue)
    {
        return neither::left(s);
    }
    else
    {
        return neither::right<A>(m.value);
    }
}

/**
Extract from Maybe<A> with default value
*/
template <class A> static A from_maybe(neither::Maybe<A> m, A a)
{
    if (!m.hasValue)
    {
        return a;
    }
    else
    {
        return m.value;
    }
}

/**
Apply function to A if @m has value, otherwise return default
*/
template <class A, class B> static B maybe_to_val(const neither::Maybe<A> &m, std::function<B(A)> f, B b)
{
    if (!m.hasValue)
    {
        return b;
    }
    else
    {
        return f(m.value);
    }
}

/**
Apply function to A if @m has value, otherwise return default
*/
template <class A> static std::optional<A> maybeToOpt(const neither::Maybe<A> &m)
{
    if (m.hasValue)
    {
        return m.value;
    }
    else
    {
        return std::nullopt;
    }
}

/**
Apply function to A if @e has value, otherwise return default
*/
template <class A, class B, class C>
static C either_to_val(const std::variant<A, B> &e, std::function<C(A)> f, std::function<C(B)> g)
{
    if (std::holds_alternative<A>(e))
    {
        return f(std::get<A>(e));
    }
    else
    {
        return g(std::get<B>(e));
    }
}

template <class A, class B> static std::variant<A, B> eitherToVariant(const neither::Either<A, B> &eth)
{
    if (eth.isLeft)
    {
        return eth.leftValue;
    }
    else
    {
        return eth.rightValue;
    }
}

/**
Returns first success
*/
template <class A, class B, class C>
static neither::Either<A, neither::Either<B, C>> either_of(const neither::Either<A, B> &e1,
                                                           const neither::Either<A, C> &e2)
{
    if (!e1.isLeft)
    {
        return e1.rightMap([](const B &l) { return neither::Either<B, C>(neither::left(l)); });
    }
    else
    {
        return e2.rightMap([](const C &r) { return neither::Either<B, C>(neither::right(r)); });
    }
}

/**
Apply function to value in std::optional
If std::optional is empty then an empty std::optional is returned
*/
template <class F, class A> static auto map_opt(F &&f, const std::optional<A> &opt)
{
    using B = std::decay_t<decltype(f(opt.value()))>;
    if (opt.has_value())
    {
        return std::optional<B>(f(opt.value()));
    }
    else
    {
        std::optional<B> b = std::nullopt;
        return b;
    }
}

/**
Apply function to members of vector. Keep only those on success.
@return returns only those members of the vector for the result of f(B) != Left
*/
template <class A, class B, class C>
static std::vector<C> map_either(const std::vector<B> &vec, std::function<neither::Either<A, C>(B)> f)
{
    std::vector<C> out;
    for (const B &b : vec)
    {
        neither::Either<A, C> c = f(b);
        if (!c.isLeft)
        {
            out.push_back(c.rightValue);
        }
    }

    return out;
}

} // namespace fractals::common
#pragma once

#include <bencode/bencode.h>
#include <memory>
#include <neither/neither.hpp>
#include <optional>

using namespace neither;

template <class A>
static neither::Maybe<A> to_maybe(const std::optional<A> &opt) {
    if(opt.has_value()) { return opt.value(); }
    else                { return {}; }
}

static neither::Maybe<bencode::bstring> to_bstring(const bencode::bdata &bd) {
    return to_maybe(bd.get_bstring());
}

static neither::Maybe<bencode::bint> to_bint(const bencode::bdata &bd) {
    return to_maybe(bd.get_bint());
}

static neither::Maybe<bencode::blist> to_blist(const bencode::bdata &bd) {
    return to_maybe(bd.get_blist());
}

static neither::Maybe<bencode::bdict> to_bdict(const bencode::bdata &bd) {
    return to_maybe(bd.get_bdict());
}

template <class A,class B>
static Maybe<std::vector<B>> mmap_vector(const std::vector<A> &vec,std::function<Maybe<B>(A)> f) {
    std::vector<B> vec_out;
    for(auto &a : vec) {
        auto mb = f(a);
        if(!mb.hasValue) { return {}; }
        else { vec_out.push_back(mb.value); }
    }
    return vec_out;
}

template <class A,class B, class C>
static Either<B,std::vector<C>> mmap_vector(const std::vector<A> &vec,std::function<Either<B,C>(A)> f) {
    std::vector<C> vec_out;
    for(auto &a : vec) {
        auto eb = f(a);
        if(eb.isLeft) { return neither::left<B>(eb.leftValue); }
        else { vec_out.push_back(eb.rightValue); }
    }
    return neither::right<std::vector<C>>(vec_out);
}

template <class A>
static neither::Maybe<A> choice (Maybe<A> a1,Maybe<A> a2) {
    if(a1.hasValue) { return a1; }
    else            { return a2; }
}

template <class A,class B>
static neither::Either<A,B> choice (Either<A,B> a1,Either<A,B> a2) {
    if (a1.isLeft) { return a2; }
    else           { return a1; }
}


template <class A,class B,class C>
static Either<A,std::vector<C>> mmap_vector(std::vector<B> vec,std::function<Either<A,C>(B)> f) {
    std::vector<C> vec_out;
    for(auto &a : vec) {
        const auto mb = f(a);
        if(mb.isLeft) { return Either<A,std::vector<C>>(left(mb.leftValue)); }
        else { vec_out.push_back(mb.rightValue); }
    }

    const std::vector<C> cvec_out = vec_out;
    return Either<A,std::vector<C>>(right(cvec_out));
}

template <class A>
static Either<string,A> maybe_to_either(const Maybe<A> m,const std::string &s) {
    if(!m.hasValue) { return left(s); }
    else            { return right(m.value); }
}

template <class A>
static A from_maybe(Maybe<A> m, A a) {
    if(!m.hasValue) { return a; }
    else            { return m.value; }
}

template <class A,class B>
static B maybe_to_val(const Maybe<A> &m,std::function<B(A)> f,B b) {
    if(!m.hasValue) { return b; }
    else            { return f(m.value); }
}

template <class A,class B,class C>
static C either_to_val(const Either<A,B> &e,std::function<C(A)> f,std::function<C(B)> g) {
    if(e.isLeft) { return f(e.leftValue); }
    else         { return g(e.rightValue); }
}

template<class A,class B,class C>
static Either<A,Either<B,C>> either_of(const Either<A,B> &e1,const Either<A,C> &e2) {
    if (!e1.isLeft) { return e1.rightMap([](const B &l) { return Either<B,C>(left(l)); }); }
    else            { return e2.rightMap([](const C &r) { return Either<B,C>(right(r)); }); }
}

template<class A,class B,class C>
static std::vector<C> map_either(const std::vector<B> &vec,std::function<Either<A,C>(B)> f) {
    std::vector<C> out;
    for(B &b : vec) {
        Either<A,C> c = f(b);
        if(c.isRight) {
            out.push_back(c.rightValue);
        }
    }

    return out;
}
#pragma once

#include <bencode/bencode.h>
#include <memory>
#include <neither/neither.hpp>
#include <optional>

using namespace neither;

template <class A>
neither::Maybe<A> to_maybe(std::optional<A> opt) {
    if(opt.has_value()) { return opt.value(); }
    else                { return {}; }
}

neither::Maybe<bencode::bstring> to_bstring(bencode::bdata bd) {
    return to_maybe(bd.get_bstring());
}

neither::Maybe<bencode::bint> to_bint(bencode::bdata bd) {
    return to_maybe(bd.get_bint());
}

neither::Maybe<bencode::blist> to_blist(bencode::bdata bd) {
    return to_maybe(bd.get_blist());
}

neither::Maybe<bencode::bdict> to_bdict(bencode::bdata bd) {
    return to_maybe(bd.get_bdict());
}

template <class A,class B>
Maybe<std::vector<B>> mmap_vector(std::vector<A> vec,std::function<Maybe<B>(A)> f) {
    std::vector<B> vec_out;
    for(auto &a : vec) {
        auto mb = f(a);
        if(!mb.hasValue) { return {}; }
        else { vec_out.push_back(mb.value); }
    }
    return vec_out;
}

template <class A>
Either<string,A> maybe_to_either(const Maybe<A> m,const std::string &s) {
    if(!m.hasValue) { return left(s); }
    else            { return right(m.value); }
}

template <class A>
A from_maybe(Maybe<A> m, A a) {
    if(!m.hasValue) { return a; }
    else            { return m.value; }
}

template<class A,class B,class C>
Either<A,Either<B,C>> either_of(Either<A,B> e1,Either<A,C> e2) {
    if (!e1.isLeft) { return e1.rightMap([](const B &l) { return Either<B,C>(left(l)); }); }
    else            { return e2.rightMap([](const C &r) { return Either<B,C>(right(r)); }); }
}


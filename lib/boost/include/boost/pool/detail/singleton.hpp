// boost/pool/detail/singleton.hpp — compatibility shim for Boost 1.87+
// The original singleton_default was removed from Boost.Pool.
// This provides a minimal Meyers-singleton replacement.
#ifndef BOOST_POOL_DETAIL_SINGLETON_HPP
#define BOOST_POOL_DETAIL_SINGLETON_HPP

namespace boost { namespace details { namespace pool {

template <typename T>
struct singleton_default
{
    static T& instance()
    {
        static T obj;
        return obj;
    }

private:
    singleton_default() = delete;
    singleton_default(const singleton_default&) = delete;
    singleton_default& operator=(const singleton_default&) = delete;
};

}}} // namespace boost::details::pool

#endif // BOOST_POOL_DETAIL_SINGLETON_HPP

#include <type_traits> // std::conditional

#ifndef LENGTH
#define LENGTH 100
#endif


template <int...>  struct Seq { using type = Seq; };
template <class T> struct id  { using type = T; };
  
template< int N, int... Is >
struct MakeSeq
{
   using type = typename std::conditional< N == 0,  // if
                  id< Seq<Is...> >,                 // then
                  MakeSeq< N-1, Is..., N-1 >        // else
               >::type::type;
};

// alias template
template <int N>
using MakeSeq_t = typename MakeSeq<N>::type;

template <int... Is>
constexpr unsigned size(Seq<Is...>) 
{
  return sizeof...(Is);
}

int main() 
{
  static_assert( size(MakeSeq_t<(LENGTH)>()) == LENGTH, "" );
}
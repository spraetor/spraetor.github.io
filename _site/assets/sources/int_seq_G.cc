#ifndef LENGTH
#define LENGTH 100
#endif

template <int...> struct Seq {};

template <int s, class S>
struct Concat;

template <int s, int ... i>
struct Concat<s, Seq<i... >>
{
    using type = Seq<i..., (s + i)... >;
};

template <bool, class S>
struct IncSeq_if { using type = S; };

template <int... Is>
struct IncSeq_if<true, Seq<Is...>>
{
  using type = Seq<Is..., sizeof...(Is)>;
};

template <int N>
struct MakeSeq 
{
  using type = typename IncSeq_if< (N % 2 != 0), 
    typename Concat<N/2, typename MakeSeq<N/2>::type>::type >::type;
};

 // break condition
template <> struct MakeSeq<0> { using type = Seq<>; };

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
  static_assert( size(MakeSeq_t<(LENGTH)>()) == (LENGTH), "" );
}
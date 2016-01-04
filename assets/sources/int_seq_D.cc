#ifndef LENGTH
#define LENGTH 100
#endif

template <int... Is>
struct Seq
{
  using next = Seq<Is..., sizeof...(Is)>;
};

template <int N>
struct MakeSeq
{
  using type = typename MakeSeq<N-1>::type::next;
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
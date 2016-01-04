#ifndef LENGTH
#define LENGTH 100
#endif

template <int... Is> struct Seq {};

// template class declaration
template <int I, class S> struct PushBack;

template <int I, int... Is>
struct PushBack<I, Seq<Is...>> 
{
  using type = Seq<Is..., I>;
};
  
template <int N>
struct MakeSeq 
{
  using type = typename PushBack<N, typename MakeSeq<N-1>::type>::type;
};

// break condition
template <> struct MakeSeq<0> { using type = Seq<0>; };

// lias template
template <int N>
using MakeSeq_t = typename MakeSeq<N>::type;

template <int... Is>
constexpr unsigned size(Seq<Is...>) 
{
  return sizeof...(Is);
}

int main() 
{
  static_assert( size(MakeSeq_t<(LENGTH)-1>()) == LENGTH, "" );
}
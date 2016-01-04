#ifndef LENGTH
#define LENGTH 100
#endif


template <int...> struct Seq {};

template <class S1, class S2> struct Concat;
template <class S1, class S2> using  Concat_t = typename Concat<S1,S2>::type;

template <int... I1s, int... I2s>
struct Concat<Seq<I1s...>, Seq<I2s...>> 
{
  using type = Seq<I1s..., I2s...>; 
};
  
template <int I, int N> struct MakeSeqImpl;
template <int I, int N> using  MakeSeqImpl_t = typename MakeSeqImpl<I, N>::type;
template <int N>        using  MakeSeq_t     = typename MakeSeqImpl<0, N-1>::type;

template <int Start, int End>
struct MakeSeqImpl 
{
  using type = Concat_t<MakeSeqImpl_t<Start, (Start+End)/2>, 
			MakeSeqImpl_t<(Start+End)/2+1, End> >;
};

// break condition
template <int I> struct MakeSeqImpl<I,I> { using type = Seq<I>; };

template <int... Is>
constexpr unsigned size(Seq<Is...>) 
{
  return sizeof...(Is);
}


int main() 
{
  static_assert( size(MakeSeq_t<(LENGTH)>()) == (LENGTH), "" );
}
#ifndef LENGTH
#define LENGTH 100
#endif

template <int...> struct Seq {};

template <class S1, class S2> struct Concat;
template <class S1, class S2> using  Concat_t = typename Concat<S1, S2>::type;

template <int... I1, int... I2>
struct Concat<Seq<I1...>, Seq<I2...>> 
{
  using type = Seq<I1..., (sizeof...(I1)+I2)...>;
};


template <int N> struct MakeSeq;
template <int N> using  MakeSeq_t = typename MakeSeq<N>::type;

template <int N>
struct MakeSeq 
{
  using type = Concat_t<MakeSeq_t<N/2>, MakeSeq_t<N - N/2>>;
};

template <> struct MakeSeq<1> { using type = Seq<0>; };
template <> struct MakeSeq<0> { using type = Seq<>; };

template <int... Is>
constexpr unsigned size(Seq<Is...>) 
{
  return sizeof...(Is);
}

int main() 
{
  static_assert( size(MakeSeq_t<(LENGTH)>()) == LENGTH, "" );
}
#include <iostream>
#include <climits>
#include <type_traits>

template <long I> struct Fibo    { static constexpr long value = 0; };
template <>       struct Fibo<1> { static constexpr long value = 1; };

template <long I>
  requires (I > 1)
struct Fibo<I>
{
  // recursion formula
  static constexpr long value = Fibo<I-1>::value + Fibo<I-2>::value;
};

// -----------------------------------------------------------------------------

// class that behaves like an integer and stores an integral constant
template <int I> using int_ = std::integral_constant<int, I>;
// template <int I> struct int_ { constexpr operator int() const { return I; } };

// default behavior (i>=N): do nothing
template <int I, int Step, int N>
struct ForImpl 
{
  static_assert( Step != 0, "Stepsize 0 not allowed!" );
  template <class F> static void loop(F) {} 
};  

// class that performes the loop over all inndices [I, N), for i < N
template <int I, int Step, int N> 
  requires (Step > 0 && I < N) || (Step < 0 && I > N)
struct ForImpl<I, Step, N> 
{
  template <class F>
  static void loop(F&& f) 
  {
    f(int_<I>());        		     // call the functor on the ith index
    ForImpl<I+Step, Step, N>::loop(std::forward<F>(f)); // go to next loop iteration
  }
};


// default behavior (i>=N): do nothing
template <int I, int Step, int N = INT_MAX>
struct For : ForImpl<I,Step,N> {
  static_assert( (Step > 0 && I <= N) || (Step < 0 && I >= N), 
		 "Range definition does not make sense!" );
};

template <int I, int N>
struct For<I, N, INT_MAX> : ForImpl<I, (N >= I ? 1 : -1), N> {};

int main() 
{  
  For<10,-1,0>::loop([](auto const I)
  { 
    std::cout << "Fibo<" << I << ">::value = " << Fibo<I>::value << "\n"; 
  });
}

// compile with gcc6:
// /usr/local/gcc6/bin/g++ -std=c++1z ct_loop3.cc
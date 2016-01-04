---
layout: post
title:  "Compile-Time loops"
date:   2015-12-26 12:19:58
author: Simon Praetorius
comments: true
tags: c++,templates
summary: >
  We want to develop a compile-time loop structure that is flexible and allows 
  to print templates and to unroll arbitrary loops index-wise.
---
The classical example for C++ meta-programming is to define a recursion formula 
by template parameter specialization. Implementing a meta-program to calculate 
the Nth Fibonacci number (see [Wikipedia](https://en.wikipedia.org/wiki/Fibonacci_number)) could look like:

{% highlight c++ %}
template <long I>
struct Fibo // recursion formula
{
  static const long value = Fibo<I-1>::value + Fibo<I-2>::value;
};

template <> struct Fibo<1> { static const long value = 1; };
template <> struct Fibo<0> { static const long value = 0; };
{% endhighlight %}
The primary template defines the recursion formula and two template-specializations 
for I=0 and I=1 introduce break conditions. A static constant value is defined 
for the template Fibo, to access the resulting value of the Fibonacci calculations. 
Thus, the Nth Fibonacci number can be read by `Fibo<N>::value`.

The same can be written using concepts-lite technique (in C++17, [Technical specification](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2015/n4553.pdf), [Tutorial](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3701.pdf)) by formulating 
the recursion formula using a requires clause:
{% highlight c++ %}
template <long I> struct Fibo    { static constexpr long value = 0; };
template <>       struct Fibo<1> { static constexpr long value = 1; };

template <long I>
  requires (I > 1)
struct Fibo<I> // recursion formula
{
  static constexpr long value = Fibo<I-1>::value + Fibo<I-2>::value;
};
{% endhighlight %}
Here the order of definition has changed, i.e. at first we have implemented the 
break conditions and finally the recursion formula for all integer greater than 1. 
Thus, for negative indices we set the Fibonacci number to 0. To compile this code, 
currently you have to use the trunk version of gcc (HowTo: [build gcc with concepts](http://stackoverflow.com/questions/30290240/how-do-i-build-gcc-with-c-concepts-concepts-lite-support)), with
{% highlight bash %}
g++ -std=c++1z SOURCE.cc
{% endhighlight %}

In order to print all Fibonacci numbers from 0 to 50 one has to write a 
compile-time loop. By explicitly implementing a print-loop for the Fibo class 
this can be solved:
{% highlight c++ %}
template <int I, int N>
struct Print {
  static void run() {
    std::cout << "Fibo<" << I << ">::value = " << Fibo<I>::value << "n";
    Print<I+1,N>::run();
  }
};

template <int N>
struct Print<N,N> { static void run() {} };
{% endhighlight %}
It instantiates the template `Print<I,N>` from an initial I to the (not included) 
upper bound N. Thus, we have to call
{% highlight c++ %}
Print<0,51>::run();
{% endhighlight %}
to print all Fibonacci numbers from 0 to 50. This loop is specialized for the 
Fibo class. A more general implementation takes a functor for the output operation 
instead:
{% highlight c++ %}
template <int I, int N, class F>
struct Loop {
  static void run() {
    F::template eval<I>();
    Loop<I+1,N,F>::run();
  }
};

template <int N, class F>
struct Loop<N,N,F> { static void run() {} };

struct PrintFibo
{
  template <int I>
  static void eval() {
    std::cout << "Fibo<" << I << ">::value = " << Fibo<I>::value << "n";
  }
};
{% endhighlight %}
The static functor PrintFibo implements a static method eval, that takes an 
integer template argument. In order to call this method in a templated Loop 
class, we have to explicitly specify that the method is templated:
{% highlight c++ %}
F::template eval<I>();
{% endhighlight %}
where F corresponds to the PrintFibo class. The printing loop can be 
instantiated, by
{% highlight c++ %}
Loop<0,51, PrintFibo>::run();
{% endhighlight %}

Instead of calling a templated eval method by explicitly specifying the template 
parameter, we could implement a classical static functor, that takes an argument 
that represents the integral integer parameter, by using a helper class `int_`:

{% highlight c++ %}
template <int I> struct int_ {};

template <int I, int N, class F>
struct Loop {
  static void run() {
    F::eval(int_<I>()); // pass a templated struct to the eval method
    Loop<I+1,N,F>::run();
  }
};

template <int N, class F>
struct Loop<N,N,F> { static void run() {} };

struct PrintFibo
{
  template <int I>
  static void eval(int_<I>) {
    std::cout << "Fibo<" << I << ">::value = " << Fibo<I>::value << "n";
  }
};
{% endhighlight %}

This is slightly simpler to use, by passing an instance of the `int_<I>` class 
to the eval method. So far, the class PrintFibo is passed as template argument. 
Thus, we have to use a static eval method in the functor. Classically functors 
implement an `operator()` to be called. In order to allow (dynamic) functors to 
be used in the compile-time loop, we modify the implementation slightly:

{% highlight c++ %}
template <int I> struct int_ {};

template <int I, int N>
struct Loop {
  template <class F>
  static void run(F f) {
    f(int_<I>()); // pass an templated struct to the eval method
    Loop<I+1,N>::run(f);
  }
};

template <int N>
struct Loop<N,N> { template <class F> static void run(F) {} };

struct PrintFibo
{
  template <int I>
  void operator()(int_<I>) const {
    std::cout << "Fibo<" << I << ">::value = " << Fibo<I>::value << "n";
  }
};
{% endhighlight %}
by passing the functor as argument to the `run()` method.
Now, the loop can be instantiated by

{% highlight c++ %}
Loop<0,51>::run(PrintFibo());
{% endhighlight %}

This is much more flexible, but requires a functor that has a templated 
`operator()` with integer non-type parameter. So, we can not simply use 
lambda-expressions as functors. This is a heavy restriction and should be 
overcome. In order to do so, we change the implementation of the `int_` class 
slightly, by adding a constexpr cast operator:

{% highlight c++ %}
template <int I> struct int_ { constexpr operator int() const { return I; } };
{% endhighlight %}
This now allows to use `int_<I>` as template parameter for integer arguments and
the loop can be instantiated using a generic lambda, that does not need to know
its template parameter explicitly:

{% highlight c++ %}
Loop<0,51>::run([](auto I) 
{ 
  std::cout << "Fibo<" << I << ">::value = " << Fibo<I>::value << "n"; 
});
{% endhighlight %}

Adding some more tests and the flexibility to iterate forward, backward, or in 
stepsize greater than 1, and using concepts-lite as a tool for simplified 
implementation, we arrive at the final version:

{% highlight c++ %}
// class that behaves like an integer and stores an integral constant
// Here, simply using std::integral_constant from <type_traits>
template <int I> using int_ = std::integral_constant<int, I>;

// default behavior (i>=N): do nothing
template <int I, int Step, int N>
struct ForImpl 
{
  static_assert( Step != 0, "Stepsize 0 not allowed!" );
  template <class F> static void loop(F) {} 
};

// class that performs the loop over all indices [I, N), for i < N
template <int I, int Step, int N> 
  requires (Step > 0 && I < N) || (Step < 0 && I > N)
struct ForImpl<I, Step, N> 
{
  template <class F>
  static void loop(F&& f) // using universal references
  {
    f(int_<I>());
    ForImpl<I+Step, Step, N>::loop(std::forward<F>(f));
  }
};

template <int I, int Step, int N = INT_MAX>
struct For : ForImpl<I, Step, N> 
{
  static_assert( (Step > 0 && I <= N) || (Step < 0 && I >= N), 
    "Illegal range definition!" );
};

// if N not given explicitly, use Stepsize=1 or -1
template <int I, int N>
struct For<I, N, INT_MAX> : ForImpl<I, (N >= I ? 1 : -1), N> {};
{% endhighlight %}
Here, we have implemented the break condition with boolean template constraints
for upper and lower bound. Instead of passing the functor by copy to the next
iterate, it is now passed by *universal reference*. The `int_` template is
changed to the standard compatible `std::integral_constant`, since this provides
the used cast operator and some more attributes. But also the user-defined `int_`
class can be used.

The loop can be used as follows:

{% highlight c++ %}
// loop backward from 10 down to one and print all Fibonacci-numbers
For<10,-1,0>::loop([](auto I)
{ 
  std::cout << "Fibo<" << I << ">::value = " << Fibo<I>::value << "\n"; 
});
{% endhighlight %}

Output:

    Fibo<10>::value = 55
    Fibo<9>::value = 34
    Fibo<8>::value = 21
    Fibo<7>::value = 13
    Fibo<6>::value = 8
    Fibo<5>::value = 5
    Fibo<4>::value = 3
    Fibo<3>::value = 2
    Fibo<2>::value = 1
    Fibo<1>::value = 1

## Download Source

You can download the source of the final implementation from 
[here]({{ site.url }}/assets/sources/ct_loop3.cc). 
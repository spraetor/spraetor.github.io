---
layout: post
title:  "Compile-Time loops"
date:   2015-12-26 12:19:58
categories: cpp templates
---
The classical example for C++ meta-programming is to define a recursion formula by template parameter specialization. Implementing a meta-program to calculate the Nth fibonacci number could look like:
{% highlight c++ %}
template <long I>
struct Fibo
{
  static const long value = Fibo<I-1>::value + Fibo<I-2>::value;
};

template <> struct Fibo<1> { static const long value = 1; };
template <> struct Fibo<0> { static const long value = 0; };
{% endhighlight %}
The primary template defines the recusion formular and two template-specializations for I=0 and I=1 introduce break conditions. A static constant value is defined for the template Fibo, to access the resulting value of the fibonacci calculations. Thus, the Nth fibonacci number can be read by Fibo<N>::value.

The same can be written using concepts-lite technique (in C++17) by formulating the recursion formula using a requires clause:
{% highlight c++ %}
template <long I> struct Fibo    { static constexpr long value = 0; };
template <>       struct Fibo<1> { static constexpr long value = 1; };

template <long I>
  requires (I > 1)
struct Fibo<I>
{
  // recursion formula
  static constexpr long value = Fibo<I-1>::value + Fibo<I-2>::value;
};
{% endhighlight %}
Here the order of definition has changed, i.e. at first we have implemented the break conditions and finally the recusion formula for all integer greater than 1. Thus, for negative indices we set the fibonacci number to 0. To compile this code, currently you have to use the trunk version of gcc, with
{% highlight bash %}
g++ -std=c++1z SOURCE.cc
{% endhighlight %}

In order to print all fibonacci numbers from 0 to 50 one has to write a compile-time loop. By explicitely implementing a print-loop for the Fibo class this can be solved:
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
It instantionaes the template Print<I,N> from an initial I to the upper bound N. Thus, we have to call
{% highlight c++ %}
Print<0,51>::run();
{% endhighlight %}
This loop is specialized for the Fibo class. A more general implementation takes a functor for the output operation instead:
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
The static functor PrintFibo implementes a static methos eval, that takes an integer template argument. In order to call this method in a templated Loop class, we have to explicitly specify that the method is templated:
{% highlight c++ %}
F::template eval<I>();
{% endhighlight %}
where F corresponds to the PrintFibo class. The printing loop can be instantiated, by
{% highlight c++ %}
Loop<0,51, PrintFibo>::run();
{% endhighlight %}

Instead of calling a templated eval method by explicitly specifying the template parameter, we could implement a classical static functor, that takes an argument that represents the integral ineteger parameter:

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
This is slightly simpler to use, by passing an instance of the int_<I> class to the eval method. So far, the class PrintFibo is passed as template argument. Thus, we have to use a static eval method in the functor. Classically functors implement an operator() to be called. In order to allow (dynamic) functors to be used in the compile-time loop, we modify the implementation slightly:

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
That can be instantiated by

{% highlight c++ %}
Loop<0,51>::run(PrintFibo());
{% endhighlight %}
This is much more flexible, but requires a functor that has a templated operator() with inetegr non-type parameter. So, we can not simply use lambda-expressions as functors. This is a heavy restriction and should be overcome. In order to do so, we change the implementation of the int_ class slightly, by adding a constexpr cast operator:

{% highlight c++ %}
template <int I> struct int_ { constexpr operator int() const { return I; } };
{% endhighlight %}
This now allowes to use int_<I> as template paramater for integer arguments:

{% highlight c++ %}
Loop<0,51>::run([](auto I) 
{ 
  std::cout << "Fibo<" << I << ">::value = " << Fibo<I>::value << "n"; 
});
{% endhighlight %}
were we have used the generic lambda feature.
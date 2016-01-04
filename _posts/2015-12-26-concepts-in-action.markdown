---
layout: post
title:  "Concepts in action"
date:   2015-12-25 10:19:58
author: Simon Praetorius
comments: true
tags: c++,templates,concepts
summary: >
  The concepts-lite extension for c++ is accepted for the upcoming C++17 standard.
  Some tutorials are already available and with the current gcc trunk an experimental
  implementation of this feature in a c++ compiler. We will see some examples of 
  concepts-lite in action.
---
The task is to write a library that overloads to `operator*` in order to scale an
iterable object by a scalar factor, e.g. scale a vector by a double value. In
order to be *very* generic, we want to write a templated function for the scaling:

{% highlight c++ %}
template <class Vector, class Scalar>
Vector& operator*=(Vector& v, Scalar s)
{
  for (auto& v_i : v)
    v_i *= s;
  return v;
}

template <class Vector, class Scalar>
Vector const operator*(Vector v, Scalar s)
{
  return v *= s;
}
{% endhighlight %}
This implementation provides a scaling of a vector by a scalar factor from the right. The
problem with this implementation is, that we can not distiguish between the vector
argument and the scalar argument, since both are unconstrained templates. Passing
a scalar as first argument and a vector as second argument is a valid call to the
function, but results in a compiler error in the inner algorithm, i.e. in the
range-based for-loop, since a scalar is not iterable.

It is possible to write an overload of `operator*` for a left-sided multiplication 
by using tag-dispatiching, SFINAE techniques, or some other *tricks*. But we want 
to write the code in a natural way. Therefore, we use constraints by the concepts-lite
specification ([Technical specification](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2015/n4553.pdf), [Tutorial](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3701.pdf)).

What is a vector? Or at least an iterable object? We require, that a range-based
for-loop can be used for the container, i.e. there exist `begin()` and `end()`
(member) functions and the current iterate, i.e. the value-type of a vector, is
multiplicable by a scalar.

What is a scalar, on the other hand? For now we simply require, that it is an
arithmetic type, that fulfills the type-trait `std::is_arithmetic`. Later,
we can refine this concept, by explicitly specifying requirements on scalar arguments.

In order to use type-constraints, we have to define concepts:

{% highlight c++ %}
template <typename V>
concept bool Iterable =
  requires(V v) {
    begin(v);
    end(v);
  } || requires(V v) {
    v.begin();
    v.end();
  };

template <typename S>
concept bool Arithmetic = std::is_arithmetic<S>::value;

template <typename T0, typename T1>
concept bool Multiplicable =
  requires(T0 t0, T1 t1) {
    t0 * t1;
    t1 * t0;
    t0 *= t1;
    t1 *= t0;
  };
{% endhighlight %}
The first one (*Iterable*) provides a simplified concept for iterable objects, i.e.
there must exist either a free function `begin()` and `end()` or member functions
with the same names. Here, nothing is said about what these functions should return.
A more advanced implementation could be a requirement that the result of `begin()` and
`end()` are Iterators (Input- or Output-Iterators, depending on other requirements).
The second concepts simply redirects to the type-traits implementation of the STL.
The third concepts is a binary concept, since it implements a relation between two 
types. Here, we just require, that a multiplication operation between these two types
exist.

Combined with the operator definition above, we can now define `vector*scalar` and
`scalar*vector` concept-enabled function overloads:

{% highlight c++ %}
template <Iterable Vector, Arithmetic Scalar>
  requires Multiplicable<Value_type<Vector>, Scalar>
Vector& operator*=(Vector& v, Scalar s)
{
  for (auto& v_i : v)
    v_i *= s;
  return v;
}

template <Iterable Vector, Arithmetic Scalar>
Vector const operator*(Vector v, Scalar s)
{
  return v *= s;
}

template <Arithmetic Scalar, Iterable Vector>
Vector const operator*(Scalar s, Vector v)
{
  return v *= s;
}
{% endhighlight %}
where `Value_type` is an alias template leading to the value type of a container.
A possible implementation could be:
{% highlight c++ %}
namespace impl 
{
  template <typename V>
  struct Value_type;
  
  template <typename V>
    requires requires() { typename V::value_type; }
  struct Value_type<V> {
    using type = typename V::value_type;
  };
} // end namespace impl

template <typename V>
using Value_type = typename impl::Value_type<V>::type;
{% endhighlight %}

So, we can now multiply any iterable object by any arithmetic type, if the value-types
are multiplicable, e.g.
{% highlight c++ %}
std::vector<double>   v1{1.0,2.0,3.0};
std::array<double, 3> v2{1.0,2.0,3.0};
std::list<float>      v3{1.0,2.0,3.0};

auto v1_scaled = v1 * 3;
auto v2_scaled = v2 * 3.0;
auto v3_scaled = v3 * 3.0;

auto v4_scaled = 3 * v1;
{% endhighlight %}

## Download Source

You can download the source of the final implementation from 
[here]({{ site.url }}/assets/sources/scale_vector.cc).

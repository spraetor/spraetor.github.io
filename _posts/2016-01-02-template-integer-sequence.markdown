---
layout: post
title:  "Template integer-sequence"
date:   2016-01-02 17:34:58
author: Simon Praetorius
comments: true
tags: c++,templates
summary: >
  Since C++14 a generator for compile-time integer sequences is available in the
  standard library. How does this work? How to create your own implementation if
  this is not available in your distribution? How to modify the sequence? Here, 
  we want to look at these questions and want to show and analyze some different
  implementations found in the web.
---
A compile-time integer sequence is a variadic class template with integer non-type
template parameters, e.g.
{% highlight c++ %}
template <int... Is>
struct Seq { using type = Seq; };
{% endhighlight %}
Sometimes it is necessary to have a specific sequence of increasing values, that
can be used in parameter pack expansions. As an example we want to store all values
of an `std::array` in a C-Array, using the template function `std::get<I>(array)`.
Here, an integer sequence is used as a function parameter that can be deduces
and unpacked automatically, to have all the indices at hand and to be used in
a pack expansion:

{% highlight c++ %}
template <class T, int N, int... Is>
void foo(std::array<T, N> const& source, Seq<Is...>) {
  T target[] = {std::get<Is>(source)...};
  // ...
}

// function call:
foo(array, Seq<0,1,2,3,4,5,6,(...)>());
{% endhighlight %}

The question is, how to generate the template `Seq<0,1,2,3,(...),N-1>` automatically,
for any given upper index `N-1`.

## Linear complexity

# Version A/B

The simplest (direct) approach has linear complexity in the template recursion 
depth and recursively pushes back an integer number to the sequence. If we have
the template class `Seq` as above, then a push-back class that adds an integer to
the end of a sequence could look like:

{% highlight c++ %}
// primary template
template <int I, class S>
struct PushBack;

// specialization for Seq<I0,I1,(...)>
template <int I, int... Is>
struct PushBack<I, Seq<Is...>> {
  using type = Seq<Is..., I>;
};
{% endhighlight %}
or simply
{% highlight c++ %}
template <int I, int... Is>
struct PushBack<I, Seq<Is...>> : Seq<Is..., I> {};
{% endhighlight %}
for the specialization, since `Seq` itself provides a `type` type-attribute.

An integer sequence can now be generated, by adding the upper index recursively
to an already generated smaller sequence. Therefore, we have to write a break
condition for the smallest sequence, e.g. a sequence with length 0. We
write a template class `MakeSeq<N>` with a `type` attribute, that represents the
`Seq<0,1,...,N-1>` sequence with N integers:

{% highlight c++ %}
template <int N>
struct MakeSeq 
{
  using type = typename PushBack<N-1, typename MakeSeq<N-2>::type>::type;
};

template <> struct MakeSeq<1> { using type = Seq<0>; };
template <> struct MakeSeq<0> { using type = Seq<>; };
{% endhighlight %}
or a bit shorter:

{% highlight c++ %}
template <int N>
struct MakeSeq : PushBack<N-1, typename MakeSeq<N-2>::type>::type {};

template <> struct MakeSeq<1> : Seq<0> {};
template <> struct MakeSeq<0> : Seq<> {};
{% endhighlight %}

That's it. A small test could be to compare the length of the sequence to N:

{% highlight c++ %}
template <int... Is>
constexpr unsigned size(Seq<Is...>) {
  return sizeof...(Is);
}

int main() {
  static_assert( size(typename MakeSeq<LENGTH>::type()) == LENGTH, "" );
}
{% endhighlight %}

# Version C

On [Stackoverflow](http://stackoverflow.com/questions/17424477/implementation-c14-make-integer-sequence/17426611)
another variant of the sequential implementation to create an integer sequence
is published. There, the break condition is realized using a `std::conditional`
statement and the `PushBack` helper class is included in the `MakeSeq` class directly.
This may reduce the number of templates to be instantiated by the compiler and may be 
preferable in this context:

{% highlight c++ %}
// helper meta-class that represents the template type.
template <class T> struct id  { using type = T; };

template <int N, int... Is>
struct MakeSeq
{
   using type = typename std::conditional< N == 0, // if
                  id< Seq<Is...> >,                // then
                  MakeSeq< N-1, N-1, Is...          // else
               >::type::type;
};
{% endhighlight %}

# Version D

Motivated by the implementation in the standard library a variant of the integer
sequence generation is provided that works directly on the variadic integer 
sequences, i.e. the `Seq` template, and adds a type attribute `next` that
represents a sequence with one more integer appended:

{% highlight c++ %}
template <int... Is>
struct Seq
{
  using next = Seq<Is..., sizeof...(Is)>;
};
{% endhighlight %}

The generator template `MakeSeq` now simply adds an alias that *calls* the `next`
type:

{% highlight c++ %}
template <int N>
struct MakeSeq
{
  using type = typename MakeSeq<N-1>::type::next;
};

template <> struct MakeSeq<0> { using type = Seq<>; };
{% endhighlight %}


# Time measurement

A measurement of the time to instantiate (generate) the integer-sequence shows, that
there is a difference between compilers and also between the way of implementing
the classes, i.e. using an own type attribute (Version A), deriving from the 
`Seq` class (Version B), or using the Stackoverflow one-class implementation (Version C). 
For LENGTH=900 we get the average timings in seconds

| Version   |     A     |     B     |     C     |     D     |  
| --------- | --------- | --------- | --------- | --------- |  
| clang-3.6 | 0.690     | 0.692     | 0.572     | **0.311** |  
| clang-3.7 | 0.636     | 0.624     | 0.516     | **0.280** |  
| g++-4.8.5 | 0.865     | 1.273     | 0.877     | **0.727** |  
| g++-5.3.0 | 1.727     | 2.530     | 1.719     | **1.428** |  

for the call `COMPILER -std=c++11 -DLENGTH=900 -ftemplate-depth=1000 SOURCE.cc`. The
clang compilers have problems with larger `LENGTH` parameters and stop with a 
Segmentation fault. This is not the case for g++. There the template instantiation 
depth could be increased a lot further.

## Logarithmic complexity

How to improve the implementation further? In a linear implementation where indices
are appended to the sequence one by one, the compiler can not reuse already instantiated
parts of the template and must go down the full depth.

A recursive implementation with logarithmic instantiation depth can be formulated
by splitting the sequence into two parts [0,N/2] and [N/2+1,N], creating sequences
for both parts recursively, and finally concatenating the partial sequences.

# Version E

The first implementation creates a linear sequence [start, start+1, start+2, ..., end]
by splitting in the middle of the interval [start, end]. Therefore, we introduce a
template with two parameters and a corresponding alias template as shortcut:

{% highlight c++ %}
template <int Start, int End>
struct MakeSeqImpl;

template <int Start, int End>
using MakeSeqImpl_t = typename MakeSeqImpl<Start, End>::type;
{% endhighlight %}
and the `MakeSeq` class can be recapitulated from `MakeSeqImpl` by template specialization:

{% highlight c++ %}
template <int N> 
using MakeSeq_t = typename MakeSeqImpl<0, N-1>::type;
{% endhighlight %}
here implemented as alias template.

Similar to the `PushBack` template of above, a `Concat` template is used that 
pushes all the indices from the second sequence at the end of the indices of the 
first sequence:

{% highlight c++ %}
template <class S1, class S2> struct Concat;
template <class S1, class S2> using  Concat_t = typename Concat<S1,S2>::type;

template <int... I1s, int... I2s>
struct Concat<Seq<I1s...>, Seq<I2s...>> {
  using type = Seq<I1s..., I2s...>;
};
{% endhighlight %}

The implementation of `MakeSeqImpl` now calls `Concat` of two subsequences:

{% highlight c++ %}
template <int Start, int End>
struct MakeSeqImpl<Start, End> {
  using type = Concat_t<MakeSeqImpl_t<Start, (Start+End)/2>, 
                        MakeSeqImpl_t<(Start+End)/2+1, End> >;
};

// break condition:
template <int I>
struct MakeSeqImpl<I,I> {
  using type = Seq<I>;
};
{% endhighlight %}

# Version F

A slightly more involved implementation introduces a shift in the concatenation
and creates recursively two sequences that do overlap, where the second one
is shifted by the length of the first one. This variant is show on
[Stackoverflow/Answer-1](http://stackoverflow.com/questions/17424477/implementation-c14-make-integer-sequence/17426611)
by user *Xeo*.

The `Concat` template is modified slightly: 
{% highlight c++ %}
template <int... I1, int... I2>
struct Concat<Seq<I1...>, Seq<I2...>> {
  using type = Seq<I1..., (sizeof...(I1) + I2)...>;
};
{% endhighlight %}
and the `MakeSeq` class can now be implemented directly, without the help of the 
class `MakeSeqImpl`, by

{% highlight c++ %}
template <int N> struct MakeSeq;
template <int N> using  MakeSeq_t = typename MakeSeq<N>::type;

template <int N> 
struct MakeSeq
{
  using type = Concat_t<MakeSeq_t<N/2>, MakeSeq_t<N - N/2>>;
};

// break conditions
template <> struct MakeSeq<1> { using type = Seq<0>; };
template <> struct MakeSeq<0> { using type = Seq<>; };
{% endhighlight %}
where the parameter `N` is now, as before, the length of the sequence. We need both
break conditions, since each of those can not be implemented without the other one.

The advantage of this variant is that both ranges may be the same and thus, 
the compiler may reuse the instantiation of one.

# Version G

On [Stackoverflow/Answer-2](http://stackoverflow.com/questions/17424477/implementation-c14-make-integer-sequence/17426611)
the user *Khurshid* posted an answer that uses this idea even more strictly, 
by creating twice the same sequence and concatenating those, using `Concat` as 
above. Only in the case that `N` is not divisible by two, a final index is added 
to the sequence.

{% highlight c++ %}
template <int N>
struct MakeSeq {
  using type = typename IncSeq_if< (N % 2 != 0), 
    typename Concat<N/2, typename MakeSeq<N/2>::type>::type >::type;
};

// break condition
template <> struct MakeSeq<0> { using type = Seq<>; };
{% endhighlight %}
where `IncSeq_if` implements the push-back of the final index, in the case that the 
first template argument is `true`:

{% highlight c++ %}
template <bool /* = false */, class S>
struct IncSeq_if { using type = S; }; // do not increase

template <int... Is>
struct IncSeq_if<true, Seq<Is...>> // Seq<0,1,...,Size-1>
{
  using type = Seq<Is..., sizeof...(Is)>;
};
{% endhighlight %}
and `Concat` is slightly modified to reuse the same sequence twice:

{% highlight c++ %}
template <int Size, class S>
struct Concat;

template <int Size, int... Is>
struct Concat<Size, Seq<Is... >>
{
    using type = Seq<Is..., (Size + Is)... >;
};
{% endhighlight %}

# Time measurement

In a same way as above we measure the time to compile the various variants of
the logarithmic integer sequence implementation, using the compilers clang and
g++:

| Version   |     E     |     F     |     G     |  
| --------- | --------- | --------- | --------- |  
| clang-3.6 | 0.115     | 0.062     | **0.060** |  
| clang-3.7 | 0.104     | 0.057     | **0.055** |  
| g++-4.8.5 | 0.131     | 0.059     | **0.034** |  
| g++-5.3.0 | 0.169     | 0.049     | **0.047** |  

We see that the compilation times are about a factor > 5 lower than for the linear
implementation and that the most specialized variant F outperforms all the other
implementations. Thus, it makes sense to optimize the way of instantiating templates
when large instantiation depth are necessary and if we want to reduce compilation times.

# Download source files

You can download all the source-files, by following the links in the table below:

| Version | Link                                                |  
| ------- | --------------------------------------------------- |  
| A       | [source]({{ site.url }}/assets/sources/int_seq_A.cc)|  
| B       | [source]({{ site.url }}/assets/sources/int_seq_B.cc)|  
| C       | [source]({{ site.url }}/assets/sources/int_seq_C.cc)|  
| D       | [source]({{ site.url }}/assets/sources/int_seq_D.cc)|  
| E       | [source]({{ site.url }}/assets/sources/int_seq_E.cc)|  
| F       | [source]({{ site.url }}/assets/sources/int_seq_F.cc)|  
| G       | [source]({{ site.url }}/assets/sources/int_seq_G.cc)|  


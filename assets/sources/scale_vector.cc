#include <iostream> // std::cout
#include <vector>   // std::vector
#include <array>    // std::array
#include <list>     // std::list
#include <type_traits>  // is_arithmetic

namespace scprog
{  
  template <typename V>
  concept bool Iterable =
    requires(V v) {
      begin(v);
      end(v);
    } || requires(V v) {
      v.begin();
      v.end();
    };
    
  template <typename T0, typename T1>
  concept bool Multiplicable =
    requires(T0 t0, T1 t1) {
      t0 * t1;
      t1 * t0;
      t0 *= t1;
      t1 *= t0;
    };
    
  template <typename S>
  concept bool Arithmetic = std::is_arithmetic<S>::value;

    
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

  template <Iterable Vector>
  void print(Vector const& v)
  {
    if (v.size() == 0)
      std::cout << "[]";
    else {
      auto it = v.begin();
      std::cout << "[" << *it;
      for (++it; it != v.end(); ++it)
	std::cout << " " << *it;
      std::cout << "]";
    }
  }
} // end namespace scprog

int main() 
{
  using namespace scprog;
  
  std::vector<double>   v1{1.0,2.0,3.0};
  std::array<double, 3> v2{1.0,2.0,3.0};
  std::list<float>      v3{1.0,2.0,3.0};
  
  auto v1_ = v1 * 3;
  auto v2_ = v2 * 3.0;
  auto v3_ = v3 * 3.0;
  
  auto v4_ = 3 * v1;
  
  print(v1);  std::cout << "\n";
  print(v1_); std::cout << "\n";
  print(v2);  std::cout << "\n";
  print(v2_); std::cout << "\n";
  print(v3);  std::cout << "\n";
  print(v3_); std::cout << "\n";
  print(v1);  std::cout << "\n";
  print(v4_); std::cout << "\n";
  
//   auto v5_ = v1 * v2; // error: "constraints not satisfied"
}

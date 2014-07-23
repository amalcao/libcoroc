#ifndef __CXX_REFCNT_T_H__
#define __CXX_REFCNT_T_H__

#include <libcoroc.h>


template <typename T>
class __CXX_refcnt_t {
  T ref;

public:
  // the default constructor
  // NOTE the type T must be a pointer
  __CXX_refcnt_t() : ref(0) {}

  __CXX_refcnt_t(T _ref) : ref(_ref) {
    __refcnt_get(ref);
  }

  // the copy constructor 
  __CXX_refcnt_t(const __CXX_refcnt_t &cr) {
    ref = cr.ref;
    __refcnt_get(ref);
  }

  // the destructor
  ~__CXX_refcnt_t(void) {
    if (ref) __refcnt_put(ref);
  }

  // overload the operators '*'
  T operator*(void) {
    return ref;
  }

  // overload the operator '='
  T operator=(T _ref) {
    if (ref) __refcnt_put(ref);
    ref = _ref;
    __refcnt_get(ref);
    return ref;
  }

  T operator=(const __CXX_refcnt_t& cr) {
    return operator= (cr.ref);
  }

  // detach the reference from the smart pointer
  T detach(void) {
    T _ref = ref;
    ref = 0;
    return _ref;
  }
  // TODO
};

#endif // __CXX_REFCNT_T_H__

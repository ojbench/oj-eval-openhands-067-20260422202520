#include <iostream>
#include "code"
int main(){
  RefCell<int> c(5);
  auto r1 = c.borrow();
  auto r2 = c.try_borrow();
  std::cout<<*r1<<" "<<(r2?**r2: -1)<<"\n";
  return 0;
}

#include <unity.h>
#include <operators/zip.hpp>
#include <sources/constant.hpp>

void test_zip_zips_to_tuple() {
  auto a = rheo::constant(3);
  auto b = rheo::constant(std::string("hello"));
  auto a_b = rheo::zip(a, b);
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_zip_zips_to_tuple);
  UNITY_END();
}

#include "minunit.h"

int tests_run = 0;

extern char *test_header();

static char *all_tests() {
  mu_run_test(test_header);

  return 0;
}

int main() {
  char *result = all_tests();
  if (result != 0) {
    printf("%s\n", result);
  } else {
    printf("ALL TESTS PASSED\n");
  }
  printf("Tests run: %d\n", tests_run);

  return result != 0;
}

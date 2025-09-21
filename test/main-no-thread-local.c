#define SAXAMAPHONE_FILE_BUFFER_SIZE 0
#define SAXAMAPHONE_NODE_BUFFER_SIZE 0

#include <stdlib.h>

#include "test.h"

int main() {

  ASSERT_NULL(sax_parser(NULL));
  ASSERT_NULL(sax_parser(&(sax_config_t){
      .string = "",
  }));

  return EXIT_SUCCESS;
}
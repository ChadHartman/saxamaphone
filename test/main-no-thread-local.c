#define SAXAMAPHONE_FILE_BUFFER_SIZE 0
#define SAXAMAPHONE_NODE_BUFFER_SIZE 0

#include <stdlib.h>

#include "test.h"

int main() {

  ASSERT_NULL(sax_parser(NULL));
  ASSERT_NULL(sax_parser(&(sax_config_t){
      .string = "",
  }));

  uint8_t buf[2048];
  ASSERT_NON_NULL(sax_parser(&(sax_config_t){
      .arena = buf,
      .arena_size = sizeof(buf),
      .string = "",
  }));
  
  {
    uint8_t buf[2048];
    sax_parser_t *parser = sax_parser(&(sax_config_t){
        .arena = buf,
        .arena_size = sizeof(buf),
        .path = "path",
    });
    ASSERT_NON_NULL(parser);
    ASSERT_EQ(SAX_EVENT_ERROR, sax_next(parser));
  }

  return EXIT_SUCCESS;
}
#include <stdio.h>
#include <stdlib.h>

#define READ_ONLY_VAR 0
#define NOT_MAPPED_VAR 1

void read_only_area_error(void) {
    char *s = "Hello goat";
    *s = 'a';
}

void not_mapped_address_error(void) {
   char *test = NULL;
   fprintf(stderr, "%s", test);
}

void test_function(int variant) {
   switch (variant){
      case READ_ONLY_VAR:
         read_only_area_error();
         break;
      case NOT_MAPPED_VAR:
         not_mapped_address_error();
         break;
      default:
         // not expected reach here
         exit(EXIT_SUCCESS);
         break;
   }
}

int main(int argc, char *argv[])
{
   int variant;
   if (argc != 2) {
      // not expected reach here
      return 0;
   }
   variant = atoi(argv[1]);
   test_function(variant);
   return 0;
}

void culprit_function(void) {
    char *s = "Hello goat";
    // write to read-only area to result Segmentation Fault
    *s = 'H';
}

void test_function(void) {
   culprit_function();
}

int main() {
   test_function();
   return 0;
}

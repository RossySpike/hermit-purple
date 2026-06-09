#include <stddef.h>
#include <stdio.h>
#include <string.h>
int main() {
  unsigned short curr_img = 0;
  unsigned long long id_img = 1;
  const char *data = "foobar";
  unsigned long long data_bits = 8 * strlen(data);
  printf("HUMAN READABLE:\ncurr_img: %hu\tid_img: %llu\tdata_bits: "
         "%llu\tdata:%s\n",
         curr_img, id_img, data_bits, data);
  printf("MACHINE READABLE(BIN):\ncurr_img: %hB\tid_img: %llB\tdata_bits: "
         "%llB\tdata: ",
         curr_img, id_img, data_bits);
  for (size_t i = 0; i < strlen(data); i++)
    printf("%hB ", data[i]);
  printf("\n");
  printf("MACHINE READABLE(HEX):\ncurr_img: %hX\tid_img: %llX\tdata_bits: "
         "%llX\tdata: ",
         curr_img, id_img, data_bits);
  for (size_t i = 0; i < strlen(data); i++)
    printf("%hX ", data[i]);
  printf("\n");
  return 0;
}

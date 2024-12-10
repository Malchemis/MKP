#include <stdio.h>

#include "lib/utils.h"

int main(void) {
    data* d = parse_instance("Instances_MKP/100M5_1.txt");
    print_data(d);
    destroy_data(d);
    return 0;
}
#include <stdio.h>

static int sum(int n) {
    int total = 0;
    for (int i = 1; i <= n; ++i) {
        total += i;
    }
    return total;
}

int main(void) {
    printf("sum(10) = %d\n", sum(10));
    return 0;
}

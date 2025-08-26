#include <stdio.h>
#include <time.h>

int main() {
    for (int i = 0; i < 100; i++){
        printf("%c", time(NULL)%8);
    }
    return 0;
}
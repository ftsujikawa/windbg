#include <stdio.h>

int add(int a, int b)
{
    return a + b;
}

int main()
{
    int x = 10;

    printf("hello\n");

    x++;

    printf("%d\n", x);

    printf("%d\n", add(5, 3));

    return 0;
}
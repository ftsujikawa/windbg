#include <stdio.h>

struct test_struct {
    int a;
    int b;
};

int add(int a, int b)
{
    return a + b;
}

int main()
{
    int x = 10;
    struct test_struct s = {1, 2};

    printf("hello\n");

    x++;

    printf("%d\n", x);

    printf("%d\n", add(5, 3));

    printf("%d\n", s.a);
    printf("%d\n", s.b);
    
    return 0;
}
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
    int a[5] = {1, 2, 3, 4, 5};
    struct test_struct *ps = &s;
    struct test_struct sa[3] = {{1, 2}, {3, 4}, {5, 6}};
    
    printf("hello\n");

    x++;

    printf("%d\n", x);

    printf("%d\n", add(5, 3));

    printf("%d\n", s.a);
    printf("%d\n", s.b);

    printf("%d\n", a[0]);
    printf("%d\n", a[1]);
    printf("%d\n", a[2]);
    printf("%d\n", a[3]);
    printf("%d\n", a[4]);

    return 0;
}
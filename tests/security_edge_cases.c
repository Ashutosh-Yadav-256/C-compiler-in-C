int test_func(int a, int b, int c, int d, int e, int f, int g, int h) {
    return a + b + c + d + e + f + g + h;
}

int main() {
    int a = 10;
    int b = 2;
    int div = a / b;
    int mod = a % b;
    int sum_args = test_func(1, 2, 3, 4, 5, 6, 7, 8);
    char *msg = "Sec\nTest\t\"Esc\"\\123\x41";
    char first_ch = msg[0];
    if (first_ch == 'S' && div == 5 && mod == 0 && sum_args == 36) {
        return 42;
    }
    return 1;
}

int add(int a, int b) {
    return a + b;
}

int recursive_sum(int n) {
    if (n <= 1) {
        return n;
    }
    return n + recursive_sum(n - 1);
}

int many_args(int a, int b, int c, int d, int e, int f, int g, int h) {
    return a + b + c + d + e + f + g + h;
}

int main() {
    int x = add(10, 5);
    int y = recursive_sum(5);
    int z = many_args(1, 2, 3, 4, 5, 6, 7, 8);
    return x + y + z;
}


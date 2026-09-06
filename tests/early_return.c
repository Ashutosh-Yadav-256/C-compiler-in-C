int check_branch(int x) {
    if (x) return 42;
    return 99;
}

int recursive_fib(int n) {
    if (n <= 0) return 0;
    if (n == 1) return 1;
    return recursive_fib(n - 1) + recursive_fib(n - 2);
}

int main() {
    int r1 = check_branch(1);
    int r2 = check_branch(0);
    if (r1 != 42 || r2 != 99) return 1;

    int fib = recursive_fib(7);
    if (fib != 13) return 2;

    return 42;
}


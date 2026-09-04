int test_flow(int cond) {
    int x = 10;
    if (cond) {
        x = 42;
    } else {
        x = 99;
    }
    return x;
}

int main() {
    int a = test_flow(1);
    int b = test_flow(0);
    if (a == 42 && b == 99) {
        return 42;
    }
    return 0;
}


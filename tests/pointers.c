int main() {
    int x = 10;
    int *p = &x;
    *p = 40;
    int y = *p + 2;
    return y;
}


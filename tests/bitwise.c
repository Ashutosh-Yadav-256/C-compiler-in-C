int main() {
    int a = 0x0F;
    int b = 0xF0;
    int c = (a | b);
    int d = (c & 0xAA);
    int e = (d ^ 0xFF);
    int f = (1 << 5) + (e - 75);
    return f;
}


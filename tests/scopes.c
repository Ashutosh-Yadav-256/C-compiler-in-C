int main() {
    int a = 10;
    {
        int a = 20;
        int b = 5;
    }
    return a + 15;
}


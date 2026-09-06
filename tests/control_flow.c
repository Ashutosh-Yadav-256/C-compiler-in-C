int main() {
    int sum = 0;
    int i = 0;
    while (i < 10) {
        if (i == 5) {
            i = i + 1;
            continue;
        }
        if (i == 8) {
            break;
        }
        sum = sum + i;
        i = i + 1;
    }

    int for_sum = 0;
    int j;
    for (j = 0; j < 5; j = j + 1) {
        for_sum = for_sum + j;
    }

    return sum + for_sum;
}


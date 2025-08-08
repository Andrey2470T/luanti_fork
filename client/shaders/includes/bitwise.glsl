int bitwiseAND(int a, int b) {
    int result = 0;
    int n = 1;

    for(int i = 0; i < 32; i++) {
        if ((mod(float(a), 2.0*n) >= n) && (mod(float(b), 2.0*n) >= n))
            result += n;

        n = n * 2;
    }
    return result;
}

int bitwiseOR(int a, int b) {
    int result = 0;
    int n = 1;

    for(int i = 0; i < 32; i++) {
        if ((mod(float(a), 2.0*n) >= n) || (mod(float(b), 2.0*n) >= n))
            result += n;

        n = n * 2;
    }
    return result;
}

int bitwiseShiftLeft(int a, int n)
{
	return a * int(pow(2, n));
}

int bitwiseShiftRight(int a, int n)
{
	return a / int(pow(2, n));
}

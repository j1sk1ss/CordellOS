#include <math.h>

int abs(int x) {
	if (x < 0) return -x;
	return x;
}

int sgn(int x) {  //function to find the sign of an integer.
	if (x < 0) return -1;
	return 1;
}

double sqrt(double number) {  //function to find the square root of a rational number.
    int start = 0, end = number;
    int mid;
    double ans;

    while (start <= end) {
        mid = (start + end) / 2;
        if (mid * mid == number) {
            ans = mid;
            break;
        }

        if (mid * mid < number) {
            ans=start;
            start = mid + 1;
        }
        else end = mid - 1;
    }

    double increment = 0.1;
	int i;

    for (i = 0; i < 5; i++) {
        while (ans * ans <= number) 
            ans += increment;

        ans = ans - increment;
        increment = increment / 10;
    }

    return ans;
}

double fabs(double value) {
    return value < 0 ? -value : value;
}

//function for calculating the cos.
double cos(double x) {
    if (x < 0) {
        int q = -x / DOUBLE_PI;
		q += 1;
        double y = q * DOUBLE_PI;
        x = -(x - y);
    }

    if (x >= DOUBLE_PI) {
        int q = x / DOUBLE_PI;
        double y = q * DOUBLE_PI;
        x = x - y;
    }

    int s = 1;
    if (x >= PI) {
        s = -1;
        x -= PI;
    }

    if (x > HALF_PI) {
        x = PI - x;
        s = -s;
    }

    double z = x * x;
    double r = z * (z * (SIN_CURVE_A - SIN_CURVE_B * z) - 0.5) + 1.0;

    if (r > 1.0) r = r - 2.0;

    if (s > 0) return r;

    else return -r;
}

//sin calculation function.
double sin(double x) {
    return cos(x - HALF_PI);
}

double pow(double base, double exponent) {
    double result = 1.0;
    int whole_exponent = (int)exponent;

    // Handle negative exponents
    if (whole_exponent < 0) {
        base = 1.0 / base;
        whole_exponent = -whole_exponent;
    }

    // Calculate the power using repeated multiplication
    for (int i = 0; i < whole_exponent; i++)
        result *= base;
    
    return result;
}

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <keyboard.h>

int main(int argc, char* argv[]) {
    int interface = argc != 0 ? 0 : 1;
    int result    = -1;
    if (interface != 1) {
        int operand1 = atoi(argv[0]);
        char op      = argv[1][0];
        int operand2 = atoi(argv[2]);
        if (op == '+') result = operand1 + operand2;
        else if (op == '-') result = operand1 - operand2;
        else if (op == '*') result = operand1 * operand2;
        else if (op == '^') result = pow(operand1, operand2);
        else if (op == '/') result = operand1 / operand2;
    }

    return result;
}
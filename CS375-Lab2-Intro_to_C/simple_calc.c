#include <stdio.h>
#include <stdlib.h>

double add(double a, double b);
double subtract(double a, double b);
double multiply(double a, double b);
double divide(double a, double b);

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: ./simple_calc <number1> <operator> <number2>\n");
        return 1;
    }

    double num1 = atof(argv[1]);
    double num2 = atof(argv[3]);
    char op = argv[2][0];
    double result;

    switch (op) {
        case '+':
            result = add(num1, num2);
            break;
        case '-':
            result = subtract(num1, num2);
            break;
        case '*':
            result = multiply(num1, num2);
            break;
        case '/':
            if (num2 == 0) {
                printf("Error: you cannot divide by zero.\n");
                return 1;
            }
            result = divide(num1, num2);
            break;
        default:
            printf("Error: the operator you used is unsupported '%c'. Use +, -, *, or /.\n", op);
            return 1;
    }

    printf("Result: %.2f\n", result);
    return 0;
}

double add(double a, double b) {
    return a + b;
}

double subtract(double a, double b) {
    return a - b;
}

double multiply(double a, double b) {
    return a * b;
}

double divide(double a, double b) {
    return a / b;
}

#ifndef CALCULATOR_H
#define CALCULATOR_H

#include <iostream>

class Calculator {		// 계산기를 위한 클래스
public:
    Calculator(double num1 = 0, char op = ' ', double num2 = 0);	// 생성자
    void input( ) {   std::cin >> num1 >> op >> num2;  }            // 입력을 위한 함수
    double doCalculate( ) {                                         // 계산을 수행할 함수
        double result = 0.0;                                        // 결과값의 저장을 위한 변수 선언
        switch(op) {                                                // 연산자에 따라서 각각 다른 계산
        case '+': result = num1 + num2; break;                      // 덧셈
        case '-': result = num1 - num2; break;                      // 뺄셈
        case '*': case 'x': result = num1 * num2; break;            // 곱셈
        case '/': result = num1 / (double)num2; break;              // 나눗셈
        };
        return result;                                              // 결과값을 반환
    }

private:
    double num1, num2;                                              // 입력값 저장을 위한 변수
    char op;                                                        //  연산자(+, -, *, /) 저장을 위한 변수
};

#endif // CALCULATOR_H

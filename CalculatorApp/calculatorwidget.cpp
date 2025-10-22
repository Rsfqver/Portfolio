
#include "calculatorwidget.h"
#include "calculator.h"

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QButtonGroup>

#define WIDTH 4

CalculatorWidget::CalculatorWidget(QWidget *parent)
    : QWidget(parent), r(a), c(10)
{
    a = 10;
//    c = 10;
    const char ButtonChar[16][2] = {	// 문자열의 배열 : 16개의 문자열
        "7", "8", "9", "/",	// 문자열을 반드시 NULL 문자로 종료
        "4", "5", "6", "x",
        "1", "2", "3", "-",
        "0", "C", "=", "+", };
    label = new QLabel("0", this);       	// "QLabel *label;" 클래스의 멤버로 추가
//    label->setGeometry(10, 5, 250, 40);		// 위치와 크기 설정
    label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    label->setFrameShape(QFrame::Box); 	// 외각의 프레임 설정
    label->setMaximumHeight(30);		// 레이블의 최대 높이 설정

    QFont font = label->font();
    font.setPixelSize(24);
    label->setFont(font);

    QGridLayout *gridLayout = new QGridLayout( );
    QPushButton *buttons[16];
    buttonGroup = new QButtonGroup(this);

    for(int y = 0; y < WIDTH; y++)		// #define WIDTH 	4
        for(int x = 0; x < WIDTH; x++) {
            // QPushButton 생성자의 첫번째 인자로 문자열을 사용
            buttons[x+y*WIDTH] = new QPushButton(ButtonChar[x+y*WIDTH], this);
//            buttons[x+y*WIDTH]->setGeometry(5+65*x, 50+60*y, 60, 60);
            buttonGroup->addButton(buttons[x+y*WIDTH], x+y*WIDTH);
            gridLayout->addWidget(buttons[x+y*WIDTH], y, x);
            buttons[x+y*WIDTH]->setFont(font);
        }

    connect(buttonGroup, SIGNAL(idClicked(int)), SLOT(click(int)));
/*
    connect(buttons[0], SIGNAL(clicked()), SLOT(processNum()));
    connect(buttons[1], SIGNAL(clicked()), SLOT(processNum()));
    connect(buttons[2], SIGNAL(clicked()), SLOT(processNum()));
    connect(buttons[3], SIGNAL(clicked()), SLOT(processOp()));
    connect(buttons[4], SIGNAL(clicked()), SLOT(processNum()));
    connect(buttons[5], SIGNAL(clicked()), SLOT(processNum()));
    connect(buttons[6], SIGNAL(clicked()), SLOT(processNum()));
    connect(buttons[7], SIGNAL(clicked()), SLOT(processOp()));
    connect(buttons[8], SIGNAL(clicked()), SLOT(processNum()));
    connect(buttons[9], SIGNAL(clicked()), SLOT(processNum()));
    connect(buttons[10], SIGNAL(clicked()), SLOT(processNum()));
    connect(buttons[11], SIGNAL(clicked()), SLOT(processOp()));
    connect(buttons[12], SIGNAL(clicked()), SLOT(processNum()));
    connect(buttons[13], SIGNAL(clicked()), SLOT(clear()));
    connect(buttons[14], SIGNAL(clicked()), SLOT(calculate()));
    connect(buttons[15], SIGNAL(clicked()), SLOT(processOp()));
*/

    setWindowTitle("Calculator");               // 윈도우의 타이틀바 설정

    // QVBoxLayout으로 QLabel과 QGridLayout을 위젯의 메인 레이아웃으로 설정
    QVBoxLayout *vBoxLayout = new QVBoxLayout(this);
    vBoxLayout->setContentsMargins(6, 6, 6, 6);	// 외각의 마진을 모두 0으로 설정
    vBoxLayout->addWidget(label);               // 위젯을 추가
    vBoxLayout->addLayout(gridLayout);          // 레아아웃을 추가
    setLayout(vBoxLayout);
}

void CalculatorWidget::click(int id) {
    button = (QPushButton*)buttonGroup->button(id);
    switch(id) {
    case 0: case 1: case 2:
    case 4: case 5: case 6:
    case 8: case 9: case 10:
    case 12:
        processNum(); break;
    case 3: case 7: case 11: case 15:
        processOp(); break;
    case 13: clear(); break;
    case 14: calculate(); break;
    }
}

CalculatorWidget::~CalculatorWidget()
{
    if(label != nullptr)
        delete label;
}

void CalculatorWidget::processNum()
{
//    QPushButton* button = static_cast<QPushButton*>(sender());
    if(button != nullptr) {
        if(label->text().toInt() == 0)
            label->setText(button->text());
        else
            label->setText(label->text()+button->text());
    }
}

void CalculatorWidget::processOp()
{
//    QPushButton* button = dynamic_cast<QPushButton*>(sender());
    num1 = label->text();
    op = button->text();
    label->setText("0");
}

void CalculatorWidget::calculate()
{
    Calculator cal(num1.toDouble(), op[0].toLatin1(), label->text().toDouble());
    label->setText(QString::number(cal.doCalculate()));
}

void CalculatorWidget::clear()
{
    label->setText("0");
}

#ifndef CALCULATORWIDGET_H
#define CALCULATORWIDGET_H

#include <QWidget>

class QLabel;
class QButtonGroup;
class QPushButton;

class CalculatorWidget : public QWidget
{
    Q_OBJECT

public:
    CalculatorWidget(QWidget *parent = nullptr);
    ~CalculatorWidget();

private:
    QLabel* label;
    QString num1, op;
    int a;
    int &r;
    const int c;
    QButtonGroup *buttonGroup;  // QButtonGroup을 위한 멤버 객체
    QPushButton* button = nullptr;

public slots:
    void processNum();
    void processOp();
    void calculate();
    void clear();
    void click(int id);
};
#endif // CALCULATORWIDGET_H

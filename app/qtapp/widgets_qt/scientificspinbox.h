#ifndef scientificspinboxH
#define scientificspinboxH
#include <QDoubleSpinBox>

class ScientificSpinBox : public QDoubleSpinBox
{
    Q_OBJECT

public:
    ScientificSpinBox(QWidget * parent = nullptr);

    int decimals() const;
    void setDecimals(int value);

    QString textFromValue(double value) const;
    double valueFromText(const QString & text) const;

private:
    int m_displayDecimals;
    QDoubleValidator *m_validator;

private:
    QVariant validateAndInterpret(QString &input, int &pos, QValidator::State &state) const;
    QValidator::State validate(QString &text, int &pos) const;
    void fixup(QString &input) const;
    QString stripped(const QString &t, int *pos) const;
    void stepBy(int steps);
};
#endif

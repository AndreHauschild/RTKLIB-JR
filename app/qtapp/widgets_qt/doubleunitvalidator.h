#ifndef DOUBLEUNITVALIDATOR_H
#define DOUBLEUNITVALIDATOR_H

#include <QDoubleValidator>

class DoubleUnitValidator : public QDoubleValidator
{
    Q_OBJECT
public:
    DoubleUnitValidator(QObject *parent = nullptr);
    DoubleUnitValidator(double bottom, double top, int decimals, const QString &suffix, QObject *parent = nullptr);

    virtual QValidator::State validate(QString &input, int &pos) const override;
    void fixup(QString &input) const override;
protected:
    QString stripped(const QString &) const;

    QString m_suffix;
};

#endif // DOUBLEUNITVALIDATOR_H

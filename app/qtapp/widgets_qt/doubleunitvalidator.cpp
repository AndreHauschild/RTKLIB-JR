#include "doubleunitvalidator.h"

DoubleUnitValidator::DoubleUnitValidator(QObject *parent): QDoubleValidator(parent)
{
};

DoubleUnitValidator::DoubleUnitValidator(double bottom, double top, int decimals, const QString &suffix, QObject *parent)
    : QDoubleValidator(bottom, top, decimals, parent), m_suffix(suffix)
{
};

QValidator::State DoubleUnitValidator::validate(QString &input, int &pos) const
{
    QString text = stripped(input);

    return QDoubleValidator::validate(text, pos);
}

void DoubleUnitValidator::fixup(QString &input) const
{
    QString text = stripped(input);

    QDoubleValidator::fixup(text);

    input = text + m_suffix;
}

QString DoubleUnitValidator::stripped(const QString &input) const
{
    QString text = input;
    int size;

    if (m_suffix.size() && text.endsWith(m_suffix)) {
        size = text.size() - m_suffix.size();
        text = text.mid(0, size).trimmed();
    }

    return text;
}

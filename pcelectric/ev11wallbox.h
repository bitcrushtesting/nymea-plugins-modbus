#ifndef EV11WALLBOX_H
#define EV11WALLBOX_H

#include <QObject>

class Ev11Wallbox : public QObject
{
    Q_OBJECT
public:
    explicit Ev11Wallbox(QObject *parent = nullptr);

signals:
};

#endif // EV11WALLBOX_H

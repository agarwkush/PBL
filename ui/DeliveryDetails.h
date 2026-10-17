#pragma once
#include <QDialog>
class LogisticsSystem;

class DeliveryDetails : public QDialog
{
    Q_OBJECT
public:
    DeliveryDetails(LogisticsSystem* sys, int deliveryId, QWidget* parent = nullptr);
};

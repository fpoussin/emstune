#include "scalarparam.h"
#include "ui_scalarparam.h"

ScalarParam::ScalarParam(QWidget *parent)
    : QWidget(parent), ui(new Ui::ScalarParam)
{
    ui->setupUi(this);
    connect(ui->lineEdit, SIGNAL(editingFinished()), this, SLOT(lineEditFinished()));
}

ScalarParam::~ScalarParam()
{
    delete ui;
}
void ScalarParam::setName(QString name)
{
    ui->label->setText(name);
}

void ScalarParam::setConfig(ConfigData *data)
{
    connect(data, SIGNAL(update()), this, SLOT(dataUpdate()));
    m_data = data;
    ui->lineEdit->setText(QString::number(m_data->value().toFloat(), 'f', 2));
}
void ScalarParam::dataUpdate()
{
    ui->lineEdit->setText(QString::number(m_data->value().toFloat(), 'f', 2));
}
void ScalarParam::saveValue()
{
    //m_data->setValue(ui->lineEdit->text().toFloat());
    m_data->saveToFlash();
}
void ScalarParam::lineEditFinished()
{
    m_data->setValue(ui->lineEdit->text().toFloat());
}

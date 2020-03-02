#include "comboparam.h"
#include "ui_comboparam.h"

ComboParam::ComboParam(QWidget *parent)
    : QWidget(parent), ui(new Ui::ComboParam)
{
    ui->setupUi(this);
    connect(ui->comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(currentIndexChanged(int)));
}

ComboParam::~ComboParam()
{
    delete ui;
}
void ComboParam::setName(QString name)
{
    ui->label->setText(name);
}

void ComboParam::setConfig(ConfigData *data)
{
    disconnect(ui->comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(currentIndexChanged(int)));
    ui->comboBox->addItems(data->getEnumValues());
    ui->comboBox->setCurrentIndex(data->value().toInt());
    m_data = data;
    connect(ui->comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(currentIndexChanged(int)));
}

void ComboParam::saveValue()
{
    //m_data->setValue(ui->comboBox->currentIndex());
    m_data->saveToFlash();
}
void ComboParam::dataUpdate()
{
    disconnect(ui->comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(currentIndexChanged(int)));
    ui->comboBox->setCurrentIndex(m_data->value().toInt());
    connect(ui->comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(currentIndexChanged(int)));
}
void ComboParam::currentIndexChanged(int index)
{
    m_data->setValue(index);
}

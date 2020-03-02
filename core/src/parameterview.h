#ifndef PARAMETERVIEW_H
#define PARAMETERVIEW_H

#include <QWidget>
#include <QMdiSubWindow>
#include "ui_parameterview.h"
#include "configblock.h"
#include "parameterwidget.h"
#include "configdata.h"
#include "memorymetadata.h"
#include "menusetup.h"
#include "emscomms.h"
class ParameterView : public QWidget
{
    Q_OBJECT
public:
    explicit ParameterView(QWidget *parent = 0);
    void passMenuList(MenuSetup menu);
    void passConfigBlockList(QMap<QString, QList<ConfigBlock>> blocklist);
    void generateDialog(QString title, QList<DialogField> fieldlist);
    //void passEmsData(EmsData *data);
    void passMetaData(MemoryMetaData *metadata);
    void updateValues();
    double calcAxis(unsigned short val, QList<QPair<QString, double>> metadata);
    void addConfig(QString name, ConfigData *data);
    void setActiveComms(EmsComms *comms);

private:
    EmsComms *m_emsComms;
    MemoryMetaData *m_metaData;
    QList<ParameterWidget *> paramWidgetList;
    QMap<QLineEdit *, ConfigBlock> lineEditToConfigBlockMap;
    QMap<QWidget *, QList<DialogField>> widgetToFieldMap;
    //EmsData *m_emsData;
    MenuSetup m_metaMenu;
    QList<ConfigBlock> m_memoryConfigBlockList;
    Ui::Form ui;

protected:
    void closeEvent(QCloseEvent *event);
signals:
    void saveSingleData(unsigned short locationid, QByteArray data, unsigned short offset, unsigned short size);
    void windowHiding(QMdiSubWindow *parent);
    void showTable(QString table);
public slots:
    void currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *prev);
    void itemSelectionChanged();
};

#endif // PARAMETERVIEW_H

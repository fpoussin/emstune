#ifndef FIRMWAREDEBUGVIEW_H
#define FIRMWAREDEBUGVIEW_H

#include <QWidget>
#include "ui_firmwaredebugview.h"
#include <QCloseEvent>
#include <QMdiSubWindow>

class FirmwareDebugView : public QWidget
{
	Q_OBJECT

public:
	explicit FirmwareDebugView(QWidget *parent = 0);
	~FirmwareDebugView();
protected:
	void closeEvent(QCloseEvent *event);
private:
	Ui::FirmwareDebugView ui;
public slots:
	void firmwareMessage(QString text);
private slots:
	void clearButtonClicked();
};

#endif // FIRMWAREDEBUGVIEW_H

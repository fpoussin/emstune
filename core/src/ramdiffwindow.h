#ifndef RAMDIFFWINDOW_H
#define RAMDIFFWINDOW_H

#include <QWidget>

namespace Ui {
class RamDiffWindow;
}

class RamDiffWindow : public QWidget
{
	Q_OBJECT
	
public:
	explicit RamDiffWindow(QWidget *parent = 0);
	~RamDiffWindow();
	void setDirtyLocation(unsigned short locationid);
	void setAcceptText(QString text);
	void setRejectText(QString text);
private:
	Ui::RamDiffWindow *ui;
signals:
	void acceptLocalChanges();
	void rejectLocalChanges();
};

#endif // RAMDIFFWINDOW_H

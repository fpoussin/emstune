#include "firmwaredebugview.h"

FirmwareDebugView::FirmwareDebugView(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);
}

FirmwareDebugView::~FirmwareDebugView()
{
}
void FirmwareDebugView::closeEvent(QCloseEvent *event)
{
    event->ignore();
    ((QMdiSubWindow *)this->parent())->hide();
}
void FirmwareDebugView::firmwareMessage(QString text)
{
    ui.debugPlainTextEdit->moveCursor(QTextCursor::End);
    ui.debugPlainTextEdit->insertPlainText(text);
    ui.debugPlainTextEdit->moveCursor(QTextCursor::End);
    ui.debugPlainTextEdit->ensureCursorVisible();
}
void FirmwareDebugView::clearButtonClicked()
{
    ui.debugPlainTextEdit->clear();
}

#include "pluginmanager.h"
#include <QDir>
#include <QFile>
#include <QPluginLoader>
#include <QRadioButton>

#define define2string_p(x) #x
#define define2string(x) define2string_p(x)

PluginManager::PluginManager(QWidget *parent) : QWidget(parent)
{
	ui.setupUi(this);
	/*
	pluginLoader = new QPluginLoader(this);
	pluginLoader->setFileName(m_pluginFileName);
	qDebug() << pluginLoader->metaData();
	for (QJsonObject::const_iterator i = pluginLoader->metaData().constBegin();i!=pluginLoader->metaData().constEnd();i++)
	{
		qDebug() << i.key() << i.value();
	}

	QLOG_INFO() << "Attempting to load plugin:" << m_pluginFileName;
*/
	QStringList dirlist;
	dirlist.append("plugins"); //Default local
	dirlist.append(QString(define2string(INSTALL_PREFIX)) + "/share/emstudio/plugins"); //make installed on linux
	foreach (QString dirstr,dirlist)
	{
		QDir dir(dirstr);
		foreach (QString file,dir.entryList())
		{
			QString absolutepath = dir.absoluteFilePath(file);
			QPluginLoader loader(absolutepath);
			QJsonObject meta = loader.metaData();
			if (meta.contains("IID"))
			{
				QRadioButton *button = new QRadioButton(this);
				connect(button,SIGNAL(clicked()),this,SLOT(radioButtonClicked()));
				button->setText(meta.value("IID").toString());
				QVBoxLayout *layout = qobject_cast<QVBoxLayout*>(ui.groupBox->layout());
				layout->insertWidget(1,button);
				m_buttonToFilenameMap.insert(button,absolutepath);
			}

		}
	}
	connect(ui.savePushButton,SIGNAL(clicked()),this,SLOT(selectClicked()));
	connect(ui.cancelPushButton,SIGNAL(clicked()),this,SLOT(cancelClicked()));
}

PluginManager::~PluginManager()
{
}
void PluginManager::radioButtonClicked()
{
	QRadioButton *send = qobject_cast<QRadioButton*>(sender());
	if (m_buttonToFilenameMap.contains(send))
	{
		m_currentFilename = m_buttonToFilenameMap.value(send);
	}
}
void PluginManager::selectClicked()
{
	emit fileSelected(m_currentFilename);
	this->close();
}

void PluginManager::cancelClicked()
{
	this->close();
}

// $(SolutionDir)\upx391w\upx.exe -8 "$(TargetPath)"

#include <QtPlugin>
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin);

#include "MainWindow.h"

#include <QApplication>
#include <QLocale>
#include <QDateTime>
#include <QDebug>

#ifdef _DEBUG
static QString g_strLogFile = "";
#endif

void myMessageOutput(QtMsgType type, const QMessageLogContext& ctx, const QString& msg)
{
#ifndef _DEBUG
	if (type != QtFatalMsg)
		return;
	else
		_exit(1);
#else
	QString txt;

	switch (type)
	{
	case QtDebugMsg:
		txt = QString("%1: Debug: %2").arg(QDateTime::currentDateTime().toString(Qt::ISODate)).arg(msg);
		break;
	case QtWarningMsg:
		txt = QString("%1: Warning: %2").arg(QDateTime::currentDateTime().toString(Qt::ISODate)).arg(msg);
		break;
	case QtCriticalMsg:
		txt = QString("%1: Critical: %2").arg(QDateTime::currentDateTime().toString(Qt::ISODate)).arg(msg);
		break;
	case QtFatalMsg:
		txt = QString("%1: Fatal: %2").arg(QDateTime::currentDateTime().toString(Qt::ISODate)).arg(msg);
		break;
	}

	{ // stack frame to ensure output even id abort() called below
		QFile outFile(g_strLogFile);
		outFile.open(QIODevice::WriteOnly | QIODevice::Append);
		QTextStream ts(&outFile);
		ts << txt << endl;
	}

	if (type == QtFatalMsg)
		abort();
#endif
}

int main(int argc, char *argv[])
{
#ifdef _DEBUG
	g_strLogFile = tempFolderPath() + "\\" + QString("_tmp%1_mp4video1click").arg(argc) + ".log";
#endif

	qInstallMessageHandler(myMessageOutput);

	////////////////////////////////////

	QApplication a(argc, argv);
	QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));
	a.setStyleSheet("QMessageBox { messagebox-text-interaction-flags: 5; }");

	qDebug() << Q_FUNC_INFO << "QApplication created";

	MainWindow w(0, Qt::WindowStaysOnTopHint);
	w.show();
	w.activateWindow();
	w.raise();

	return a.exec();
}

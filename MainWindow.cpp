#include "MainWindow.h"

#include <QCoreApplication>
#include <QApplication>
#include <QProcessEnvironment>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QVBoxLayout>
#include <QProgressBar>
#include <QPushButton>
#include <QLabel>
#include <QSizePolicy>
#include <QStatusBar>
#include <QMessageBox>
#include <QIcon>
#include <QFileInfo>
#include <QDir>
#include <QTime>
#include <QTimer>
#include <QDebug>
#include <QTemporaryFile>

#include <cmath>

#include <Shobjidl.h>

static ITaskbarList3* s_pTaskBarlist = NULL;

/////////////////////////////////////////////////////////

#define VERSION_FFMPEG_WINDOWS_1_CLICK 100


MainWindow::MainWindow(QWidget *parent, Qt::WindowFlags flags)
    :   QMainWindow(parent, flags)
	,	m_pLayoutWidget(0)
	,	m_pConsoleTextEdit(0)
	,	m_pShowMorePushButton(0)
	,	m_nTotalFrames(0)
	,	m_fMeanVolume(1E11)
	,	m_fMaxVolume(1E11)
	,	m_fMinLuminosity(1E11)
	,	m_fMaxLuminosity(1E11)
	,	m_nHeight(0)
	,	m_bNoCurves(false)
	,	m_bStreamFound(false)
	,	m_nHeightFromVideo(0)
	,	m_bDuringClose(false)
	,	m_nCurrentFrames(0)
	,	m_bExtractMp3(false)
{

	if (s_pTaskBarlist == NULL)
		::CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_ALL, IID_ITaskbarList3, (void**)&s_pTaskBarlist);

	if (s_pTaskBarlist)
		s_pTaskBarlist->SetProgressState( (HWND)effectiveWinId(), TBPF_INDETERMINATE );

	QIcon icon(":/MainWindow/Resources/favicon_source.png");
	setWindowIcon(icon);

	if (isFFMPEGInputFile(QCoreApplication::arguments()[QCoreApplication::arguments().size() - 1]))
	{
		if (isConvertToStdMp4Requested(QCoreApplication::arguments()))
		{
			createVisualLayout(VisualLayout_Convert);
			
			bool bOk = false;
			m_nHeight = QCoreApplication::arguments()[2].toInt(&bOk);

			if (!bOk) // m_nHeight == 0 => no scale, but may be also "no curves" requested?
			{
				if (QCoreApplication::arguments()[2].toUpper() == "NSNC")
					m_bNoCurves = true;
			}

			analyseBeforeConvertToStdMp4(QCoreApplication::arguments()[3]);
		}
		else
		if (isExtractNormalizedMp3Requested(QCoreApplication::arguments()))
		{
			m_bExtractMp3 = true;
			createVisualLayout(VisualLayout_Convert);
			analyseBeforeExtractNormalizedMp3(QCoreApplication::arguments()[3]);
		}
		else
		if (isPlayRequested(QCoreApplication::arguments()))
		{
			createVisualLayout(VisualLayout_Play);

			play(QCoreApplication::arguments()[2]);
		}
		if (isConsoleRequested(QCoreApplication::arguments()))
		{
			createVisualLayout(VisualLayout_Console);

			console(QCoreApplication::arguments()[2], QCoreApplication::arguments()[3]);
		}
	}
	else
	{
		QTimer::singleShot(500, this, SLOT(close()));
	}
}

QString MainWindow::fullPathNameToExecutable(const QString& strNameExt, bool b64)
{
	QString strInstallPath = QFileInfo(QCoreApplication::applicationFilePath()).absolutePath();
	QString strExeFile = strInstallPath + QString("/%1/%2").arg(b64 ? "x64" : "win32").arg(strNameExt);
	return strExeFile;
}

QString MainWindow::fullDirNameToExecutable(const QString& strNameExt, bool b64)
{
	QString strInstallPath = QFileInfo(QCoreApplication::applicationFilePath()).absolutePath();
	QString strExeDir = strInstallPath + QString("/%1").arg(b64 ? "x64" : "win32");
	return strExeDir;
}

void MainWindow::createVisualLayout(VisualLayout vl)
{
	int nHeightForAppFont = QApplication::font().pointSize();

	setMenuBar(new QMenuBar(this));
	QMenu* pMenuHelp = menuBar()->addMenu(tr("Help"));

	QAction* pActionHelp = pMenuHelp->addAction(tr("Online help about FFMPEG"));
	QAction* pActionAbout = pMenuHelp->addAction(tr("About this program"));

	QObject::connect(pActionHelp, SIGNAL(triggered()), this, SLOT(onTriggeredHelp()));
	QObject::connect(pActionAbout, SIGNAL(triggered()), this, SLOT(onTriggeredAbout()));

	m_pLayoutWidget = new QWidget(this);

	QVBoxLayout* pLayout = new QVBoxLayout(m_pLayoutWidget);
	pLayout->setContentsMargins(0, 0, 0, 0);
	pLayout->setSpacing(0);

	// Widget 0 - QProgressBar

	pLayout->insertWidget(0, new QProgressBar(m_pLayoutWidget));

	m_pLayoutWidget->layout()->itemAt(0)->widget()->setMinimumHeight(nHeightForAppFont * 4);
	m_pLayoutWidget->layout()->itemAt(0)->widget()->setMaximumHeight(nHeightForAppFont * 4);

	m_pLayoutWidget->layout()->itemAt(0)->widget()->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));

	qobject_cast<QProgressBar*>(m_pLayoutWidget->layout()->itemAt(0)->widget())->setMinimum(0);
	qobject_cast<QProgressBar*>(m_pLayoutWidget->layout()->itemAt(0)->widget())->setMaximum(100);
	qobject_cast<QProgressBar*>(m_pLayoutWidget->layout()->itemAt(0)->widget())->setValue(0);
	if (s_pTaskBarlist)
		s_pTaskBarlist->SetProgressValue( (HWND)effectiveWinId(), 0, 100 );

	if ((vl == VisualLayout_Play) || (vl == VisualLayout_Console))
		m_pLayoutWidget->layout()->itemAt(0)->widget()->hide();

	// Widget 1 - QPushButton

	pLayout->insertWidget(1, new QPushButton(m_pLayoutWidget));

	m_pLayoutWidget->layout()->itemAt(1)->widget()->setMinimumWidth(768);

	m_pLayoutWidget->layout()->itemAt(1)->widget()->setMinimumHeight(nHeightForAppFont * 4);
	m_pLayoutWidget->layout()->itemAt(1)->widget()->setMaximumHeight(nHeightForAppFont * 4);

	m_pLayoutWidget->layout()->itemAt(1)->widget()->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));

	qobject_cast<QPushButton*>(m_pLayoutWidget->layout()->itemAt(1)->widget())->setText(tr("Show details..."));

	bool b = QObject::connect(m_pLayoutWidget->layout()->itemAt(1)->widget(), SIGNAL(clicked()), this, SLOT(onToggleShowHideDetails()));

	// Widget 2 - QLabel

	pLayout->insertWidget(2, new QLabel(m_pLayoutWidget), 1000);

	m_pLayoutWidget->layout()->itemAt(2)->widget()->setMinimumHeight(nHeightForAppFont * 8);
	m_pLayoutWidget->layout()->itemAt(2)->widget()->setMaximumHeight(nHeightForAppFont * 8);

	m_pLayoutWidget->layout()->itemAt(2)->widget()->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));

	qobject_cast<QLabel*>(m_pLayoutWidget->layout()->itemAt(2)->widget())->setText(tr("Use mouse right click-moves to navigate through video timeline, position from the left edge of the video is proportional to the time selected.\nMinimize this window, then focus on the video window and press SPACE to play/pause the video, ESC to close the player."));

	if ((vl == VisualLayout_Convert) || (vl == VisualLayout_Console))
		m_pLayoutWidget->layout()->itemAt(2)->widget()->hide();

	// Widget 3 - QTextEdit

	pLayout->insertWidget(3, new QTextEdit(m_pLayoutWidget), 1000);

	m_pLayoutWidget->layout()->itemAt(3)->widget()->setMinimumHeight(256);

	m_pLayoutWidget->layout()->itemAt(3)->widget()->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));

	qobject_cast<QTextEdit*>(m_pLayoutWidget->layout()->itemAt(3)->widget())->setLineWrapMode(QTextEdit::NoWrap);
	qobject_cast<QTextEdit*>(m_pLayoutWidget->layout()->itemAt(3)->widget())->setHtml("");
	qobject_cast<QTextEdit*>(m_pLayoutWidget->layout()->itemAt(3)->widget())->setReadOnly(true);

	pLayout->addStretch(0);

	setCentralWidget(m_pLayoutWidget);

	m_pLayoutWidget->layout()->itemAt(3)->widget()->hide();

	m_pConsoleTextEdit = qobject_cast<QTextEdit*>(m_pLayoutWidget->layout()->itemAt(3)->widget());
	m_pShowMorePushButton = qobject_cast<QPushButton*>(m_pLayoutWidget->layout()->itemAt(1)->widget());
}

void MainWindow::keyPressEvent(QKeyEvent* pEvent)
{
	if(pEvent->key() == Qt::Key_Escape)
	{
		pEvent->accept();
		close();
	}
	else
	{
		QMainWindow::keyPressEvent(pEvent);
	}
}

void MainWindow::killExtProcessIfRunning()
{
	qDebug() << Q_FUNC_INFO;

	if (m_pExtProgProcess && (m_pExtProgProcess->state() == QProcess::Running))
		m_pExtProgProcess->kill();

	m_pExtProgProcess.reset(0);
}

void MainWindow::closeEvent(QCloseEvent* pEvent)
{
	qDebug() << Q_FUNC_INFO;

	m_bDuringClose = true;

	killExtProcessIfRunning();

	pEvent->accept();
}

void MainWindow::onTriggeredHelp()
{
	const QString strStyleCSS = "<style>a:link { color: Navy; text-decoration: none; }</style>";
	const QString strHomeHttpM = "<a href='https://www.ffmpeg.org/'><b>https://www.ffmpeg.org/</b></a>";
	QString strHomeM = strStyleCSS + strHomeHttpM;

	QMessageBox msgBox(0);
	msgBox.setModal(true);
	msgBox.setTextFormat(Qt::RichText);
	msgBox.addButton(QMessageBox::Close);
	msgBox.setWindowTitle(tr("Help - Mp4 Video 1 Click"));
	msgBox.setText(tr("Visit official FFMPEG site for up-to-date online help:<br/>%1.").arg(strHomeM));
	msgBox.setInformativeText("<small>" + tr("FFMPEG is very sophisticated and rapidly developing tool, so it is impossible to embed all the comprehensive info about it in this application package.") + "</small>");

	QIcon icon(":/MainWindow/Resources/favicon_source.png");
	msgBox.setWindowIcon(icon);
	msgBox.setIcon(QMessageBox::Information);

	msgBox.exec();
}

static QString stringVersion(int nVer)
{
	int nFix = nVer % 10;
	nVer /= 10;
	int nMinor = nVer % 10;
	nVer /= 10;
	int nMajor = nVer % 10;

	QString strResult = QString("%1.%2.%3").arg(nMajor).arg(nMinor).arg(nFix);
	return strResult;
}

void MainWindow::onTriggeredAbout()
{
	const QString strStyleCSS = "<style>a:link { color: Navy; text-decoration: none; }</style>";
	//const QString strHomeHttp = "<a href='http://2048-windows-download.en.softonic.com/'><b>http://2048-windows-download.en.softonic.com/</b></a>";
	const QString strHomeHttpM = "<a href='https://sourceforge.net/projects/mp4video1click/'><b>https://sourceforge.net/projects/mp4video1click/</b></a><br/>or<br/><a href='https://github.com/federicadomani/Mp4-Video-1-Click'><b>https://github.com/federicadomani/Mp4-Video-1-Click</b></a>";
	const QString trCompanyNameJuridical = MainWindow::tr("Open Source Developer Federica Domani (federicadomani.wordpress.com)");

	QString strVer = stringVersion(VERSION_FFMPEG_WINDOWS_1_CLICK);
	//QString strHome = strStyleCSS + strHomeHttp;
	QString strHomeM = strStyleCSS + strHomeHttpM;

	QMessageBox msgBox(0);
	msgBox.setModal(true);
	msgBox.setTextFormat(Qt::RichText);
	msgBox.addButton(QMessageBox::Close);
	msgBox.setWindowTitle(tr("About - Mp4 Video 1 Click"));
	msgBox.setText(tr("<b>Mp4 Video 1 Click</b>,<br/>version <b>%1</b>,<br/>build %2 - %3.<br/><br/>Visit our&nbsp;site:<br/>%4.<br/><br/>Donate bitcoins to 1ENPhPJ1k8q3k2SWqieeNFdLG4Mp1zXFkc.<br/><br/>").arg(strVer).arg(__DATE__).arg(__TIME__).arg(strHomeM));
	msgBox.setInformativeText("<small>" + tr("Copyright (c) 2017-2018 ") + trCompanyNameJuridical + ".</small>");

	QIcon icon(":/MainWindow/Resources/favicon_source.png");
	msgBox.setWindowIcon(icon);
	msgBox.setIcon(QMessageBox::Information);

	msgBox.exec();
}

void MainWindow::onToggleShowHideDetails()
{
	if (m_pConsoleTextEdit->isVisible())
	{
		m_pConsoleTextEdit->hide();
		m_pShowMorePushButton->setText(tr("Show details..."));
	}
	else
	{
		m_pConsoleTextEdit->show();
		m_pShowMorePushButton->setText(tr("Hide details..."));
	}
}

bool MainWindow::isFFMPEGInputFile(const QString& strInputFilePath)
{
	QFileInfo fiInputFile(strInputFilePath);

	setWindowTitle(tr("%1 - Mp4 Video 1 Click").arg(fiInputFile.fileName()));

	if (fiInputFile.isReadable())
	{
		m_strInputFilePath = strInputFilePath;
		return true;
	}
	else
	{
		m_strInputFilePath = "";
		return false;
	}
}

bool MainWindow::isConvertToStdMp4Requested(const QStringList& lstCommandArgs)
{
	if (lstCommandArgs.size() != 4)
		return false;

	if (lstCommandArgs[1].toUpper() == "-MP4")
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool MainWindow::isExtractNormalizedMp3Requested(const QStringList& lstCommandArgs)
{
	if (lstCommandArgs.size() != 4)
		return false;

	if (lstCommandArgs[1].toUpper() == "-MP3")
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool MainWindow::isPlayRequested(const QStringList& lstCommandArgs)
{
	if (lstCommandArgs.size() != 3)
		return false;

	if (lstCommandArgs[1].toUpper() == "-PLAY")
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool MainWindow::isConsoleRequested(const QStringList& lstCommandArgs)
{
	if (lstCommandArgs.size() != 4)
		return false;

	if (lstCommandArgs[1].toUpper() == "-CONSOLE")
	{
		return true;
	}
	else
	{
		return false;
	}
}

static bool isWindows64bitCompliant()
{
	wchar_t buf[65536];
	UINT nRes = ::GetSystemWow64DirectoryW(buf, 65536);

	bool b64bit = (nRes != 0);

	return b64bit;
}

void MainWindow::analyseBeforeConvertToStdMp4(const QString& strInputFilePath)
{
	QTemporaryFile tempPaletteFile(QDir::tempPath() + "/XXXXXX_ffmpeg_palette.png");
	tempPaletteFile.setAutoRemove(false);

	if (tempPaletteFile.open())
	{
		m_strTempPaletteFilePath = tempPaletteFile.fileName();
		tempPaletteFile.close();
	}

	m_pConsoleTextEdit->append(tr("Analysing file %1 for video brightness and stereo volume before it will be converted...").arg(QFileInfo(strInputFilePath).fileName()));

	QString strExeFile = fullPathNameToExecutable("ffmpeg.exe", isWindows64bitCompliant());

	QFileInfo fiInputFile(strInputFilePath);
	QString strWorkingDir = fiInputFile.absolutePath();

	m_lstConstDetectLuminosityVolumeArgs << "-i" << "INPUT"
		<< "-vf" << "select='eq(pict_type\\,I)',crop=2/3*in_w:2/3*in_h,palettegen=max_colors=256:reserve_transparent=0:stats_mode=full"
		<< "-vsync" << "vfr" << "PALETTE.png"
		<< "-af" << "volumedetect"
		<< "-c:v" << "copy"
		<< "-f" << "null"
		<< "-y" << "NUL";

	m_lstActualArgs = m_lstConstDetectLuminosityVolumeArgs;
	m_lstActualArgs[1] = strInputFilePath;
	m_lstActualArgs[6] = m_strTempPaletteFilePath;

	m_lstActualShowArgs = m_lstActualArgs;
	m_lstActualShowArgs[1] = QString("<b><font color='blue'>%1</font></b>").arg(QFileInfo(m_lstActualArgs[1]).fileName());
	m_lstActualShowArgs[6] = QString("<b><font color='magenta'>%1</font></b>").arg(QFileInfo(m_lstActualArgs[6]).fileName());

	if (QFileInfo(strExeFile).exists() && QFileInfo(strExeFile).isExecutable())
	{
		startExecutableToAnalyseMp4(strExeFile, m_lstActualArgs, m_lstActualShowArgs, strWorkingDir);
	}
	else
	{
		m_pConsoleTextEdit->append(tr("<font color='red'><b>Error: no executable file path found: %1.</b></font>").arg(strExeFile));

		if (!m_pConsoleTextEdit->isVisible() && !m_bDuringClose)
		{
			QMetaObject::invokeMethod(m_pShowMorePushButton, "click");
		}
	}
}

void MainWindow::analyseBeforeExtractNormalizedMp3(const QString& strInputFilePath)
{
	m_pConsoleTextEdit->append(tr("Analysing file %1 for stereo volume before normalized mp3 will be extracted...").arg(QFileInfo(strInputFilePath).fileName()));

	QString strExeFile = fullPathNameToExecutable("ffmpeg.exe", isWindows64bitCompliant());

	QFileInfo fiInputFile(strInputFilePath);
	QString strWorkingDir = fiInputFile.absolutePath();

	m_lstConstDetectOnlyVolumeArgs << "-i" << "INPUT"
		<< "-af" << "volumedetect"
		<< "-c:v" << "copy"
		<< "-f" << "null"
		<< "-y" << "NUL";

	m_lstActualArgs = m_lstConstDetectOnlyVolumeArgs;
	m_lstActualArgs[1] = strInputFilePath;

	m_lstActualShowArgs = m_lstActualArgs;
	m_lstActualShowArgs[1] = QString("<b><font color='blue'>%1</font></b>").arg(QFileInfo(m_lstActualArgs[1]).fileName());

	if (QFileInfo(strExeFile).exists() && QFileInfo(strExeFile).isExecutable())
	{
		startExecutableToAnalyseMp3(strExeFile, m_lstActualArgs, m_lstActualShowArgs, strWorkingDir);
	}
	else
	{
		m_pConsoleTextEdit->append(tr("<font color='red'><b>Error: no executable file path found: %1.</b></font>").arg(strExeFile));

		if (!m_pConsoleTextEdit->isVisible() && !m_bDuringClose)
		{
			QMetaObject::invokeMethod(m_pShowMorePushButton, "click");
		}
	}
}

void MainWindow::convertToMpegDashMp4()
{

}

void MainWindow::play(const QString& strInputFilePath)
{
	m_pConsoleTextEdit->append(tr("Playing file %1 ...").arg(QFileInfo(strInputFilePath).fileName()));

	QString strExeFile = fullPathNameToExecutable("ffplay.exe", isWindows64bitCompliant());

	QFileInfo fiInputFile(strInputFilePath);
	QString strWorkingDir = fiInputFile.absolutePath();

	m_lstConstPlayArgs << "INPUT";

	m_lstActualArgs = m_lstConstPlayArgs;
	m_lstActualArgs[0] = strInputFilePath;

	m_lstActualShowArgs = m_lstActualArgs;
	m_lstActualShowArgs[0] = QString("<b><font color='blue'>%1</font></b>").arg(QFileInfo(m_lstActualArgs[0]).fileName());

	if (QFileInfo(strExeFile).exists() && QFileInfo(strExeFile).isExecutable())
	{
		startExecutableToPlay(strExeFile, m_lstActualArgs, m_lstActualShowArgs, strWorkingDir);
		QTimer::singleShot(3000, this, SLOT(showMinimized()));
	}
	else
	{
		m_pConsoleTextEdit->append(tr("<font color='red'><b>Error: no executable file path found: %1.</b></font>").arg(strExeFile));

		if (!m_pConsoleTextEdit->isVisible() && !m_bDuringClose)
		{
			QMetaObject::invokeMethod(m_pShowMorePushButton, "click");
		}
	}
}

void MainWindow::startExecutableToPlay(const QString& strExeFile, const QStringList& lstExeArgs, const QStringList& lstExeShowArgs, const QString& strWorkingDir)
{
	killExtProcessIfRunning();

	m_pExtProgProcess.reset(new QProcess(this));

	m_pExtProgProcess->setWorkingDirectory(strWorkingDir);
	m_pExtProgProcess->setProcessChannelMode(QProcess::MergedChannels);
	QObject::connect(m_pExtProgProcess.data(), SIGNAL(started()), this, SLOT(onExePlayStarted()));
	QObject::connect(m_pExtProgProcess.data(), SIGNAL(error(QProcess::ProcessError)), this, SLOT(onExePlayError(QProcess::ProcessError)));
	QObject::connect(m_pExtProgProcess.data(), SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(onExePlayFinished(int, QProcess::ExitStatus)));
	QObject::connect(m_pExtProgProcess.data(), SIGNAL(readyReadStandardOutput()), this, SLOT(onExePlayReadyReadStandardOutput()));
	m_pConsoleTextEdit->append(tr("Play: starting executable: %1.").arg(strExeFile));
	m_pExtProgProcess->start(strExeFile, lstExeArgs);

	m_pConsoleTextEdit->append(tr("Working folder: %1.").arg(strWorkingDir));
	m_pConsoleTextEdit->append(QFileInfo(strExeFile).fileName() + " " + lstExeShowArgs.join(QChar(' ')));
}

static QString my_qt_create_commandline(const QString &program, const QStringList &arguments)
{
	QString args;
	if (!program.isEmpty()) {
		QString programName = program;
		if (!programName.startsWith(QLatin1Char('\"')) && !programName.endsWith(QLatin1Char('\"')) && programName.contains(QLatin1Char(' ')))
			programName = QLatin1Char('\"') + programName + QLatin1Char('\"');
		programName.replace(QLatin1Char('/'), QLatin1Char('\\'));

		// add the prgram as the first arg ... it works better
		args = programName + QLatin1Char(' ');
	}

	for (int i = 0; i<arguments.size(); ++i) {
		QString tmp = arguments.at(i);
		// Quotes are escaped and their preceding backslashes are doubled.
		tmp.replace(QRegExp(QLatin1String("(\\\\*)\"")), QLatin1String("\\1\\1\\\""));
		if (tmp.isEmpty() || tmp.contains(QLatin1Char(' ')) || tmp.contains(QLatin1Char('\t'))) {
			// The argument must not end with a \ since this would be interpreted
			// as escaping the quote -- rather put the \ behind the quote: e.g.
			// rather use "foo"\ than "foo\"
			int i = tmp.length();
			while (i > 0 && tmp.at(i - 1) == QLatin1Char('\\'))
				--i;
			tmp.insert(i, QLatin1Char('"'));
			tmp.prepend(QLatin1Char('"'));
		}
		args += QLatin1Char(' ') + tmp;
	}
	return args;
}

static bool my_startDetached(const QString &program, const QStringList &arguments, const QString &workingDir)
{
	static const DWORD errorElevationRequired = 740;

	QString args = my_qt_create_commandline(program, arguments);
	bool success = false;
	PROCESS_INFORMATION pinfo;

	DWORD dwCreationFlags = 0; // Federica changed
	dwCreationFlags |= CREATE_UNICODE_ENVIRONMENT;
	STARTUPINFOW startupInfo = { sizeof(STARTUPINFO), 0, 0, 0,
		(ulong)CW_USEDEFAULT, (ulong)CW_USEDEFAULT,
		(ulong)CW_USEDEFAULT, (ulong)CW_USEDEFAULT,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};
	success = CreateProcess(0, (wchar_t*)args.utf16(),
		0, 0, FALSE, dwCreationFlags, 0,
		workingDir.isEmpty() ? 0 : (wchar_t*)workingDir.utf16(),
		&startupInfo, &pinfo);

	if (success) {
		CloseHandle(pinfo.hThread);
		CloseHandle(pinfo.hProcess);
	}

	return success;
}

void MainWindow::console(const QString& str64bit, const QString& strInputFilePath)
{
	QFileInfo fiInputFile(strInputFilePath);
	QString strWorkingDir = fiInputFile.absolutePath();
	QString strInputName = fiInputFile.fileName();

	QString strFileName = fiInputFile.fileName();
	QStringList lstFileNameParts = strFileName.split(QChar('.'), QString::SkipEmptyParts);
	QString strBaseName = lstFileNameParts.join(QChar('-'));

	QString strOutputFile = strBaseName + QChar('-') + QString("p-console.mp4");

	QString strBatFile = fullPathNameToExecutable("ffconsole.bat", (str64bit == "64") ? true : false);
	QString strExeFile = QProcessEnvironment::systemEnvironment().value("comspec", "c:\\Windows\\System32\\cmd.exe");

	QString strFFMPEGDir = fullDirNameToExecutable("ffconsole.bat", (str64bit == "64") ? true : false);

	QStringList lstExeArgs;
	lstExeArgs << "/k" << strBatFile << strInputName << strOutputFile << strFFMPEGDir;

	QFileInfo fiBatFile(strBatFile);

	if (fiBatFile.exists() && fiBatFile.isExecutable())
	{
		m_pConsoleTextEdit->append(tr("Console: starting batch: %1.").arg(strBatFile));
		bool tmp = my_startDetached(strExeFile, lstExeArgs, strWorkingDir);
		QTimer::singleShot(100, this, SLOT(close()));
	}
	else
	{
		m_pConsoleTextEdit->append(tr("<font color='red'><b>Error: console: no batch file found: %1.</b></font>").arg(strBatFile));

		if (!m_pConsoleTextEdit->isVisible() && !m_bDuringClose)
		{
			QMetaObject::invokeMethod(m_pShowMorePushButton, "click");
		}
	}
}

void MainWindow::startExecutableToAnalyseMp4(const QString& strExeFile, const QStringList& lstExeArgs, const QStringList& lstExeShowArgs, const QString& strWorkingDir)
{
	killExtProcessIfRunning();

	m_pExtProgProcess.reset(new QProcess(this));

	m_pExtProgProcess->setWorkingDirectory(strWorkingDir);
	m_pExtProgProcess->setProcessChannelMode(QProcess::MergedChannels);
	QObject::connect(m_pExtProgProcess.data(), SIGNAL(started()), this, SLOT(onExeAnalyseStarted()));
	QObject::connect(m_pExtProgProcess.data(), SIGNAL(error(QProcess::ProcessError)), this, SLOT(onExeAnalyseError(QProcess::ProcessError)));
	QObject::connect(m_pExtProgProcess.data(), SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(onExeAnalyseFinishedMp4(int, QProcess::ExitStatus)));
	QObject::connect(m_pExtProgProcess.data(), SIGNAL(readyReadStandardOutput()), this, SLOT(onExeAnalyseReadyReadStandardOutput()));
	m_pConsoleTextEdit->append(tr("Analyze: starting executable: %1.").arg(strExeFile));
	m_pExtProgProcess->start(strExeFile, lstExeArgs);

	m_pConsoleTextEdit->append(tr("Working folder: %1.").arg(strWorkingDir));
	m_pConsoleTextEdit->append(QFileInfo(strExeFile).fileName() + " " + lstExeShowArgs.join(QChar(' ')));
}

void MainWindow::startExecutableToAnalyseMp3(const QString& strExeFile, const QStringList& lstExeArgs, const QStringList& lstExeShowArgs, const QString& strWorkingDir)
{
	killExtProcessIfRunning();

	m_pExtProgProcess.reset(new QProcess(this));

	m_pExtProgProcess->setWorkingDirectory(strWorkingDir);
	m_pExtProgProcess->setProcessChannelMode(QProcess::MergedChannels);
	QObject::connect(m_pExtProgProcess.data(), SIGNAL(started()), this, SLOT(onExeAnalyseStarted()));
	QObject::connect(m_pExtProgProcess.data(), SIGNAL(error(QProcess::ProcessError)), this, SLOT(onExeAnalyseError(QProcess::ProcessError)));
	QObject::connect(m_pExtProgProcess.data(), SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(onExeAnalyseFinishedMp3(int, QProcess::ExitStatus)));
	QObject::connect(m_pExtProgProcess.data(), SIGNAL(readyReadStandardOutput()), this, SLOT(onExeAnalyseReadyReadStandardOutput()));
	m_pConsoleTextEdit->append(tr("Analyze: starting executable: %1.").arg(strExeFile));
	m_pExtProgProcess->start(strExeFile, lstExeArgs);

	m_pConsoleTextEdit->append(tr("Working folder: %1.").arg(strWorkingDir));
	m_pConsoleTextEdit->append(QFileInfo(strExeFile).fileName() + " " + lstExeShowArgs.join(QChar(' ')));
}

void MainWindow::startExecutableToConvert(const QString& strExeFile, const QString& strWorkingDir)
{
	QStringList lstExeArgs = m_lstActualArgs;
	QStringList lstExeShowArgs = m_lstActualShowArgs;

	bool bOkFps = false;

	double fFps = m_strFpsValue.toDouble(&bOkFps);

	if (bOkFps)
	{
		if (!m_bExtractMp3)
		{
			if (fFps > 30)
			{
				lstExeArgs.insert(2, "25");
				lstExeArgs.insert(2, "-r");

				lstExeShowArgs.insert(2, "25");
				lstExeShowArgs.insert(2, "-r");
			}
		}
	}
	else
	{
		if (!m_bExtractMp3)
		{
			m_pConsoleTextEdit->append(tr("<font color='orange'><b>Warning: Can't detect FPS, thus I take no care of it.</b></font>"));

			if (!m_pConsoleTextEdit->isVisible() && !m_bDuringClose)
			{
				QMetaObject::invokeMethod(m_pShowMorePushButton, "click");
			}
		}
	}

	killExtProcessIfRunning();

	m_pExtProgProcess.reset(new QProcess(this));

	m_pExtProgProcess->setWorkingDirectory(strWorkingDir);
	m_pExtProgProcess->setProcessChannelMode(QProcess::MergedChannels);
	QObject::connect(m_pExtProgProcess.data(), SIGNAL(started()), this, SLOT(onExeConvertStarted()));
	QObject::connect(m_pExtProgProcess.data(), SIGNAL(error(QProcess::ProcessError)), this, SLOT(onExeConvertError(QProcess::ProcessError)));
	QObject::connect(m_pExtProgProcess.data(), SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(onExeConvertFinished(int, QProcess::ExitStatus)));
	QObject::connect(m_pExtProgProcess.data(), SIGNAL(readyReadStandardOutput()), this, SLOT(onExeConvertReadyReadStandardOutput()));
	m_pConsoleTextEdit->append(tr("Convert: starting executable: %1.").arg(strExeFile));
	m_pExtProgProcess->start(strExeFile, lstExeArgs);

	m_pConsoleTextEdit->append(tr("Working folder: %1.").arg(strWorkingDir));
	m_pConsoleTextEdit->append(QFileInfo(strExeFile).fileName() + " " + lstExeShowArgs.join(QChar(' ')));
}

void MainWindow::onExeAnalyseStarted()
{
	m_pConsoleTextEdit->append(tr("...started."));
}

void MainWindow::onExeAnalyseError(QProcess::ProcessError e)
{
	m_pConsoleTextEdit->append(tr("<font color='red'><b>Error: during analyze: process error %1.</b></font>").arg(e));

	if (!m_pConsoleTextEdit->isVisible() && !m_bDuringClose)
	{
		QMetaObject::invokeMethod(m_pShowMorePushButton, "click");
	}
}

void MainWindow::onExeAnalyseFinishedMp4(int exitCode, QProcess::ExitStatus exitStatus)
{
	if ((exitCode == 0) && (exitStatus == QProcess::NormalExit))
	{
		m_pConsoleTextEdit->append(tr("Analyzing finished."));

		// Here there is a place to start palette analyzing and actual converting.

		QImage imgPalette(m_strTempPaletteFilePath);

		double fYmin = 1.0;
		double fYmax = 0.0;

		for (int x = 0; x < imgPalette.size().width(); ++x)
		{
			for (int y = 0; y < imgPalette.size().height(); ++y)
			{
				QRgb pixValue = imgPalette.pixel(x, y);
				double fRed = double(qRed(pixValue)) / double(255);
				double fGreen = double(qGreen(pixValue)) / double(255);
				double fBlue = double(qBlue(pixValue)) / double(255);

				double fYprim = 0.2126 * fRed + 0.7152 * fGreen + 0.0722 * fBlue;

				if (fYprim > fYmax)
					fYmax = fYprim;

				if (fYprim < fYmin)
					fYmin = fYprim;
			}
		}

		if (fYmin <= fYmax)
		{
			// Detected OK.

			m_fMinLuminosity = fYmin;
			m_fMaxLuminosity = fYmax;

			convertToStdMp4(); // m_strInputFilePath, m_nHeight are set
		}
		else
		{
			m_pConsoleTextEdit->append(tr("<font color='red'><b>Error: could not detect fYmin <= fYmax.</b></font>"));

			if (!m_pConsoleTextEdit->isVisible() && !m_bDuringClose)
			{
				QMetaObject::invokeMethod(m_pShowMorePushButton, "click");
			}
		}
	}
	else
	{
		m_pConsoleTextEdit->append(tr("<font color='red'><b>Error: abnormal program termination during analyze: %1, exit code %2, exit status %3.</b></font>").arg("ffmpeg.exe").arg(exitCode).arg(exitStatus));

		if (!m_pConsoleTextEdit->isVisible() && !m_bDuringClose)
		{
			QMetaObject::invokeMethod(m_pShowMorePushButton, "click");
		}
	}
}

void MainWindow::onExeAnalyseFinishedMp3(int exitCode, QProcess::ExitStatus exitStatus)
{
	if ((exitCode == 0) && (exitStatus == QProcess::NormalExit))
	{
		m_pConsoleTextEdit->append(tr("Analyzing finished."));

		convertToStdMp3();
	}
	else
	{
		m_pConsoleTextEdit->append(tr("<font color='red'><b>Error: abnormal program termination during analyze: %1, exit code %2, exit status %3.</b></font>").arg("ffmpeg.exe").arg(exitCode).arg(exitStatus));

		if (!m_pConsoleTextEdit->isVisible() && !m_bDuringClose)
		{
			QMetaObject::invokeMethod(m_pShowMorePushButton, "click");
		}
	}
}

void MainWindow::convertToStdMp4()
{
	m_pConsoleTextEdit->append(tr("Converting file %1 to format mp4/h264/yuv420/aac with auto adjusting video brightness and stereo volume...").arg(QFileInfo(m_strInputFilePath).fileName()));

	QString strExeFile = fullPathNameToExecutable("ffmpeg.exe", isWindows64bitCompliant());

	QFileInfo fiInputFile(m_strInputFilePath);
	QString strWorkingDir = fiInputFile.absolutePath();

	if ((m_nHeight <= 0) && (m_nHeightFromVideo <= 0))
	{
		m_pConsoleTextEdit->append(tr("Warning: no height is set neither from command line nor from input file. Video bitrate will be %1.").arg(bvForHeight(0)));
	}

	m_lstConstActualConvertArgs << "-i" << "INPUT"
		<< "-vf" << "curves=master='%1/0.00 %2/0.01 %3/0.05 %4/0.10 %5/0.50 %6/0.90 %7/0.95 %8/0.99 %9/1.00', format=pix_fmts=yuv420p, scale=trunc(oh*a/2)*2:%10:interl=-1:sws_flags=fast_bilinear"
		<< "-af" << "aresample=out_channel_layout=FL+FR,compand=attacks=0.3 0.3:decays=0.8 0.8:points=-90/-900 -70/-50 %1/-12 %2/0 0/0 100/0:soft-knee=0.01:gain=0:volume=-90:delay=0.8"
		<< "-c:v" << "VIDEOCODEC" << "-b:v" << bvForHeight((m_nHeight > 0) ? m_nHeight : m_nHeightFromVideo) << "-preset" << "PRESET" << "-profile:v" << profileForHeight((m_nHeight > 0) ? m_nHeight : m_nHeightFromVideo) << "-level" << levelForHeight((m_nHeight > 0) ? m_nHeight : m_nHeightFromVideo)
		<< "-c:a" << "aac" << "-ac" << "2" << "-b:a" << baForHeight((m_nHeight > 0) ? m_nHeight : m_nHeightFromVideo)
		<< "-c:s" << "mov_text"
		<< "-y" << "OUTPUT";

	double fLuminosityFor000 = m_fMinLuminosity; // 0.00 -> 0.00
	double fLuminosityFor001 = m_fMinLuminosity + 0.005 * (m_fMaxLuminosity - m_fMinLuminosity); // 0.005 -> 0.01
	double fLuminosityFor005 = m_fMinLuminosity + 0.015 * (m_fMaxLuminosity - m_fMinLuminosity); // 0.015 -> 0.05
	double fLuminosityFor010 = m_fMinLuminosity + 0.06 * (m_fMaxLuminosity - m_fMinLuminosity); // 0.06 -> 0.10
	double fLuminosityFor050 = m_fMinLuminosity + 0.45 * (m_fMaxLuminosity - m_fMinLuminosity); // 0.45 -> 0.50
	double fLuminosityFor090 = m_fMinLuminosity + 0.90 * (m_fMaxLuminosity - m_fMinLuminosity); // 0.90 -> 0.90
	double fLuminosityFor095 = m_fMinLuminosity + 0.95 * (m_fMaxLuminosity - m_fMinLuminosity); // 0.95 -> 0.95
	double fLuminosityFor099 = m_fMinLuminosity + 0.99 * (m_fMaxLuminosity - m_fMinLuminosity); // 0.99 -> 0.99
	double fLuminosityFor100 = m_fMaxLuminosity; // 1.00 -> 1.00

	m_lstActualArgs = m_lstConstActualConvertArgs;
	m_lstActualArgs[1] = m_strInputFilePath;

	QFileInfo fiInputFilePath(m_strInputFilePath);
	QString strFileName = fiInputFilePath.fileName();
	QStringList lstFileNameParts = strFileName.split(QChar('.'), QString::SkipEmptyParts);
	QString strBaseName = lstFileNameParts.join(QChar('-'));
	QString strOutputFilePath = "";

	if (m_nHeight > 0)
	{
		m_lstActualArgs[3] = m_lstActualArgs[3].arg(fLuminosityFor000).arg(fLuminosityFor001).arg(fLuminosityFor005).arg(fLuminosityFor010).arg(fLuminosityFor050).arg(fLuminosityFor090).arg(fLuminosityFor095).arg(fLuminosityFor099).arg(fLuminosityFor100).arg(m_nHeight);
		strOutputFilePath = fiInputFilePath.absolutePath() + QChar('/') + strBaseName + QChar('-') + QString("%1p-brightness-loudness.mp4").arg(m_nHeight);
	}
	else
	{
		if (!m_bNoCurves)
		{
			m_lstActualArgs[3] = QString("curves=master='%1/0.00 %2/0.01 %3/0.05 %4/0.10 %5/0.50 %6/0.90 %7/0.95 %8/0.99 %9/1.00', format=pix_fmts=yuv420p").arg(fLuminosityFor000).arg(fLuminosityFor001).arg(fLuminosityFor005).arg(fLuminosityFor010).arg(fLuminosityFor050).arg(fLuminosityFor090).arg(fLuminosityFor095).arg(fLuminosityFor099).arg(fLuminosityFor100);
			strOutputFilePath = fiInputFilePath.absolutePath() + QChar('/') + strBaseName + QChar('-') + QString("noscale-brightness-loudness.mp4");
		}
		else
		{
			m_lstActualArgs[3] = QString("format=pix_fmts=yuv420p");
			strOutputFilePath = fiInputFilePath.absolutePath() + QChar('/') + strBaseName + QChar('-') + QString("noscale-loudness.mp4");
		}
	}

	m_lstActualArgs[5] = m_lstActualArgs[5].arg(m_fMeanVolume).arg(m_fMaxVolume);
	m_lstActualArgs[25] = strOutputFilePath;

	m_lstActualShowArgs = m_lstActualArgs;
	m_lstActualShowArgs[1] = QString("<b><font color='blue'>%1</font></b>").arg(QFileInfo(m_lstActualArgs[1]).fileName());
	m_lstActualShowArgs[25] = QString("<b><font color='green'>%1</font></b>").arg(QFileInfo(m_lstActualArgs[25]).fileName());

	m_lstActualShowArgs[7] = m_lstActualArgs[7] = "h264_qsv";
	m_lstActualShowArgs[11] = m_lstActualArgs[11] = "veryfast";

	if (QFileInfo(strExeFile).exists() && QFileInfo(strExeFile).isExecutable())
	{
		startExecutableToConvert(strExeFile, strWorkingDir);
	}
	else
	{
		m_pConsoleTextEdit->append(tr("<font color='red'><b>Error: no executable file path found: %1.</b></font>").arg(strExeFile));

		if (!m_pConsoleTextEdit->isVisible() && !m_bDuringClose)
		{
			QMetaObject::invokeMethod(m_pShowMorePushButton, "click");
		}
	}
}

void MainWindow::convertToStdMp3()
{
	m_pConsoleTextEdit->append(tr("Converting file %1 to format mp4/h264/yuv420/aac with auto adjusting video brightness and stereo volume...").arg(QFileInfo(m_strInputFilePath).fileName()));

	QString strExeFile = fullPathNameToExecutable("ffmpeg.exe", isWindows64bitCompliant());

	QFileInfo fiInputFile(m_strInputFilePath);
	QString strWorkingDir = fiInputFile.absolutePath();

	m_lstConstActualConvertArgs << "-i" << "INPUT"
		<< "-af" << "aresample=out_channel_layout=FL+FR,compand=attacks=0.3 0.3:decays=0.8 0.8:points=-90/-900 -70/-50 %1/-12 %2/0 0/0 100/0:soft-knee=0.01:gain=0:volume=-90:delay=0.8"
		<< "-c:a" << "libmp3lame" << "-ac" << "2" << "-b:a" << "320k"
		<< "-y" << "OUTPUT";

	m_lstActualArgs = m_lstConstActualConvertArgs;
	m_lstActualArgs[1] = m_strInputFilePath;

	QFileInfo fiInputFilePath(m_strInputFilePath);
	QString strFileName = fiInputFilePath.fileName();
	QStringList lstFileNameParts = strFileName.split(QChar('.'), QString::SkipEmptyParts);
	QString strBaseName = lstFileNameParts.join(QChar('-'));
	QString strOutputFilePath = fiInputFilePath.absolutePath() + QChar('/') + strBaseName + QChar('-') + QString("loudness.mp3");

	m_lstActualArgs[3] = m_lstActualArgs[3].arg(m_fMeanVolume).arg(m_fMaxVolume);
	m_lstActualArgs[11] = strOutputFilePath;

	m_lstActualShowArgs = m_lstActualArgs;
	m_lstActualShowArgs[1] = QString("<b><font color='blue'>%1</font></b>").arg(QFileInfo(m_lstActualArgs[1]).fileName());
	m_lstActualShowArgs[11] = QString("<b><font color='green'>%1</font></b>").arg(QFileInfo(m_lstActualArgs[11]).fileName());

	if (QFileInfo(strExeFile).exists() && QFileInfo(strExeFile).isExecutable())
	{
		startExecutableToConvert(strExeFile, strWorkingDir);
	}
	else
	{
		m_pConsoleTextEdit->append(tr("<font color='red'><b>Error: no executable file path found: %1.</b></font>").arg(strExeFile));

		if (!m_pConsoleTextEdit->isVisible() && !m_bDuringClose)
		{
			QMetaObject::invokeMethod(m_pShowMorePushButton, "click");
		}
	}
}

int MainWindow::crfForHeight(int nHeight)
{
	if (nHeight <= 0)
		return 19; // no-scaling requested => highest quality
	if (nHeight < 500)
		return 25;
	if (nHeight < 1000)
		return 23;
	return 21;
}

QString MainWindow::bvForHeight(int nHeight)
{
	if (nHeight <= 0)
		return "15000k"; // no-scaling requested => highest quality

	QString strResult = QString("%1k").arg(int(1500.0 * pow(double(nHeight) / double(480), 2.3)));
	return strResult;
	/*
	if (nHeight < 500)
		return "1500k";
	if (nHeight < 1000)
		return "6000k";
	return "8000k";
	*/
}

QString MainWindow::profileForHeight(int nHeight)
{
	if (nHeight <= 720)
		return "baseline";
	return "main";
}

QString MainWindow::levelForHeight(int nHeight)
{
	if (nHeight <= 720)
		return "3.1";
	return "4.0";
}

QString MainWindow::baForHeight(int nHeight)
{
	if (nHeight <= 0)
		return "256k"; // no-scaling requested => highest quality
	if (nHeight < 500)
		return "96k";
	if (nHeight < 1000)
		return "128k";
	return "192k";
}

void MainWindow::onExeConvertStarted()
{

}

void MainWindow::onExeConvertError(QProcess::ProcessError e )
{
	if (!m_bExtractMp3)
		m_pConsoleTextEdit->append(tr("<font color='orange'><b>Warning: convert error %1. Codec name: %2.</b></font>").arg(e).arg(m_lstActualShowArgs[7]));
	else
		m_pConsoleTextEdit->append(tr("<font color='orange'><b>Warning: mp3 extraction error %1.</b></font>").arg(e));

	if (!m_pConsoleTextEdit->isVisible() && !m_bDuringClose)
	{
		QMetaObject::invokeMethod(m_pShowMorePushButton, "click");
	}
}

void MainWindow::onExeConvertFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
	if ((exitCode == 0) && (exitStatus == QProcess::NormalExit))
	{
		qobject_cast<QProgressBar*>(m_pLayoutWidget->layout()->itemAt(0)->widget())->setValue(100);
		if (s_pTaskBarlist)
			s_pTaskBarlist->SetProgressValue( (HWND)effectiveWinId(), 100, 100 );

		for (int i = 0; i < 10; ++i)
		{
			::Sleep(100);
			QCoreApplication::processEvents();
		}

		close();
	}
	else
	{
		if (!m_bExtractMp3)
		{
			if (m_lstActualArgs[7] == "libx264")
			{
				m_pConsoleTextEdit->append(tr("<font color='red'><b>Error: abnormal program termination: %1, exit code %2, exit status %3.</b></font>").arg("ffmpeg.exe").arg(exitCode).arg(exitStatus));

				if (!m_pConsoleTextEdit->isVisible() && !m_bDuringClose)
				{
					QMetaObject::invokeMethod(m_pShowMorePushButton, "click");
				}
			}
			else
			{
				m_pConsoleTextEdit->append(tr("Accelerated codec <font color='orange'><b>%1</b></font> is not supported by drivers/hardware, trying next one...").arg(m_lstActualShowArgs[7]));

				QString strExeFile = fullPathNameToExecutable("ffmpeg.exe", isWindows64bitCompliant());

				QFileInfo fiInputFile(m_strInputFilePath);
				QString strWorkingDir = fiInputFile.absolutePath();

				if (m_lstActualArgs[7] == "h264_qsv")
				{
					m_lstActualShowArgs[7] = m_lstActualArgs[7] = "h264_nvenc";
					m_lstActualShowArgs[11] = m_lstActualArgs[11] = "default"; // NVENC has its own presets
				}
				else
				{
					if (m_lstActualArgs[7] == "h264_nvenc")
					{
						m_lstActualShowArgs[7] = m_lstActualArgs[7] = "libx264";
						m_lstActualShowArgs[11] = m_lstActualArgs[11] = "veryfast";
					}
				}

				if (QFileInfo(strExeFile).exists() && QFileInfo(strExeFile).isExecutable())
				{
					startExecutableToConvert(strExeFile, strWorkingDir);
				}
				else
				{
					m_pConsoleTextEdit->append(tr("<font color='red'><b>Error: no executable file path found: %1.</b></font>").arg(strExeFile));

					if (!m_pConsoleTextEdit->isVisible() && !m_bDuringClose)
					{
						QMetaObject::invokeMethod(m_pShowMorePushButton, "click");
					}
				}
			}
		}
		else
		{
			m_pConsoleTextEdit->append(tr("<font color='red'><b>Error: no sound track can be extracted.</b></font>"));

			if (!m_pConsoleTextEdit->isVisible() && !m_bDuringClose)
			{
				QMetaObject::invokeMethod(m_pShowMorePushButton, "click");
			}
		}
	}
}

void MainWindow::onExeConvertReadyReadStandardOutput()
{
	qDebug() << Q_FUNC_INFO;

	if (!m_pExtProgProcess)
		return;

	QByteArray ba = m_pExtProgProcess->readAllStandardOutput();
	QString strOut = QString::fromLatin1(ba.data(), ba.size()).trimmed();

	m_pConsoleTextEdit->append(strOut);

	QStringList lstOutParts = strOut.split(QChar('\r'), QString::SkipEmptyParts);

	QString strPart;
	foreach (strPart, lstOutParts)
	{
		analyzeProcessOutputPhase2(strPart);
	}
}

void MainWindow::onExePlayStarted()
{

}

void MainWindow::onExePlayError(QProcess::ProcessError e)
{
	m_pConsoleTextEdit->append(tr("<font color='red'><b>Playing error %1.</b></font>").arg(e));

	if (!m_pConsoleTextEdit->isVisible() && !m_bDuringClose)
	{
		QMetaObject::invokeMethod(m_pShowMorePushButton, "click");
	}
}

void MainWindow::onExePlayFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
	if ((exitCode == 0) && (exitStatus == QProcess::NormalExit))
	{
		for (int i = 0; i < 10; ++i)
		{
			::Sleep(10);
			QCoreApplication::processEvents();
		}

		close();
	}
	else
	{
		m_pConsoleTextEdit->append(tr("<font color='red'><b>Error: abnormal program termination: %1, exit code %2, exit status %3.</b></font>").arg("ffplay.exe").arg(exitCode).arg(exitStatus));

		if (!m_pConsoleTextEdit->isVisible() && !m_bDuringClose)
		{
			QMetaObject::invokeMethod(m_pShowMorePushButton, "click");
		}
	}
}

void MainWindow::onExePlayReadyReadStandardOutput()
{
	qDebug() << Q_FUNC_INFO;

	if (!m_pExtProgProcess)
		return;

	QByteArray ba = m_pExtProgProcess->readAllStandardOutput();
	QString strOut = QString::fromLatin1(ba.data(), ba.size()).trimmed();

	m_pConsoleTextEdit->append(strOut);
}

void MainWindow::analyzeProcessOutputPhase2(const QString& strOut)
{
	QString strTimeValue = findBetweenStrings(strOut, "time=", "bitrate=");

	if (strTimeValue != "")
	{
		qint64 nCurrentFrames = durationWordToFrames(strTimeValue, m_strFpsValue);
		int nPercent = 10 + int(ceil(90.0 * (double(nCurrentFrames) / double(m_nTotalFrames))));
		qobject_cast<QProgressBar*>(m_pLayoutWidget->layout()->itemAt(0)->widget())->setValue(nPercent);
		if (s_pTaskBarlist)
			s_pTaskBarlist->SetProgressValue( (HWND)effectiveWinId(), nPercent, 100 );
	}
}

QString MainWindow::findBetweenStrings(const QString& strOut, const QString& strBeforeMark, const QString& strAfterMark)
{
	int nBeforeMarkPos = strOut.indexOf(strBeforeMark);

	if (nBeforeMarkPos != -1)
	{
		int nBeforeMarkPos2 = nBeforeMarkPos + strBeforeMark.size();
		int nAfterMarkPos = strOut.indexOf(strAfterMark, nBeforeMarkPos2);

		if (nAfterMarkPos != -1)
		{
			QString strValue = strOut.mid(nBeforeMarkPos2, nAfterMarkPos - nBeforeMarkPos2);
			return strValue.trimmed();
		}

	}

	return "";
}

QString MainWindow::findBetweenStringsReverse(const QString& strOut, const QString& strBeforeMark, const QString& strAfterMark)
{
	int nBeforeMarkPos = strOut.lastIndexOf(strBeforeMark);

	if (nBeforeMarkPos != -1)
	{
		int nAfterMarkPos = strOut.lastIndexOf(strAfterMark, nBeforeMarkPos);

		if (nAfterMarkPos != -1)
		{
			int nAfterMarkPos2 = nAfterMarkPos + strAfterMark.size();
			QString strValue = strOut.mid(nAfterMarkPos2, -(nAfterMarkPos2 - nBeforeMarkPos));
			return strValue.trimmed();
		}

	}

	return "";
}

void MainWindow::onExeAnalyseReadyReadStandardOutput()
{
	qDebug() << Q_FUNC_INFO;

	if (!m_pExtProgProcess)
		return;

	QByteArray ba = m_pExtProgProcess->readAllStandardOutput();
	QString strOut = QString::fromLatin1(ba.data(), ba.size()).trimmed();

	m_pConsoleTextEdit->append(strOut);

	QStringList lstOutParts = strOut.split(QChar('\r'), QString::SkipEmptyParts);

	QString strPart;
	foreach (strPart, lstOutParts)
	{
		analyzeProcessOutputPhase1(strPart);
	}
}

qint64 MainWindow::durationWordToFrames(const QString& strDurationValue, const QString& strFpsValue)
{
	QStringList lstDurationParts = strDurationValue.split(QChar(':'), QString::SkipEmptyParts);

	if (lstDurationParts.size() >= 3)
	{
		qint64 nTotalFrames = 0;
		nTotalFrames+= int(ceil(lstDurationParts[2].toDouble() * strFpsValue.toDouble()));
		nTotalFrames+= int(ceil(lstDurationParts[1].toInt() * 60 * strFpsValue.toDouble()));
		nTotalFrames+= int(ceil(lstDurationParts[0].toInt() * 60 * 60 * strFpsValue.toDouble()));

		return nTotalFrames;
	}
	
	return 0;
}

void MainWindow::analyzeProcessOutputPhase1(const QString& strOut)
{
	// Duration: 00:00:22.96, start: 0.000000, bitrate: 6642 kb/s
	// Stream #0:0(eng): Video: h264 (High), yuv420p, 1920x1080, SAR 1:1 DAR 16:9, 24 fps, 24 tbr, 1k tbn, 48 tbc (default)

	if (m_strDurationValue == "")
	{
		QString strDurationValue = findBetweenStrings(strOut, "Duration:", ",");

		if (strDurationValue != "")
			m_strDurationValue = strDurationValue;
	}

	if (m_strFpsValue == "")
	{
		int nStreamWordPos = strOut.indexOf("Stream");

		if (nStreamWordPos != -1)
		{
			QString strVideoStreamValue = strOut.mid(nStreamWordPos);

			QString strFpsValue = findBetweenStringsReverse(strVideoStreamValue, "fps,", ",");

			if (strFpsValue != "")
				m_strFpsValue = strFpsValue;
		}
	}

	if ((m_nTotalFrames == 0) && (m_strDurationValue != "") && (m_strFpsValue != ""))
	{
		m_nTotalFrames = durationWordToFrames(m_strDurationValue, m_strFpsValue);
	}

	if ((m_fMeanVolume >= 1E10) && (m_fMaxVolume >= 1E10))
	{
		QString strTimeValue = findBetweenStrings(strOut, "time=", "bitrate=");

		if (strTimeValue != "")
		{
			m_nCurrentFrames = durationWordToFrames(strTimeValue, m_strFpsValue);

			if (m_nTotalFrames > 0)
			{
				int nPercent = 0 + int(ceil(10.0 * (double(m_nCurrentFrames) / double(m_nTotalFrames))));
				qobject_cast<QProgressBar*>(m_pLayoutWidget->layout()->itemAt(0)->widget())->setValue(nPercent);
				if (s_pTaskBarlist)
					s_pTaskBarlist->SetProgressValue( (HWND)effectiveWinId(), nPercent, 100 );
			}
		}
	}
	else
	{
		if (!(m_nTotalFrames > 0))
		{
			m_nTotalFrames = m_nCurrentFrames;
		}
	}

	if (m_fMeanVolume >= 1E10)
	{
		QString strMeanVolumeValue = findBetweenStrings(strOut, "mean_volume:", "dB");

		if (strMeanVolumeValue != "")
			m_fMeanVolume = strMeanVolumeValue.toDouble();
	}

	if (m_fMaxVolume >= 1E10)
	{
		QString strMaxVolumeValue = findBetweenStrings(strOut, "max_volume:", "dB");

		if (strMaxVolumeValue != "")
			m_fMaxVolume = strMaxVolumeValue.toDouble();
	}

	int nStreamPos = -1;

	if (!m_bStreamFound)
	{
		nStreamPos = strOut.indexOf("Stream #");

		if (nStreamPos != -1)
		{
			m_bStreamFound = true;
		}
	}

	if ((m_bStreamFound) && (m_nHeightFromVideo == 0))
	{
		int nVideoPos = strOut.indexOf("Video:", (nStreamPos != -1) ? nStreamPos : 0);

		if (nVideoPos != -1)
		{
			int nXPos = strOut.indexOf("x", nVideoPos);

			if (nXPos != -1)
			{
				int nSpacePos = strOut.indexOf(" ", nXPos);

				if (nSpacePos != -1)
				{
					int nCommaPos = strOut.indexOf(",", nXPos);

					if ((nCommaPos != -1) && (nCommaPos < nSpacePos)) // More priority
						nSpacePos = nCommaPos;

					QString strHeight = strOut.mid(nXPos + 1, nSpacePos - nXPos - 1);

					if (strHeight != "")
					{
						bool bOk = false;
						m_nHeightFromVideo = strHeight.toInt(&bOk);
					}
				}
			}
		}
	}
}

MainWindow::~MainWindow()
{
	qDebug() << Q_FUNC_INFO;
	
	killExtProcessIfRunning();

	bool bRmTmp = QFile::remove(m_strTempPaletteFilePath);

	qDebug() << Q_FUNC_INFO << "bRmTmp" << bRmTmp;

	if (s_pTaskBarlist)
		s_pTaskBarlist->Release();
}

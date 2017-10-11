#pragma once

#include <QMainWindow>
#include <QAction>
#include <QKeyEvent>
#include <QCloseEvent>
#include <QSettings>
#include <QScopedPointer>
#include <QProcess>
#include <QThread>
#include <QTextEdit>
#include <QPushButton>


// Forward declarations

class GameField;


class MainWindow : public QMainWindow
{
	Q_OBJECT

public:

	MainWindow(QWidget *parent = 0, Qt::WindowFlags flags = Qt::WindowFlags());
	virtual ~MainWindow();

protected slots:

	void onTriggeredHelp();
	void onTriggeredAbout();
	void onToggleShowHideDetails();
	void onExeAnalyseStarted();
	void onExeAnalyseError(QProcess::ProcessError);
	void onExeAnalyseFinishedMp4(int exitCode, QProcess::ExitStatus exitStatus);
	void onExeAnalyseFinishedMp3(int exitCode, QProcess::ExitStatus exitStatus);
	void onExeAnalyseReadyReadStandardOutput();
	void onExeConvertStarted();
	void onExeConvertError(QProcess::ProcessError);
	void onExeConvertFinished(int exitCode, QProcess::ExitStatus exitStatus);
	void onExeConvertReadyReadStandardOutput();
	void onExePlayStarted();
	void onExePlayError(QProcess::ProcessError);
	void onExePlayFinished(int exitCode, QProcess::ExitStatus exitStatus);
	void onExePlayReadyReadStandardOutput();

protected:

	void keyPressEvent(QKeyEvent* pEvent); // We re-implement from Qt
	void closeEvent(QCloseEvent* pEvent); // We re-implement from Qt

	enum VisualLayout
	{
		VisualLayout_Convert
		, VisualLayout_Play
		, VisualLayout_Console
	};

	void createVisualLayout(VisualLayout vl);

	QString fullPathNameToExecutable(const QString& strNameExt, bool b64 = false);
	QString fullDirNameToExecutable(const QString& strNameExt, bool b64 = false);
	bool isConvertToStdMp4Requested(const QStringList& lstCommandArgs);
	bool isExtractNormalizedMp3Requested(const QStringList& lstCommandArgs);
	bool isPlayRequested(const QStringList& lstCommandArgs);
	bool isConsoleRequested(const QStringList& lstCommandArgs);
	void analyseBeforeConvertToStdMp4(const QString& strInputFilePath);
	void analyseBeforeExtractNormalizedMp3(const QString& strInputFilePath);
	void convertToStdMp4();
	void convertToStdMp3();
	void convertToMpegDashMp4();
	void play(const QString& strInputFilePath);
	void console(const QString& str64bit, const QString& strInputFilePath);

	void startExecutableToAnalyseMp4(const QString& strExeFile, const QStringList& lstExeArgs, const QStringList& lstExeShowArgs, const QString& strWorkingDir);
	void startExecutableToAnalyseMp3(const QString& strExeFile, const QStringList& lstExeArgs, const QStringList& lstExeShowArgs, const QString& strWorkingDir);
	void startExecutableToConvert(const QString& strExeFile, const QString& strWorkingDir);
	void startExecutableToPlay(const QString& strExeFile, const QStringList& lstExeArgs, const QStringList& lstExeShowArgs, const QString& strWorkingDir);
	QString findBetweenStrings(const QString& strOut, const QString& strBeforeMark, const QString& strAfterMark);
	QString findBetweenStringsReverse(const QString& strOut, const QString& strBeforeMark, const QString& strAfterMark);
	qint64 durationWordToFrames(const QString& strDurationValue, const QString& strFpsValue);
	void analyzeProcessOutputPhase1(const QString& strOut);
	void analyzeProcessOutputPhase2(const QString& strOut);
	void killExtProcessIfRunning();


private:

	bool isFFMPEGInputFile(const QString& strInputFilePath);
	int crfForHeight(int nHeight);
	QString bvForHeight(int nHeight);
	QString baForHeight(int nHeight);
	QString profileForHeight(int nHeight);
	QString levelForHeight(int nHeight);

	QWidget* m_pLayoutWidget;
	QTextEdit* m_pConsoleTextEdit;
	QPushButton* m_pShowMorePushButton;

	QScopedPointer<QProcess> m_pExtProgProcess;

	QString m_strInputFilePath;
	QString m_strTempPaletteFilePath;

	QStringList m_lstConstDetectLuminosityVolumeArgs;
	QStringList m_lstConstDetectOnlyVolumeArgs;
	QStringList m_lstConstActualConvertArgs;
	QStringList m_lstConstPlayArgs;

	QStringList m_lstActualArgs;
	QStringList m_lstActualShowArgs;

	QString m_strCodecName;
	QString m_strDurationValue;
	QString m_strFpsValue;

	qint64 m_nTotalFrames;

	double m_fMeanVolume;
	double m_fMaxVolume;

	double m_fMinLuminosity;
	double m_fMaxLuminosity;

	int m_nHeight;
	bool m_bNoCurves;

	bool m_bStreamFound;
	int m_nHeightFromVideo;

	bool m_bDuringClose;
	bool m_bExtractMp3;

	qint64 m_nCurrentFrames;
};

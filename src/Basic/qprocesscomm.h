#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include <QProcess>
#include "odprocess.h"

QT_BEGIN_NAMESPACE

/*!
\brief QProcess communication class

  Internal object, to hide Qt's signal/slot mechanism.
*/

class QProcessComm : public QObject
{
    Q_OBJECT
    friend class	OD::Process;

protected:

QProcessComm( QProcess* qprocess, OD::Process* process )
    : qprocess_(qprocess)
    , process_(process)
{
    connect( qprocess_, &QProcess::started, this,
	     &QProcessComm::started );
    connect( qprocess_, &QProcess::finished, this,
	     &QProcessComm::finished );
    connect( qprocess_, &QProcess::errorOccurred, this,
	     &QProcessComm::errorOccurred );
    connect( qprocess_, &QProcess::stateChanged, this,
	     &QProcessComm::stateChanged );
    connect( qprocess_, &QProcess::readyReadStandardOutput, this,
	     &QProcessComm::readyReadStandardOutput );
    connect( qprocess_, &QProcess::readyReadStandardError, this,
	     &QProcessComm::readyReadStandardError );
}

public:

~QProcessComm()
{
    deactivate();
}


void deactivate()
{
    qprocess_ = nullptr;
    process_ = nullptr;
}

private slots:

void started()
{
    process_->started.trigger();
}


void finished( int exitcode, QProcess::ExitStatus qexitstatus )
{
    const OD::Process::ExitStatus exitstatus =
		qexitstatus == QProcess::NormalExit
		? OD::Process::ExitStatus::NormalExit
		: OD::Process::ExitStatus::CrashExit;

    const std::pair<int, OD::Process::ExitStatus> caps( exitcode, exitstatus );
    process_->finished.trigger( caps );
}


void errorOccurred( QProcess::ProcessError qerror )
{
    OD::Process::Error error = OD::Process::Error::UnknownError;
    switch( qerror )
    {
	case QProcess::FailedToStart:
	    error = OD::Process::Error::FailedToStart;
	    break;
	case QProcess::Crashed:
	    error = OD::Process::Error::Crashed;
	    break;
	case QProcess::Timedout:
	    error = OD::Process::Error::Timedout;
	    break;
	case QProcess::ReadError:
	    error = OD::Process::Error::ReadError;
	    break;
	case QProcess::WriteError:
	    error = OD::Process::Error::WriteError;
	    break;
	case QProcess::UnknownError:
	    error = OD::Process::Error::UnknownError;
	    break;
	default:
	    break;
    }

    process_->errorOccurred.trigger( error );
}


void stateChanged( QProcess::ProcessState qstate )
{
    OD::Process::State state = OD::Process::State::NotRunning;
    switch( qstate )
    {
	case QProcess::NotRunning:
	    state = OD::Process::State::NotRunning;
	    break;
	case QProcess::Starting:
	    state = OD::Process::State::Starting;
	    break;
	case QProcess::Running:
	    state = OD::Process::State::Running;
	    break;
	default:
	    break;
    }

    process_->stateChanged.trigger( state );
}


void readyReadStandardOutput()
{
    process_->readyReadStandardOutput.trigger();
}


void readyReadStandardError()
{
    process_->readyReadStandardError.trigger();
}

private:

    QProcess*		qprocess_;
    OD::Process*	process_;

};

QT_END_NAMESPACE

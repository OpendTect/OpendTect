/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "odprocess.h"

#include "qprocesscomm.h"
#include "od_ostream.h"

#ifndef OD_NO_QT
# include <QFile>
# include <QProcess>
# include <QTextStream>
#endif

mUseQtnamespace

mDefineEnumUtils(OD::Process,Error,"Process Error")
{
    "Failed to start",
    "Crashed",
    "Timedout",
    "Read error",
    "Write error",
    "Unknown error",
    nullptr
};

mDefineEnumUtils(OD::Process,State,"Process State")
{
    "Not running",
    "Starting",
    "Running",
    nullptr
};

mDefineEnumUtils(OD::Process,Channel,"Process Channel")
{
    "Standard Output",
    "Standard Error",
    nullptr
};

mDefineEnumUtils(OD::Process,ChannelMode,"Process Channel Mode")
{
    "Seoarate Channels",
    "Merged Channels",
    "Forwarded Channels",
    "Forwarded Output Channel",
    "Forwarded Error Channel",
    nullptr
};

mDefineEnumUtils(OD::Process,InputChannelMode,"Input Channel Mode")
{
    "Managed Input Channel",
    "Forwarded Input Channel",
    nullptr
};

mDefineEnumUtils(OD::Process,ExitStatus,"Exit Status")
{
    "Normal exit",
    "Crash exit",
    nullptr
};


OD::Process::Process( const char* nm )
    : NamedCallBacker(nm)
    , started(this)
    , finished(this)
    , errorOccurred(this)
    , stateChanged(this)
    , readyReadStandardOutput(this)
    , readyReadStandardError(this)
    , process_(new QProcess())
    , comm_(new QProcessComm(process_,this))
{}


OD::Process::~Process()
{
    detachAllNotifiers();
    comm_->deactivate();
    delete stdoutstrm_;
    delete stderrstrm_;
    delete stdtxtoutstrm_;
    delete stdtxterrstrm_;
    delete stdoutstr_;
    delete stderrstr_;
    delete stdfileoutstrm_;
    delete stdfileerrstrm_;
    delete stdfout_;
    delete stdferr_;
    delete process_;
    delete comm_;
}


OD::Process& OD::Process::setProgram( const char* prognm )
{
    const QString qprog( prognm );
    process_->setProgram( prognm );
    return *this;
}


OD::Process& OD::Process::setArguments( const BufferStringSet& args )
{
    QStringList qargs;
    args.fill( qargs );
    process_->setArguments( qargs );
    return *this;
}


OD::Process& OD::Process::setWorkingDirectory( const char* dirnm )
{
    const QString qdirnm( dirnm );
    process_->setWorkingDirectory( dirnm );
    return *this;
}


OD::Process& OD::Process::setEnvironment( const BufferStringSet& env )
{
    QStringList qenv;
    env.fill( qenv );
    process_->setEnvironment( qenv );
    return *this;
}


OD::Process& OD::Process::setProcessChannelMode( ChannelMode mode )
{
    QProcess::ProcessChannelMode qmode = QProcess::SeparateChannels;
    switch( mode )
    {
	case ChannelMode::SeparateChannels:
	    qmode = QProcess::SeparateChannels;
	    break;
	case ChannelMode::MergedChannels:
	    qmode = QProcess::MergedChannels;
	    break;
	case ChannelMode::ForwardedChannels:
	    qmode = QProcess::ForwardedChannels;
	    break;
	case ChannelMode::ForwardedOutputChannel:
	    qmode = QProcess::ForwardedOutputChannel;
	    break;
	case ChannelMode::ForwardedErrorChannel:
	    qmode = QProcess::ForwardedErrorChannel;
	    break;
    }

    process_->setProcessChannelMode( qmode );
    return *this;
}


OD::Process& OD::Process::setInputChannelMode( InputChannelMode mode )
{
    const QProcess::InputChannelMode qmode =
	mode == InputChannelMode::ManagedInputChannel
		? QProcess::ManagedInputChannel
		: QProcess::ForwardedInputChannel;
    process_->setInputChannelMode( qmode );
    return *this;
}


void OD::Process::start( const char* program, const BufferStringSet& arguments )
{
    const QString qprog( program );
    QStringList qargs;
    arguments.fill( qargs );
    process_->start( qprog, qargs, QIODevice::ReadWrite );
}


void OD::Process::start()
{
    process_->start();
}


bool OD::Process::startDetached( PID_Type* pid )
{
    qint64 qpid = 0;
    const bool res = process_->startDetached( &qpid );
    if ( pid )
	*pid = (PID_Type) qpid ;

    return res;
}


void OD::Process::closeReadChannel( Channel channel )
{
    const QProcess::ProcessChannel qchannel =
		channel == Channel::StandardOutput
		    ? QProcess::StandardOutput
		    : QProcess::StandardError;
    process_->closeReadChannel( qchannel );
}


void OD::Process::closeWriteChannel()
{
    process_->closeWriteChannel();
}



void OD::Process::setStandardInputFile( const char* fnm )
{
    const QString filenm( fnm );
    process_->setStandardInputFile( filenm );
}


void OD::Process::setStandardOutputFile( const char* fnm )
{
    const QString filenm( fnm == sNullStreamFnm()
			    ? QProcess::nullDevice()
			    : QString(fnm) );
    process_->setStandardOutputFile( filenm );
}


void OD::Process::setStandardOutputStream( bool console, bool txtbuf,
					   const char* fnm )
{
    deleteAndNullPtr( stdoutstrm_ );
    if ( console )
	stdoutstrm_ = new QTextStream( stdout );

    deleteAndNullPtr( stdtxtoutstrm_ );
    deleteAndNullPtr( stdoutstr_ );
    if ( txtbuf )
    {
	stdoutstr_ = new QString();
	stdtxtoutstrm_ = new QTextStream( stdoutstr_ );
    }

    deleteAndNullPtr( stdfileoutstrm_ );
    deleteAndNullPtr( stdfout_ );
    const StringView filenm( fnm );
    if ( !filenm.isEmpty() && filenm != sNullStreamFnm() )
    {
	stdfout_ = new QFile( filenm.buf() );
	if ( stdfout_->open(QIODevice::WriteOnly | QIODevice::Text) )
	    stdfileoutstrm_ = new QTextStream( stdfout_ );
	else
	    deleteAndNullPtr( stdfout_ );
    }

    mAttachCBIfNotAttached( readyReadStandardOutput,
			    Process::readyReadStandardOutputCB );
}


void OD::Process::setStandardOutputProcess( const Process& proc2 )
{
    process_->setStandardOutputProcess( proc2.process_ );
}


void OD::Process::setStandardErrorFile( const char* fnm )
{
    const QString filenm( fnm == sNullStreamFnm()
			    ? QProcess::nullDevice()
			    : QString(fnm) );
    process_->setStandardErrorFile( filenm );
}


void OD::Process::setStandardErrorStream( bool console, bool txtbuf,
					  const char* fnm )
{
    deleteAndNullPtr( stderrstrm_ );
    if ( console )
	stderrstrm_ = new QTextStream( stderr );

    deleteAndNullPtr( stdtxterrstrm_ );
    deleteAndNullPtr( stderrstr_ );
    if ( txtbuf )
    {
	stderrstr_ = new QString();
	stdtxterrstrm_ = new QTextStream( stderrstr_ );
    }

    deleteAndNullPtr( stdfileerrstrm_ );
    deleteAndNullPtr( stdferr_ );
    const StringView filenm( fnm );
    if ( !filenm.isEmpty() && filenm != sNullStreamFnm() )
    {
	stdferr_ = new QFile( filenm.buf() );
	if ( stdferr_->open(QIODevice::WriteOnly | QIODevice::Text) )
	    stdfileerrstrm_ = new QTextStream( stdferr_ );
	else
	    deleteAndNullPtr( stdferr_ );
    }

    mAttachCBIfNotAttached( readyReadStandardError,
			    Process::readyReadStandardErrorCB );
}


BufferString OD::Process::program() const
{
    return process_->program();
}


BufferStringSet OD::Process::arguments() const
{
    const QStringList qargs = process_->arguments();
    BufferStringSet ret;
    ret.use( qargs );
    return ret;
}

BufferString OD::Process::workingDirectory() const
{
    const QString qworkdir = process_->workingDirectory();
    return BufferString( qworkdir );
}


BufferStringSet OD::Process::environment() const
{
    const QStringList qenv = process_->environment();
    BufferStringSet env;
    env.use( qenv );
    return env;
}


OD::Process::Error OD::Process::error() const
{
    const QProcess::ProcessError qprocerror = process_->error();
    switch( qprocerror )
    {
	case QProcess::FailedToStart: return Error::FailedToStart;
	case QProcess::Crashed: return Error::Crashed;
	case QProcess::Timedout: return Error::Timedout;
	case QProcess::ReadError: return Error::ReadError;
	case QProcess::WriteError: return Error::WriteError;
	case QProcess::UnknownError: return Error::UnknownError;
	default: return Error::UnknownError;
    }
}


OD::Process::State OD::Process::state() const
{
    const QProcess::ProcessState qprocstate = process_->state();
    switch( qprocstate )
    {
	case QProcess::NotRunning: return State::NotRunning;
	case QProcess::Starting: return State::Starting;
	case QProcess::Running: return State::Running;
	default: return State::NotRunning;
    }
}


bool OD::Process::isRunning() const
{
    return process_->state() == QProcess::Running;
}


PID_Type OD::Process::processId() const
{
#ifndef OD_NO_QT

#if QT_VERSION >= QT_VERSION_CHECK(5,3,0)
    const qint64 qpid = process_->processId();
    return mCast( PID_Type, qpid );
#else
# ifdef __win__
    const PROCESS_INFORMATION* pi = (PROCESS_INFORMATION*) process_->pid();
    return pi->dwProcessId;
# else
    return process_->pid();
# endif
#endif

#else
    return 0;
#endif
}


bool OD::Process::waitForStarted( int msecs )
{
    return process_->waitForStarted( msecs );
}


bool OD::Process::waitForReadRead( int msecs )
{
    return process_->waitForReadyRead( msecs );
}


bool OD::Process::waitForBytesWritten( int msecs )
{
    return process_->waitForBytesWritten( msecs );
}


bool OD::Process::waitForFinished( int msecs )
{
    return process_->waitForFinished( msecs );
}


bool OD::Process::isWritable() const
{
    return process_->isWritable();
}


bool OD::Process::hasStdInput() const
{
    return process_->isWritable();
}


od_int64 OD::Process::write( const char* data, od_int64 maxsz )
{
    return process_->write( data, maxsz );
}


od_int64 OD::Process::write( const char* data )
{
    return process_->write( data );
}


QTextStream* OD::Process::getStrm( bool stdoutstrm  )
{
    return stdoutstrm ? stdtxtoutstrm_ : stdtxterrstrm_;
}


bool OD::Process::getLine( BufferString& ret, bool stdoutstrm,
			   bool* newline_found )
{
    QTextStream* strm = getStrm( stdoutstrm );
    if ( !strm )
	return false;

    ret = strm->readLine();
    if ( newline_found )
	*newline_found = !strm->atEnd();

    return strm->status() == QTextStream::Ok;
}



bool OD::Process::getAll( BufferStringSet& ret, bool stdoutstrm )
{
    QTextStream* strm = getStrm( stdoutstrm );
    if ( !strm )
	return false;

    while( !strm->atEnd() )
	ret.add( strm->readLine() );

    return strm->status() == QTextStream::Ok;
}


bool OD::Process::getAll( BufferString& ret, bool stdoutstrm )
{
    BufferStringSet alllines;
    const bool res = getAll( alllines, stdoutstrm );
    ret = alllines.cat();
    return res;
}



void OD::Process::close()
{
    process_->close();
}


int OD::Process::exitCode() const
{
    return process_->exitCode();
}


OD::Process::ExitStatus OD::Process::exitStatus() const
{
    return process_->exitStatus() == QProcess::NormalExit
		? ExitStatus::NormalExit
		: ExitStatus::CrashExit;
}


void OD::Process::readyReadStandardOutputCB( CallBacker* )
{
    if ( !isRunning() )
	return;

    const QByteArray data = process_->readAllStandardOutput();
    if ( stdoutstrm_ ) // Write to console
    {
	*stdoutstrm_ << data;
	stdoutstrm_->flush();
    }

    if ( stdtxtoutstrm_ )
	*stdtxtoutstrm_ << data;

    if ( stdfileoutstrm_ ) // Write to file
    {
	*stdfileoutstrm_ << data;
	stdfileoutstrm_->flush();
    }
}


void OD::Process::readyReadStandardErrorCB( CallBacker* )
{
    if ( !isRunning() )
	return;

    const QByteArray data = process_->readAllStandardError();
    if ( stderrstrm_ ) // Write to console
    {
	*stderrstrm_ << data;
	stderrstrm_->flush();
    }

    if ( stdtxterrstrm_ )
	*stdtxterrstrm_ << data;

    if ( stdfileerrstrm_ ) // Write to file
    {
	*stdfileerrstrm_ << data;
	stdfileerrstrm_->flush();
    }
}


const char* OD::Process::sNullStreamFnm()
{
    return od_ostream::nullStream().fileName();
}


BufferStringSet OD::Process::systemEnvironment()
{
    const QStringList qenv = QProcess::systemEnvironment();
    BufferStringSet ret;
    ret.use( qenv );
    return ret;
}

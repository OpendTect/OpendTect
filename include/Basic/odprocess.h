#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "basicmod.h"

#include "enums.h"
#include "refcount.h"

mFDQtclass(QFile)
mFDQtclass(QProcess)
mFDQtclass(QProcessComm)
mFDQtclass(QTextStream)

namespace OD
{

/*!
\brief Process class.
*/


mExpClass(Basic) Process : public NamedCallBacker
			 , public ReferencedObject
{
public:

    enum class Error {
	FailedToStart=0,
	Crashed=1,
	Timedout=2,
	ReadError=3,
	WriteError=4,
	UnknownError=5
    };
    mDeclareEnumUtils(Error)

    enum class State {
	NotRunning=0,
	Starting=1,
	Running=2
    };
    mDeclareEnumUtils(State)

    enum class Channel {
	StandardOutput=0,
	StandardError=1
    };
    mDeclareEnumUtils(Channel)

    enum class ChannelMode {
	SeparateChannels=0,
	MergedChannels=1,
	ForwardedChannels=2,
	ForwardedOutputChannel=3,
	ForwardedErrorChannel=4
    };
    mDeclareEnumUtils(ChannelMode)

    enum class InputChannelMode {
	ManagedInputChannel=0,
	ForwardedInputChannel=1
    };
    mDeclareEnumUtils(InputChannelMode)

    enum class ExitStatus {
	NormalExit = 0,
	CrashExit = 1
    };
    mDeclareEnumUtils(ExitStatus)

			Process(const char* nm="Process");
			mOD_DisableCopy(Process);

    OD::Process&    setProgram(const char*);
    OD::Process&    setArguments(const BufferStringSet&);
    OD::Process&    setWorkingDirectory(const char* dir);
    OD::Process&    setEnvironment(const BufferStringSet&);
    OD::Process&    setProcessChannelMode(ChannelMode);
    OD::Process&    setInputChannelMode(InputChannelMode);

    void	    start(const char* program,const BufferStringSet& arguments);
    void	    start();
    bool	    startDetached(PID_Type* pid=nullptr);

    void    closeReadChannel(Channel);
    void    closeWriteChannel();

    void    setStandardInputFile(const char* fnm);
    void    setStandardOutputFile(const char* fnm);
    void    setStandardOutputStream(bool console,bool txtbuf,const char* fnm);
    void    setStandardOutputProcess(const Process&);
    void    setStandardErrorFile(const char* fnm);
    void    setStandardErrorStream(bool console,bool txtbuf,const char* fnm);

    BufferString program() const;
    BufferStringSet arguments() const;
    BufferString workingDirectory() const;
    BufferStringSet environment() const;

    Error	error() const;
    State	state() const;
    bool	isRunning() const;

    PID_Type	processId() const;

    bool	waitForStarted(int msec=30000);
    bool	waitForReadRead(int msecs=30000);
    bool	waitForBytesWritten(int msecs=30000);
    bool	waitForFinished(int msecs=30000);

    bool	isWritable() const;
    bool	hasStdInput() const;
    bool	hasStdOutput() const	{ return stdtxtoutstrm_; }
    bool	hasStdError() const	{ return stdtxterrstrm_; }

    od_int64	write(const char*,od_int64 maxsz);
    od_int64	write(const char*);
			    //!< String must be null terminated
    bool	getLine(BufferString&,bool stdoutstrm=true,
			bool* newline_found=nullptr);
    bool	getAll(BufferStringSet&,bool stdoutstrm=true);
    bool	getAll(BufferString&,bool stdoutstrm=true);

    void	close();
    int		exitCode() const;
    ExitStatus	exitStatus() const;

    Notifier<Process>	started;
    CNotifier<Process,std::pair<int,ExitStatus> > finished;
    CNotifier<Process,Error> errorOccurred;
    CNotifier<Process,State> stateChanged;

    Notifier<Process>	readyReadStandardOutput;
    Notifier<Process>	readyReadStandardError;

    QProcess* process()		{ return process_; }
	    //!< Only for setting platform specific flags

    static const char* sNullStreamFnm();
    static BufferStringSet systemEnvironment();
	    //!< Not cached, will be updated on setenv calls

protected:
			~Process();

    QTextStream*	getStrm(bool stdoutstrm);

    void    readyReadStandardOutputCB(CallBacker*);
    void    readyReadStandardErrorCB(CallBacker*);

    mQtclass(QProcess*)		process_;
    mQtclass(QProcessComm*)	comm_;

    QTextStream*	stdoutstrm_	= nullptr;
    QTextStream*	stderrstrm_	= nullptr;
    QString*		stdoutstr_	= nullptr;
    QString*		stderrstr_	= nullptr;
    QTextStream*	stdtxtoutstrm_	= nullptr;
    QTextStream*	stdtxterrstrm_	= nullptr;
    QFile*		stdfout_	= nullptr;
    QFile*		stdferr_	= nullptr;
    QTextStream*	stdfileoutstrm_ = nullptr;
    QTextStream*	stdfileerrstrm_ = nullptr;

};

} // namespace OD

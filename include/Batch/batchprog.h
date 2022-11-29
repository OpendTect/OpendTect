#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "batchmod.h"

#include "applicationdata.h"
#include "batchjobdispatch.h"
#include "enums.h"

#include "plugins.h"
#include "debug.h"
#include "od_ostream.h"
#include "odruncontext.h"
#include "genc.h"


#ifdef __msvc__
# ifndef _CONSOLE
#  include "winmain.h"
# endif
#endif

/*
  Batch programs should include this header, and define a BatchProgram::go().
  If program args are needed outside this method, BP() can be accessed.
*/


class BatchServiceServerMgr;
class CommandLineParser;
class JobCommunic;
class od_ostream;
class Timer;

/*!
\brief Main object for 'standard' batch programs.

  Most 'interesting' batch programs need a lot of parameters to do the work.
  Therefore, in OpendTect, BatchPrograms need a 'parameter file', with all
  the info needed in IOPar format, i.e., keyword/value pairs.

  This object takes over the details of reading that file, extracting
  'standard' components from the parameters, opening sockets etc.,

  To use the object, instead of defining a function 'main', you should define
  both functions:
	 'BatchProgram::loadModules': to load the required basic modules
	 'BatchProgram::doWork':      to perform the core work
  Both functions have full access to a Qt event loop (required for timer,
  networking, ...).

  If you need argc and/or argv outside these two functions,
  the BP() singleton instance can be accessed.
*/

mClass(Prog) BatchProgram : public NamedCallBacker
{ mODTextTranslationClass(BatchProgram);
    mGlobal(Batch) friend	BatchProgram& BP();

public:

    enum Status { Start, ParseFail, ParseOK, CommFail, CommOK, LogFail, LogOK,
		  WorkWait, WorkStarted, WorkFail, WorkPaused, MoreToDo, WorkOK,
		  Killed, LockOK };
			mDeclareEnumUtils(Status);

    mExp(Batch) void		loadModules();
			/*!<Must be implemented to load the basic modules
			    required by the batch program, for example:
			    OD::ModDeps().ensureLoaded( "Attributes" );
			    Can only be empty if the batch program does not
			    depend on modules above Network	 */

    mExp(Batch) bool		doWork(od_ostream&);
			/*!< This method must be defined by user, and should
			     contain the implementation			  */

    mExp(Batch) bool	isOK() const;
    const CommandLineParser&	clParser()	{ return *clparser_; }
    const IOPar&	pars() const		{ return *iopar_; }
    IOPar&		pars()			{ return *iopar_; }

    mExp(Batch) bool	pauseRequested();
			//<! pause requested (via socket) by primary host?
    mExp(Batch) void	setResumed();

    mExp(Batch) bool	errorMsg(const uiString&,bool cc_stderr=false);
    mExp(Batch) bool	infoMsg(const char*,bool cc_stdout=false);

    static const char*	sKeyDataDir()	{ return "datadir"; }
    static const char*	sKeySurveyDir() { return "surveydir"; }
    static const char*	sKeySimpleBatch()	{ return "noparfile"; }
    static const char*	sKeyFinishMsg();

    Notifier<BatchProgram>  eventLoopStarted;
    Notifier<BatchProgram>  startDoWork;
    Notifier<BatchProgram>  pause;
    Notifier<BatchProgram>  resume;
    Notifier<BatchProgram>  killed;
    Notifier<BatchProgram>  endWork;

private:

			BatchProgram();
			~BatchProgram();

    CommandLineParser*	clparser_ = nullptr;
    IOPar*		iopar_ = nullptr;
    od_ostream*		strm_ = nullptr;

    void		progKilled(CallBacker*);
    void		killNotify(bool yn);

    const BufferString	getLockFileFP() const;

    JobCommunic*	mmComm()		{ return comm_; }
    Batch::ID		jobId()			{ return jobid_; }
    mExp(Batch) float	getPriority() const	{ return priority_; }

    Status		status_ = Start;
    Threads::Lock	statelock_;

    JobCommunic*	comm_ = nullptr;
    Batch::ID		jobid_;
    float		priority_ = 0.f;
    bool		strmismine_ = true;
    Timer*		timer_;
    Threads::Thread*	thread_ = nullptr;
    Threads::Lock	batchprogthreadlock_;

    ObjectSet<OD::JSON::Object> requests_;

    void		eventLoopStartedCB(CallBacker*);
    void		workMonitorCB(CallBacker*);
    mExp(Batch) bool    canReceiveRequests() const;
    mExp(Batch) void    initWork();
    mExp(Batch) void    startTimer();
    void		doWorkCB(CallBacker*);
    mExp(Batch) void    postWork(bool res);
    void		doFinalize();
    mExp(Batch) void	endWorkCB(CallBacker*);

    mExp(Batch) bool	init();
    bool		parseArguments();
			//<! Parses command line arguments
    bool		initComm();
			//<! Initializes job communication with primary host
    bool		initLogging();
			//<! Initialized logging stream

    mExp(Batch) void	modulesLoaded();
    bool		canStartdoWork() const;
    bool		updateLockFilePars() const;

    static BatchProgram* inst_;

    mExp(Batch) static void	deleteInstance(int retcode);
    friend void		Execute_batch(int*,char**);
    friend void		loadModulesCB(CallBacker*);
    friend void		launchDoWorkCB(CallBacker*);
    friend void		doWorkCB(CallBacker*);
    friend class BatchServiceServerMgr;

};

#ifdef __unix__
void Execute_batch(int*,char**);
void loadModulesCB(CallBacker*);
void launchDoWorkCB(CallBacker*);
void doWorkCB(CallBacker*);
#endif
mGlobal(Batch) BatchProgram& BP();


#define mDefLoadModules() \
    void BatchProgram::loadModules() {

#define mLoad1Module(mod1nm) \
    mDefLoadModules() \
	OD::ModDeps().ensureLoaded( mod1nm ); }
#define mLoad2Modules(mod1nm,mod2nm) \
    mDefLoadModules() \
	OD::ModDeps().ensureLoaded( mod1nm ); \
	OD::ModDeps().ensureLoaded( mod2nm ); }
#define mLoad3Modules(mod1nm,mod2nm,mod3nm) \
    mDefLoadModules() \
	OD::ModDeps().ensureLoaded( mod1nm ); \
	OD::ModDeps().ensureLoaded( mod2nm ); \
	OD::ModDeps().ensureLoaded( mod3nm ); }

#define mRetError(s) \
{ errorMsg(::toUiString(s)); mDestroyWorkers; return false; }

#define mRetJobErr(s) \
{  \
    if ( comm_ ) \
	comm_->setState( JobCommunic::JobError ); \
    mRetError(s) \
}

#define mRetHostErr(s) \
{  \
    if ( comm_ ) comm_->setState( JobCommunic::HostError ); \
	mRetError(s) \
}

#define mStrmWithProcID(s) \
strm << "\n[" << process_id << "]: " << s << "." << od_newline

#define mMessage(s) \
strm << s << '.' << od_newline

#define mSetCommState(State) \
if ( comm_ ) \
{ \
    comm_->setState( JobCommunic::State ); \
    if ( !comm_->updateState() ) \
	mRetHostErr( comm_->errMsg() ) \
}

#ifdef __prog__
# ifdef __win__
#  include "moddepmgr.h"
#  include "_execbatch.h"
# endif
#define mMainIsDefined
    int doMain( int argc, char** argv )
    {
	OD::SetRunContext( OD::BatchProgCtxt );
	SetProgramArgs( argc, argv );
	ApplicationData app;
	Execute_batch( &argc, argv );
	BP().eventLoopStarted.notify(mSCB(loadModulesCB));
	BP().startDoWork.notify(mSCB(launchDoWorkCB));

	return ApplicationData::exec();
    }

    int main( int argc, char** argv )
    {
	ExitProgram( doMain(argc,argv) );
    }

#endif // __prog__

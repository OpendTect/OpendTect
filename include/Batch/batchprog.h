#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		14-9-1998
 RCS:		$Id$
________________________________________________________________________

 Batch programs should include this header, and define a BatchProgram::go().
 If program args are needed outside this method, BP() can be accessed.

*/

#include "batchmod.h"

#include "applicationdata.h"
#include "batchjobdispatch.h"
#include "prog.h"
#include "namedobj.h"
#include "bufstringset.h"
#include "genc.h"
#include "file.h"
#include "odbatchservice.h"

class CommandLineParser;
class IOObj;
class IOObjContext;
class od_ostream;
class MMSockCommunic;
class JobCommunic;
class StreamData;
class Timer;

/*!
\brief Main object for 'standard' batch programs.

  Most 'interesting' batch programs need a lot of parameters to do the work.
  Therefore, in OpendTect, BatchPrograms need a 'parameter file', with all
  the info needed in IOPar format, i.e., keyword/value pairs.

  This object takes over the details of reading that file, extracting
  'standard' components from the parameters, opening sockets etc.,

  To use the object, instead of defining a function 'main', you should define
  the function 'BatchProgram::go'.

  If you need argc and/or argv outside go(), the BP() singleton instance can
  be accessed.
*/

#define mRetJobErr(s) \
{  \
    if ( comm_ ) comm_->setState( JobCommunic::JobError ); \
    mRetError(s) \
}


#define mRetError(s) \
{ errorMsg(toUiString(s)); mDestroyWorkers; return false; }

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

mClass(Prog) BatchProgram : public NamedObject
{ mODTextTranslationClass(BatchProgram);
    mGlobal(Batch) friend	BatchProgram& BP();

public:

    const IOPar&		pars() const	{ return *iopar_; }
    IOPar&			pars()		{ return *iopar_; }

    const CommandLineParser&	clParser()	{ return *clparser_; }

    bool		isStartDoWork() { return startdoworknow_; }
    mExp(Batch) IOObj*	getIOObjFromPars(const char* keybase,bool mknew,
					 const IOObjContext& ctxt,
					 bool msgiffail=true) const;

    od_ostream&		getLogStream() { return strm_ ? *strm_ : od_cout(); }

			//! pause requested (via socket) by master?
    mExp(Batch) bool	pauseRequested() const;

    mExp(Batch) bool	errorMsg( const uiString& msg, bool cc_stderr=false);
    mExp(Batch) bool	infoMsg( const char* msg, bool cc_stdout=false);

    mExp(Batch) static void	deleteInstance(int retcode);

    void		removeAllNotifiers() { detachAllNotifiers(); }

    static const char*	sKeyMasterHost();
    static const char*	sKeyMasterPort();
    static const char*	sKeyBG();
    static const char*	sKeyJobID();
    static const char*	sKeyDataDir()		{ return "datadir"; }
    static const char*	sKeyFinishMsg() { return "Finished batch processing."; }
    static const char*	sKeySurveyDir() { return "surveydir"; }
    static const char*	sKeySimpleBatch() { return "noparfile"; }

    bool		isStillOK() { return stillok_; }
    void		setStillOK( bool yn ) { stillok_ = yn; }
    bool		initWork(od_ostream& log_stream);
			//Implementation will be updated
	    //! This method must be defined by user when using service manager
    bool		doWork(od_ostream& log_stream);
				//! This method must be defined by user

    Notifier<BatchProgram>     programStarted;
    Notifier<BatchProgram>     startDoWork;

protected:

    friend void		Execute_batch(int*,char**);

    //friend class	JobCommunic;

			BatchProgram();
			~BatchProgram();

    mExp(Batch) void	init();
    static BatchProgram* inst_;


    bool		stillok_ = false;
    bool		inbg_ = false;
    od_ostream*		strm_ = nullptr;
    IOPar*		iopar_ = nullptr;
    CommandLineParser*	clparser_ = nullptr;

    BufferStringSet	requests_;
    BufferString	finishmsg_;	//Dot NOT use, will be removed after 6.2

    mExp(Batch) bool	initOutput();
    mExp(Batch) void	progKilled(CallBacker*);
    mExp(Batch) void	killNotify( bool yn );

    JobCommunic*	mmComm()		{ return comm_; }
    int			jobId()			{ return jobid_; }

private:

    JobCommunic*		comm_ = nullptr;
    int				jobid_;
    bool			strmismine_ = true;
    Timer*			timer_;
    bool			startdoworknow_;
    void			eventLoopStarted(CallBacker*);
    void			doWorkCB(CallBacker*);

};


void Execute_batch(int*,char**);
mGlobal(Batch) BatchProgram& BP();

#ifdef __prog__
# ifdef __win__
#  include "_execbatch.h"
# endif

    static void eventLoopStartedCB(CallBacker*cb)
    {
	if ( !BP().isStillOK() )
	{
	    BP().removeAllNotifiers();
	    BatchProgram::deleteInstance( 0 );
	    return;
	}

	if ( BP().getLogStream().isOK() && BP().initWork(BP().getLogStream())
						&& BP().isStartDoWork() )
	    BP().setStillOK( BP().doWork(BP().getLogStream()) );

    }

    static void doWorkCB( CallBacker* cb )
    {
	BP().setStillOK( BP().doWork(BP().getLogStream()) );

	if ( BP().isStillOK() )
	{
	    ODBatchService& odsm = ODBatchService::getMgr( true );
	    odsm.processingComplete();
	}
    }


    int main( int argc, char** argv )
    {
	SetProgramArgs( argc, argv );
	ApplicationData app;
	Execute_batch( &argc, argv );
	BP().programStarted.notify( mSCB(eventLoopStartedCB) );
	if ( !BP().isStartDoWork() )
	    BP().startDoWork.notify( mSCB(doWorkCB) );

	const int ret = ApplicationData::exec();

	return ExitProgram( ret );
    }


#endif // __prog__


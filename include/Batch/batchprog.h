#ifndef batchprog_h
#define batchprog_h
 
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
#include "prog.h"
#include "namedobj.h"
#include "bufstringset.h"
#include "genc.h"

#include <iosfwd>

class CommandLineParser;
class IOPar;
class IOObj;
class IOObjContext;
class MMSockCommunic;
class JobCommunic;
class StreamData;

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

mClass(Prog) BatchProgram : public NamedObject
{
    mGlobal(Batch) friend	BatchProgram& BP();

public:

    const IOPar&		pars() const	{ return *iopar_; }
    IOPar&			pars()		{ return *iopar_; }
    
    const CommandLineParser&	clParser()	{ return *clparser_; }

			//! This method must be defined by user
    bool		go(std::ostream& log_stream);

    mExp(Batch) IOObj*	getIOObjFromPars(const char* keybase,bool mknew,
					 const IOObjContext& ctxt,
					 bool msgiffail=true) const;

			//! pause requested (via socket) by master?
    mExp(Batch) bool	pauseRequested() const;

    mExp(Batch) bool	errorMsg( const char* msg, bool cc_stderr=false);
    mExp(Batch) bool	infoMsg( const char* msg, bool cc_stdout=false);

    mExp(Batch) static void	deleteInstance();

    
    static const char*	sKeyMasterHost() 	{ return "masterhost"; }
    static const char*	sKeyMasterPort()	{ return "masterport"; }
    static const char*	sKeyBG()		{ return "bg"; }
    static const char*	sKeyJobID()		{ return "jobid"; }
    
protected:

    friend int		Execute_batch(int*,char**);
    
    //friend class	JobCommunic;

			BatchProgram();
			~BatchProgram();

    mExp(Batch) void	init();
    static BatchProgram* inst_;


    bool		stillok_;
    bool		inbg_;
    StreamData&		sdout_;
    IOPar*		iopar_;
    CommandLineParser*	clparser_;
	
    BufferString	parversion_;
    BufferStringSet	requests_;
    BufferString	finishmsg_;

    mExp(Batch) bool	initOutput();
    mExp(Batch) void	progKilled(CallBacker*);
    mExp(Batch) void	killNotify( bool yn );

    JobCommunic*	mmComm()		{ return comm_; }
    int 		jobId()			{ return jobid_; }

private:

    JobCommunic*	comm_;
    int			jobid_;
};


int Execute_batch(int*,char**);
mGlobal(Batch) BatchProgram& BP();

#define mRetJobErr(s) \
{  \
    if ( comm_ ) comm_->setState( JobCommunic::JobError ); \
    mRetError(s) \
}


#define mRetError(s) \
{ errorMsg(s); mDestroyWorkers; return false; }

#define mRetHostErr(s) \
{  \
    if ( comm_ ) comm_->setState( JobCommunic::HostError ); \
	mRetError(s) \
}

#define mStrmWithProcID(s) \
strm << "\n[" << process_id << "]: " << s << "." << std::endl

#define mSetCommState(State) \
if ( comm_ ) \
{ \
    comm_->setState( JobCommunic::State ); \
    if ( !comm_->updateState() ) \
	mRetHostErr( comm_->errMsg() ) \
}

#ifdef __prog__
# ifdef __win__
#  include "_execbatch.h"
# endif
    int main( int argc, char** argv )
    {
	SetProgramArgs( argc, argv );
	int ret = Execute_batch(&argc,argv);
	ExitProgram( ret );
	return ret;
    }

#endif // __prog__

#endif


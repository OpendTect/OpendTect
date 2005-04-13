#ifndef batchprog_h
#define batchprog_h
 
/*
________________________________________________________________________
 
 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		14-9-1998
 RCS:		$Id: batchprog.h,v 1.28 2005-04-13 14:43:15 cvsarend Exp $
________________________________________________________________________

 Batch programs should include this header, and define a BatchProgram::go().
 If program args are needed outside this method, BP() can be accessed.
 
*/

#include "prog.h"
#include "uidobj.h"
#include "bufstringset.h"
#include "genc.h"
#include "timefun.h"
#include <iosfwd>
class IOPar;
class IOObj;
class Socket;
class StreamData;
class IOObjContext;
class MMSockCommunic;

/*!\brief Main object for 'standard' batch programs.

  Most 'interesting' batch programs need a lot of parameters to do the work.
  In dTect, these accept a 'parameter file', with all the info needed in
  IOPar format, i.e. keyword/value pairs.

  This object takes over the details of reading that file, extracting
  'standard' components from the parameters, opening sockets, etc. etc.

  To use the object, instead of defining a function 'main', you should define
  the function 'BatchProgram::go'.

  If you need argc and/or argv outside go(), the BP() singleton instance can
  be accessed.

*/

class BatchProgram : public UserIDObject
{
public:

    const IOPar&	pars() const		{ return *iopar; }
    IOPar&		pars()			{ return *iopar; }

    int			nrArgs() const		{ return *pargc - argshift; }
    const char*		arg( int idx ) const	{ return argv_[idx+argshift]; }
    const char*		fullPath() const	{ return fullpath; }
    const char*		progName() const;

			//! This method must be defined by user
    bool		go(std::ostream& log_stream);

			// For situations where you need the old-style stuff
    char**		argv()			{ return argv_; }
    int			argc()			{ return *pargc; }
    int&		argc_r()		{ return *pargc; }
    int			realArgsStartAt() const	{ return argshift; }
    BufferStringSet&	cmdLineOpts()		{ return opts; }

    IOObj*		getIOObjFromPars(const char* keybase,bool mknew,
					 const IOObjContext& ctxt,
					 bool msgiffail=true) const;

			//! pause requested (via socket) by master?
    bool		pauseRequested()	{ return pausereq; }

    bool		errorMsg( const char* msg, bool cc_stderr=false);
    bool		infoMsg( const char* msg, bool cc_stdout=false);

protected:

    friend int		Execute_batch(int*,char**);
    friend const BatchProgram& BP();
    friend class	MMSockCommunic;

			BatchProgram(int*,char**);
			~BatchProgram();

    static BatchProgram* inst;

    int*		pargc;
    char**		argv_;
    int			argshift;
    FileNameString	fullpath;
    bool		stillok;
    bool		inbg;
    StreamData&		sdout;
    IOPar*		iopar;
    BufferStringSet	opts;

    bool		initOutput();
    void		progKilled(CallBacker*);
    void		killNotify( bool yn );

    MMSockCommunic*	mmComm()		{ return comm; }

    int 		jobId()			{ return jobid; }
    void		setPauseReq( bool yn )	{ pausereq = yn; }

private:

    MMSockCommunic*	comm;
    bool		pausereq;
    int			jobid;
};


inline const BatchProgram& BP() { return *BatchProgram::inst; }

#ifdef __prog__
# ifdef __win__
#  include "_execbatch.h"
# endif

    int main( int argc, char** argv )
    {
	int ret = Execute_batch(&argc,argv);
	return exitProgram( ret );
    }

#endif // __prog__


#define mReturn( ret ) { \
    if ( ret ) { nrattempts = 0; return true; } \
    if ( nrattempts++ < maxtries ) return true; \
    stillok = false; \
    setErrMsg("Lost connection with master[1]."); \
    return false; \
}

#define mTryMaxtries( fn ) { \
    for ( int i=0; i<maxtries; i++ ) \
    { \
	bool ret = fn; \
	if ( ret ) return true; \
	Time_sleep(1); \
    } \
    stillok = false; \
    setErrMsg("Lost connection with master[2]."); \
    return false; \
}


/*! \brief Multi-machine socket communicator
 * 
 */ 
class MMSockCommunic
{
public:
    enum State		{ Undef, Working, Finished, Done, Paused, JobError,
	                  HostError, Killed, Timeout };

			MMSockCommunic( const char* host, int port,
					BatchProgram& );

    bool		ok()		{ return stillok; }
    const char*		errMsg()	{ return errmsg; }

    State		state()	const	{ return stat; }
    void		setState( State s ) { stat = s; }
			
    bool		updateState()
			    { bool ret = sendState_(stat); mReturn(ret) }
    bool		updateProgress( int p )
			    { bool ret = sendProgress_(p); mReturn(ret) }
			    
    bool		sendState(  bool isexit=false )
			    { mTryMaxtries( sendState_(stat,isexit) ) }
    bool		sendProgress( int p )
			    { mTryMaxtries( sendProgress_(p) ) }

			//! hostrelated error messages are more serious.
    bool		sendErrMsg( const char* msg )
			    { mTryMaxtries( sendErrMsg_(msg) ) }
    bool		sendPID( int pid )
			    { mTryMaxtries( sendPID_(pid) ) }


protected:

    BufferString	masterhost;
    int			masterport;
    bool		stillok;
    State		stat;
    BufferString	errmsg;

    Socket*		mkSocket();

    bool		sendState_( State, bool isexit=false );
    bool		sendProgress_( int );
    bool		sendPID_( int );
    bool		sendErrMsg_( const char* msg );

private:    

    BatchProgram&	bp;

    bool		updateMsg( char tag, int, const char* msg=0 );
    bool		sendMsg( char tag, int, const char* msg=0 );

			//! directly to bp.stdout.ostrem or std::cerr.
    void		directMsg( const char* msg );

    void		setErrMsg( const char* m )
			{
			    errmsg = ("["); errmsg += getPID(); errmsg += "]: ";
			    errmsg += m;
			}

    int			timestamp;
    int			nrattempts;
    int 		maxtries;
};

#undef mReturn

#endif

#ifndef batchprog_h
#define batchprog_h
 
/*
________________________________________________________________________
 
 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		14-9-1998
 RCS:		$Id: batchprog.h,v 1.22 2004-04-18 17:51:12 dgb Exp $
________________________________________________________________________

 Batch programs should include this header, and define a BatchProgram::go().
 If program args are needed outside this method, BP() can be accessed.
 
*/

#include "prog.h"
#include "uidobj.h"
#include "mmdefs.h"
#include "bufstringset.h"
#include "genc.h"
class IOPar;
class IOObj;
class Socket;
class StreamData;
class IOObjContext;


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

    const IOPar&	pars() const		{ return *iopar_; }
    int			nrArgs() const		{ return *pargc_ - argshift_; }
    const char*		arg( int idx ) const	{ return argv_[idx+argshift_]; }
    const char*		fullPath() const	{ return fullpath_; }
    const char*		progName() const;

			//! This method must be defined by user
    bool		go(ostream& log_stream);

			// For situations where you need the old-style stuff
    char**		argv()			{ return argv_; }
    int			argc()			{ return *pargc_; }
    int&		argc_r()		{ return *pargc_; }
    int			realArgsStartAt() const	{ return argshift_; }
    BufferStringSet&	cmdLineOpts()	{ return opts_; }

    IOObj*		getIOObjFromPars(const char* keybase,bool mknew,
					 const IOObjContext& ctxt,
					 bool msgiffail=true) const;

    			// Socket stuff.

			//! write status over sock if sock avail.
    inline bool		writeStatus( char tag, int stat, const char* errmsg=0 )
			    { return writeStatus_( tag, stat, errmsg, true ); }
			//! update status over sock if sock avail.
    inline bool		updateStatus( char tag, int stat, const char* errmsg=0 )
			    { return writeStatus_( tag, stat, errmsg, false ); }

			//! write error msg over sock if sock avail.
    bool		writeErrorMsg( const char* msg );
			//! pause requested (via socket) by master?
    bool		pauseRequested()	{ return pausereq_; }

protected:

    friend int		Execute_batch(int*,char**);
    friend const BatchProgram& BP();

			BatchProgram(int*,char**);
			~BatchProgram();

    static BatchProgram* inst_;

    int*		pargc_;
    char**		argv_;
    int			argshift_;
    FileNameString	fullpath_;
    bool		stillok_;
    bool		inbg_;
    StreamData&		sdout_;
    IOPar*		iopar_;
    BufferStringSet	opts_;

    Socket*		mkSocket();
    BufferString	masterhost_;
    int			masterport_;
    bool		usesock_;
    int			jobid_;
    bool		pausereq_;

    bool		initOutput();
    void		progKilled(CallBacker*);
    void		killNotify( bool yn );

    bool		writeStatus_( char tag, int, const char*, bool force );

    int			exitstat_;
    int			timestamp_;

};


inline const BatchProgram& BP() { return *BatchProgram::inst_; }

#ifdef __prog__
# ifdef __win__
#  include "_execbatch.h"
# endif

    int main( int argc, char** argv )
    {
	int ret = Execute_batch(&argc,argv);
	exitProgram( ret );
    }

#endif // __prog__

#endif

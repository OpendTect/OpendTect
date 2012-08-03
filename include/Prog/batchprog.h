#ifndef batchprog_h
#define batchprog_h
 
/*
________________________________________________________________________
 
 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		14-9-1998
 RCS:		$Id: batchprog.h,v 1.44 2012-08-03 13:00:34 cvskris Exp $
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
class IOPar;
class IOObj;
class IOObjContext;
class MMSockCommunic;
class JobCommunic;
class StreamData;


/*!\brief Main object for 'standard' batch programs.

  Most 'interesting' batch programs need a lot of parameters to do the work.
  Therefore, in OpendTect, BatchPrograms need a 'parameter file', with all
  the info needed in IOPar format, i.e. keyword/value pairs.

  This object takes over the details of reading that file, extracting
  'standard' components from the parameters, opening sockets, etc. etc.

  To use the object, instead of defining a function 'main', you should define
  the function 'BatchProgram::go'.

  If you need argc and/or argv outside go(), the BP() singleton instance can
  be accessed.

*/


mClass(Batch) BatchProgram : public NamedObject
{
    mGlobal(Batch) friend	BatchProgram& BP();
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
    int&		argc()			{ return *pargc; }
    int			argc() const		{ return *pargc; }
    int			realArgsStartAt() const	{ return argshift; }
    BufferStringSet&	cmdLineOpts()		{ return opts; }

    IOObj*		getIOObjFromPars(const char* keybase,bool mknew,
					 const IOObjContext& ctxt,
					 bool msgiffail=true) const;

			//! pause requested (via socket) by master?
    bool		pauseRequested() const;

    bool		errorMsg( const char* msg, bool cc_stderr=false);
    bool		infoMsg( const char* msg, bool cc_stdout=false);

    static void		deleteInstance();

protected:

    friend int		Execute_batch(int*,char**);
    
    //friend class	JobCommunic;

			BatchProgram();
			~BatchProgram();

    void		init(int*,char**);
    static BatchProgram* inst_;

    int*		pargc;
    char**		argv_;
    int			argshift;
    FileNameString	fullpath;
    bool		stillok;
    bool		inbg;
    StreamData&		sdout;
    IOPar*		iopar;
    BufferStringSet	opts;
    BufferString	parversion_;
    BufferStringSet	requests_;
    BufferString	finishmsg_;

    bool		initOutput();
    void		progKilled(CallBacker*);
    void		killNotify( bool yn );

    JobCommunic*	mmComm()		{ return comm; }
    int 		jobId()			{ return jobid; }

private:

    JobCommunic*	comm;
    int			jobid;
};


int Execute_batch(int*,char**);
mGlobal(Batch) BatchProgram& BP();


#ifdef __prog__
# ifdef __win__
#  include "_execbatch.h"
# endif
    int main( int argc, char** argv )
    {
	int ret = Execute_batch(&argc,argv);
	ExitProgram( ret );
	return ret;
    }

#endif // __prog__

#endif


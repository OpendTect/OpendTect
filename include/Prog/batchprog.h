#ifndef batchprog_H
#define batchprog_H
 
/*
________________________________________________________________________
 
 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		14-9-1998
 RCS:		$Id: batchprog.h,v 1.5 2002-12-17 15:08:35 arend Exp $
________________________________________________________________________

 Batch programs should include this header, and define a BatchProgram::go().
 If program args are needed outside this method, BP() can be accessed.
 
*/

#include <uidobj.h>
#include <mmdefs.h>
class IOPar;
class StreamData;
class Socket;


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

    bool		writeStatus( char tag, int );

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

    Socket*		mkSocket();
    BufferString	masterhost_;
    int			masterport_;
    bool		usesock_;
    int			jobid_;

    bool		initOutput();
    void		progKilled(CallBacker*);
    void		killNotify( bool yn );

    int			exitstat_;

};


inline const BatchProgram& BP() { return *BatchProgram::inst_; }

#ifdef __prog__
    int main( int argc, char** argv )
	{ return Execute_batch(&argc,argv); }
#endif


#endif

#ifndef executor_H
#define executor_H

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		11-7-1996
 Contents:	Executor
 RCS:		$Id: executor.h,v 1.3 2001-05-31 12:55:04 windev Exp $
________________________________________________________________________

-*/

#include <uidobj.h>
#include <basictask.h>
#include <iosfwd>

/*!\brief specification to enable chunkwise execution a process.

The execute() utility executes while logging message() etc. to a stream.
If nextStep returns -1 (Failure) the error message should be in message().

*/

class Executor : public UserIDObject
	       , public BasicTask
{
public:
			Executor( const char* nm )
			: UserIDObject(nm)		{}
    virtual		~Executor()			{}

    virtual const char*	message() const			= 0;
    virtual int		nextStep()			= 0;
			/*!< return value > 1 should mean a warning is
			     available */

    virtual int		totalNr() const			{ return -1; }
    virtual int		nrDone() const			{ return 0; }
    virtual const char*	nrDoneText() const		{ return "Nr done"; }

    virtual bool	execute(ostream* log=0,
				bool isfirst=YES,bool islast=YES);

};


#endif

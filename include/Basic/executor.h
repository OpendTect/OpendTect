#ifndef executor_H
#define executor_H

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		11-7-1996
 RCS:		$Id: executor.h,v 1.8 2002-04-19 16:09:11 bert Exp $
________________________________________________________________________

-*/

#include "uidobj.h"
#include "basictask.h"
#include <iosfwd>

template <class T> class ObjectSet;

/*!\brief specification to enable chunkwise execution a process.

Interface enabling separation of the control of execution of any process from
what actually is going on. The work is done by calling the nextStep() method
until either ErrorOccurred or Finished is returned. To enable logging and/or
communication with the user, two types of info can be made available (the
methods will be called before the step is executed). Firstly, a message.
Secondly, info on the progress.
It is common that Executors are combined to a new Executor object. This is
the most common reason why totalNr() can change.

If nextStep returns -1 (Failure) the error message should be in message().

The execute() utility executes the process while logging message() etc. to
a stream. Useful in batch situations.

*/

class Executor : public UserIDObject
	       , public BasicTask
{
public:
			Executor( const char* nm )
			: UserIDObject(nm)		{}
    virtual		~Executor()			{}

    virtual int		nextStep()			= 0;
    			//!< Essentially returns as required by BasicTask
    static const int	ErrorOccurred;		// -1
    static const int	Finished;		// 0
    static const int	MoreToDo;		// 1
    static const int	WarningAvailable;	// 2

    virtual const char*	message() const			{ return "Working"; }
    virtual int		totalNr() const			{ return -1; }
    			//!< If unknown, return -1
    virtual int		nrDone() const			{ return 0; }
    virtual const char*	nrDoneText() const		{ return "Nr done"; }

    virtual bool	execute(ostream* log=0,bool isfirst=true,
	    			bool islast=true,int delaybetwnstepsinms=0);
};


class ExecutorRunner : public BasicTaskRunner
{
public:

    			ExecutorRunner( Executor* ex )
			: BasicTaskRunner(ex)	{}

    virtual const char* lastMsg() const		= 0;

};


class ExecutorBatchTaskRunner : public ExecutorRunner
{
public:
			ExecutorBatchTaskRunner( Executor* ex, ostream* strm=0,
					bool isfirst=true, bool islast=true )
			: ExecutorRunner(ex)
			, logstrm_(strm)
			, isfirst_(isfirst)
			, islast_(islast)		{}

    virtual bool	execute()
    			{
			    mDynamicCastGet(Executor*,ex,task_);
			    if ( !ex )
			    	{ lastmsg_ = "No Executor!"; return false; }
			    bool res = ex->execute(logstrm_,isfirst_,islast_);
			    lastmsg_ = ex->message();
			    return res;
			}
    virtual const char*	lastMsg() const		{ return lastmsg_; }

protected:

    ostream*		logstrm_;
    bool		isfirst_;
    bool		islast_;
    BufferString	lastmsg_;
};


/*!\brief Executor consisting of other executors.

Executors may be added on the fly while processing. The executors are
executed in the order in which they were added.

*/


class ExecutorGroup : public Executor
{
public:
    			ExecutorGroup( const char* nm );
    virtual		~ExecutorGroup();
    virtual void	add( Executor* );
    			/*!< You will become mine!! */

    virtual int		nextStep();
    virtual const char*	message() const;
    virtual int		totalNr() const;
    virtual int		nrDone() const;
    virtual const char*	nrDoneText() const;

    void		setNrDoneText( const char* txt )
			{ nrdonetext = txt; }
    			//!< If set, will use this and the counted nrdone

protected:

    int			currentexec;
    int			nrdone;
    BufferString	nrdonetext;
    ObjectSet<Executor>& executors;

};

#endif

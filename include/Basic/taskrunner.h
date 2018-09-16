#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert/Kris
 Date:		Sep 2017
________________________________________________________________________

-*/

#include "basicmod.h"
#include "gendefs.h"
#include "od_iosfwd.h"
class Task;


/*!\brief Class that can execute a task.

  Can be used as such, be inherited by fancy subclasses with user interface
  and progressbars etc.

*/

mExpClass(Basic) TaskRunner
{
public:

			TaskRunner() : execres_(false)	{}
    virtual		~TaskRunner()			{}

    virtual bool	execute(Task&)			= 0;
    virtual bool	execResult() const		{ return execres_; }

    static bool		execute(TaskRunner*,Task&);	//!< runner may be null

    virtual void	emitErrorMessage(const uiString&,
			    bool iswarn=false) const	= 0;

protected:

			TaskRunner(const TaskRunner&)	= delete;

    bool		execres_;

};


/*!\brief can make a TaskRunner when needed.

 Some processes may take a lot of time, but may also be done in microseconds.
 To leave it to the receiver whether a TaskRunner needs to be constructed, pass
 a TaskRunnerProvider.

 The usage of this class is usualy preferred above creating a TaskRunner
 directly.

*/

mExpClass(Basic) TaskRunnerProvider
{
public:

    virtual		~TaskRunnerProvider()	{ retire(); }

    bool		execute( Task& t ) const { return runner().execute(t); }

    virtual void	retire() const
			{ if ( ismine_ ) delete runner_; runner_ = 0; }

    static bool		execute(const TaskRunnerProvider*,Task&);
				//!< may be null

    virtual TaskRunner&	runner() const					= 0;
    virtual void	emitErrorMessage( const uiString& msg,
					  bool wrn=false ) const
			{ runner().emitErrorMessage(msg,wrn); }

protected:

    mutable TaskRunner*	runner_ = 0;
    mutable bool	ismine_ = true;

};


/*!\brief just does the work and tells you whether it succeeded. */

mExpClass(Basic) SilentTaskRunner : public TaskRunner
{
public:

    virtual bool	execute(Task&);
    virtual void	emitErrorMessage(const uiString&,
					 bool iswarn=false) const;
				// puts message in log file

};


/*!\brief even if long hard work is needed, still no feedback is given. */

mExpClass(Basic) SilentTaskRunnerProvider : public TaskRunnerProvider
{
public:

    virtual TaskRunner&	runner() const
			{
			    if ( !runner_ )
				runner_ = new SilentTaskRunner;
			    return *runner_;
			}
};


/*!\brief uses existing TaskRunner to do the work. */

mExpClass(Basic) ExistingTaskRunnerProvider : public TaskRunnerProvider
{
public:

			ExistingTaskRunnerProvider( TaskRunner* r )
			{
			    runner_ = r;
			    ismine_ = false;
			}

    virtual TaskRunner&	runner() const
			{
			    if ( !runner_ )
			    {
				runner_ = new SilentTaskRunner;
				ismine_ = true;
			    }
			    return *runner_;
			}

};



/*!\brief TaskRunner to show progress of a Task in text format.  */

mExpClass(Basic) LoggedTaskRunner : public TaskRunner
{
public:
			LoggedTaskRunner( od_ostream& strm )
			    : strm_(strm)	{}

    virtual bool	execute(Task&);
    virtual void	emitErrorMessage(const uiString&,bool wrn=false) const;

protected:

    od_ostream&		strm_;

};


/*!\brief uses existing TaskRunner to do the work. */

mExpClass(Basic) LoggedTaskRunnerProvider : public TaskRunnerProvider
{
public:

			LoggedTaskRunnerProvider( od_ostream& strm )
			    : strm_(strm)	{}

    virtual TaskRunner&	runner() const
			{
			    if ( !runner_ )
				runner_ = new LoggedTaskRunner( strm_ );
			    return *runner_;
			}

protected:

    od_ostream&		strm_;

};

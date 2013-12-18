#ifndef batchjobdispatch_h
#define batchjobdispatch_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Dec 2013
 RCS:           $Id$
________________________________________________________________________

-*/

#include "generalmod.h"
#include "factory.h"
#include "iopar.h"

namespace Batch
{

/*!\brief just the data we need to specify a batch job. */

mExpClass(General) JobSpec
{
public:

    BufferString	prognm_;
    IOPar		pars_;

};


/*!\brief Base class (with factory) for methods to kick-off a batch job.

  Subclasses are expected to be ranging from simple single-prcess starters to
  elaborate cluster-based job splitting monsters.

  Feedback to the user, if any, us left to the subclass to provide.

 */

mExpClass(General) JobDispatcher
{
public:

			JobDispatcher()			{}

    virtual		~JobDispatcher()		{}

    virtual const char*	description() const		= 0;
    virtual bool	isSuitedFor(const JobSpec&,
			    BufferString* whynot=0) const = 0;

    bool		go(const JobSpec&);
    const char*		errMsg() const			{ return errmsg_; }

    mDefineFactoryInClass(JobDispatcher,factory);

protected:

    virtual bool	launch()				= 0;

    JobSpec		jobspec_;
    BufferString	errmsg_;

};


} // namespace Batch


#endif

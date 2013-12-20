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

/*!\brief the data we need to specify a batch job. */

mExpClass(General) JobSpec
{
public:

    enum ProcType	{ Attrib, AttribEM, Grid2D, PreStack, SEGY, T2D,
			  VelConv, Vol };

    			JobSpec(ProcType);
    			JobSpec( const char* pnm=0, bool isodprog=true )
			    : prognm_(pnm), isodprog_(isodprog)		{}

    BufferString	prognm_;
    bool		isodprog_;
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
    virtual bool	isAlwaysLocal() const		{ return true; }
    void		setRemoteHost( const char* rh )	{ remotehost_ = rh; }
				//!< only used when !isAlwaysLocal()
    virtual bool	hasLog() const		{ return true; }
    void		setLog( const char* logspec )	{ logspec_ = logspec; }
    const char*		logSpec() const			{ return logspec_; }

    bool		go(const JobSpec&);
    const char*		errMsg() const			{ return errmsg_; }

    mDefineFactoryInClass(JobDispatcher,factory);

protected:

    virtual bool	launch()				= 0;

    JobSpec		jobspec_;
    BufferString	logspec_;
    BufferString	remotehost_;
    BufferString	errmsg_;

};


/*!\brief kicks off unattended batch jobs without GUI.
 
  To get a sort-of UI you can specify "window" as log.
  Default is "Std-IO", which starts the batch program in a terminal.

 */

mExpClass(General) SingleJobDispatcher : public JobDispatcher
{
public:

			SingleJobDispatcher()		{}
    virtual		~SingleJobDispatcher()		{}

    virtual const char*	description() const;
    virtual bool	isSuitedFor(const JobSpec&,
			    BufferString* whynot=0) const { return true; }
    virtual bool	isAlwaysLocal() const		{ return false; }
    virtual bool	hasLog() const			{ return true; }


protected:

    virtual bool	launch();

};


} // namespace Batch


#endif

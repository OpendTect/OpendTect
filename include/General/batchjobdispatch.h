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
#include "oscommand.h"
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
    OSCommandExecPars	execpars_;	//!< may be ignored by dispatcher

};


/*!\brief Base class (with factory) for methods to kick-off a batch job.

  Subclasses are expected to be ranging from simple single-prcess starters to
  elaborate cluster-based job splitting monsters.

 */

mExpClass(General) JobDispatcher
{
public:

			JobDispatcher()			{}

    virtual		~JobDispatcher()		{}

    virtual const char*	description() const		= 0;
    virtual bool	ignoresExecPars() const		{ return false; }
    virtual bool	isSuitedFor(const JobSpec&,
			    BufferString* whynot=0) const = 0;

    virtual bool	isAlwaysLocal() const		{ return true; }
    void		setRemoteHost( const char* rh )	{ remotehost_ = rh; }
				//!< only used when !isAlwaysLocal()

    bool		go(const JobSpec&);
    const char*		errMsg() const			{ return errmsg_; }

    mDefineFactoryInClass(JobDispatcher,factory);

protected:

    virtual bool	init()				{ return true; }
    virtual bool	launch()			= 0;

    JobSpec		jobspec_;
    BufferString	remotehost_;
    BufferString	errmsg_;

};


/*!\brief kicks off unattended batch jobs. */

mExpClass(General) SingleJobDispatcher : public JobDispatcher
{
public:

			SingleJobDispatcher();
    virtual		~SingleJobDispatcher()		{}

    virtual const char*	description() const;
    virtual bool	isSuitedFor(const JobSpec&,
			    BufferString* whynot=0) const { return true; }
    virtual bool	isAlwaysLocal() const		{ return false; }

    static const char*	sFactoryKey()		{ return "Single Process"; }

protected:

    virtual bool	init();
    virtual bool	launch();

    BufferString	parfnm_;
    bool		tostdio_;

public:

    static void		initClass();
    static JobDispatcher* create() { return new SingleJobDispatcher; }

};


} // namespace Batch


#endif

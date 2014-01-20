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

/*!\brief the data we need to specify an OD batch job. */

mExpClass(General) JobSpec
{
public:

    enum ProcType	{ NonODBase, Attrib, AttribEM, Grid2D, PreStack, SEGY,
			  T2D, VelConv, Vol };

			JobSpec(ProcType);
			JobSpec( const char* pnm=0 )
			    : prognm_(pnm), execpars_(true)		{}

    BufferString	prognm_;
    BufferString	clargs_;
    IOPar		pars_;
    OS::CommandExecPars	execpars_;	//!< just a hint for some dispatchers

};


/*!\brief Base class (with factory) for methods to kick-off an OD batch job.

  Subclasses are expected to be ranging from simple single-prcess starters to
  elaborate cluster-based job splitting monsters.

  isSuitedFor() determines whether a certain type of dispatcher can handle any
  job for this program. canHandle() decides on the whole JobSpec.

 */

mExpClass(General) JobDispatcher
{
public:

			JobDispatcher()			{}

    virtual		~JobDispatcher()		{}

    virtual const char*	description() const		= 0;
    virtual bool	isSuitedFor(const char* prognm) const = 0;
    virtual bool	canHandle(const JobSpec&) const;

    bool		go(const JobSpec&);
    const char*		errMsg() const			{ return errmsg_; }

    mDefineFactoryInClass(JobDispatcher,factory);

protected:

    virtual bool	init()				{ return true; }
    virtual bool	launch()			= 0;

    JobSpec		jobspec_;
    BufferString	errmsg_;

};


/*!\brief kicks off OD batch jobs in a single process. */

mExpClass(General) SingleJobDispatcher : public JobDispatcher
{
public:

			SingleJobDispatcher();
    virtual		~SingleJobDispatcher()		{}

    virtual const char*	description() const;
    virtual bool	isSuitedFor(const char*) const	{ return true; }

    void		setParFileName( const char* s )	{ parfnm_ = s; }
    void		setRemoteHost( const char* rh )	{ remotehost_ = rh; }
    void		setRemoteExec( const char* re )	{ remoteexec_ = re; }

    mDefaultFactoryInstantiation(JobDispatcher,SingleJobDispatcher,
				 "Single Process","Single Process");

protected:

    virtual bool	init();
    virtual bool	launch();

    BufferString	parfnm_;
    BufferString	remotehost_;
    BufferString	remoteexec_;

};


} // namespace Batch


#endif

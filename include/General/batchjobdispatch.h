#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Dec 2013
 RCS:           $Id$
________________________________________________________________________

-*/

#include "enums.h"
#include "generalmod.h"
#include "factory.h"
#include "oscommand.h"
#include "iopar.h"
#include "uistring.h"


namespace Batch
{

/*!\brief the data we need to specify an OD batch job. */

mExpClass(General) JobSpec
{ mODTextTranslationClass(JobSpec);
public:

    enum ProcType	{ NonODBase, Attrib, AttribEM, Grid2D,
			  PreStack, SEGY, T2D, TwoDto3D, VelConv, Vol };

			JobSpec(ProcType);
			JobSpec(const char* pnm=0);
			JobSpec(const IOPar&);

    static const char*	progNameFor(ProcType);
    static ProcType	procTypeFor(const char*);

    BufferString	prognm_;
    BufferStringSet	clargs_;
    IOPar		pars_;
    OS::LaunchType	launchtype_ = OS::Batch;
    BufferString	servernm_;
    OS::CommandExecPars	execpars_;	//!< just a hint for some dispatchers

    void		usePar(const IOPar&);
    void		fillPar(IOPar&) const;

};


/*!\brief Base class (with factory) for methods to kick-off an OD batch job.

  Subclasses are expected to be ranging from simple single-process starters to
  elaborate cluster-based job splitting monsters.

  isSuitedFor() determines whether a certain type of dispatcher can handle any
  job for this program. canHandle() decides on the whole JobSpec. If the
  job creates intermediate results, then it may be able to resume after
  the user has stopped. In that case, canResume() can return true.

  Every job will sooner or later be written to a par file. We want to have user
  select job names, not par file names. Thus, there are both job names and
  the actual parameter file full path.
  Par files are written to, and expected to be in the 'Proc' directory. If that
  is the case, than job name <-> par file goes both ways.

 */

mExpClass(General) JobDispatcher
{ mODTextTranslationClass(JobDispatcher);
public:

			JobDispatcher()			{}

    virtual		~JobDispatcher()		{}

    virtual uiString	description() const		= 0;
    virtual bool	isSuitedFor(const char* prognm) const = 0;
    virtual bool	canHandle(const JobSpec&) const;
    virtual bool	canResume(const JobSpec&) const { return false; }

    bool		go(const JobSpec&);
    uiString		errMsg() const			{ return errmsg_; }

    mDefineFactoryInClass(JobDispatcher,factory);

    static void		getDefParFilename(const char* prognm,BufferString&);
    void		setToDefParFileName()
				{ getDefParFilename(jobspec_.prognm_,parfnm_); }

    static void		getJobNames(BufferStringSet&);
    static BufferString	getJobName(const char*);
    void		setJobName(const char*);
    BufferString	jobName() const	{ return getJobName(parfnm_.buf()); }

    static void		setUserWantsResume(IOPar&,bool);
    static bool		userWantsResume(const IOPar&);

    JobSpec		jobspec_;
    BufferString	parfnm_;

protected:

    virtual bool	init()				{ return true; }
    virtual bool	launch()			= 0;

    mutable uiString    errmsg_;

    bool		writeParFile() const;

};


/*!\brief kicks off OD batch jobs in a single process. */

mExpClass(General) SingleJobDispatcher : public JobDispatcher
{ mODTextTranslationClass(SingleJobDispatcher);
public:

			SingleJobDispatcher();
    virtual		~SingleJobDispatcher()		{}

    virtual uiString	description() const;
    virtual bool	isSuitedFor(const char*) const	{ return true; }

    mDefaultFactoryInstantiation(JobDispatcher,SingleJobDispatcher,
				 "Single Process",tr("Single Process"));

    BufferString	remotehost_;
    BufferString	remoteexec_;

protected:

    virtual bool	init();
    virtual bool	launch();

};


} // namespace Batch



#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		Aug 2016
________________________________________________________________________

-*/

#include "uimmptestmod.h"
#include "uimmbatchjobdispatch.h"

#include "jobdescprov.h"
#include "mmbatchjobdispatch.h"
#include "sets.h"
#include "uistring.h"

namespace Batch
{

mExpClass(uiMMPTest) TestMMProgDef : public MMProgDef
{
public:
			TestMMProgDef() : MMProgDef("od_MMPTestLaunch") {}
    virtual bool	isSuitedFor(const char*) const;
    virtual bool	canHandle(const JobSpec&) const;
    virtual bool	canResume(const JobSpec&) const;
};

} // namespace Batch


/*!\brief JobDesc for MMPTest. */
mExpClass(uiMMPTest) MMTestJobDescProv : public JobDescProv
{
public:
			MMTestJobDescProv(const IOPar&,int nrmachs);
			~MMTestJobDescProv();

    virtual int		nrJobs() const;
    virtual void	getJob(int,IOPar&) const;
    virtual const char* objType() const;
    virtual const char* objName(int) const;
    virtual void	dump(od_ostream&) const;

protected:

    int			nrmachs_;

};


mExpClass(uiMMPTest) uiMMPTestProc : public uiMMBatchJobDispatcher
{ mODTextTranslationClass(uiMMPtest);
public:

			uiMMPTestProc(uiParent*,const IOPar&);
			~uiMMPTestProc();

protected:

    virtual bool	initWork(bool);
    virtual bool	needConfirmEarlyStop() const	{ return false; }
    virtual bool	prepareCurrentJob();
    virtual bool	acceptOK();
    virtual bool	rejectOK();
};

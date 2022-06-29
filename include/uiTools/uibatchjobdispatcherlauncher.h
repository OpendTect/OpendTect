#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2014
________________________________________________________________________

-*/

#include "uitoolsmod.h"

#include "batchjobdispatch.h"
#include "factory.h"
#include "hostdata.h"
#include "uistring.h"

class uiBatchJobDispatcherSel;
namespace Batch {
    class JobSpec; class JobDispatcher; class SingleJobDispatcher; }
class uiParent;

/*!\brief launches the UI of a Batch::JobDispatcher */

mExpClass(uiTools) uiBatchJobDispatcherLauncher
{ mODTextTranslationClass(uiBatchJobDispatcherLauncher);
public:

    virtual			~uiBatchJobDispatcherLauncher() {}

    virtual bool		isSuitedFor(const char* prognm) const;
    virtual bool		canHandleJobSpec() const;
    virtual bool		hasOptions() const		{ return false;}
    virtual void		editOptions(uiParent*)		{}
    virtual uiString		getInfo() const;
    virtual bool		go(uiParent*,Batch::ID* =nullptr);

    Batch::JobSpec&		jobSpec()		{ return jobspec_; }
    const Batch::JobSpec&	jobSpec() const { return jobspec_; }
    Batch::JobDispatcher&	dispatcher()		{ return gtDsptchr(); }
    const Batch::JobDispatcher& dispatcher() const;

    uiString		name() const { return factoryDisplayName(); }

    mDefineFactory1ParamInClass(uiBatchJobDispatcherLauncher,
			Batch::JobSpec&,factory);

protected:
			    uiBatchJobDispatcherLauncher(Batch::JobSpec& js)
				: jobspec_(js)
			    {}

    Batch::JobSpec&	jobspec_;
    virtual Batch::JobDispatcher& gtDsptchr()	= 0;

};


/*!\brief launcher for a single-process job dispatcher */

mExpClass(uiTools) uiSingleBatchJobDispatcherLauncher
			: public uiBatchJobDispatcherLauncher
{ mODTextTranslationClass(uiSingleBatchJobDispatcherLauncher);
public:

			uiSingleBatchJobDispatcherLauncher(Batch::JobSpec&);
			~uiSingleBatchJobDispatcherLauncher();

    bool		hasOptions() const override	{ return true; }

    void		editOptions(uiParent*) override;
    bool		go(uiParent*,Batch::ID* =nullptr) override;

    mDefaultFactoryInstantiation1Param(uiBatchJobDispatcherLauncher,
			    uiSingleBatchJobDispatcherLauncher,
			    Batch::JobSpec&,"Single Process",
			    tr("Single Process"));

protected:

    virtual Batch::JobDispatcher&	gtDsptchr() override;
    Batch::SingleJobDispatcher&		sjd_;
    HostDataList			hdl_;

};

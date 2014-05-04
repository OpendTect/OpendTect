#ifndef uibatchjobdispatcherlauncher_h
#define uibatchjobdispatcherlauncher_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2014
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "factory.h"

class uiBatchJobDispatcherSel;
namespace Batch {
    class JobSpec; class JobDispatcher; class SingleJobDispatcher; }
class uiParent;

/*!\brief launches the UI of a Batch::JobDispatcher */

mExpClass(uiTools) uiBatchJobDispatcherLauncher
{
public:

				uiBatchJobDispatcherLauncher(Batch::JobSpec& js)
				    : jobspec_(js)		{}
    virtual			~uiBatchJobDispatcherLauncher() {}

    virtual bool		isSuitedFor(const char* prognm) const;
    virtual bool		canHandleJobSpec() const;
    virtual bool		hasOptions() const		{ return false;}
    virtual void		editOptions(uiParent*)		{}
    virtual uiString		getInfo() const;
    virtual bool		go(uiParent*);

    Batch::JobSpec&		jobSpec()		{ return jobspec_; }
    const Batch::JobSpec&	jobSpec() const 	{ return jobspec_; }
    Batch::JobDispatcher&	dispatcher()		{ return gtDsptchr(); }
    const Batch::JobDispatcher& dispatcher() const;

    const char*		name() const { return factoryDisplayName(); }

    mDefineFactory1ParamInClass(uiBatchJobDispatcherLauncher,
			Batch::JobSpec&,factory);

protected:

    Batch::JobSpec&	jobspec_;

    virtual Batch::JobDispatcher& gtDsptchr()	= 0;

};


/*!\brief launcher for a single-process job dispatcher */

mExpClass(uiTools) uiSingleBatchJobDispatcherLauncher
			: public uiBatchJobDispatcherLauncher
{
public:

			uiSingleBatchJobDispatcherLauncher(Batch::JobSpec&);
			~uiSingleBatchJobDispatcherLauncher();

    virtual bool	hasOptions() const			{ return true; }
    virtual void	editOptions(uiParent*);

    mDefaultFactoryInstantiation1Param(uiBatchJobDispatcherLauncher,
			    uiSingleBatchJobDispatcherLauncher,
			    Batch::JobSpec&,"Single Process","Single Process");

protected:

    virtual Batch::JobDispatcher&	gtDsptchr();
    Batch::SingleJobDispatcher&		sjd_;

};


#endif

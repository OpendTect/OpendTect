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
namespace Batch { class JobSpec; class SingleJobDispatcher; }


/*!\brief launches the UI of a Batch::JobDispatcher */

mExpClass(uiTools) uiBatchJobDispatcherLauncher
{
public:

			uiBatchJobDispatcherLauncher( Batch::JobSpec& js )
			    : jobspec_(js)			{}

    virtual bool	isSuitedFor(const char* prognm) const	{ return false;}
    virtual bool	canHandleJobSpec() const;
    virtual bool	hasOptions() const			{ return false;}
    virtual void	editOptions(uiParent*)			{}
    virtual const char*	getInfo() const				= 0;
    virtual bool	go(uiParent*)				= 0;

    const char*		name() const { return factoryDisplayName(); }

    mDefineFactory1ParamInClass(uiBatchJobDispatcherLauncher,
			Batch::JobSpec&,factory);

    Batch::JobSpec&	jobspec_;

};


/*!\brief launcher for a single-process job dispatcher */

mExpClass(uiTools) uiSingleBatchJobDispatcherLauncher
			: public uiBatchJobDispatcherLauncher
{
public:

			uiSingleBatchJobDispatcherLauncher(Batch::JobSpec&);
			~uiSingleBatchJobDispatcherLauncher();

    virtual bool	isSuitedFor(const char*) const		{ return true; }
    virtual bool	hasOptions() const			{ return true; }
    virtual const char*	getInfo() const;
    virtual void	editOptions(uiParent*);
    virtual bool	go(uiParent*);

    mDefaultFactoryInstantiation1Param(uiBatchJobDispatcherLauncher,
			    uiSingleBatchJobDispatcherLauncher,
			    Batch::JobSpec&,"Single Process","Single Process");

    Batch::SingleJobDispatcher&	sjd_;

};


#endif

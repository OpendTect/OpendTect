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
namespace Batch { class JobSpec; }


/*!\brief launches the UI of a Batch::JobDispatcher */

mExpClass(uiTools) uiBatchJobDispatcherLauncher
{
public:

    virtual bool	isSuitedFor(const char* prognm) const	{ return false;}
    virtual bool	canHandle(const Batch::JobSpec&) const;
    virtual bool	hasOptions() const			{ return false;}
    virtual void	editOptions(uiBatchJobDispatcherSel*)	{}
    virtual const char*	getInfo() const				= 0;
    virtual bool	go(uiParent*,const Batch::JobSpec&)	= 0;

    const char*		name() const { return factoryDisplayName(); }

    mDefineFactoryInClass(uiBatchJobDispatcherLauncher,factory);

};


/*!\brief launcher for a single-process job dispatcher */

mExpClass(uiTools) uiSingleBatchJobDispatcherLauncher
			: public uiBatchJobDispatcherLauncher
{
public:

    virtual bool	isSuitedFor(const char*) const		{ return true; }
    virtual bool	hasOptions() const			{ return true; }
    virtual const char*	getInfo() const;
    virtual void	editOptions(uiBatchJobDispatcherSel*);
    virtual bool	go(uiParent*,const Batch::JobSpec&);

    mDefaultFactoryInstantiation(uiBatchJobDispatcherLauncher,
	    			 uiSingleBatchJobDispatcherLauncher,
				 "Single Process","Single Process");

};


#endif

#ifndef uibatchjobdispatcher_h
#define uibatchjobdispatcher_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2014
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uidialog.h"
#include "uigroup.h"
#include "factory.h"
class uiGenInput;
namespace Batch { class JobSpec; }


/*!\brief launches the UI of a Batch::JobDispatcher */

mExpClass(uiTools) uiBatchJobDispatcherLauncher : public NamedObject
{
public:

			uiBatchJobDispatcherLauncher( const char* nm )
			    : NamedObject(nm)			{}

    virtual bool	canHandle( const Batch::JobSpec& )	{ return true; }
    virtual const char*	getInfo() const				= 0;
    virtual bool	go(uiParent*,const Batch::JobSpec&)	= 0;

    mDefineFactoryInClass(uiBatchJobDispatcherLauncher,factory);

};


/*!\brief lets user select a batch job disptacher suited for the job */

mExpClass(uiTools) uiBatchJobDispatcherSel : public uiGroup
{
public:

			uiBatchJobDispatcherSel(uiParent*,bool isoptional=false);
			~uiBatchJobDispatcherSel();

    void		setJobSpec(const Batch::JobSpec&);
			//!< must be done at every change
			//!< the spec will determine what user can select

    bool		wantBatch() const;	//! can be false if isoptional
    bool		start();

protected:

    uiGenInput*		selfld_;

    Batch::JobSpec&	jobspec_;
    ObjectSet<uiBatchJobDispatcherLauncher> uidispatchers_;

};


/*!\brief launcher for a single-process job dispatcher */

mExpClass(uiTools) uiSingleBatchJobDispatcherLauncher
			: public uiBatchJobDispatcherLauncher
{
public:

			uiSingleBatchJobDispatcherLauncher()
			    : uiBatchJobDispatcherLauncher("Single process") {}

    virtual const char*	getInfo() const;
    virtual bool	go(uiParent*,const Batch::JobSpec&);

};


#endif

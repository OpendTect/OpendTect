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
#include "uigroup.h"
#include "factory.h"
class uiGenInput;
class uiButton;
class OSCommandExecPars;
namespace Batch { class JobSpec; }


/*!\brief launches the UI of a Batch::JobDispatcher */

mExpClass(uiTools) uiBatchJobDispatcherLauncher : public NamedObject
{
public:

			uiBatchJobDispatcherLauncher( const char* nm )
			    : NamedObject(nm)			{}

    virtual bool	canHandle( const Batch::JobSpec& )	{ return true; }
    virtual bool	hasOptions() const			{ return false;}
    virtual void	editOptions(uiParent*)			{}
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

    const char*		selected() const;
    const char*		selectedInfo() const;

    bool		wantBatch() const;	//! can be false if isoptional
    bool		start();

    Notifier<uiBatchJobDispatcherSel> selectionChange;

protected:

    uiGenInput*		selfld_;
    uiButton*		optsbut_;

    Batch::JobSpec&	jobspec_;
    ObjectSet<uiBatchJobDispatcherLauncher> uidispatchers_;

    int			selIdx() const;

    void		selChg(CallBacker*);
    void		fldChck(CallBacker*);
    void		optsPush(CallBacker*);

};


/*!\brief launcher for a single-process job dispatcher */

mExpClass(uiTools) uiSingleBatchJobDispatcherLauncher
			: public uiBatchJobDispatcherLauncher
{
public:

			uiSingleBatchJobDispatcherLauncher();
			~uiSingleBatchJobDispatcherLauncher();

    virtual const char*	getInfo() const;
    virtual bool	hasOptions() const		{ return true; }
    virtual void	editOptions(uiParent*);
    virtual bool	go(uiParent*,const Batch::JobSpec&);

    static void		initClass();
    static uiBatchJobDispatcherLauncher* create()
			{ return new uiSingleBatchJobDispatcherLauncher; }

protected:

    OSCommandExecPars&	execpars_;


};


#endif

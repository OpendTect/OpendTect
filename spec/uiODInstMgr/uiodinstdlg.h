#ifndef uiodinstdlg_h
#define uiodinstdlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Nov 2011
 RCS:           $Id: uiodinstdlg.h 8009 2013-06-20 06:11:26Z kristofer.tingdahl@dgbes.com $
________________________________________________________________________

-*/

#include "uiodinstmgrmod.h"
#include "uidialog.h"
#include "odinstdlhandler.h"

class uiODInstDlg;


mDefClass(uiODInstMgr) uiODInstDLFailHndlr : public ODInst::DLHandler::FailHndlr
{
public:

    			uiODInstDLFailHndlr(uiODInstDlg*);

    virtual bool	handle(ODDLSite&,BufferString&,float&);

    uiODInstDlg*	par_;

};


/* Base class providing Internet access

Note that the DLHandler is passed but it is not managed by this class.
A pointer is passed because the uiODInstMgr must be able to be constructed
with no DLHandler yet alive.

   */

mDefClass(uiODInstMgr) uiODInstDlg : public uiDialog
{
public:

    			uiODInstDlg(uiParent*,const uiDialog::Setup&,
				    ODInst::DLHandler*);
			~uiODInstDlg();

    ODInst::DLHandler&	dlHandler()		{ return *dlhndlr_; }
    uiODInstDLFailHndlr& dlFailHandler()	{ return dlfailhndlr_; }

    void		reportError(const char* msg,const char* detail=0,
	    			    bool sendtosupport=false) const;

protected:

    ODInst::DLHandler*	dlhndlr_;
    uiODInstDLFailHndlr	dlfailhndlr_;

private:

    void		addStuff(CallBacker*);
    virtual void	addPreFinaliseStuff();

};

#define mEnableProceed(yn) button(OK)->setSensitive( yn )

#endif


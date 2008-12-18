#ifndef uipsviewerpreproctab_h
#define uipsviewerpreproctab_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Yuancheng Liu
 Date:		May 2008
 RCS:		$Id: uipsviewerpreproctab.h,v 1.4 2008-12-18 15:21:06 cvsyuancheng Exp $
________________________________________________________________________


-*/

#include "uidlggroup.h"

class uiGenInput;
class uiPushButton;

namespace PreStack { class ProcessManager; class uiProcessorManager; }

namespace PreStackView
{

class PreStackViewer;
class uiPSViewerMgr;

class uiPSViewerPreProcTab :  public uiDlgGroup
{
public:
				uiPSViewerPreProcTab(uiParent*,PreStackViewer&,
					uiPSViewerMgr&,
					PreStack::ProcessManager&);
				~uiPSViewerPreProcTab();
    bool			acceptOK();

    void			applyToAll(bool yn)	{ applyall_ = yn; }
    bool			applyToAll()		{ return applyall_; }

protected:
    
    void				applyButPushedCB(CallBacker*);
    void				processorChangeCB(CallBacker*);

    PreStackViewer&			vwr_;
    uiPSViewerMgr&			mgr_;
    PreStack::ProcessManager*		preprocmgr_;
    PreStack::uiProcessorManager*	uipreprocmgr_;

    uiPushButton*			applybut_;    
    bool				applyall_;
};


}; //namespace

#endif

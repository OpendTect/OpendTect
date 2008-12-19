#ifndef uipsviewerpreproctab_h
#define uipsviewerpreproctab_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Yuancheng Liu
 Date:		May 2008
 RCS:		$Id: uipsviewerpreproctab.h,v 1.5 2008-12-19 21:58:00 cvsyuancheng Exp $
________________________________________________________________________


-*/

#include "uidlggroup.h"

class uiGenInput;
class uiPushButton;

namespace PreStack { class ProcessManager; class uiProcessorManager; }

namespace PreStackView
{

class Viewer;
class uiViewerMgr;

class uiViewerPreProcTab :  public uiDlgGroup
{
public:
				uiViewerPreProcTab(uiParent*,
					PreStackView::Viewer&,
					uiViewerMgr&,
					PreStack::ProcessManager&);
				~uiViewerPreProcTab();
    bool			acceptOK();

    void			applyToAll(bool yn)	{ applyall_ = yn; }
    bool			applyToAll()		{ return applyall_; }

protected:
    
    void				applyButPushedCB(CallBacker*);
    void				processorChangeCB(CallBacker*);

    PreStackView::Viewer&		vwr_;
    uiViewerMgr&			mgr_;
    PreStack::ProcessManager*		preprocmgr_;
    PreStack::uiProcessorManager*	uipreprocmgr_;

    uiPushButton*			applybut_;    
    bool				applyall_;
};


}; //namespace

#endif

#ifndef uipsviewerpreproctab_h
#define uipsviewerpreproctab_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Yuancheng Liu
 Date:		May 2008
 RCS:		$Id: uipsviewerpreproctab.h,v 1.1 2008-05-15 18:48:39 cvsyuancheng Exp $
________________________________________________________________________


-*/

#include "uidlggroup.h"

class uiGenInput;

namespace PreStack { class ProcessManager; class uiProcessorManager; }

namespace PreStackView
{

class PreStackViewer;
class uiPSViewerMgr;

class uiPSViewerPreProcTab :  public uiDlgGroup
{
public:
				uiPSViewerPreProcTab(uiParent*,PreStackViewer&,
						     uiPSViewerMgr&);
				~uiPSViewerPreProcTab();
    bool			acceptOK();

    void			applyToAll(bool yn)	{ applyall_ = yn; }
    bool			applyToAll()		{ return applyall_; }

protected:

    PreStackViewer&			vwr_;
    uiPSViewerMgr&			mgr_;
    PreStack::ProcessManager*		preprocmgr_;
    PreStack::uiProcessorManager*	uipreprocmgr_;
    
    bool				applyall_;
};


}; //namespace

#endif

#ifndef uipsviewersettingdlg_h
#define uipsviewersettingdlg_h

/*+
________________________________________________________________________
 
 CopyRight:     (C) dGB Beheer B.V.
 Author:        Yuancheng Liu
 Date:          August 2007
 RCS:           $Id: uipsviewersettingdlg.h,v 1.2 2008-05-27 22:53:41 cvsyuancheng Exp $
________________________________________________________________________

-*/

#include "uidlggroup.h"

class uiCheckBox;
namespace PreStack { class ProcessManager; }

namespace PreStackView 
{ 
    class PreStackViewer; 
    class uiPSViewerMgr; 
    class uiPSViewerColTab;
    class uiPSViewerShapeTab;
    class uiPSViewerPreProcTab;


class uiPSViewerSettingDlg : public uiTabStackDlg
{
public:
				uiPSViewerSettingDlg(uiParent*,PreStackViewer&,
						     uiPSViewerMgr&,
						     PreStack::ProcessManager&);
protected:

    bool			acceptOK(CallBacker*);

    uiPSViewerShapeTab*		shapetab_;
    uiPSViewerColTab*		coltab_;
    uiPSViewerPreProcTab* 	preproctab_;
    uiCheckBox*			applytoallfld_;
};


}; //namespace 

#endif


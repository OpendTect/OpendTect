#ifndef uipsviewersettingdlg_h
#define uipsviewersettingdlg_h

/*+
________________________________________________________________________
 
 CopyRight:     (C) dGB Beheer B.V.
 Author:        Yuancheng Liu
 Date:          August 2007
 RCS:           $Id: uipsviewersettingdlg.h,v 1.3 2008-12-19 21:58:00 cvsyuancheng Exp $
________________________________________________________________________

-*/

#include "uidlggroup.h"

class uiCheckBox;
namespace PreStack { class ProcessManager; }

namespace PreStackView 
{ 
    class Viewer; 
    class uiViewerMgr; 
    class uiViewerColTab;
    class uiViewerShapeTab;
    class uiViewerPreProcTab;


class uiViewerSettingDlg : public uiTabStackDlg
{
public:
				uiViewerSettingDlg(uiParent*,
						   PreStackView::Viewer&,
						   uiViewerMgr&,
						   PreStack::ProcessManager&);
protected:

    bool			acceptOK(CallBacker*);

    uiViewerShapeTab*		shapetab_;
    uiViewerColTab*		coltab_;
    uiViewerPreProcTab* 	preproctab_;
    uiCheckBox*			applytoallfld_;
};


}; //namespace 

#endif


#ifndef uipsviewersettingdlg_h
#define uipsviewersettingdlg_h

/*+
________________________________________________________________________
 
 CopyRight:     (C) dGB Beheer B.V.
 Author:        Yuancheng Liu
 Date:          August 2007
 RCS:           $Id: uipsviewersettingdlg.h,v 1.1 2008-05-15 18:51:35 cvsyuancheng Exp $
________________________________________________________________________

-*/

#include "uidlggroup.h"

class uiCheckBox;

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
						     uiPSViewerMgr&);
protected:

    bool			acceptOK(CallBacker*);
   
    uiPSViewerShapeTab*		shapetab_;
    uiPSViewerColTab*		coltab_;
    uiPSViewerPreProcTab* 	preproctab_;
    uiCheckBox*			applytoallfld_;
};


}; //namespace 

#endif


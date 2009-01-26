#ifndef uipsviewersettingdlg_h
#define uipsviewersettingdlg_h

/*+
________________________________________________________________________
 
 CopyRight:     (C) dGB Beheer B.V.
 Author:        Yuancheng Liu
 Date:          August 2007
 RCS:           $Id: uipsviewersettingdlg.h,v 1.6 2009-01-26 15:09:09 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidlggroup.h"

class uiCheckBox;
namespace PreStack { class ProcessManager; }

namespace PreStackView 
{ 
    class Viewer3D; 
    class uiViewer3DMgr;
    class uiViewer3DScalingTab;
    class uiViewer3DAppearanceTab; 
    class uiViewer3DShapeTab;
    class uiViewer3DPreProcTab;


class uiViewer3DSettingDlg : public uiTabStackDlg
{
public:
				uiViewer3DSettingDlg(uiParent*,
						   PreStackView::Viewer3D&,
						   uiViewer3DMgr&,
						   PreStack::ProcessManager&);
protected:

    bool			acceptOK(CallBacker*);

    uiViewer3DShapeTab*		shapetab_;
    uiViewer3DScalingTab*	scaletab_;
    uiViewer3DAppearanceTab*	apptab_;
    uiViewer3DPreProcTab* 	preproctab_;
    uiCheckBox*			applytoallfld_;
};


}; //namespace 

#endif


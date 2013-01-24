#ifndef uipsviewersettingdlg_h
#define uipsviewersettingdlg_h

/*+
________________________________________________________________________
 
 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Yuancheng Liu
 Date:          August 2007
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uidlggroup.h"
#include "multiid.h"

class uiCheckBox;
namespace PreStack { class ProcessManager; }
namespace visSurvey { class PreStackDisplay; }

namespace PreStackView 
{ 
    class uiViewer3DMgr;
    class uiViewer3DScalingTab;
    class uiViewer3DAppearanceTab; 
    class uiViewer3DShapeTab;
    class uiViewer3DPreProcTab;
    class uiViewer3DEventsTab;

mClass(uiPreStackViewer) uiViewer3DSettingDlg : public uiTabStackDlg
{
public:
				uiViewer3DSettingDlg(uiParent*,
						   visSurvey::PreStackDisplay&,
						   uiViewer3DMgr&,
						   PreStack::ProcessManager&);
protected:

    bool			acceptOK(CallBacker*);

    uiViewer3DShapeTab*		shapetab_;
    uiViewer3DScalingTab*	scaletab_;
    uiViewer3DAppearanceTab*	apptab_;
    uiViewer3DPreProcTab* 	preproctab_;
    uiViewer3DEventsTab*	eventstab_;
    uiCheckBox*			applytoallfld_;
};


}; //namespace 

#endif


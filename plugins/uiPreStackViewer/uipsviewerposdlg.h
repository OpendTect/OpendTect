#ifndef uipsviewerposdlg_h
#define uipsviewerposdlg_h

/*+
________________________________________________________________________
 
 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Yuancheng Liu
 Date:          August 2008
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uidialog.h"

class uiLabel;
class uiSpinBox;
class uiCheckBox;
class uiPushButton;
class uiLabeledSpinBox;

namespace visSurvey { class PreStackDisplay; }

namespace PreStackView 
{ 

mClass(uiPreStackViewer) uiViewer3DPositionDlg : public uiDialog
{
public:			

    			uiViewer3DPositionDlg(uiParent*,
					      visSurvey::PreStackDisplay&);
			~uiViewer3DPositionDlg();

    bool		is3D() const;
    bool		isInl() const;

protected:

    uiSpinBox*		posfld_;
    uiLabeledSpinBox*	stepfld_;
    uiCheckBox*		oobox_;
    uiCheckBox*		applybox_;
    uiPushButton*	applybut_;
    uiLabel*		steplbl_;
    BufferString	ootxt_;

    void		ooBoxSel(CallBacker*);
    void		applBoxSel(CallBacker*);
    void		posChg(CallBacker*);
    void		renewFld(CallBacker*);

    void		stepCB(CallBacker*);
    void		atStart(CallBacker*);
    bool		applyCB(CallBacker*);

    bool		rejectOK(CallBacker*);

    visSurvey::PreStackDisplay& viewer_;
};

} // namespace 

#endif

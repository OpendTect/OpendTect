#ifndef uipsviewerposdlg_h
#define uipsviewerposdlg_h

/*+
________________________________________________________________________
 
 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Yuancheng Liu
 Date:          August 2008
 RCS:           $Id: uipsviewerposdlg.h,v 1.12 2009/07/22 16:01:28 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class uiLabel;
class uiSpinBox;
class uiCheckBox;
class uiPushButton;
class uiLabeledSpinBox;

namespace PreStackView 
{ 
class Viewer3D; 


class uiViewer3DPositionDlg : public uiDialog
{
public:			

    			uiViewer3DPositionDlg(uiParent*,
					      PreStackView::Viewer3D&);
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
    
    PreStackView::Viewer3D& viewer_;
};


}; //namespace 

#endif


#ifndef uipsviewerposdlg_h
#define uipsviewerposdlg_h

/*+
________________________________________________________________________
 
 CopyRight:     (C) dGB Beheer B.V.
 Author:        Yuancheng Liu
 Date:          August 2008
 RCS:           $Id: uipsviewerposdlg.h,v 1.11 2009-02-12 15:01:52 cvsbert Exp $
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


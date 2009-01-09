#ifndef uipsviewerposdlg_h
#define uipsviewerposdlg_h

/*+
________________________________________________________________________
 
 CopyRight:     (C) dGB Beheer B.V.
 Author:        Yuancheng Liu
 Date:          August 2008
 RCS:           $Id: uipsviewerposdlg.h,v 1.8 2009-01-09 11:02:26 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class uiSpinBox;
class uiPushButton;
class uiCheckBox;

namespace PreStackView 
{ 
class Viewer3D; 


class uiViewer3DPositionDlg : public uiDialog
{
public:			

    			uiViewer3DPositionDlg(uiParent*,
					      PreStackView::Viewer3D&);

    bool		is3D() const;
    bool		isInl() const;

protected:

    uiSpinBox*		posfld_;
    uiSpinBox*		stepfld_;
    uiCheckBox*		applybox_;
    uiPushButton*	applybut_;

    void		boxSel(CallBacker*);
    void		posChg(CallBacker*);
    void		renewFld(CallBacker*);

    void		stepCB(CallBacker*);
    void		atStart(CallBacker*);
    bool		applyCB(CallBacker*);
    
    PreStackView::Viewer3D& viewer_;
};


}; //namespace 

#endif


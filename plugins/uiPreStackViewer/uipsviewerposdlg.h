#ifndef uipsviewerposdlg_h
#define uipsviewerposdlg_h

/*+
________________________________________________________________________
 
 CopyRight:     (C) dGB Beheer B.V.
 Author:        Yuancheng Liu
 Date:          August 2008
 RCS:           $Id: uipsviewerposdlg.h,v 1.4 2008-12-19 21:58:00 cvsyuancheng Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class uiSpinBox;
class uiPushButton;
class uiCheckBox;

namespace PreStackView 
{ 
class Viewer; 


class uiViewerPositionDlg : public uiDialog
{
public:			

    			uiViewerPositionDlg(uiParent*,PreStackView::Viewer&);

    bool		is3D() const;
    bool		isInl() const;

protected:

    PreStackView::Viewer& viewer_;

    uiSpinBox*		posfld_;
    uiCheckBox*		applybox_;
    uiPushButton*	applybut_;

    void		boxSel(CallBacker*);
    void		posChg(CallBacker*);
    void		renewFld(CallBacker*);

    void		atStart(CallBacker*);
    bool		acceptOK(CallBacker*);
    bool		applyCB(CallBacker*);
};


}; //namespace 

#endif


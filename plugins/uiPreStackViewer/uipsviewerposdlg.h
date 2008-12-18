#ifndef uipsviewerposdlg_h
#define uipsviewerposdlg_h

/*+
________________________________________________________________________
 
 CopyRight:     (C) dGB Beheer B.V.
 Author:        Yuancheng Liu
 Date:          August 2008
 RCS:           $Id: uipsviewerposdlg.h,v 1.2 2008-12-18 11:04:55 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class uiSpinBox;
class uiPushButton;
class uiCheckBox;

namespace PreStackView 
{ 
class PreStackViewer; 


class uiPSViewerPositionDlg : public uiDialog
{
public:			

    			uiPSViewerPositionDlg(uiParent*,PreStackViewer&);

    bool		is3D() const;
    bool		isInl() const;

protected:

    PreStackViewer&	viewer_;

    uiSpinBox*		posfld_;
    uiCheckBox*		applybox_;
    uiPushButton*	applybut_;

    void		boxSel(CallBacker*);
    void		posChg(CallBacker*);
    void		applyCB(CallBacker*);

    void		atStart(CallBacker*);
    bool		acceptOK(CallBacker*);

};


}; //namespace 

#endif


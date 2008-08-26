#ifndef uipsviewerposdlg_h
#define uipsviewerposdlg_h

/*+
________________________________________________________________________
 
 CopyRight:     (C) dGB Beheer B.V.
 Author:        Yuancheng Liu
 Date:          August 2008
 RCS:           $Id: uipsviewerposdlg.h,v 1.1 2008-08-26 14:24:44 cvsyuancheng Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class uiGenInput;
class uiPushButton;

namespace PreStackView 
{ 
    class PreStackViewer; 

class uiPSViewerPositionDlg : public uiDialog
{
public:			
    			uiPSViewerPositionDlg(uiParent*,PreStackViewer&);

protected:

    bool		acceptOK(CallBacker*);
    void		applyCB(CallBacker*);

    uiGenInput*		posfld_;
    PreStackViewer&	viewer_;
    uiPushButton*	applybut_;
};


}; //namespace 

#endif


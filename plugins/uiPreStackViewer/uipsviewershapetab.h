#ifndef uipsviewershapetab_h
#define uipsviewershapetab_h

/*+
________________________________________________________________________
 
 CopyRight:     (C) dGB Beheer B.V.
 Author:        Yuancheng Liu
 Date:          August 2007
 RCS:           $Id: uipsviewershapetab.h,v 1.3 2008-12-22 15:45:35 cvsyuancheng Exp $
________________________________________________________________________

-*/


#include "uidlggroup.h"

class uiGenInput;
class uiPushButton;
class uiSlider;

namespace PreStackView
{

class Viewer3D;
class uiViewer3DMgr;

class uiViewer3DShapeTab : public uiDlgGroup
{
public:
			uiViewer3DShapeTab(uiParent*,PreStackView::Viewer3D&, 
					   uiViewer3DMgr&);
			~uiViewer3DShapeTab();
    bool		acceptOK();
    bool		rejectOK(CallBacker*);
    
    void		applyToAll(bool yn)	{ applyall_ = yn; }
    bool		applyToAll()		{ return applyall_; }

    void		saveAsDefault(bool yn)	{ savedefault_ = yn; }
    bool		saveAsDefault()		{ return savedefault_; }

protected:

    void		widthTypeChangeCB(CallBacker*);
    void		factorMoveCB(CallBacker*);
    void		widthMoveCB(CallBacker*);
    void		switchPushCB(CallBacker*);
    
    uiGenInput*		autowidthfld_;
    uiSlider*		factorslider_;
    uiSlider*		widthslider_;
    uiPushButton*	switchsidebutton_;
    
    PreStackView::Viewer3D& viewer_;
    uiViewer3DMgr&	mgr_;
    
    float		initialfactor_;
    float		initialwidth_;
    bool		initialautowidth_;
    bool		initialside_;
    bool		applyall_;
    bool		savedefault_;
};


}; //namespace

#endif


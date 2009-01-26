#ifndef uipsviewerappearancetab_h
#define uipsviewerappearancetab_h

/*+
________________________________________________________________________
 
 CopyRight:     (C) dGB Beheer B.V.
 Author:        Yuancheng Liu
 Date:          May 2008
 RCS:           $Id: uipsviewerappearancetab.h,v 1.1 2009-01-26 15:09:08 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidlggroup.h"

class uiColorTable;
class uiLabel;
class uiPushButton;
class uiGenInput;
namespace visBase { class FlatViewer; };

namespace PreStackView
{

class uiViewer3DMgr; 
class Viewer3D;

class uiViewer3DAppearanceTab : public uiDlgGroup 
{
public:
				uiViewer3DAppearanceTab(uiParent*,
						 PreStackView::Viewer3D&,
  						 uiViewer3DMgr&);
    bool			acceptOK();
    void			applyToAll(bool yn)	{ applyall_ = yn; }
    bool			applyToAll()		{ return applyall_; }
    void			saveAsDefault(bool yn)  { savedefault_ = yn; }
    bool			saveAsDefault()         { return savedefault_; }

protected:

    void			applyButPushedCB(CallBacker*);
    
    uiColorTable*       	uicoltab_;
    uiLabel*            	uicoltablbl_; 
    uiGenInput*            	zannotfld_; 
    uiGenInput*            	offsannotfld_; 
    uiPushButton*		applybut_;
    uiViewer3DMgr&		mgr_;
    visBase::FlatViewer*	vwr_;
    bool			applyall_;
    bool			savedefault_;
};


}; //namespace

#endif


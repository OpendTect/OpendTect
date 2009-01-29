#ifndef uipsviewerappearancetab_h
#define uipsviewerappearancetab_h

/*+
________________________________________________________________________
 
 CopyRight:     (C) dGB Beheer B.V.
 Author:        Yuancheng Liu
 Date:          May 2008
 RCS:           $Id: uipsviewerappearancetab.h,v 1.2 2009-01-29 22:16:32 cvsyuancheng Exp $
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
    void			updateZFlds(CallBacker*);
    void			updateOffsFlds(CallBacker*);
    
    uiColorTable*       	uicoltab_;
    uiLabel*            	uicoltablbl_; 
    uiLabel*            	zgridrangelbl_; 
    uiLabel*            	offsgridrangelbl_; 
    uiGenInput*            	zgridfld_; 
    uiGenInput*            	zgridautofld_; 
    uiGenInput*            	zgridrangefld_; 
    uiGenInput*            	offsgridfld_; 
    uiGenInput*            	offsgridautofld_; 
    uiGenInput*            	offsgridrangefld_; 
    uiPushButton*		applybut_;
    uiViewer3DMgr&		mgr_;
    visBase::FlatViewer*	vwr_;
    bool			applyall_;
    bool			savedefault_;
};


}; //namespace

#endif


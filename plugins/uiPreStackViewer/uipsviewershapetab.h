#ifndef uipsviewershapetab_h
#define uipsviewershapetab_h

/*+
________________________________________________________________________
 
 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Yuancheng Liu
 Date:          August 2007
 RCS:           $Id$
________________________________________________________________________

-*/


#include "uidlggroup.h"

class uiGenInput;
class uiPushButton;
class uiSlider;
namespace visSurvey { class PreStackDisplay; }

namespace PreStackView
{
class uiViewer3DMgr;

class uiViewer3DShapeTab : public uiDlgGroup
{
public:
			uiViewer3DShapeTab(uiParent*,
					   visSurvey::PreStackDisplay&,
					   uiViewer3DMgr&);
			~uiViewer3DShapeTab();
    bool		acceptOK();
    bool		rejectOK();
    
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
    
    visSurvey::PreStackDisplay& viewer_;
    uiViewer3DMgr&	mgr_;
    
    float		initialfactor_;
    float		initialwidth_;
    bool		initialautowidth_;
    bool		initialside_;
    bool		applyall_;
    bool		savedefault_;
};

} // namespace

#endif

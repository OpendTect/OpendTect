#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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

mClass(uiPreStackViewer) uiViewer3DShapeTab : public uiDlgGroup
{ mODTextTranslationClass(uiViewer3DShapeTab);
public:
			uiViewer3DShapeTab(uiParent*,
					   visSurvey::PreStackDisplay&,
					   uiViewer3DMgr&);
			~uiViewer3DShapeTab();
    bool		acceptOK() override;
    bool		rejectOK() override;
    
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
    bool		applyall_			= false;
    bool		savedefault_			= false;
};

} // namespace PreStackView

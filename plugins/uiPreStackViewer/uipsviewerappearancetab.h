#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uidlggroup.h"
#include "samplingdata.h"

class uiColorTableGroup;
class uiLabel;
class uiButton;
class uiGenInput;
namespace visBase { class FlatViewer; };
namespace visSurvey { class PreStackDisplay; }

namespace PreStackView
{

class uiViewer3DMgr;

mClass(uiPreStackViewer) uiViewer3DAppearanceTab : public uiDlgGroup
{ mODTextTranslationClass(uiViewer3DAppearanceTab);
public:
				uiViewer3DAppearanceTab(uiParent*,
						 visSurvey::PreStackDisplay&,
						 uiViewer3DMgr&);
				~uiViewer3DAppearanceTab();

    bool			acceptOK();
    void			applyToAll(bool yn)	{ applyall_ = yn; }
    bool			applyToAll()		{ return applyall_; }
    void			saveAsDefault(bool yn)  { savedefault_ = yn; }
    bool			saveAsDefault()         { return savedefault_; }

protected:

    void			applyButPushedCB(CallBacker*);
    void			updateZFlds(CallBacker*);
    void			colTabChanged(CallBacker*);
    void			updateColTab(CallBacker*);
    void			updateOffsFlds(CallBacker*);
    void			updateFlds(uiGenInput* gridfld,
					   uiGenInput* autofld,
					   uiGenInput* rgfld,
					   uiLabel* lblfld,bool x1);

    uiColorTableGroup*	uicoltab_;
    uiLabel*	uicoltablbl_;
    uiLabel*	zgridrangelbl_;
    uiLabel*	offsgridrangelbl_;
    uiGenInput*	zgridfld_;
    uiGenInput*	zgridautofld_;
    uiGenInput*	zgridrangefld_;
    uiGenInput*	offsgridfld_;
    uiGenInput*	offsgridautofld_;
    uiGenInput*	offsgridrangefld_;
    uiButton*			applybut_;
    uiViewer3DMgr&		mgr_;
    visBase::FlatViewer*	vwr_;
    bool			applyall_;
    bool			savedefault_;
    SamplingData<float>		manuzsampl_;
    SamplingData<float>		manuoffssampl_;
};

} // namespace

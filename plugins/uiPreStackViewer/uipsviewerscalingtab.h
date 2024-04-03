#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiflatviewproptabs.h"

class uiButton;
namespace visSurvey { class PreStackDisplay; }

namespace PreStackView
{

class uiViewer3DMgr;

mClass(uiPreStackViewer) uiViewer3DScalingTab : public uiFlatViewDataDispPropTab
{ mODTextTranslationClass(uiViewer3DScalingTab);
public:
				uiViewer3DScalingTab(uiParent*,
						 visSurvey::PreStackDisplay&,
						 uiViewer3DMgr&);
				~uiViewer3DScalingTab();

    void			putToScreen() override;
    void			setData() override	{ doSetData(true); }

    bool			acceptOK() override;
    void			applyToAll(bool yn)	{ applyall_ = yn; }
    bool			applyToAll()		{ return applyall_; }
    void			saveAsDefault(bool yn)  { savedefault_ = yn; }
    bool			saveAsDefault()         { return savedefault_; }

protected:

    void			applyButPushedCB(CallBacker*);
    bool			apply();
    bool			settingCheck();

    BufferString		dataName() const override;
    void			dispSel(CallBacker*);
    void			dispChgCB(CallBacker*);
    void			handleFieldDisplay(bool) override {}
    FlatView::DataDispPars::Common& commonPars() override;

    uiButton*			applybut_;
    uiViewer3DMgr&		mgr_;
    bool			applyall_;
    bool			savedefault_;
};


} // namespace

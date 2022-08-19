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

    virtual void		putToScreen();
    virtual void		setData()		{ doSetData(true); }

    bool			acceptOK();
    void			applyToAll(bool yn)	{ applyall_ = yn; }
    bool			applyToAll()		{ return applyall_; }
    void			saveAsDefault(bool yn)  { savedefault_ = yn; }
    bool			saveAsDefault()         { return savedefault_; }

protected:

    void			applyButPushedCB(CallBacker*);
    bool			apply();
    bool			settingCheck();

    virtual BufferString	dataName() const;
    void	dispSel(CallBacker*);
    void			dispChgCB(CallBacker*);
    virtual void		handleFieldDisplay(bool) {}
    FlatView::DataDispPars::Common& commonPars();

    uiButton*			applybut_;
    uiViewer3DMgr&		mgr_;
    bool			applyall_;
    bool			savedefault_;
};


} // namespace

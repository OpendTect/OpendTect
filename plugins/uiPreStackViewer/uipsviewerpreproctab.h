#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uidlggroup.h"

class uiButton;

namespace PreStack { class uiProcessorManager; }
namespace visSurvey { class PreStackDisplay; }

namespace PreStackView
{

class uiViewer3DMgr;

mClass(uiPreStackViewer) uiViewer3DPreProcTab :  public uiDlgGroup
{ mODTextTranslationClass(uiViewer3DPreProcTab);
public:
				uiViewer3DPreProcTab(uiParent*,
					visSurvey::PreStackDisplay&,
					uiViewer3DMgr&);
				~uiViewer3DPreProcTab();
    bool			acceptOK();

    void			applyToAll(bool yn)	{ applyall_ = yn; }
    bool			applyToAll()		{ return applyall_; }

protected:

    void				applyButPushedCB(CallBacker*);
    bool				apply();
    void				processorChangeCB(CallBacker*);

    visSurvey::PreStackDisplay&		vwr_;
    uiViewer3DMgr&			mgr_;
    PreStack::uiProcessorManager*	uipreprocmgr_;

    uiButton*				applybut_;
    bool				applyall_;
};

} // namespace PreStackView

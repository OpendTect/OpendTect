#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodmainmod.h"

#include "callback.h"
#include "integerid.h"
#include "uistring.h"

class uiMeasureDlg;
class uiODMain;

namespace Pick { class SetMgr; }
namespace visSurvey { class PickSetDisplay; }

mClass(uiODMain) MeasureToolMan : public CallBacker
{ mODTextTranslationClass(MeasureToolMan);
public:
			MeasureToolMan(uiODMain&);
			~MeasureToolMan();

protected:

    void		objSelected(CallBacker*);
    void		buttonClicked(CallBacker*);
    void		addScene(SceneID);
    void		sceneAdded(CallBacker*);
    void		sceneClosed(CallBacker*);
    void		sceneChanged(CallBacker*);
    void		changeCB(CallBacker*);
    void		lineStyleChangeCB(CallBacker*);
    void		clearCB(CallBacker*);
    void		velocityChangeCB(CallBacker*);
    void		dlgClosed(CallBacker*);
    void		manageDlg(bool);
    void		surveyChanged(CallBacker*);
    void		update();
    SceneID		getActiveSceneID() const;

    uiODMain&		appl_;

    Pick::SetMgr&	picksetmgr_;
    TypeSet<SceneID>	sceneids_;
    ObjectSet<visSurvey::PickSetDisplay>	displayobjs_;

    uiMeasureDlg*	measuredlg_;
    int			butidx_;
};

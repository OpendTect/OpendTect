#ifndef measuretoolman_h
#define measuretoolman_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		July 2008
 RCS:		$Id: measuretoolman.h,v 1.9 2011/12/15 22:54:52 cvsnanne Exp $
________________________________________________________________________

-*/

#include "callback.h"

class uiMeasureDlg;
class uiODMain;

namespace Pick { class SetMgr; }
namespace visSurvey { class PickSetDisplay; }

namespace Annotations
{

class MeasureToolMan : public CallBacker
{
public:
			MeasureToolMan(uiODMain&);

protected:

    void		objSelected(CallBacker*);
    void		buttonClicked(CallBacker*);
    void		addScene(int);
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
    int			getActiveSceneID() const;

    uiODMain&		appl_;

    Pick::SetMgr&	picksetmgr_;
    TypeSet<int>	sceneids_;
    ObjectSet<visSurvey::PickSetDisplay>	displayobjs_;

    uiMeasureDlg*	measuredlg_;
    int			butidx_;
};

} // namespace Annotations

#endif

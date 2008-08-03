#ifndef measuretoolman_h
#define measuretoolman_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		July 2008
 RCS:		$Id: measuretoolman.h,v 1.2 2008-08-03 18:08:30 cvsnanne Exp $
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

    void		buttonClicked(CallBacker*);
    void		sceneAdded(CallBacker*);
    void		sceneClosed(CallBacker*);
    void		changeCB(CallBacker*);
    void		lineStyleChangeCB(CallBacker*);
    void		clearCB(CallBacker*);

    uiODMain&		appl_;

    Pick::SetMgr&	picksetmgr_;
    TypeSet<int>	sceneids_;
    ObjectSet<visSurvey::PickSetDisplay>	displayobjs_;

    uiMeasureDlg*	measuredlg_;
};

} // namespace Annotations

#endif

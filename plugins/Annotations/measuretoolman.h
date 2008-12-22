#ifndef measuretoolman_h
#define measuretoolman_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		July 2008
 RCS:		$Id: measuretoolman.h,v 1.4 2008-12-22 12:31:04 cvsnageswara Exp $
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
    void		sceneAdded(CallBacker*);
    void		sceneClosed(CallBacker*);
    void		sceneChanged(CallBacker*);
    void		changeCB(CallBacker*);
    void		lineStyleChangeCB(CallBacker*);
    void		clearCB(CallBacker*);
    void		dlgClosed(CallBacker*); 
    void		manageDlg(bool); 

    uiODMain&		appl_;

    Pick::SetMgr&	picksetmgr_;
    TypeSet<int>	sceneids_;
    ObjectSet<visSurvey::PickSetDisplay>	displayobjs_;

    uiMeasureDlg*	measuredlg_;
    int			butidx_;
};

} // namespace Annotations

#endif

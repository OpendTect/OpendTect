#ifndef measuretoolman_h
#define measuretoolman_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		July 2008
 RCS:		$Id: measuretoolman.h,v 1.1 2008-08-01 12:12:38 cvsnanne Exp $
________________________________________________________________________

-*/

#include "callback.h"

class uiMeasureDlg;
class uiODMain;

namespace Annotations
{

class MeasureDisplay;

class MeasureToolMan : public CallBacker
{
public:
			MeasureToolMan(uiODMain&);

protected:

    void		buttonClicked(CallBacker*);
    void		sceneAdded(CallBacker*);
    void		sceneClosed(CallBacker*);
    void		changeCB(CallBacker*);
    void		propChangeCB(CallBacker*);
    void		clearCB(CallBacker*);

    uiODMain&		appl_;

    TypeSet<int>	sceneids_;
    ObjectSet<MeasureDisplay>	measureobjs_;

    uiMeasureDlg*	measuredlg_;
};

} // namespace Annotations

#endif

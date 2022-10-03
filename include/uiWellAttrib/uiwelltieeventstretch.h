#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwellattribmod.h"
#include "callback.h"

namespace Well { class D2TModel; class Track;}

namespace WellTie
{

class Marker;
class PickSetMgr;
class D2TModelMgr;

mExpClass(uiWellAttrib) EventStretch : public CallBacker
{
public:
					EventStretch(PickSetMgr&,D2TModelMgr&);
					~EventStretch();

    void				doWork(CallBacker*);
    void				setD2TModel(const Well::D2TModel* d2t)
					{ d2t_ = d2t; }
    void				setTrack(const Well::Track* track)
					{ track_ = track; }
protected:

    PickSetMgr&				pmgr_;
    D2TModelMgr&			d2tmgr_;
    const Well::D2TModel*		d2t_ = nullptr;
    const Well::Track*			track_ = nullptr;

    const TypeSet<Marker>&		synthpickset_;
    const TypeSet<Marker>&		seispickset_;

    void				doStretchWork();
    void				doStaticShift();
    void				doStretchSqueeze();
};

} // namespace WellTie

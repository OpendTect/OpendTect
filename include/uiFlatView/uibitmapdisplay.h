#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiflatviewmod.h"
#include "uigeom.h"
#include "callback.h"
#include "datapack.h"

class uiBitMapDisplayTask;
class uiDynamicImageItem;
class uiGraphicsItem;
class DataPackMgr;
class FlatDataPack;
class Task;

namespace FlatView { class Appearance; }


/*!
\brief Takes the flat-data from a FlatViewer and puts it into a uiGraphicsItem.
*/

mExpClass(uiFlatView) uiBitMapDisplay : public CallBacker
{
public:
			uiBitMapDisplay(FlatView::Appearance&,
					bool withalpha=true);
			~uiBitMapDisplay();

    void		update();
			//When inputs or settings have changed

    uiGraphicsItem*	getDisplay();
    void		removeDisplay();

    void		setOverlap(float v) { overlap_ = v; }
			/*!<If overlap is more than 0, a larger dynamic image
			    than requested will be made. The result
			    is that smaller pan/zoom movements will still
			    be covered by the dynamic image.
			    An overlap of 1 means 1 with will be added in each
			    direction, giving an image that is 9 times as
			    large.*/
    float		getOverlap() const  { return overlap_; }

    Interval<float>	getDataRange(bool iswva) const;
    const uiWorldRect&	boundingBox() const;
    void		setBoundingBox(const uiWorldRect&);
    void		setDataPack(const WeakPtr<FlatDataPack>&,bool wva);
    Notifier<uiBitMapDisplay>	rangeUpdated;

private:

    void			reGenerateCB(CallBacker*);
    void			dynamicTaskFinishCB(CallBacker*);

    Task*			createDynamicTask(bool issnapshot);

    bool			isVisible(bool wva) const;
    StepInterval<double>	getDataPackRange(bool wva,bool x1) const;
    uiWorldRect			getBoundingBox(bool wva) const;

    FlatView::Appearance&	appearance_;
    WeakPtr<FlatDataPack>	wvapack_;
    WeakPtr<FlatDataPack>	vdpack_;
    uiWorldRect			boundingbox_;
    float			overlap_;
    int				workqueueid_;
    bool			withalpha_;

    uiDynamicImageItem*		display_;
    uiBitMapDisplayTask*	basetask_;

    CallBack			finishedcb_;
};

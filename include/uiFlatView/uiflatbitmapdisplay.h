#ifndef uiflatbitmapdisplay_h
#define uiflatbitmapdisplay_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2007
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiflatviewmod.h"
#include "callback.h"

class uiDynamicImageItem;
class uiFlatViewer;
class uiGraphicsItem;
class Task;

namespace FlatView
{

class uiBitMapDisplayTask;

/*!
\brief Takes the flat-data from a FlatViewer and puts it into a uiGraphicsItem.
*/

mExpClass(uiFlatView) uiBitMapDisplay : public CallBacker
{
public:
    			uiBitMapDisplay(uiFlatViewer&);
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

private:

    void			reGenerateCB(CallBacker*);
    void			dynamicTaskFinishCB(CallBacker*);

    Task*			createDynamicTask(bool issnapshot);

    uiFlatViewer&		viewer_;
    int				workqueueid_;
    float			overlap_;

    uiDynamicImageItem*		display_;

    uiBitMapDisplayTask*	basetask_;

    CallBack			finishedcb_;
};


} // namespace FlatView


#endif


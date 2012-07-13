#ifndef uiflatbitmapdisplay_h
#define uiflatbitmapdisplay_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2007
 RCS:           $Id: uiflatbitmapdisplay.h,v 1.5 2012-07-13 09:51:28 cvsbruno Exp $
________________________________________________________________________

-*/

#include "datapack.h"

#include "array2dbitmap.h"
#include "arrayndimpl.h"
#include "uigeom.h"

class uiDynamicImageItem;
class FlatDataPack;
class uiGraphicsItem;
class A2DBitMapGenerator;
class uiRGBArray;

namespace FlatView
{

class BitMapMgr;
class BitMap2RGB;
class Appearance;
class uiBitMapDisplayTask;
class Viewer;

/*Takes the flat-data from a flatviewer and puts it into a uiGraphicsItem */

mClass uiBitMapDisplay : public CallBacker
{
public:
    			uiBitMapDisplay(Viewer&);
			~uiBitMapDisplay();

    void		update();
    			//When inputs or settings have changed

    uiGraphicsItem*	getDisplay();
    void		removeDisplay();

    Interval<float>	getDataRange(bool iswva) const;

    void		setXExtraFactor(float fac) 
    			{ xextfac_ = fac; }

    void		setViewRect(const uiRect& uir) 
    			{ viewrect_ = uir; }

protected:

    void			reGenerateCB(CallBacker*);
    void			dynamicTaskFinishCB(CallBacker*);

    Viewer&			viewer_;
    int				workqueueid_;
    uiRect                      viewrect_;
    float			xextfac_;

    uiDynamicImageItem*		display_;

    uiBitMapDisplayTask*	basetask_;
    uiBitMapDisplayTask*	dynamictask_;

    CallBack			finishedcb_;
};


} // namespace FlatView


#endif

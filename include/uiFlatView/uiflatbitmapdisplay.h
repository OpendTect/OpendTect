#ifndef uiflatbitmapdisplay_h
#define uiflatbitmapdisplay_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2007
 RCS:           $Id: uiflatbitmapdisplay.h,v 1.2 2012-04-06 12:27:32 cvskris Exp $
________________________________________________________________________

-*/

#include "datapack.h"

#include "array2dbitmap.h"
#include "arrayndimpl.h"

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
    			uiBitMapDisplay(Viewer&,bool wva);
			~uiBitMapDisplay();

    void		update();
    			//When inputs or settings have changed

    uiGraphicsItem*	getDisplay();
    void		removeDisplay();

protected:

    void			reGenerateCB(CallBacker*);
    void			dymamicTaskFinishCB(CallBacker*);

    Viewer&			viewer_;
    bool			wva_;
    bool			isworking_;

    uiDynamicImageItem*		display_;

    uiBitMapDisplayTask*	basetask_;
    uiBitMapDisplayTask*	dynamictask_;

    CallBack			finishedcb_;
};


} // namespace FlatView


#endif

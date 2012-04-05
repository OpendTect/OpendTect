#ifndef uiflatbitmapdisplay_h
#define uiflatbitmapdisplay_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2007
 RCS:           $Id: uiflatbitmapdisplay.h,v 1.1 2012-04-05 12:09:47 cvskris Exp $
________________________________________________________________________

-*/

#include "datapack.h"

#include "array2dbitmap.h"
#include "arrayndimpl.h"

class uiDynamicPixmapItem;
class FlatDataPack;
class uiGraphicsItem;
class A2DBitMapGenerator;
class uiRGBArray;
class uiFlatViewer;

namespace FlatView
{

class BitMapMgr;
class BitMap2RGB;
class Appearance;

mClass uiBitMapDisplay : public CallBacker
{
public:
    			uiBitMapDisplay(uiFlatViewer&);
			~uiBitMapDisplay();

    void		update();
    			//When inputs or settings have changed

    uiGraphicsItem*	getDisplay();
    void		removeDisplay() { display_ = 0; }

protected:

    void			reGenerateCB(CallBacker*);
    uiFlatViewer&		viewer_;

    BitMapMgr*			wvabmpmgr_;
    BitMapMgr*			vdbmpmgr_;

    uiRGBArray*			baseimage_;
    BitMap2RGB*			basebitmap2baseimage_;

    uiRGBArray*			dynamicimage_;
    A2DBitMapImpl*		dynamicbitmap_;

    uiDynamicPixmapItem*	display_;

};


} // namespace FlatView


#endif

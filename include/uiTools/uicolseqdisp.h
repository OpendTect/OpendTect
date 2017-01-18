#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2017
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uirgbarraycanvas.h"
class uiTextItem;
class uiRGBArray;


mExpClass(uiTools) uiColorSeqDisp : public uiRGBArrayCanvas
{
public:

				uiColorSeqDisp(uiParent*);
				~uiColorSeqDisp();

    const char*			seqName() const		{ return seqnm_; }
    void			setSeqName(const char*);
    bool			isFlipped() const	{ return flipped_; }
    void			setFlipped(bool);

    Notifier<uiColorSeqDisp>	selReq; //!< click or enter
    Notifier<uiColorSeqDisp>	menuReq; //!< right-click or space
    Notifier<uiColorSeqDisp>	upReq;	 //!< wheel-up, key-up or page-up
    Notifier<uiColorSeqDisp>	downReq; //!< wheel-down, key-down or page-down

protected:

    BufferString		seqnm_;
    bool			flipped_;
    uiRGBArray*			rgbarr_;
    uiTextItem*			nmitm_;

    void			initCB(CallBacker*);
    void			mouseButCB(CallBacker*);
    void			mouseWheelCB(CallBacker*);
    void			keybCB(CallBacker*);
    inline void			reSizeCB(CallBacker*);

    void			reDraw();

private:

    uiRGBArray&			mkRGBArr();

};

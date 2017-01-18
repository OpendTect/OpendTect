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


mExpClass(uiTools) uiColorSeqDisp : public uiRGBArrayCanvas
{
public:

				uiColorSeqDisp(uiParent*);
				~uiColorSeqDisp();

    void			setSeqName(const char*);
    const char*			seqName() const	    { return seqnm_; }

    Notifier<uiColorSeqSel>	selReq; //!< click or enter
    Notifier<uiColorSeqSel>	menuReq; //!< right-click or space
    Notifier<uiColorSeqSel>	upReq;	 //!< wheel-up, key-up or page-up
    Notifier<uiColorSeqSel>	downReq; //!< wheel-down, key-down or page-down

protected:

    mutable BufferString	seqnm_;

    void			initCB(CallBacker*);
    void			mouseButCB(CallBacker*);
    void			keybCB(CallBacker*);
    inline void			reSizeCB(CallBacker*);

    void			reDraw();

private:

    uiRGBArray&			mkRGBArr();

};

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
#include "coltabsequence.h"
class uiRGBArray;


mExpClass(uiTools) uiColSeqDisp : public uiRGBArrayCanvas
{
public:

    typedef ColTab::Sequence	Sequence;
    typedef Sequence::PosType	PosType;

				uiColSeqDisp(uiParent*,OD::Orientation o
							    =OD::Horizontal);
				~uiColSeqDisp();

    ConstRefMan<Sequence>	sequence() const	{ return colseq_; }
    OD::Orientation		orientation() const	{ return orientation_; }
    ColTab::SeqUseMode		seqUseMode() const	{ return sequsemode_; }
    const char*			seqName() const;
    void			setSequence(const Sequence&);
    void			setSeqUseMode(ColTab::SeqUseMode);
    void			setOrientation(OD::Orientation);
    void			setSeqName(const char*);

    PosType			seqPosFor(const uiPoint&) const;

				// Note that the PosType may be undef
    CNotifier<uiColSeqDisp,PosType>	selReq; //!< click or enter
    CNotifier<uiColSeqDisp,PosType>	menuReq; //!< right-click or space
    CNotifier<uiColSeqDisp,PosType>	upReq;	 //!< wheel-, key- or page-up
    CNotifier<uiColSeqDisp,PosType>	downReq; //!< wheel-, key- or page-down

protected:

    ColTab::SeqUseMode		sequsemode_;
    ConstRefMan<Sequence>	colseq_;
    OD::Orientation		orientation_;
    uiRGBArray*			rgbarr_;

    void			initCB(CallBacker*);
    void			mouseWheelCB(CallBacker*);
    void			keybCB(CallBacker*);
    void			reSizeCB(CallBacker*);
    void			mousePressCB( CallBacker* )
						{ handleMouseBut(true); }
    void			mouseReleaseCB( CallBacker* )
						{ handleMouseBut(false); }
    void			seqChgCB( CallBacker* )	    { reDraw(); }

    void			setNewSeq(const Sequence&);
    void			handleMouseBut(bool);
    virtual bool		handleLongTabletPress();

    void			reDraw();

private:

    uiRGBArray&			mkRGBArr();

};

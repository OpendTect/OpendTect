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
#include "coltab.h"
class uiRGBArray;
class uiPixmap;
class MouseEvent;
class uiTextItem;
class uiRectItem;


namespace ColTab
{

    mGlobal(uiTools) void fillRGBArray(uiRGBArray&,const Sequence&,
					const Mapper* mpr=0,
					OD::Orientation orient=OD::Horizontal,
					int bordernrpix=0);
    mGlobal(uiTools) uiPixmap* getuiPixmap(const Sequence&,int szx,int szy,
					const Mapper* mpr=0,
					OD::Orientation orient=OD::Horizontal,
					int bordernrpix=0);
}


mExpClass(uiTools) uiColSeqDisp : public uiRGBArrayCanvas
{
public:

    typedef ColTab::Sequence	Sequence;
    typedef ColTab::Mapper	Mapper;
    typedef ColTab::PosType	PosType;
    typedef ColTab::ValueType	ValueType;

				uiColSeqDisp(uiParent*,OD::Orientation,
					     bool withudfcoldisp=true,
					     bool wantnamedisplayed=true);
				~uiColSeqDisp();

    const Sequence&		sequence() const	{ return *colseq_; }
    OD::Orientation		orientation() const	{ return orientation_; }
    const Mapper*		mapper() const		{ return mapper_; }
    const char*			seqName() const;
    void			setSequence(const Sequence&);
    void			setSeqName(const char*);
    void			setOrientation(OD::Orientation);
    void			setMapper(const Mapper*);

    int				pixFor(ValueType) const;

				// The PosType is the relpos [0,1] in the bar
    CNotifier<uiColSeqDisp,PosType>	selReq; //!< click or enter
    CNotifier<uiColSeqDisp,PosType>	menuReq; //!< right-click or space
    CNotifier<uiColSeqDisp,PosType>	upReq;	 //!< wheel-, key- or page-up
    CNotifier<uiColSeqDisp,PosType>	downReq; //!< wheel-, key- or page-down

protected:

    ConstRefMan<Sequence>	colseq_;
    ConstRefMan<Mapper>		mapper_;
    OD::Orientation		orientation_;
    const bool			withudfcoldisp_;
    const bool			withnamedisp_;
    uiRGBArray*			rgbarr_;
    uiTextItem*			nmitm_;
    uiRectItem*			nmbgrectitm_;

    void			initCB(CallBacker*);
    void			mouseWheelCB(CallBacker*);
    void			keybCB(CallBacker*);
    void			reSizeCB(CallBacker*);
    void			mousePressCB( CallBacker* )
						{ handleMouseBut(true); }
    void			mouseReleaseCB( CallBacker* )
						{ handleMouseBut(false); }
    void			seqChgCB( CallBacker* )		{ reDraw(); }
    void			mapperChgCB( CallBacker* )	{ reDraw(); }
    int				longSz() const;
    int				shortSz() const;
    PosType			relPosFor(const MouseEvent&) const;

    bool			setNewSeq(const Sequence&);
    void			handleMouseBut(bool);
    virtual bool		handleLongTabletPress();

    void			reDraw();

private:

    uiRGBArray&			mkRGBArr();

};

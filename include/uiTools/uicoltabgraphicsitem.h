#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		May 2009
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigraphicsitem.h"
#include "coltabsequence.h"
#include "coltabmapper.h"
#include "draw.h"

class uiPixmapItem;
class uiRectItem;
class uiAdvancedTextItem;


mExpClass(uiTools) uiColTabItem : public uiGraphicsItem
{
public:

    mExpClass(uiTools) Setup
    {
    public:
		Setup( bool h ) //!< horizontal?
		    : hor_(h)
		    , sz_(h?100:25,h?25:100)
		    , startal_(OD::Alignment::HCenter,OD::Alignment::Bottom)
		    , stopal_(OD::Alignment::HCenter,OD::Alignment::Top) {}
	mDefSetupMemb(bool,hor)
	mDefSetupMemb(uiSize,sz)
	mDefSetupMemb(OD::Alignment,startal)
	mDefSetupMemb(OD::Alignment,stopal)
    };

			uiColTabItem(const Setup&);
			~uiColTabItem();
    Setup		setup() const			{ return setup_; }
    void		setSetup(const Setup&);

    void		setSeqName(const char* nm);
    void		setSequence(const ColTab::Sequence&);
    void		setMapper(const ColTab::Mapper*);

protected:
    void		adjustLabel();
    void		setPixmap();

    Setup		setup_;
    ConstRefMan<ColTab::Sequence> sequence_;
    ConstRefMan<ColTab::Mapper> mapper_;

    uiPixmapItem*	ctseqitm_;
    uiRectItem*		borderitm_;
    uiAdvancedTextItem* minvalitm_;
    uiAdvancedTextItem* maxvalitm_;

    void		seqChgCB(CallBacker*);
    void		mapperChgCB(CallBacker*);

};

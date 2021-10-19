#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		May 2009
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uigraphicsitem.h"
#include "coltabsequence.h"
#include "draw.h"

class uiAdvancedTextItem;
class uiPixmapItem;
class uiRectItem;
namespace ColTab { class MapperSetup; }


mExpClass(uiBase) uiColTabItem : public uiGraphicsItem
{
public:

    mExpClass(uiBase) Setup
    {
    public:
			Setup( bool h ) //!< horizontal?
			    : hor_(h)
			    , sz_(h?100:25,h?25:100)
			    , startal_(Alignment::HCenter,Alignment::Bottom)
			    , stopal_(Alignment::HCenter,Alignment::Top) {}
	mDefSetupMemb(bool,hor)
	mDefSetupMemb(uiSize,sz)
	mDefSetupMemb(Alignment,startal)
	mDefSetupMemb(Alignment,stopal)
    };

			uiColTabItem(const Setup&);
			~uiColTabItem();

    Setup&		setup()		{ return setup_; }
    const Setup&	setup() const	{ return setup_; }

    void		setColTab(const char* coltabnm);
    void		setColTabSequence(const ColTab::Sequence&);
    void		setColTabMapperSetup(const ColTab::MapperSetup&);

    void		setupChanged(); // Call this function whenever the setup
					// is changed

protected:
    void		adjustLabel();
    void		setPixmap();

    Setup		setup_;
    ColTab::Sequence	ctseq_;

    uiPixmapItem*	ctseqitm_;
    uiRectItem*		borderitm_;
    uiAdvancedTextItem* minvalitm_;
    uiAdvancedTextItem* maxvalitm_;
};

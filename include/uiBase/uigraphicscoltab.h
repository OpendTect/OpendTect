#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uigraphicsitem.h"
#include "coltabmapper.h"
#include "coltabsequence.h"
#include "draw.h"

class uiAdvancedTextItem;
class uiPixmapItem;
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
			    , startal_(Alignment::HCenter,Alignment::Top)
			    , stopal_(Alignment::HCenter,Alignment::Bottom) {}
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

    void		update(); // Call this function whenever the setup,
				  // Sequence or MapperSetup is changed

    mDeprecated("Use update()")
    void		setupChanged()		{ update(); }

protected:
    void		adjustLabel();
    void		setPixmap();

    Setup		setup_;
    ColTab::Sequence	ctseq_;
    ColTab::MapperSetup	ctms_;

    uiPixmapItem*	ctseqitm_;
    uiAdvancedTextItem* minvalitm_;
    uiAdvancedTextItem* maxvalitm_;
};

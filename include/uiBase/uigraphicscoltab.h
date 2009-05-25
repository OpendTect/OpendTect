#ifndef uigraphicscoltab_h
#define uigraphicscoltab_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		May 2009
 RCS:		$Id: uigraphicscoltab.h,v 1.2 2009-05-25 15:46:49 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigraphicsitem.h"
#include "draw.h"
#include "coltabsequence.h"

class uiPixmapItem;
class uiTextItem;
namespace ColTab { struct MapperSetup; }


mClass uiColTabItem : public uiGraphicsItemGroup
{
public:

    mClass Setup
    {
    public:
			Setup( bool h ) //!< horizontal?
			    : hor_(h)
			    , sz_(h?100:25,h?25:100)
			    , startal_(h?Alignment::HCenter:Alignment::Right,
				       h?Alignment::Top:Alignment::VCenter)
			    , stopal_( h?Alignment::HCenter:Alignment::Left,
				       h?Alignment::Bottom:Alignment::VCenter)
			    , startalong_(false)
			    , stopalong_(false)		{}
	mDefSetupMemb(bool,hor)
	mDefSetupMemb(uiSize,sz)
	mDefSetupMemb(Alignment,startal)
	mDefSetupMemb(Alignment,stopal)
	mDefSetupMemb(bool,startalong)	// put number along color bar
	mDefSetupMemb(bool,stopalong)	// put number along color bar
    };

    			uiColTabItem(const Setup&);
			~uiColTabItem();
    Setup&		setup()		{ return setup_; }
    const Setup&	setup() const	{ return setup_; }

    void		setColTabSequence(const ColTab::Sequence&);
    void		setColTabMapperSetup(const ColTab::MapperSetup&);

    uiPoint		getPixmapPos() const	{ return curpos_; }
    void		setPixmapPos(const uiPoint&);
    void		setPos(const uiPoint&);

    void		setupChanged();

protected:

    Setup		setup_;
    ColTab::Sequence	ctseq_;
    uiPoint		curpos_;

    uiPixmapItem*	ctseqitm_;
    uiTextItem*		minvalitm_;
    uiTextItem*		maxvalitm_;

    void		setPixmap();
    void		setPixmapPos();

};

#endif

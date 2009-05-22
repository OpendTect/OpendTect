#ifndef uigraphicscoltab_h
#define uigraphicscoltab_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		May 2009
 RCS:		$Id: uigraphicscoltab.h,v 1.1 2009-05-22 08:31:03 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uigraphicsitem.h"

class uiPixmapItem;
class uiTextItem;
namespace ColTab { struct MapperSetup; class Sequence; }


mClass uiColTabItem : public uiGraphicsItemGroup
{
public:
    			uiColTabItem();
			~uiColTabItem();

    void		setColTabSequence(const ColTab::Sequence&);
    void		setColTabMapperSetup(const ColTab::MapperSetup&);
    void		setSize( const uiSize& sz )	{ ctbarsz_ = sz; }
    			//!<Sets the size of the colorbar pixmap
    uiSize		size() const			{ return ctbarsz_; }
    void		setPos(const uiPoint&);
    			//!<Position is top-left corner of pixmap item

protected:
    uiPixmapItem*	ctseqitm_;
    uiTextItem*		minvalitm_;
    uiTextItem*		maxvalitm_;

    uiSize		ctbarsz_;
};

#endif

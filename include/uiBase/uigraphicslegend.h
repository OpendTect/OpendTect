#ifndef uigraphicslegend_h
#define uigraphicslegend_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Henrique Mageste
 Date:		July 2015
 RCS:		$Id: $
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uigraphicsitem.h"
#include "uigraphicsitemimpl.h"

class uiRectItem;
class uiAdvancedTextItem;

/*!
\brief A legend, mainly used in maps
*/

mExpClass(uiBase) uiLegendItem : public uiRectItem
{
public:
			uiLegendItem();
			~uiLegendItem();
    uiAdvancedTextItem* title_;
    uiAdvancedTextItem* country_;
    uiAdvancedTextItem* block_;
    uiAdvancedTextItem* license_;
    uiAdvancedTextItem* modelname_;
    uiAdvancedTextItem* horizonname_;
    uiAdvancedTextItem* mapscale_;
    uiAdvancedTextItem* contourinc_;
    uiAdvancedTextItem* username_;
    uiAdvancedTextItem* date_;
    uiAdvancedTextItem* sign_;

protected:

    void		buildLayout();
    void		setProperties();
    void		init();

};

#endif


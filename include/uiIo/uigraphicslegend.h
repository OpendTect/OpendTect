#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Henrique Mageste
 Date:		July 2015
 RCS:		$Id: $
________________________________________________________________________

-*/

#include "uiiocommon.h"
#include "uigraphicsitem.h"
#include "uigraphicsitemimpl.h"
#include "uistrings.h"

class uiRectItem;
class uiAdvancedTextItem;

/*!\brief A legend, mainly used in maps */

mExpClass(uiIo) uiLegendItem : public uiRectItem
{ mODTextTranslationClass(uiLegendItem)
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

    FontData&		headerfont_;
    FontData&		infofont_;

private:

    const uiString	sCountry()	    { return tr("Country"); }
    const uiString	sBlock()	    { return tr("Block"); }
    const uiString	sLicense()	    { return uiStrings::sLicense(); }
    const uiString	sModelNm()	    { return tr("Model Name"); }
    const uiString	sHorNm()	    { return tr("Horizon Name"); }
    const uiString	sScale()	    { return uiStrings::sScale(); }
    const uiString	sContourInc()	    { return tr("Contour Increment"); }
    const uiString	sUserNm()	    { return tr("User Name"); }
    const uiString	sSignature()	    { return tr("Signature"); }
    const uiString	sMap()		    { return tr("Map"); }

};

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          June 2001
 RCS:           $Id: uiconvpos.cc,v 1.8 2002-03-21 16:06:28 nanne Exp $
________________________________________________________________________

-*/

#include "uiconvpos.h"
#include "pixmap.h"
#include "survinfo.h"
#include "uibutton.h"
#include "uidialog.h"
#include "uigeninput.h"

uiConvertPos::uiConvertPos( uiParent* p, SurveyInfo* si )
	: uiDialog(p, uiDialog::Setup("Position conversion",
		   "Coordinates vs Inline/X-line","0.3.7"))
	, survinfo(si)

{
    uiGroup* flds = new uiGroup( this );
    IntInpSpec iis; iis.setHSzP(SzPolicySpec::small);
    DoubleInpSpec dis; dis.setHSzP(SzPolicySpec::small);
    inlfld = new uiGenInput( flds, "In-line", iis );
    crlfld = new uiGenInput( flds, "Cross-line", iis );
    xfld = new uiGenInput( flds, "X-coordinate", dis );
    yfld = new uiGenInput( flds, "Y-coordinate", dis );
    crlfld->attach( alignedBelow, inlfld );
    xfld->attach( rightTo, inlfld );
    yfld->attach( alignedBelow, xfld );
    yfld->attach( rightTo, crlfld );

    uiGroup* buts = new uiGroup( this );
    const ioPixmap right( GetDataFileName("forward.xpm") );
    const ioPixmap left( GetDataFileName("back.xpm") );
    docoordbut = new uiToolButton( buts, "", right );
    docoordbut->activated.notify( mCB(this,uiConvertPos, getCoord) );
    dobinidbut = new uiToolButton( buts, "", left );
    dobinidbut->activated.notify( mCB(this,uiConvertPos, getBinID) );
    docoordbut->attach( rightOf, dobinidbut, 0 );

    flds->attach( centeredBelow, buts );

    setOkText("Quit");
    setCancelText("");
}

void uiConvertPos::getCoord()
{
    BinID binid( inlfld->getIntValue(), crlfld->getIntValue() );
    Coord coord( survinfo->transform( binid ) );
    xfld->setValue( coord.x );
    yfld->setValue( coord.y );
}

void uiConvertPos::getBinID()
{
    Coord coord( xfld->getValue(), yfld->getValue() );
    BinID binid( survinfo->transform( coord ) );
    inlfld->setValue( binid.inl );
    crlfld->setValue( binid.crl );
}


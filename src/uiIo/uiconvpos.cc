/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          June 2001
 RCS:           $Id: uiconvpos.cc,v 1.12 2004-06-16 14:54:19 bert Exp $
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
    uiGroup* fldgrp = new uiGroup( this );
    inlfld = new uiGenInput( fldgrp, "In-line", IntInpSpec() );
    inlfld->setElemSzPol( uiObject::small );
    crlfld = new uiGenInput( fldgrp, "Cross-line", IntInpSpec() );
    crlfld->setElemSzPol( uiObject::small );
    xfld = new uiGenInput( fldgrp, "X-coordinate", DoubleInpSpec() );
    xfld->setElemSzPol( uiObject::small );
    yfld = new uiGenInput( fldgrp, "Y-coordinate", DoubleInpSpec() );
    yfld->setElemSzPol( uiObject::small );
    crlfld->attach( alignedBelow, inlfld );
    xfld->attach( rightTo, inlfld );
    yfld->attach( alignedBelow, xfld );
    yfld->attach( rightTo, crlfld );

    uiGroup* butgrp = new uiGroup( this );
    const ioPixmap right( GetDataFileName("forward.xpm") );
    const ioPixmap left( GetDataFileName("back.xpm") );
    docoordbut = new uiToolButton( butgrp, "", right );
    docoordbut->activated.notify( mCB(this,uiConvertPos,getCoord) );
    dobinidbut = new uiToolButton( butgrp, "", left );
    dobinidbut->activated.notify( mCB(this,uiConvertPos,getBinID) );
    docoordbut->attach( rightOf, dobinidbut, 0 );

    fldgrp->attach( centeredBelow, butgrp );

    setOkText("Quit");
    setCancelText("");
}

void uiConvertPos::getCoord()
{
    BinID binid( inlfld->getIntValue(), crlfld->getIntValue() );
    Coord coord( survinfo->transform( binid ) );
    xfld->setValue( coord.x );
    yfld->setValue( coord.y );
    inlfld->setValue( binid.inl );
    crlfld->setValue( binid.crl );
}

void uiConvertPos::getBinID()
{
    Coord coord( xfld->getValue(), yfld->getValue() );
    BinID binid( survinfo->transform( coord ) );
    inlfld->setValue( binid.inl );
    crlfld->setValue( binid.crl );
    if ( mIsZero(coord.x,mDefEps) ) xfld->setValue( 0 );
    if ( mIsZero(coord.y,mDefEps) ) yfld->setValue( 0 );
}


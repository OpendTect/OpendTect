/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          June 2001
 RCS:           $Id: uiconvpos.cc,v 1.16 2006-03-10 13:34:02 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiconvpos.h"
#include "pixmap.h"
#include "survinfo.h"
#include "oddirs.h"
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
    inlfld->setElemSzPol( uiObject::Small );
    crlfld = new uiGenInput( fldgrp, "Cross-line", IntInpSpec() );
    crlfld->setElemSzPol( uiObject::Small );
    xfld = new uiGenInput( fldgrp, "X-coordinate", DoubleInpSpec() );
    xfld->setElemSzPol( uiObject::Small );
    yfld = new uiGenInput( fldgrp, "Y-coordinate", DoubleInpSpec() );
    yfld->setElemSzPol( uiObject::Small );
    crlfld->attach( alignedBelow, inlfld );
    xfld->attach( rightTo, inlfld );
    yfld->attach( alignedBelow, xfld );
    yfld->attach( rightTo, crlfld );

    uiGroup* butgrp = new uiGroup( this );
    const ioPixmap right( GetIconFileName("forward.xpm") );
    const ioPixmap left( GetIconFileName("back.xpm") );
    docoordbut = new uiToolButton( butgrp, "", right );
    docoordbut->activated.notify( mCB(this,uiConvertPos,getCoord) );
    dobinidbut = new uiToolButton( butgrp, "", left );
    dobinidbut->activated.notify( mCB(this,uiConvertPos,getBinID) );
    docoordbut->attach( rightOf, dobinidbut, 0 );

    fldgrp->attach( centeredBelow, butgrp );
    setCtrlStyle( LeaveOnly );
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


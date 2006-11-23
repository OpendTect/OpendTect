/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          April 2001
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uistepoutsel.cc,v 1.2 2006-11-23 12:55:40 cvsbert Exp $";

#include "uistepoutsel.h"
#include "uispinbox.h"
#include "uilabel.h"


uiStepOutSel::uiStepOutSel( uiParent* p, const uiStepOutSel::Setup& setup )
    : uiGroup(p,setup.seltxt_)
    , valueChanged(this)
    , fld2(0)
{
    uiLabel* lbl = new uiLabel( this, setup.seltxt_ );
    fld1 = new uiSpinBox( this, 0, "spinbox 1" );
    BufferString prefx = setup.lbl1_; prefx += ":";
    fld1->setPrefix( prefx );
    fld1->attach( rightOf, lbl );
    fld1->valueChanged.notify( mCB(this,uiStepOutSel,valChg) );
    if ( !setup.single_ )
    {
	fld2 = new uiSpinBox( this, 0, "spinbox 2" );
	prefx = setup.lbl2_; prefx += ":";
	fld2->setPrefix( prefx );
	fld2->attach( rightOf, fld1 );
	fld2->valueChanged.notify( mCB(this,uiStepOutSel,valChg) );
    }

    setHAlignObj( fld1 );
}


int uiStepOutSel::val( bool dir1 ) const
{
    return dir1 ? fld1->getValue() : (fld2 ? fld2->getValue() : mUdf(int));
}


void uiStepOutSel::setVal( bool dir1, int v )
{
    if ( dir1 )
	fld1->setValue( v );
    else if ( fld2 )
	fld2->setValue( v );
}


void uiStepOutSel::setVals( int v )
{
    fld1->setValue( v );
    if ( fld2 )
	fld2->setValue( v );
}


void uiStepOutSel::set2D( bool yn )
{
    if ( yn )
    {
	setLabel( true, "nr" );
	allowInp( false, false );
    }
    else
    {
	setLabel( true, "inl" );
	setLabel( false, "crl" );
	allowInp( false, true );
    }
}


void uiStepOutSel::setLabel( bool dir1, const char* s )
{
    BufferString lbl( s ); lbl += ":";
    if ( dir1 )
	fld1->setPrefix( lbl );
    else if ( fld2 )
	fld2->setPrefix( lbl );
}


void uiStepOutSel::allowInp( bool dir1, bool yn )
{
    if ( dir1 )
	fld1->setSensitive( yn );
    else if ( fld2 )
	fld2->setSensitive( yn );
}


bool uiStepOutSel::dir2Active() const
{
    return fld2 && fld2->sensitive();
}


void uiStepOutSel::setBinID( const BinID& bid )
{
    if ( dir2Active() )
	{ setVal(true,bid.inl); setVal(false,bid.crl); }
    else
	setVal(true,bid.crl);
}


BinID uiStepOutSel::getBinID() const
{
    return dir2Active() ? BinID( val(true), val(false) )
			: BinID( 0, val(true) );
}


void uiStepOutSel::valChg( CallBacker* cb )
{
    valueChanged.trigger( cb );
}

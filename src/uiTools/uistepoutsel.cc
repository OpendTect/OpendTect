/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          April 2001
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uistepoutsel.cc,v 1.4 2006-12-21 16:06:10 cvshelene Exp $";

#include "uistepoutsel.h"
#include "uispinbox.h"
#include "uilabel.h"


inline static BufferString mkPrefx( const char* lbl )
{
    BufferString ret( lbl );
    if ( !ret.isEmpty() )
	ret += ":";
    return ret;
}


uiStepOutSel::uiStepOutSel( uiParent* p, const uiStepOutSel::Setup& setup,
			    bool is2dsel )
    : uiGroup(p,setup.seltxt_)
    , valueChanged(this)
    , fld2(0)
    , is2d(is2dsel)
{
    uiLabel* lbl = new uiLabel( this, setup.seltxt_ );
    fld1 = new uiSpinBox( this, 0, "spinbox 1" );
    fld1->setPrefix( is2d ? mkPrefx("nr") : mkPrefx(setup.lbl1_) );
    fld1->attach( rightOf, lbl );
    fld1->valueChanged.notify( mCB(this,uiStepOutSel,valChg) );
    if ( !setup.single_ && !is2d )
    {
	fld2 = new uiSpinBox( this, 0, "spinbox 2" );
	fld2->setPrefix( mkPrefx(setup.lbl2_) );
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

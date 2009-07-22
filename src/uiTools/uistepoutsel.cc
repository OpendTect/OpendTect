/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          April 2001
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uistepoutsel.cc,v 1.11 2009-07-22 16:01:42 cvsbert Exp $";

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


uiStepOutSel::uiStepOutSel( uiParent* p, const uiStepOutSel::Setup& setup )
    : uiGroup(p,setup.seltxt_)
    , valueChanged(this)
    , valueChanging(this)
    , fld2(0)
{
    init( setup );
}


uiStepOutSel::uiStepOutSel( uiParent* p, bool single, const char* seltxt )
    : uiGroup(p,seltxt)
    , valueChanged(this)
    , valueChanging(this)
    , fld2(0)
{
    uiStepOutSel::Setup setup( single );
    setup.seltxt( seltxt );
    init( setup );
}


void uiStepOutSel::setFieldNames( const char* nmfld1, const char* nmfld2 )
{
    if ( nmfld1 && *nmfld1 )
	fld1->setName( nmfld1 );
    if ( fld2 && nmfld2 && *nmfld2 )
	fld2->setName( nmfld2 );
}


void uiStepOutSel::init( const uiStepOutSel::Setup& setup )
{
    const StepInterval<int> intv( setup.allowneg_ ? -999 : 0, 999, 1 );

    uiLabel* lbl = new uiLabel( this, setup.seltxt_ );

    fld1 = new uiSpinBox( this, 0, setup.lbl1_ );
    fld1->setPrefix( mkPrefx(setup.lbl1_) );
    fld1->attach( rightOf, lbl );
    fld1->setInterval( intv );
    fld1->valueChanged.notify( mCB(this,uiStepOutSel,valChanged) );
    fld1->valueChanging.notify( mCB(this,uiStepOutSel,valChanging) );

    if ( !setup.single_ )
    {
	fld2 = new uiSpinBox( this, 0, setup.lbl2_ );
	fld2->setPrefix( mkPrefx(setup.lbl2_) );
	fld2->attach( rightOf, fld1 );
	fld2->setInterval( intv );
	fld2->valueChanged.notify( mCB(this,uiStepOutSel,valChanged) );
	fld2->valueChanging.notify( mCB(this,uiStepOutSel,valChanging) );
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
    return fld2;
}


void uiStepOutSel::setBinID( const BinID& bid )
{
    if ( dir2Active() )
	{ setVal(true,bid.inl); setVal(false,bid.crl); }
    else
	setVal(true,bid.crl);
}


void uiStepOutSel::setInterval( StepInterval<int> inlrg, 
				StepInterval<int> crlrg )
{
    fld1->setInterval( inlrg );
    if ( dir2Active() ) fld2->setInterval( crlrg );
}


BinID uiStepOutSel::getBinID() const
{
    return dir2Active() ? BinID( val(true), val(false) )
			: BinID( 0, val(true) );
}


void uiStepOutSel::valChanged( CallBacker* cb )
{
    valueChanged.trigger( cb );
}


void uiStepOutSel::valChanging( CallBacker* cb )
{
    valueChanging.trigger( cb );
}

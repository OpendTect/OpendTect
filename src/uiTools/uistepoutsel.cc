/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uistepoutsel.h"

#include "uispinbox.h"
#include "uilabel.h"
#include "uistrings.h"


inline static uiString mkPrefx( const uiString& lbl )
{
    if ( !lbl.isEmpty() )
	return toUiString( "%1:").arg( lbl );

    return uiString();
}


uiStepOutSel::uiStepOutSel( uiParent* p, const uiStepOutSel::Setup& setup )
    : uiGroup(p,setup.seltxt_.getFullString())
    , valueChanged(this)
    , valueChanging(this)
    , fld2_(0)
{
    init( setup );
}


uiStepOutSel::uiStepOutSel( uiParent* p, bool single, const uiString& seltxt )
    : uiGroup(p,seltxt.getFullString() )
    , valueChanged(this)
    , valueChanging(this)
    , fld2_(0)
{
    uiStepOutSel::Setup setup( single );
    setup.seltxt( seltxt );
    init( setup );
}


uiStepOutSel::~uiStepOutSel()
{
}


void uiStepOutSel::setFieldNames( const char* nmfld1, const char* nmfld2 )
{
    if ( nmfld1 && *nmfld1 )
	fld1_->setName( nmfld1 );
    if ( fld2_ && nmfld2 && *nmfld2 )
	fld2_->setName( nmfld2 );
}


void uiStepOutSel::init( const uiStepOutSel::Setup& setup )
{
    const StepInterval<int> intv( setup.allowneg_ ? -999 : 0, 999, 1 );

    uiLabel* lbl = new uiLabel( this, setup.seltxt_ );

    fld1_ = new uiSpinBox( this, 0, setup.lbl1_.getFullString() );
    fld1_->setPrefix( mkPrefx(setup.lbl1_) );
    fld1_->attach( rightOf, lbl );
    fld1_->setInterval( intv );
    fld1_->valueChanged.notify( mCB(this,uiStepOutSel,valChanged) );
    fld1_->valueChanging.notify( mCB(this,uiStepOutSel,valChanging) );

    if ( !setup.single_ )
    {
	fld2_ = new uiSpinBox( this, 0, setup.lbl2_.getFullString() );
	fld2_->setPrefix( mkPrefx(setup.lbl2_) );
	fld2_->attach( rightOf, fld1_ );
	fld2_->setInterval( intv );
	fld2_->valueChanged.notify( mCB(this,uiStepOutSel,valChanged) );
	fld2_->valueChanging.notify( mCB(this,uiStepOutSel,valChanging) );
    }

    setHAlignObj( fld1_ );

    sosetup_ = setup;
}


int uiStepOutSel::val( bool dir1 ) const
{
    return dir1 ? fld1_->getIntValue()
		: (fld2_ ? fld2_->getIntValue() : mUdf(int));
}


void uiStepOutSel::setVal( bool dir1, int v )
{
    if ( dir1 )
	fld1_->setValue( v );
    else if ( fld2_ )
	fld2_->setValue( v );
}


void uiStepOutSel::setVals( int v )
{
    fld1_->setValue( v );
    if ( fld2_ )
	fld2_->setValue( v );
}


bool uiStepOutSel::dir2Active() const
{
    return fld2_;
}


void uiStepOutSel::setBinID( const BinID& bid )
{
    if ( dir2Active() )
	{ setVal(true,bid.inl()); setVal(false,bid.crl()); }
    else
	setVal(true,bid.crl());
}


void uiStepOutSel::setInterval( StepInterval<int> inlrg,
				StepInterval<int> crlrg )
{
    fld1_->setInterval( inlrg );
    if ( dir2Active() ) fld2_->setInterval( crlrg );
}


BinID uiStepOutSel::getBinID() const
{
    return dir2Active() ? BinID( val(true), val(false) )
			: BinID( 0, val(true) );
}


void uiStepOutSel::set3D( bool yn )
{
    fld1_->display( true );
    fld1_->setPrefix( mkPrefx(sosetup_.lbl1_) );
    fld2_->display( yn );
}


void uiStepOutSel::valChanged( CallBacker* cb )
{
    valueChanged.trigger( cb );
}


void uiStepOutSel::valChanging( CallBacker* cb )
{
    valueChanging.trigger( cb );
}


//-----------------------------------------------------------------------------

uiStepout3DSel::uiStepout3DSel( uiParent* p, const uiStepOutSel::Setup& setup )
	: uiStepOutSel( p, setup )
{
    fld3_ = new uiSpinBox( this, 0, "Z" );
    fld3_->setPrefix( mkPrefx(uiStrings::sZ()) );
    fld3_->attach( rightOf, setup.single_ ? fld1_ : fld2_ );
    setHAlignObj( fld1_ );
}


uiStepout3DSel::uiStepout3DSel( uiParent* p, bool single,
							const uiString& seltxt )
    : uiStepOutSel( p, single, seltxt )
{
    fld3_ = new uiSpinBox( this, 0, "Z" );
    fld3_->setPrefix( mkPrefx(uiStrings::sZ()) );
    fld3_->attach( rightOf, fld2_ ? fld2_ : fld1_ );

    setHAlignObj( fld1_ );
}


uiStepout3DSel::~uiStepout3DSel()
{}


int uiStepout3DSel::val( int dir ) const
{
    return dir<2 ? uiStepOutSel::val( dir == 0 ) : fld3_->getIntValue();
}


void uiStepout3DSel::setZVal( int val3 )
{
    fld3_->setValue( val3 );
}


void uiStepout3DSel::setVals( int val1, int val2, int val3 )
{
    fld1_->setValue( val1 );
    if ( fld2_ )
	fld2_->setValue( val2 );
    fld3_->setValue( val3 );
}


void uiStepout3DSel::setVals( int value )
{
    setVals( value, value, value );
}


int uiStepout3DSel::getZVal() const
{
    return fld3_->getIntValue();
}


void uiStepout3DSel::setZInterval( StepInterval<int> zrg )
{
    fld3_->setInterval( zrg );
}


void uiStepout3DSel::setZFieldName( const char* nmzfld )
{
    if ( nmzfld && *nmzfld )
	fld3_->setName( nmzfld );
}

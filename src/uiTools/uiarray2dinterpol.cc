/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          March 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiarray2dinterpol.cc,v 1.2 2009-04-22 21:55:20 cvskris Exp $";

#include "uiarray2dinterpol.h"

#include "array2dinterpol.h"
#include "array2dinterpolimpl.h"
#include "datainpspec.h"
#include "uigeninput.h"
#include "uimsg.h"

mImplFactory1Param( uiArray2DInterpol, uiParent*,uiArray2DInterpolSel::factory);


uiArray2DInterpolSel::uiArray2DInterpolSel( uiParent* p, bool filltype,
					    bool maxholesz,
       					    const Array2DInterpol* oldvals )
    : uiDlgGroup( p, "Array2D Interpolation" )
    , result_( 0 )
    , filltypefld_( 0 )
    , maxholeszfld_( 0 )
{
    params_.allowNull( true );
    if ( filltype )
    {
	filltypefld_ = new uiGenInput( this, "Scope",
		StringListInpSpec( Array2DInterpol::FillTypeNames() ));
	if ( oldvals ) filltypefld_->setValue( (int) oldvals->getFillType() );
    }

    if ( maxholesz )
    {
	maxholeszfld_ = new uiGenInput( this, 0, FloatInpSpec() );
	maxholeszfld_->setWithCheck( true );
	if ( filltypefld_ )
	    maxholeszfld_->attach( alignedBelow, filltypefld_ );
	if ( oldvals )
	{
	    if ( mIsUdf(oldvals->getMaxHoleSize() ) )
		maxholeszfld_->setChecked( false );
	    else
	    {
		maxholeszfld_->setChecked( true );
		maxholeszfld_->setValue( oldvals->getMaxHoleSize() );
	    }
	}
    }

    methodsel_ = new uiGenInput( this, "Algorithm",
	    StringListInpSpec(Array2DInterpol::factory().getNames(true) ) );
    if ( maxholeszfld_ || filltypefld_ )
	methodsel_->attach( alignedBelow,
		maxholeszfld_ ? maxholeszfld_ : filltypefld_ );

    const BufferStringSet& methods = Array2DInterpol::factory().getNames(false);
    const int methodidx = oldvals ? methods.indexOf( oldvals->type() ) : 0;
    if ( oldvals )
	methodsel_->setValue( methodidx );

    for ( int idx=0; idx<methods.size(); idx++ )
    {
	uiArray2DInterpol* paramfld =
	    factory().create( methods[idx]->buf(), this, true );

	if ( paramfld )
	{
	    if ( oldvals && idx==methodidx)
		paramfld->setValuesFrom( *oldvals );
	    paramfld->attach( alignedBelow, methodsel_ );
	}

	params_ += paramfld;
    }

    setHAlignObj( methodsel_ );

    selChangeCB( 0 );
    setDistanceUnit( 0 );
}


uiArray2DInterpolSel::~uiArray2DInterpolSel()
{ delete result_; }


void uiArray2DInterpolSel::setDistanceUnit( const char* du )
{
    if ( maxholeszfld_ )
    {
	BufferString res = "Keep holes larger than";
	if ( du )
	{
	    res += " ";
	    res += du;
	}

	maxholeszfld_->setTitleText( res.buf() );
    }

    for ( int idx=0; idx<params_.size(); idx++ )
    {
	if ( params_[idx] )
	    params_[idx]->setDistanceUnit( du );
    }
}


uiParent* uiArray2DInterpolSel::getTopObject()
{
    if ( filltypefld_ ) return filltypefld_;
    if ( maxholeszfld_ ) return maxholeszfld_;
    return methodsel_;
}


void uiArray2DInterpolSel::selChangeCB( CallBacker* )
{
    const int sel = methodsel_->getIntValue( 0 );
    for ( int idx=0; idx<params_.size(); idx++ )
    {
	if ( !params_[idx] )
	    continue;

	params_[idx]->display( idx==sel );
    }
}


bool uiArray2DInterpolSel::acceptOK()
{
    if ( maxholeszfld_ && maxholeszfld_->isChecked() &&
	 (mIsUdf( maxholeszfld_->getfValue() ) ||
	  maxholeszfld_->getfValue()<=0 ) )
    {
	uiMSG().error("Maximum hole size not set or is less or equal to zero");
	return false;
    }

    const int methodidx = methodsel_->getIntValue();
    const BufferStringSet& methods = Array2DInterpol::factory().getNames(false);

    if ( result_ )
	delete result_;

    if ( !params_[methodidx] )
	result_ = Array2DInterpol::factory().create(methods[methodidx]->buf());
    else
    {
	if ( !params_[methodidx]->acceptOK() )
	    return false;

	result_ = params_[methodidx]->getResult();
	if ( !result_ )
	    return false;
    }

    result_->setFillType( filltypefld_ 
	? (Array2DInterpol::FillType) filltypefld_->getIntValue()
	: Array2DInterpol::Full );

    result_->setMaxHoleSize( maxholeszfld_ && maxholeszfld_->isChecked() 
	? maxholeszfld_->getIntValue()
	: mUdf(int) );

    return true;
}


Array2DInterpol* uiArray2DInterpolSel::getResult()
{
    Array2DInterpol* res = result_;
    result_ = 0;
    return res;
}


uiArray2DInterpol::uiArray2DInterpol( uiParent* p, const char* nm )
    : uiDlgGroup( p, nm )
    , result_( 0 )
{}


uiArray2DInterpol::~uiArray2DInterpol()
{ delete result_; }


Array2DInterpol* uiArray2DInterpol::getResult()
{
    Array2DInterpol* res = result_;
    result_ = 0;
    return res;
}


void uiInverseDistanceArray2DInterpol::initClass()
{
    uiArray2DInterpolSel::factory().addCreator( create,
	    InverseDistanceArray2DInterpol::sType() );
}


uiArray2DInterpol* uiInverseDistanceArray2DInterpol::create( uiParent* p )
{ return new uiInverseDistanceArray2DInterpol( p ); }


uiInverseDistanceArray2DInterpol::uiInverseDistanceArray2DInterpol(uiParent* p)
    : uiArray2DInterpol( p, "Inverse distance" )
{
    searchradiusfld_ = new  uiGenInput( this, 0, FloatInpSpec() );
    searchradiusfld_->setWithCheck( true );
    searchradiusfld_->setChecked( true );
    searchradiusfld_->checked.notify(
	    mCB(this,uiInverseDistanceArray2DInterpol,useRadiusCB));

    cornersfirstfld_ = new  uiGenInput( this, "Compute corners first",
					BoolInpSpec(false) );
    cornersfirstfld_->attach( alignedBelow, searchradiusfld_ );
    stepsizefld_ = new uiGenInput( this, "Step size",
	    			   IntInpSpec(1) );
    stepsizefld_->attach( alignedBelow, cornersfirstfld_ );

    nrstepsfld_ = new uiGenInput( this, "[Nr steps]",
	    			  IntInpSpec() );
    nrstepsfld_->attach( alignedBelow, stepsizefld_ );
    setHAlignObj( nrstepsfld_ );

    setDistanceUnit( 0 );
    useRadiusCB( 0 );
}


void uiInverseDistanceArray2DInterpol::useRadiusCB( CallBacker* )
{
    const bool hasradius = searchradiusfld_->isChecked();
    cornersfirstfld_->display( hasradius );
    stepsizefld_->display( hasradius );
    nrstepsfld_->display( hasradius );
}


void uiInverseDistanceArray2DInterpol::setValuesFrom(const Array2DInterpol& a)
{
    mDynamicCastGet(const InverseDistanceArray2DInterpol*, ptr, &a );
    if ( !ptr )
	return;

    if ( mIsUdf( ptr->getSearchRadius() ) )
	searchradiusfld_->setChecked( false );
    else
    {
	searchradiusfld_->setChecked( true );
	searchradiusfld_->setValue( ptr->getSearchRadius() );

	nrstepsfld_->setValue( ptr->getNrSteps() );
	cornersfirstfld_->setValue( ptr->getCornersFirst() );
	stepsizefld_->setValue( ptr->getStepSize() );
    }
}


void uiInverseDistanceArray2DInterpol::setDistanceUnit( const char* d )
{
    BufferString res = "Search radius";
    if ( d )
    {
	res += " ";
	res += d;
    }

    searchradiusfld_->setTitleText( res.buf() );
}

bool uiInverseDistanceArray2DInterpol::acceptOK()
{
    if ( result_ ) { delete result_; result_ = 0; }

    const bool hasradius = searchradiusfld_->isChecked();
    const float searchradius = searchradiusfld_->getfValue( 0 );
    const int stepsize = stepsizefld_->getIntValue( 0 );
    const int nrsteps = nrstepsfld_->getIntValue( 0 );

    if ( hasradius )
    {
	if ( mIsUdf(searchradius) || searchradius<=0 ||
	     mIsUdf(stepsize) || stepsize<1 )
	{
	    uiMSG().error(
		    "Search radius and Step size must set and be more than"
		    " zero" );
	    return false;
	}

	if ( (!mIsUdf(nrsteps) && nrsteps<1 ) )
	{
	    uiMSG().error( "Nr steps must set and be more than zero" );
	    return false;
	}
    }

    InverseDistanceArray2DInterpol* res = new
	InverseDistanceArray2DInterpol;

    res->setSearchRadius( hasradius ? searchradius : mUdf(float) );
    if ( hasradius )
    {
	res->setNrSteps( nrsteps );
	res->setCornersFirst( cornersfirstfld_->getBoolValue( 0 ) );
	res->setStepSize( stepsize );
    }

    result_ = res;

    return true;
}

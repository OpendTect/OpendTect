/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          March 2008
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiarray2dinterpol.h"

#include "array2dinterpolimpl.h"
#include "iopar.h"
#include "survinfo.h"
#include "uigeninput.h"
#include "uibutton.h"
#include "uimsg.h"

mImplFactory1Param(uiArray2DInterpol,uiParent*,uiArray2DInterpolSel::factory);


uiArray2DInterpolSel::uiArray2DInterpolSel( uiParent* p, bool filltype,
					    bool maxholesz,
					    bool withclassification,
       					    const Array2DInterpol* oldvals,
					    bool withstep )
    : uiDlgGroup( p, "Array2D Interpolation" )
    , result_( 0 )
    , filltypefld_( 0 )
    , stepfld_( 0 )
    , maxholeszfld_( 0 )
    , methodsel_( 0 )
    , isclassificationfld_( 0 )
{
    params_.allowNull( true );
    uiObject* prevfld = 0;
    uiObject* halignobj = 0;
    if ( filltype )
    {
	filltypefld_ = new uiGenInput( this, "Scope",
		StringListInpSpec( Array2DInterpol::FillTypeNames() ));
	if ( oldvals ) filltypefld_->setValue( (int) oldvals->getFillType() );
	prevfld = filltypefld_->attachObj();
    }

    if ( withstep )
    {
	PositionInpSpec::Setup setup;
	PositionInpSpec spec( setup );
	stepfld_ = new uiGenInput( this, "Inl/Crl Step", spec );
	stepfld_->setValue( BinID(SI().inlStep(),SI().crlStep()) );
	if ( filltype )
	    stepfld_->attach( alignedBelow, prevfld );
	prevfld = stepfld_->attachObj();
    }

    if ( maxholesz )
    {
	maxholeszfld_ = new uiGenInput( this, 0, FloatInpSpec() );
	maxholeszfld_->setWithCheck( true );
	if ( prevfld )
	    maxholeszfld_->attach( alignedBelow, prevfld );
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

	prevfld = maxholeszfld_->attachObj();
    }


    if ( withclassification )
    {
	isclassificationfld_ = new uiGenInput( this,
		"Values are classifications", BoolInpSpec( false ) );
	if ( prevfld )
	    isclassificationfld_->attach( alignedBelow, prevfld );
	if ( oldvals )
	    isclassificationfld_->setValue( oldvals->isClassification() );
	prevfld = isclassificationfld_->attachObj();
    }

    const BufferStringSet& methods = Array2DInterpol::factory().getNames(false);
    int methodidx;
    if ( methods.size()>1 )
    {
	methodsel_ = new uiGenInput( this, "Algorithm",
	    StringListInpSpec(Array2DInterpol::factory().getNames(true) ) );

	if ( prevfld )
	    methodsel_->attach( alignedBelow, prevfld );

	methodsel_->valuechanged.notify(
		mCB( this, uiArray2DInterpolSel, selChangeCB ) );

	methodidx = oldvals ? methods.indexOf( oldvals->factoryKeyword() ) : 0;
	if ( oldvals )
	    methodsel_->setValue( methodidx );

	prevfld = methodsel_->attachObj();
	halignobj = prevfld;
    }
    else
	methodidx = 0;

    for ( int idx=0; idx<methods.size(); idx++ )
    {
	uiArray2DInterpol* paramfld =
	    factory().create( methods[idx]->buf(), this, true );

	if ( paramfld )
	{
	    if ( oldvals && idx==methodidx)
		paramfld->setValuesFrom( *oldvals );

	    if ( prevfld )
		paramfld->attach( alignedBelow, prevfld );
	}

	params_ += paramfld;
    }

    if ( halignobj )
	setHAlignObj( halignobj );
    else
    {
	for ( int idx=0; idx<params_.size(); idx++ )
	{
	    if ( params_[idx] )
	    {
		setHAlignObj( params_[idx] );
		break;
	    }
	}
    }

    selChangeCB( 0 );
    setDistanceUnit( 0 );
}


uiArray2DInterpolSel::~uiArray2DInterpolSel()
{ delete result_; }


const char* uiArray2DInterpolSel::helpID() const
{
    const int sel = methodsel_ ? methodsel_->getIntValue( 0 ) : 0;
    return params_[sel] ? params_[sel]->helpID() : 0;
}


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
    const int sel = methodsel_ ? methodsel_->getIntValue( 0 ) : 0;
    for ( int idx=0; idx<params_.size(); idx++ )
    {
	if ( !params_[idx] )
	    continue;

	params_[idx]->display( idx==sel );
    }
}


void uiArray2DInterpolSel::fillPar( IOPar& iopar ) const
{
    if ( !result_ )
	return;

    iopar.set( sKey::Name(), methodsel_->text() );
    result_->fillPar( iopar );
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

    const BufferStringSet& methods = Array2DInterpol::factory().getNames(false);
    const int methodidx = methodsel_ ? methodsel_->getIntValue() : 0;
    
    if ( methodidx>=methods.size() )
    {
	pErrMsg("Invalid method selected");
	return false;
    }

    if ( result_ )
	delete result_;

    if ( !params_[methodidx] )
    {
	result_ = Array2DInterpol::factory().create(methods[methodidx]->buf());
	BufferString msg( result_->infoMsg() );
	if ( !msg.isEmpty() )
	{
	    uiMSG().message( msg );
	    return false;
	}
    }
    else
    {
	if ( !params_[methodidx]->acceptOK() )
	    return false;

	result_ = params_[methodidx]->getResult();
	if ( !result_ )
	    return false;
    }

    if ( isclassificationfld_ )
	result_->setClassification( isclassificationfld_->getBoolValue() );

    result_->setFillType( filltypefld_ 
	? (Array2DInterpol::FillType) filltypefld_->getIntValue()
	: Array2DInterpol::Full );

    result_->setMaxHoleSize( maxholeszfld_ && maxholeszfld_->isChecked() 
			     ? maxholeszfld_->getIntValue() : mUdf(float) );

    return true;
}


void uiArray2DInterpolSel::setStep( const BinID& steps )
{
    if ( stepfld_ )
	stepfld_->setValue( steps );
}


BinID uiArray2DInterpolSel::getStep() const
{
    return stepfld_ ? stepfld_->getBinID() : BinID::udf();
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
{
}


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
	    InverseDistanceArray2DInterpol::sFactoryKeyword() );
}


uiArray2DInterpol* uiInverseDistanceArray2DInterpol::create( uiParent* p )
{ return new uiInverseDistanceArray2DInterpol( p ); }


uiInverseDistanceArray2DInterpol::uiInverseDistanceArray2DInterpol(uiParent* p)
    : uiArray2DInterpol( p, "Inverse distance" )
    , nrsteps_(mUdf(int))
    , cornersfirst_(false)
    , stepsz_(1)
{
    radiusfld_ = new  uiGenInput( this, 0, FloatInpSpec() );
    radiusfld_->setWithCheck( true );
    radiusfld_->setChecked( true );
    radiusfld_->checked.notify(
	    mCB(this,uiInverseDistanceArray2DInterpol,useRadiusCB));

    parbut_ = new uiPushButton( this, "&Parameters", 
		    mCB(this,uiInverseDistanceArray2DInterpol,doParamDlg),
		    false );
    parbut_->attach( rightOf, radiusfld_ );

    setHAlignObj( radiusfld_ );
    setDistanceUnit( 0 );
    useRadiusCB( 0 );
}


void uiInverseDistanceArray2DInterpol::useRadiusCB( CallBacker* )
{
    parbut_->display( radiusfld_->isChecked() );
}


void uiInverseDistanceArray2DInterpol::setValuesFrom( const Array2DInterpol& a )
{
    mDynamicCastGet(const InverseDistanceArray2DInterpol*, ptr, &a );
    if ( !ptr )
	return;

    if ( mIsUdf( ptr->getSearchRadius() ) )
	radiusfld_->setChecked( false );
    else
    {
	radiusfld_->setChecked( true );
	radiusfld_->setValue( ptr->getSearchRadius() );

	nrsteps_ = ptr->getNrSteps();
	cornersfirst_ = ptr->getCornersFirst();
	stepsz_ = ptr->getStepSize();
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

    radiusfld_->setTitleText( res.buf() );
}

void uiTriangulationArray2DInterpol::initClass()
{
    uiArray2DInterpolSel::factory().addCreator( create,
	    TriangulationArray2DInterpol::sFactoryKeyword() );
}


uiArray2DInterpol* uiTriangulationArray2DInterpol::create( uiParent* p )
{ return new uiTriangulationArray2DInterpol( p ); }


uiTriangulationArray2DInterpol::uiTriangulationArray2DInterpol(uiParent* p)
    : uiArray2DInterpol( p, "Inverse distance" )
{
    useneighborfld_ = new uiCheckBox( this, "Use nearest neighbor" );
    useneighborfld_->setChecked( false );
    useneighborfld_->activated.notify(
	    mCB(this,uiTriangulationArray2DInterpol,intCB) );
    
    maxdistfld_ = new uiGenInput( this, 0, FloatInpSpec() );
    maxdistfld_->setWithCheck( true );
    maxdistfld_->attach( alignedBelow, useneighborfld_ );

    setHAlignObj( useneighborfld_ );
    setDistanceUnit( 0 );
    intCB( 0 );
}


void uiTriangulationArray2DInterpol::intCB( CallBacker* )
{
    maxdistfld_->display( !useneighborfld_->isChecked() );
}


void uiTriangulationArray2DInterpol::setDistanceUnit( const char* d )
{
    BufferString res = "Max interpolate distance";
    if ( d )
    {
	res += " ";
	res += d;
    }

    maxdistfld_->setTitleText( res.buf() );
}


bool uiTriangulationArray2DInterpol::acceptOK()
{
    bool usemax = !useneighborfld_->isChecked() && maxdistfld_->isChecked();
    const float maxdist = maxdistfld_->getfValue();
    if ( usemax && !mIsUdf(maxdist) && maxdist<0 )
    {
	uiMSG().error( "Maximum distance must be > 0. " );
	return false;
    }

    if ( result_ )
    { 
	delete result_; 
	result_ = 0; 
    }
    
    TriangulationArray2DInterpol* res = new TriangulationArray2DInterpol;
    res->doInterpolation( !useneighborfld_->isChecked() );
    if ( usemax )
    	res->setMaxDistance( maxdist );
    
    result_ = res;
    return true;
}


void uiArray2DInterpolExtension::initClass()
{
    uiArray2DInterpolSel::factory().addCreator( create,
	    Array2DInterpolExtension::sFactoryKeyword() );
}


uiArray2DInterpol* uiArray2DInterpolExtension::create( uiParent* p )
{ return new uiArray2DInterpolExtension( p ); }


uiArray2DInterpolExtension::uiArray2DInterpolExtension(uiParent* p)
    : uiArray2DInterpol( p, "Extension" )
{
    nrstepsfld_ = new  uiGenInput( this, "Number of steps", IntInpSpec() );
    nrstepsfld_->setValue( 20 );
    setHAlignObj( nrstepsfld_ );
}


bool uiArray2DInterpolExtension::acceptOK()
{
    if ( nrstepsfld_->getIntValue()<1 )
    {
	uiMSG().error( "Nr steps must be > 0." );	
	return false;
    }

    if ( result_ )
	{ delete result_; result_ = 0; }
    
    Array2DInterpolExtension* res = new Array2DInterpolExtension;
    res->setNrSteps( nrstepsfld_->getIntValue() );

    result_ = res;    
    return true;
}


class uiInvDistA2DInterpolPars : public uiDialog
{
public:

uiInvDistA2DInterpolPars( uiInverseDistanceArray2DInterpol* p )
    : uiDialog(p,Setup("Inverse distance - parameters",
		"Inverse distance with search radius","104.0.13") )
    , a2di_(*p)
{
    cornersfirstfld_ = new  uiGenInput( this, "Compute corners first",
					BoolInpSpec(a2di_.cornersfirst_) );

    stepsizefld_ = new uiGenInput( this, "Step size",
	    			   IntInpSpec(a2di_.stepsz_) );
    stepsizefld_->attach( alignedBelow, cornersfirstfld_ );

    nrstepsfld_ = new uiGenInput( this, "[Nr steps]",
	    			  IntInpSpec(a2di_.nrsteps_) );
    nrstepsfld_->attach( alignedBelow, stepsizefld_ );
}

bool acceptOK( CallBacker* )
{
    const int stepsize = stepsizefld_->getIntValue( 0 );
    const int nrsteps = nrstepsfld_->getIntValue( 0 );

    if ( mIsUdf(stepsize) || stepsize<1 )
    {
	uiMSG().error( "Step size must set and > 0. In doubt, use 1." );
	return false;
    }
    if ( (!mIsUdf(nrsteps) && nrsteps<1 ) )
    {
	uiMSG().error( "Nr steps must be > 0. In doubt, leave empty." );
	return false;
    }

    a2di_.cornersfirst_ = cornersfirstfld_->getBoolValue();
    a2di_.stepsz_ = stepsize;
    a2di_.nrsteps_ = nrsteps;
    return true;
}

    uiInverseDistanceArray2DInterpol&	a2di_;
    uiGenInput*				cornersfirstfld_;
    uiGenInput*				stepsizefld_;
    uiGenInput*				nrstepsfld_;

};


void uiInverseDistanceArray2DInterpol::doParamDlg( CallBacker* )
{
    uiInvDistA2DInterpolPars dlg( this );
    dlg.go();
}


bool uiInverseDistanceArray2DInterpol::acceptOK()
{
    if ( result_ )
	{ delete result_; result_ = 0; }

    const bool hasradius = radiusfld_->isChecked();
    const float radius = hasradius ? radiusfld_->getfValue(0) : mUdf(float);

    if ( hasradius && (mIsUdf(radius) || radius<=0) )
    {
	uiMSG().error(
		"Please enter a positive value for the search radius\n"
		"(or uncheck the field)" );
	return false;
    }

    InverseDistanceArray2DInterpol* res = new
	InverseDistanceArray2DInterpol;

    res->setSearchRadius( radius );
    if ( hasradius )
    {
	res->setCornersFirst( cornersfirst_ );
	res->setStepSize( stepsz_ );
	res->setNrSteps( nrsteps_ );
    }

    result_ = res;
    return true;
}

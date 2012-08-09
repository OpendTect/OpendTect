/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID mUnusedVar = "$Id: volstatsattrib.cc,v 1.67 2012-08-09 04:38:06 cvssalil Exp $";

#include "volstatsattrib.h"

#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "attribsteering.h"
#include "statruncalc.h"
#include "survinfo.h"
#include "valseriesinterpol.h"

#define mShapeRectangle		0
#define mShapeEllipse		1
#define mShapeOpticalStack	2

#define mDirLine		0
#define mDirNorm		1

namespace Attrib
{

static int outputtypes[] =
{
    (int)Stats::Average,
    (int)Stats::Median,
    (int)Stats::Variance,
    (int)Stats::Min,
    (int)Stats::Max,
    (int)Stats::Sum,
    (int)Stats::NormVariance,
    (int)Stats::MostFreq,
    (int)Stats::RMS,
    (int)Stats::Extreme,
    -1
};


int* VolStatsBase::outputTypes() const
{
    return outputtypes;
}


void VolStatsBase::initDesc( Desc& desc )
{
    const BinID defstepout( 1, 1 );
    BinIDParam* stepout = new BinIDParam( stepoutStr() );
    stepout->setDefaultValue( defstepout );
    desc.addParam( stepout );

    ZGateParam* gate = new ZGateParam( gateStr() );
    gate->setLimits( Interval<float>(-mLargestZGate,mLargestZGate) );
    gate->setDefaultValue( Interval<float>(-28, 28) );
    desc.addParam( gate );

    IntParam* nrtrcs = new IntParam( nrtrcsStr() );
    nrtrcs->setDefaultValue( 1 );
    nrtrcs->setRequired( false );
    desc.addParam( nrtrcs );

    desc.addInput( InputSpec("Input data",true) );

    InputSpec steeringspec( "Steering data", false );
    steeringspec.issteering_ = true;
    desc.addInput( steeringspec );

    int res =0;
    while ( outputtypes[res++] != -1 )
	desc.addOutputDataType( Seis::UnknowData );
}


void VolStatsBase::updateDefaults( Desc& desc )
{
    ValParam* paramgate = desc.getValParam(gateStr());
    mDynamicCastGet( ZGateParam*, zgate, paramgate )
    float roundedzstep = SI().zStep()*SI().zDomain().userFactor();
    if ( roundedzstep > 0 )
	roundedzstep = (int)( roundedzstep );
    zgate->setDefaultValue( Interval<float>(-roundedzstep*7, roundedzstep*7) );
}


const char* VolStatsBase::shapeTypeStr( int type )
{
    return type==mShapeRectangle ? "Rectangle"
				 : type==mShapeEllipse ? "Ellipse"
				 		       : "OpticalStack";
}

   
VolStatsBase::VolStatsBase( Desc& ds )
    : Provider( ds )
    , positions_(0,BinID(0,0))
    , gate_( 0, 0 )
{
    if ( !isOK() ) return;

    inputdata_.allowNull(true);
    
    mGetBinID( stepout_, stepoutStr() );
    mGetInt( minnrtrcs_, nrtrcsStr() );
    mGetEnum( shape_, shapeStr() );
    mGetFloatInterval( gate_, gateStr() );
    gate_.scale( 1.f/zFactor() );
    gate_.sort();

    BinID pos;
    for ( pos.inl=-stepout_.inl; pos.inl<=stepout_.inl; pos.inl++ )
    {
	for ( pos.crl=-stepout_.crl; pos.crl<=stepout_.crl; pos.crl++ )
	{
	    const float relinldist =
			stepout_.inl ? ((float)pos.inl)/stepout_.inl : 0;
	    const float relcrldist =
			stepout_.crl ? ((float)pos.crl)/stepout_.crl : 0;

	    const float dist2 = relinldist*relinldist + relcrldist*relcrldist;
	    if ( shape_==mShapeEllipse && dist2>1 )
		continue;

	    positions_ += pos;
	}
    }
}


bool VolStatsBase::getInputOutput( int input, TypeSet<int>& res ) const
{
    return Provider::getInputOutput( input, res );
}


bool VolStatsBase::getInputData( const BinID& relpos, int zintv ) 
{
    return Provider::getInputData( relpos, zintv );
}


const BinID* VolStatsBase::desStepout( int inp, int out ) const
{ return inp == 0 ? &stepout_ : 0; }


#define mAdjustGate( cond, gatebound, plus )\
{\
    if ( cond )\
    {\
	int minbound = (int)(gatebound / refstep_);\
	int incvar = plus ? 1 : -1;\
	gatebound = (minbound+incvar) * refstep_;\
    }\
}
    
void VolStatsBase::prepPriorToBoundsCalc()
{
    const int truestep = mNINT32( refstep_*zFactor() );
    if ( truestep == 0 )
	return Provider::prepPriorToBoundsCalc();

    bool chgstartr = mNINT32(gate_.start*zFactor()) % truestep;
    bool chgstopr = mNINT32(gate_.stop*zFactor()) % truestep;
    bool chgstartd = mNINT32(desgate_.start*zFactor()) % truestep;
    bool chgstopd = mNINT32(desgate_.stop*zFactor()) % truestep;

    mAdjustGate( chgstartr, gate_.start, false )
    mAdjustGate( chgstopr, gate_.stop, true )
    mAdjustGate( chgstartd, desgate_.start, false )
    mAdjustGate( chgstopd, desgate_.stop, true )

    Provider::prepPriorToBoundsCalc();
}


const Interval<float>* VolStatsBase::reqZMargin( int inp, int ) const
{ 
    return &gate_; 
}


const Interval<float>* VolStatsBase::desZMargin( int inp, int ) const
{     
    if ( inp || (!desgate_.start && !desgate_.stop)  ) return 0;
    
    return &desgate_;
}




mAttrDefCreateInstance(VolStats)

void VolStats::initClass()
{
    mAttrStartInitClassWithDescAndDefaultsUpdate
    initDesc( *desc );

    EnumParam* shape = new EnumParam( shapeStr() );
    //Note: Ordering must be the same as numbering!
    shape->addEnum( shapeTypeStr(mShapeRectangle) );
    shape->addEnum( shapeTypeStr(mShapeEllipse) );
    shape->addEnum( shapeTypeStr(mShapeOpticalStack) );
    shape->setDefaultValue( mShapeEllipse );
    desc->addParam( shape );

    BoolParam* edgeeffect = new BoolParam( allowEdgeEffStr() );
    edgeeffect->setDefaultValue( true );
    edgeeffect->setRequired( false );
    desc->addParam( edgeeffect );

    BoolParam* steering = new BoolParam( steeringStr() );
    steering->setDefaultValue( false );
    desc->addParam( steering );

    IntParam* optstackstep = new IntParam( optstackstepStr() );
    optstackstep->setDefaultValue( 1 );
    optstackstep->setRequired( false );
    desc->addParam( optstackstep );

    EnumParam* osdir = new EnumParam( optstackdirStr() );
    //Note: Ordering must be the same as numbering!
    osdir->addEnum( optStackDirTypeStr(mDirLine) );
    osdir->addEnum( optStackDirTypeStr(mDirNorm) );
    osdir->setDefaultValue( mDirNorm );
    desc->addParam( osdir );

    mAttrEndInitClass
}


VolStats::VolStats( Desc& ds )
    : VolStatsBase( ds )
    , linepath_(0)
    , linetruepos_(0)
    , allowedgeeffects_( true )
{
    mGetBool( allowedgeeffects_, allowEdgeEffStr() );

    if ( allowedgeeffects_ )
	desgate_ = gate_;

    mGetInt( optstackstep_, optstackstepStr() );
    mGetEnum( optstackdir_, optstackdirStr() );
    mGetBool( dosteer_, steeringStr() );

    if ( shape_ == mShapeOpticalStack )
	stepout_ = BinID( optstackstep_, optstackstep_ );

    if ( dosteer_ )
    {
	float maxso = mMAX(stepout_.inl*inldist(), stepout_.crl*crldist());
	const float maxsecdip = maxSecureDip();
	desgate_.start = gate_.start - maxso*maxsecdip;
	desgate_.stop = gate_.stop + maxso*maxsecdip;

	for ( int idx=0; idx<positions_.size(); idx++ )
	    steerindexes_ += getSteeringIndex( positions_[idx] );
    }
}


VolStats::~VolStats()
{
    if ( linetruepos_ ) delete linetruepos_;
    if ( linepath_ ) delete linepath_;
}


const char* VolStats::optStackDirTypeStr( int type )
{
    return type==mDirLine ? "LineDir" : "NormalToLine";
}


bool VolStats::getInputOutput( int input, TypeSet<int>& res ) const
{
    if ( !dosteer_ || input<inputs_.size()-1 ) 
	return Provider::getInputOutput( input, res );

    for ( int idx=0; idx<positions_.size(); idx++ )
	res += steerindexes_[idx];

    return true;
}


bool VolStats::getInputData( const BinID& relpos, int zintv )
{
    inputdata_.erase();
    if ( shape_ == mShapeOpticalStack )
	reInitPosAndSteerIdxes();

    inputdata_.erase();

    while ( inputdata_.size()<positions_.size() )
	inputdata_ += 0;

    steeringdata_ = dosteer_ ? inputs_[1]->getData( relpos, zintv ) : 0;
    if ( dosteer_ && !steeringdata_ )
	return false;

    const BinID bidstep = inputs_[0]->getStepoutStep();
    const int nrpos = positions_.size();

    int nrvalidtrcs = 0;
    for ( int posidx=0; posidx<nrpos; posidx++ )
    {
	const bool atcenterpos = positions_[posidx] == BinID(0,0);
	const BinID truepos = relpos + positions_[posidx] * bidstep;
	const DataHolder* indata = inputs_[0]->getData( truepos, zintv );
	if ( !indata )
	{
	    if ( atcenterpos )
		return false;
	    else
		continue;
	}

	if ( dosteer_ )
	{
	    const int steeridx = steerindexes_[posidx];
	    if ( !steeringdata_->series(steeridx) )
	    {
		if ( atcenterpos )
		    return false;
		else
		    continue;
	    }
	}

	inputdata_.replace( posidx, indata );
	nrvalidtrcs++;
    }

    if ( nrvalidtrcs < minnrtrcs_ )
	return false;

    dataidx_ = getDataIndex( 0 );
    return true;
}


void VolStats::prepPriorToBoundsCalc()
{
    if ( shape_ == mShapeOpticalStack && (!linepath_ || !linetruepos_) )
    {
	errmsg_ = "Optical Stack only works on elements\n";
	errmsg_ += "which define an horizontal direction:\n";
	errmsg_ += "inlines, crosslines and random lines.";
	return;
    }

    VolStatsBase::prepPriorToBoundsCalc();
}


void VolStats::updateDesc( Desc& desc )
{
    desc.inputSpec(1).enabled_ =
	desc.getValParam(steeringStr())->getBoolValue();

    BufferString shapestr = desc.getValParam(shapeStr())->getStringValue();
    const bool isoptstack = shapestr == shapeTypeStr( mShapeOpticalStack );
    desc.setParamEnabled( stepoutStr(), !isoptstack );
    desc.setParamEnabled( optstackstepStr(), isoptstack );
    desc.setParamEnabled( optstackdirStr(), isoptstack );
}


const Interval<float>* VolStats::reqZMargin( int inp, int ) const
{
    if ( allowedgeeffects_ )
	return 0;

    return &gate_;
}


bool VolStats::computeData( const DataHolder& output, const BinID& relpos,
			    int z0, int nrsamples, int threadid ) const
{
    const int nrpos = positions_.size();
    const Interval<int> samplegate( mNINT32(gate_.start/refstep_), 
				    mNINT32(gate_.stop/refstep_) );
    const int gatesz = samplegate.width() + 1;
    const float extrasamp = output.extrazfromsamppos_/refstep_;

    Stats::CalcSetup rcsetup;
    for ( int outidx=0; outidx<outputinterest_.size(); outidx++ )
    {
	if ( outputinterest_[outidx] )
	    rcsetup.require( (Stats::Type)outputtypes[outidx] );
    }
    int statsz = 0;
    for ( int posidx=0; posidx<nrpos; posidx++ )
    {
	if ( inputdata_[posidx] )
	    statsz += gatesz;
    }

    Stats::WindowedCalc<double> wcalc( rcsetup, statsz );

    for ( int idz=samplegate.start; idz<=samplegate.stop; idz++ )
    {
	for ( int posidx=0; posidx<nrpos; posidx++ )
	{
	    const DataHolder* dh = inputdata_[posidx];
	    if ( !dh ) continue;

	    float shift = extrasamp;
	    if ( dosteer_ )
		shift += getInputValue( *steeringdata_, steerindexes_[posidx],
					idz, z0 );

	    wcalc += getInterpolInputValue( *dh, dataidx_, shift + idz, z0 );
	}
    }

    for ( int isamp=0; isamp<nrsamples; isamp++ )
    {
	if ( isamp )
	{
	    for ( int posidx=0; posidx<nrpos; posidx++ )
	    {
		const DataHolder* dh = inputdata_[posidx];
		if ( !dh ) continue;

		float shift = extrasamp;
		if ( dosteer_ )
		    shift += getInputValue(*steeringdata_,steerindexes_[posidx],
					    isamp+samplegate.stop, z0 );

		const float samplepos = isamp + shift + samplegate.stop;
		wcalc += getInterpolInputValue( *dh, dataidx_, samplepos, z0 );
	    }
	}

        const int nroutp = outputinterest_.size();
	for ( int outidx=0; outidx<nroutp; outidx++ )
	{
	    if ( outputinterest_[outidx] == 0 )
		continue;

	    const float outval = (float) wcalc.getValue(
		    			(Stats::Type)outputtypes[outidx] );
	    setOutputValue( output, outidx, isamp, z0, outval );
	}
    }

    return true;
}


void VolStats::reInitPosAndSteerIdxes()
{
    positions_.erase();
    steerindexes_.erase();

    TypeSet<BinID> truepos;
    getStackPositions( truepos );

    for ( int idx=0; idx<truepos.size(); idx++ )
    {
	const BinID tmpbid = truepos[idx]-currentbid_;
	positions_ += tmpbid;
	steerindexes_ += getSteeringIndex( tmpbid );
    }
}


void VolStats::getStackPositions( TypeSet<BinID>& pos ) const
{
    int curbididx = linepath_->indexOf( currentbid_ );
    if ( curbididx < 0 ) return;

    int trueposidx = -1;
    for ( int idx=1; idx<linetruepos_->size(); idx++ )
    {
	const int prevtrueposidx = linepath_->indexOf( (*linetruepos_)[idx-1] );
	const int nexttrueposidx = linepath_->indexOf( (*linetruepos_)[idx] );
	if ( curbididx>=prevtrueposidx && curbididx<=nexttrueposidx )
	{
	    trueposidx = idx;
	    break;
	}
    }

    if ( trueposidx < 0 ) return;

    const BinID prevpos = (*linetruepos_)[trueposidx-1];
    const BinID nextpos = (*linetruepos_)[trueposidx];
    TypeSet< Geom::Point2D<float> > idealpos;
    getIdealStackPos( currentbid_, prevpos, nextpos, idealpos );

    pos += currentbid_;

    //snap the ideal positions to existing BinIDs
    for ( int idx=0; idx<idealpos.size(); idx++ )
	pos += BinID( mNINT32(idealpos[idx].x), mNINT32(idealpos[idx].y) );
}


void VolStats::getIdealStackPos( 
			const BinID& cpos, const BinID& ppos, const BinID& npos,
			TypeSet< Geom::Point2D<float> >& idealpos ) const
{
    //compute equation (type y=ax+b) of line formed by next and previous pos
    float coeffa = mIsZero(npos.inl-ppos.inl, 1e-3) 
		    ? 0
		    : (float)(npos.crl-ppos.crl) / (float)(npos.inl-ppos.inl);

    bool isinline = false;
    bool iscrossline = false;
    if ( desiredvolume_->isFlat() )
    {
	if ( desiredvolume_->defaultDir() == CubeSampling::Inl )
	    isinline = true;
	else if ( desiredvolume_->defaultDir() == CubeSampling::Crl )
	    iscrossline = true;
    }

    if ( optstackdir_ == mDirNorm && !mIsZero(coeffa,1e-6) )
	coeffa = -1.f/coeffa;

    Geom::Point2D<float> pointa;
    Geom::Point2D<float> pointb;
    if ( (isinline && optstackdir_ == mDirLine)
	|| (iscrossline && optstackdir_ == mDirNorm) )
    {
	pointa = Geom::Point2D<float>( cpos.inl, cpos.crl-optstackstep_ );
	pointb = Geom::Point2D<float>( cpos.inl, cpos.crl+optstackstep_ );
    }
    else if ( (isinline && optstackdir_ == mDirNorm)
	    || (iscrossline && optstackdir_ == mDirLine) )
    {
	pointa = Geom::Point2D<float>( cpos.inl-optstackstep_, cpos.crl );
	pointb = Geom::Point2D<float>( cpos.inl+optstackstep_, cpos.crl );
    }
    else
    {
	const float coeffb = (float)cpos.crl - coeffa * (float)cpos.inl;

	//compute 4 intersections with 'stepout box'
	const Geom::Point2D<float> inter1( cpos.inl - optstackstep_,
				    (cpos.inl-optstackstep_)*coeffa + coeffb );
	const Geom::Point2D<float> inter2( cpos.inl + optstackstep_,
				    (cpos.inl+optstackstep_)*coeffa + coeffb );
	const float interx3 = mIsZero(coeffa,1e-6) ? cpos.inl
				    : (cpos.crl-optstackstep_-coeffb)/coeffa;
	const Geom::Point2D<float> inter3( interx3, cpos.crl - optstackstep_);
	const float interx4 = mIsZero(coeffa,1e-6) ? cpos.inl
				    : (cpos.crl+optstackstep_-coeffb)/coeffa;
	const Geom::Point2D<float> inter4( interx4, cpos.crl + optstackstep_);

	//keep 2 points that cross the 'stepout box'
	pointa = inter1.x>cpos.inl-optstackstep_
		     && inter1.x<cpos.inl+optstackstep_
		     && inter1.y>cpos.crl-optstackstep_
		     && inter1.y<cpos.crl+optstackstep_
			? inter1 : inter3;

	pointb = inter2.x>cpos.inl-optstackstep_
		     && inter2.x<cpos.inl+optstackstep_
		     && inter2.y>cpos.crl-optstackstep_
		     && inter2.y<cpos.crl+optstackstep_
			? inter2 : inter4;
    }

    //compute intermediate points, number determined by optstackstep_
    const float incinl = (pointb.x - pointa.x) / (2*optstackstep_);
    const float inccrl = (pointb.y - pointa.y) / (2*optstackstep_);
    for ( int idx=1; idx<optstackstep_; idx++ )
    {
	idealpos += Geom::Point2D<float>( cpos.inl-incinl*idx,
					  cpos.crl-inccrl*idx );
	idealpos += Geom::Point2D<float>( cpos.inl+incinl*idx,
					  cpos.crl+inccrl*idx );
    }

    idealpos += pointa;
    idealpos += pointb;
}

} // namespace Attrib

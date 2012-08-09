/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Payraudeau
 Date:          June 2005
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: similarityattrib.cc,v 1.64 2012-08-09 04:38:06 cvssalil Exp $";

#include "similarityattrib.h"

#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "attribsteering.h"
#include "genericnumer.h"
#include "statruncalc.h"
#include "survinfo.h"

#define mExtensionNone		0
#define mExtensionRot90		1
#define mExtensionRot180	2
#define mExtensionCube		3
#define mExtensionCross		4
#define mExtensionAllDir	5
#define mExtensionDiagonal	6

namespace Attrib
{

mAttrDefCreateInstance(Similarity)    
    
void Similarity::initClass()
{
    mAttrStartInitClassWithDescAndDefaultsUpdate

    ZGateParam* gate = new ZGateParam( gateStr() );
    gate->setLimits( Interval<float>(-1000,1000) );
    gate->setDefaultValue( Interval<float>(-28, 28) );
    desc->addParam( gate );

    BinIDParam* pos0 = new BinIDParam( pos0Str() );
    pos0->setDefaultValue( BinID(0,1) );    
    desc->addParam( pos0 );
    
    BinIDParam* pos1 = new BinIDParam( pos1Str() );
    pos1->setDefaultValue( BinID(0,-1) );    
    desc->addParam( pos1 );

    BinIDParam* stepout = new BinIDParam( stepoutStr() );
    stepout->setDefaultValue( BinID(1,1) );
    desc->addParam( stepout );

    //Note: Ordering must be the same as numbering!
    EnumParam* extension = new EnumParam( extensionStr() );
    extension->addEnum( extensionTypeStr(mExtensionNone) );
    extension->addEnum( extensionTypeStr(mExtensionRot90) );
    extension->addEnum( extensionTypeStr(mExtensionRot180) );
    extension->addEnum( extensionTypeStr(mExtensionCube) );
    extension->addEnum( extensionTypeStr(mExtensionCross) );
    extension->addEnum( extensionTypeStr(mExtensionAllDir) );
    extension->addEnum( extensionTypeStr(mExtensionDiagonal) );
    extension->setDefaultValue( mExtensionCross );
    desc->addParam( extension );

    BoolParam* steering = new BoolParam( steeringStr() );
    steering->setDefaultValue( true );
    desc->addParam( steering );

    desc->addParam( new BoolParam( normalizeStr(), false, false ) );
    desc->addParam( new BoolParam( browsedipStr(), false, false ) );

    FloatParam* maxdip = new FloatParam( maxdipStr() );
    maxdip->setLimits( Interval<float>(0,mUdf(float)) );
    maxdip->setDefaultValue( 250 );
    desc->addParam( maxdip );

    FloatParam* ddip = new FloatParam( ddipStr() );
    ddip->setLimits( Interval<float>(0,mUdf(float)) );
    ddip->setDefaultValue( 10 );
    desc->addParam( ddip );

    desc->addInput( InputSpec("Input data",true) );
    InputSpec steeringspec( "Steering data", false );
    steeringspec.issteering_ = true;
    desc->addInput( steeringspec );

    desc->setNrOutputs( Seis::UnknowData, 5 );

    desc->setLocality( Desc::MultiTrace );
    mAttrEndInitClass
}


void Similarity::updateDesc( Desc& desc )
{
    BufferString extstr = desc.getValParam(extensionStr())->getStringValue();
    const bool usestep = extstr == extensionTypeStr( mExtensionCube )
			|| extstr == extensionTypeStr( mExtensionCross )
			|| extstr == extensionTypeStr( mExtensionAllDir )
			|| extstr == extensionTypeStr( mExtensionDiagonal );
    const bool dosteer = desc.getValParam(steeringStr())->getBoolValue();

    desc.setParamEnabled( browsedipStr(), !dosteer );
    const bool dobrowsedip = !dosteer &&
			     desc.getValParam(browsedipStr())->getBoolValue();
    desc.setParamEnabled( pos0Str(), !usestep );
    desc.setParamEnabled( pos1Str(), !usestep );
    desc.setParamEnabled( stepoutStr(), usestep );
    desc.setParamEnabled( maxdipStr(), dobrowsedip );
    desc.setParamEnabled( ddipStr(), dobrowsedip );

    desc.inputSpec(1).enabled_ = dosteer;
    
    if ( dobrowsedip ) 
	desc.setNrOutputs( Seis::UnknowData, desc.is2D() ? 6 : 7 );
}


void Similarity::updateDefaults( Desc& desc )
{
    ValParam* paramgate = desc.getValParam(gateStr());
    mDynamicCastGet( ZGateParam*, zgate, paramgate )
    float roundedzstep = SI().zStep()*SI().zDomain().userFactor();
    if ( roundedzstep > 0 )
	roundedzstep = (int)( roundedzstep );
    zgate->setDefaultValue( Interval<float>(-roundedzstep*7, roundedzstep*7) );
}


const char* Similarity::extensionTypeStr( int type )
{
    if ( type==mExtensionNone ) return "None";
    if ( type==mExtensionRot90 ) return "90";
    if ( type==mExtensionRot180 ) return "180";
    if ( type==mExtensionCube ) return "Cube";
    if ( type==mExtensionCross ) return "Cross";
    if ( type==mExtensionAllDir ) return "AllDir";
    return "Diagonal";
}


Similarity::Similarity( Desc& desc )
    : Provider( desc )
    , dobrowsedip_( false )
{
    if ( !isOK() ) return;

    inputdata_.allowNull(true);

    mGetFloatInterval( gate_, gateStr() );
    gate_.scale( 1.f/zFactor() );

    mGetBool( donormalize_, normalizeStr() );
    mGetEnum( extension_, extensionStr() );

    mGetBool( dosteer_, steeringStr() );
    if ( !dosteer_ )
	{ mGetBool( dobrowsedip_, browsedipStr() ) }
   
    if ( dobrowsedip_ )
    {
	mGetFloat( maxdip_, maxdipStr() );
	maxdip_ = maxdip_/dipFactor();
	mGetFloat( ddip_, ddipStr() );
	ddip_ = ddip_/dipFactor();
    }

    if ( extension_>=mExtensionCube )
	mGetBinID( stepout_, stepoutStr() )
    else
    {
	mGetBinID( pos0_, pos0Str() )
	mGetBinID( pos1_, pos1Str() )

	if ( extension_==mExtensionRot90 )
	{
	    int maxstepout = mMAX( mMAX( abs(pos0_.inl), abs(pos1_.inl) ), 
		    		   mMAX( abs(pos0_.crl), abs(pos1_.crl) ) );
	    stepout_ = BinID( maxstepout, maxstepout );
	}
	else
	    stepout_ = BinID(mMAX(abs(pos0_.inl),abs(pos1_.inl)),
			     mMAX(abs(pos0_.crl),abs(pos1_.crl)) );

	    
    }
    getTrcPos();

    float maxdist = dosteer_ || dobrowsedip_ ? 
		mMAX( stepout_.inl*inldist(), stepout_.crl*crldist() ) : 0;
    if ( dobrowsedip_ )		//approx: dip from trc to trc, not central ref
	maxdist *= 2;
    
    const float secdip = dosteer_ ? maxSecureDip() : maxdip_;
    desgate_ = Interval<float>( gate_.start-maxdist*secdip, 
	    			gate_.stop+maxdist*secdip );
}


bool Similarity::getTrcPos()
{
    const bool is2d = desc_.is2D();
    trcpos_.erase();
    if ( extension_==mExtensionCube )
    {
	BinID bid;
	for ( bid.inl=-stepout_.inl; bid.inl<=stepout_.inl; bid.inl++ )
	    for ( bid.crl=-stepout_.crl; bid.crl<=stepout_.crl; bid.crl++ )
		trcpos_ += bid;

	for ( int idx=0; idx<trcpos_.size()-1; idx++ )
	{
	    for ( int idy=idx+1; idy<trcpos_.size(); idy++)
	    {
		pos0s_ += idx;
		pos1s_ += idy;
	    }
	}
    }
    else if ( extension_==mExtensionCross || extension_==mExtensionAllDir
	      || extension_==mExtensionDiagonal )
    {
	trcpos_ += BinID(0,0);

	if ( extension_==mExtensionCross )
	{
	    trcpos_ += BinID(0,stepout_.crl);
	    if ( !is2d ) 
		trcpos_ += BinID(stepout_.inl,0);

	    trcpos_ += BinID(0,-stepout_.crl);
	    if ( !is2d ) 
		trcpos_ += BinID(-stepout_.inl,0);
	}
	else if ( extension_==mExtensionAllDir )
	{
	    trcpos_ += BinID(-stepout_.inl,stepout_.crl);
	    trcpos_ += BinID(0,stepout_.crl);
	    trcpos_ += BinID(stepout_.inl,stepout_.crl);
	    trcpos_ += BinID(stepout_.inl,0);
	    trcpos_ += BinID(stepout_.inl,-stepout_.crl);
	    trcpos_ += BinID(0,-stepout_.crl);
	    trcpos_ += BinID(-stepout_.inl,-stepout_.crl);
	    trcpos_ += BinID(-stepout_.inl,0);
	}
	else
	{
	    trcpos_ += BinID(-stepout_.inl,stepout_.crl);
	    trcpos_ += BinID(stepout_.inl,stepout_.crl);
	    trcpos_ += BinID(stepout_.inl,-stepout_.crl);
	    trcpos_ += BinID(-stepout_.inl,-stepout_.crl);
	}
    }
    else
    {
	trcpos_ += pos0_;
	trcpos_ += pos1_;

	if ( extension_==mExtensionRot90 )
	{
	    trcpos_ += BinID(pos0_.crl,-pos0_.inl);
	    trcpos_ += BinID(pos1_.crl,-pos1_.inl);
	}
	else if ( extension_==mExtensionRot180 )
	{
	    trcpos_ += BinID(-pos0_.inl,-pos0_.crl);
	    trcpos_ += BinID(-pos1_.inl,-pos1_.crl);
	}
    }

    if ( dosteer_ )
    {
	steerindexes_.erase();
	for ( int idx=0; idx<trcpos_.size(); idx++ )
	    steerindexes_ += getSteeringIndex( trcpos_[idx] );
    }

    return true;
}


bool Similarity::getInputOutput( int input, TypeSet<int>& res ) const
{
    if ( !dosteer_ || !input ) return Provider::getInputOutput( input, res );

    for ( int idx=0; idx<trcpos_.size(); idx++ )
	res += steerindexes_[idx];

    return true;
}


bool Similarity::getInputData( const BinID& relpos, int zintv )
{
    while ( inputdata_.size() < trcpos_.size() )
	inputdata_ += 0;

    const BinID bidstep = inputs_[0]->getStepoutStep();
    for ( int idx=0; idx<trcpos_.size(); idx++ )
    {
	const DataHolder* data = 
		    inputs_[0]->getData( relpos+trcpos_[idx]*bidstep, zintv );
	if ( !data ) return false;
	inputdata_.replace( idx, data );
    }
    
    dataidx_ = getDataIndex( 0 );

    steeringdata_ = dosteer_ ? inputs_[1]->getData( relpos, zintv ) : 0;
    if ( dosteer_ && !steeringdata_ )
	return false;

    return true;
}


bool Similarity::computeData( const DataHolder& output, const BinID& relpos, 
			      int z0, int nrsamples, int threadid ) const
{
    if ( inputdata_.isEmpty() ) return false;

    const Interval<int> samplegate( mNINT32(gate_.start/refstep_),
				    mNINT32(gate_.stop/refstep_) );

    const int gatesz = samplegate.width() + 1;

    int nrpairs;
    const bool iscubeext = extension_==mExtensionCube;
    const bool isalldirext = extension_==mExtensionAllDir;
    const bool isperpendicularext = extension_==mExtensionCross
				|| extension_==mExtensionDiagonal;
    const bool iscenteredext = isalldirext || isperpendicularext;

    if ( iscubeext )
	nrpairs = pos0s_.size();
    else if ( isalldirext )
	nrpairs = 8;
    else if ( isperpendicularext )
	nrpairs = desc_.is2D() ? 2 : 4;
    else
	nrpairs = inputdata_.size()/2;

    const int firstsample = inputdata_[0] ? z0-inputdata_[0]->z0_ : z0;

    Stats::CalcSetup rcsetup;
    if ( outputinterest_[0] ) rcsetup.require( Stats::Average );
    if ( outputinterest_[1] ) rcsetup.require( Stats::Median );
    if ( outputinterest_[2] ) rcsetup.require( Stats::Variance );
    if ( outputinterest_[3] ) rcsetup.require( Stats::Min );
    if ( outputinterest_[4] ) rcsetup.require( Stats::Max );
    Stats::RunCalc<float> stats( rcsetup );

    float extrazfspos = mUdf(float);
    if ( needinterp_ )
	extrazfspos = getExtraZFromSampInterval( z0, nrsamples );

    for ( int idx=0; idx<nrsamples; idx++ )
    {
	stats.clear();
	float inldip = 0;
	float crldip = 0;
	float dipatmax = 0;
	for ( int pair=0; pair<nrpairs; pair++ )
	{
	    const int idx0 = iscubeext ? pos0s_[pair]
				       : iscenteredext ? 0 : pair*2;
	    const int idx1 = iscubeext ? pos1s_[pair]
				       : iscenteredext ? pair+1 : pair*2 +1;

	    float bases0 = firstsample + idx + samplegate.start;
	    float bases1 = bases0;

	    if ( !inputdata_[idx0] || !inputdata_[idx1] )
		continue;

	    float dist = 0;
	    if ( dobrowsedip_ )
	    {
		float di = abs(trcpos_[idx1].inl - trcpos_[idx0].inl)*inldist();
		float dc = abs(trcpos_[idx1].crl - trcpos_[idx0].crl)*crldist();
		dist = Math::Sqrt( di*di + dc*dc );
	    }

	    float s0 = bases0;
	    float s1 = bases1;
	    bool docontinue = true;
	    float maxsimi = 0;
	    dipatmax = 0;
	    float curdip = -maxdip_;
	    while ( docontinue )	//loop necessary for dip browser
	    {
		if ( dosteer_ )
		{
		    ValueSeries<float>* serie0 = 
			    steeringdata_->series( steerindexes_[idx0] );
		    if ( serie0 )
			s0 = bases0 + serie0->value( z0+idx-steeringdata_->z0_);

		    ValueSeries<float>* serie1 = 
			    steeringdata_->series( steerindexes_[idx1] );
		    if ( serie1 )
			s1 = bases1 + serie1->value( z0+idx-steeringdata_->z0_);
		}

		if ( dobrowsedip_ )
		    s1 = bases1 + (curdip * dist)/refstep_;


		//make sure data extracted from input DataHolders is at exact z
		float extras0 = mIsUdf(extrazfspos) ? 0 :
		    (extrazfspos-inputdata_[idx0]->extrazfromsamppos_)/refstep_;
		float extras1 = mIsUdf(extrazfspos) ? 0 :
		    (extrazfspos-inputdata_[idx1]->extrazfromsamppos_)/refstep_;

		SimiFunc vals0( *(inputdata_[idx0]->series(dataidx_)), 
				inputdata_[idx0]->nrsamples_-1 );
		SimiFunc vals1( *(inputdata_[idx1]->series(dataidx_)), 
				inputdata_[idx1]->nrsamples_-1 );
		const bool valids0 = s0>=0 && 
				     (s0+gatesz)<=inputdata_[idx0]->nrsamples_;
		if ( !valids0 ) s0 = firstsample + idx + samplegate.start;

		const bool valids1 = s1>=0 && 
				     (s1+gatesz)<=inputdata_[idx1]->nrsamples_;
		if ( !valids1 ) s1 = firstsample + idx + samplegate.start;


		float simival = similarity( vals0, vals1, s0+extras0,
					    s1+extras1, 1, gatesz,donormalize_);
		
		if ( dobrowsedip_ )
		{
		    curdip += ddip_;
		    if ( simival > maxsimi )
		    {
			maxsimi = simival;
			dipatmax = curdip;
		    }
		}
		else
		    stats += simival;

		docontinue = dobrowsedip_ ? curdip<maxdip_ : false;
		if ( dobrowsedip_ && !docontinue )
		    stats += maxsimi;
	    }

	    if ( dobrowsedip_ )
	    {
		const bool hasoutput = outputinterest_[5] ||
			(!desc_.is2D() && outputinterest_[6]);
		if ( hasoutput )
		{ 
		    if ( !pair || pair==2 || desc_.is2D() )
			crldip = pair ? (crldip + dipatmax)/2 : dipatmax;
		    else
			inldip = pair==1 ? (inldip + dipatmax)/2 : dipatmax;
		}
	    }
	}

	if ( stats.size() < 1 )
	{
	    for ( int sidx=0; sidx<outputinterest_.size(); sidx++ )
		setOutputValue( output, sidx, idx, z0, 0 );
	}
	else
	{
	    if ( outputinterest_[0] )
		setOutputValue( output, 0, idx, z0, (float) stats.average() );
	    if ( outputinterest_[1] )
		setOutputValue( output, 1, idx, z0, stats.median() );
	    if ( outputinterest_[2] ) 
		setOutputValue( output, 2, idx, z0, (float) stats.variance() );
	    if ( outputinterest_[3] )
		setOutputValue( output, 3, idx, z0, stats.min() );
	    if ( outputinterest_[4] )
	       	setOutputValue( output, 4, idx, z0, stats.max() );
	    if ( dobrowsedip_ )
	    {
		if ( outputinterest_[5] )
		    setOutputValue( output, 5, idx, z0,
				    desc_.is2D() ? crldip*dipFactor()
						 : inldip*dipFactor() );
		if ( !desc_.is2D() && outputinterest_[6] )
		    setOutputValue( output, 6, idx, z0, crldip*dipFactor() );
	    }
	}
    }

    return true;
}


const BinID* Similarity::reqStepout( int inp, int out ) const
{ return inp ? 0 : &stepout_; }


#define mAdjustGate( cond, gatebound, plus )\
{\
    if ( cond )\
    {\
	int minbound = (int)(gatebound / refstep_);\
	int incvar = plus ? 1 : -1;\
	gatebound = (minbound+incvar) * refstep_;\
    }\
}

void Similarity::prepPriorToBoundsCalc()
{
     const int truestep = mNINT32( refstep_*zFactor() );
     if ( truestep == 0 )
       	 return Provider::prepPriorToBoundsCalc();

    bool chgstartr = mNINT32(gate_.start*zFactor()) % truestep ; 
    bool chgstopr = mNINT32(gate_.stop*zFactor()) % truestep;
    bool chgstartd = mNINT32(desgate_.start*zFactor()) % truestep;
    bool chgstopd = mNINT32(desgate_.stop*zFactor()) % truestep;

    mAdjustGate( chgstartr, gate_.start, false )
    mAdjustGate( chgstopr, gate_.stop, true )
    mAdjustGate( chgstartd, desgate_.start, false )
    mAdjustGate( chgstopd, desgate_.stop, true )

    Provider::prepPriorToBoundsCalc();
}


const Interval<float>* Similarity::reqZMargin( int inp, int ) const
{
    return inp ? 0 : &gate_;
}


const Interval<float>* Similarity::desZMargin( int inp, int ) const
{
    return inp ? 0 : &desgate_;
}


}; //namespace

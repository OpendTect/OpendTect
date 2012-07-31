/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          January 2008
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: semblanceattrib.cc,v 1.12 2012-07-31 04:15:50 cvskris Exp $";

#include "semblanceattrib.h"

#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "attribsteering.h"
#include "genericnumer.h"
#include "varlenarray.h"

#define mExtensionNone		0
#define mExtensionRot90		1
#define mExtensionRot180	2
#define mExtensionCube		3

namespace Attrib
{

mAttrDefCreateInstance(Semblance)    
    
void Semblance::initClass()
{
    mAttrStartInitClassWithUpdate

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
    extension->setDefaultValue( mExtensionRot90 );
    desc->addParam( extension );

    BoolParam* steering = new BoolParam( steeringStr() );
    steering->setDefaultValue( true );
    desc->addParam( steering );

    desc->addInput( InputSpec("Input data",true) );
    desc->addOutputDataType( Seis::UnknowData );

    InputSpec steeringspec( "Steering data", false );
    steeringspec.issteering_ = true;
    desc->addInput( steeringspec );

    mAttrEndInitClass
}


void Semblance::updateDesc( Desc& desc )
{
    BufferString extstr = desc.getValParam(extensionStr())->getStringValue();
    const bool iscube = extstr == extensionTypeStr( mExtensionCube );
    desc.setParamEnabled( pos0Str(), !iscube );
    desc.setParamEnabled( pos1Str(), !iscube );
    desc.setParamEnabled( stepoutStr(), iscube );

    desc.inputSpec(1).enabled_ = desc.getValParam(steeringStr())->getBoolValue();
}


const char* Semblance::extensionTypeStr( int type )
{
    if ( type==mExtensionNone ) return "None";
    if ( type==mExtensionRot90 ) return "90";
    if ( type==mExtensionRot180 ) return "180";
    return "Cube";
}


Semblance::Semblance( Desc& desc )
    : Provider( desc )
{
    if ( !isOK() ) return;

    inputdata_.allowNull(true);

    mGetFloatInterval( gate_, gateStr() );
    gate_.scale( 1./zFactor() );

    mGetBool( dosteer_, steeringStr() );
    mGetEnum( extension_, extensionStr() );
    if ( extension_==mExtensionCube )
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

    const float maxdist = dosteer_ ? 
	mMAX( stepout_.inl*inldist(), stepout_.crl*crldist() ) : 0;
    
    const float maxsecdip = maxSecureDip();
    desgate_ = Interval<float>( gate_.start-maxdist*maxsecdip, 
	    			gate_.stop+maxdist*maxsecdip );
}


bool Semblance::getTrcPos()
{
    trcpos_.erase();
    if ( extension_==mExtensionCube )
    {
	BinID bid;
	for ( bid.inl=-stepout_.inl; bid.inl<=stepout_.inl; bid.inl++ )
	    for ( bid.crl=-stepout_.crl; bid.crl<=stepout_.crl; bid.crl++ )
		trcpos_ += bid;
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


bool Semblance::getInputOutput( int input, TypeSet<int>& res ) const
{
    if ( !dosteer_ || !input ) return Provider::getInputOutput( input, res );

    for ( int idx=0; idx<trcpos_.size(); idx++ )
	res += steerindexes_[idx];

    return true;
}


bool Semblance::getInputData( const BinID& relpos, int zintv )
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


bool Semblance::computeData( const DataHolder& output, const BinID& relpos, 
			      int z0, int nrsamples, int threadid ) const
{
    if ( inputdata_.isEmpty() ) return false;

    const Interval<int> samplegate( mNINT32(gate_.start/refstep_),
				    mNINT32(gate_.stop/refstep_) );

    const int gatesz = samplegate.width() + 1;
    const int nrtraces = inputdata_.size();

    mAllocVarLenArr( float, cache, nrtraces*gatesz );
    ObjectSet<float> semblanceinput;

    for ( int idx=0; idx<nrsamples; idx++ )
    {
	int offset = 0;
	semblanceinput.erase();

	for ( int trcidx=0; trcidx<nrtraces; trcidx++ )
	{
	    semblanceinput += cache+offset;

	    ValueSeries<float>* serie = dosteer_ 
		? steeringdata_->series( steerindexes_[trcidx] )
		: 0;

	    const DataHolder* data = inputdata_[trcidx];
	    for ( int zidx=samplegate.start; zidx<=samplegate.stop ; zidx++ )
	    {
		float sampleidx = idx + zidx;
		if ( serie )
		    sampleidx += serie->value(z0+idx-steeringdata_->z0_);

		float traceval = mUdf(float);
		if ( data )
		{
		    traceval = dosteer_
			? getInterpolInputValue( *data, dataidx_, sampleidx, z0 )
			: getInputValue( *data, dataidx_, (int)sampleidx, z0 );
		}

		cache[offset] = traceval;
		offset++;
	    }
	}

	const float semb = semblance( semblanceinput, Interval<int>(0,gatesz-1) );

	setOutputValue( output, 0, idx, z0, semb );
    }

    return true;
}


const BinID* Semblance::reqStepout( int inp, int out ) const
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

void Semblance::prepPriorToBoundsCalc()
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


const Interval<float>* Semblance::reqZMargin( int inp, int ) const
{
    return inp ? 0 : &gate_;
}


const Interval<float>* Semblance::desZMargin( int inp, int ) const
{
    return inp ? 0 : &desgate_;
}


}; //namespace

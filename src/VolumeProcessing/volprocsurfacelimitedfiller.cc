/*+
 *(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 *Author:	Y.C. Liu
 *Date:		April 2010
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "volprocsurfacelimitedfiller.h"

#include "arraynd.h"
#include "binidvalset.h"
#include "emfault3d.h"
#include "emhorizon.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emsurfaceauxdata.h"
#include "separstr.h"
#include "ioman.h"
#include "ioobj.h"
#include "mousecursor.h"
#include "survinfo.h"

namespace VolProc
{

#define mBelow		1
#define mAbove		-1


static const char*	sKeyTopHorID()		{ return "Top horizon"; }
static const char*	sKeyBotHorID()		{ return "Bottom horizon"; }
static const char*	sKeyTopValue()		{ return "Top Value"; }
static const char*	sKeyBotValue()		{ return "Bottom Value"; }
static const char*	sKeyGradient()		{ return "Gradient"; }
static const char*	sKeyUseGradient()	{ return "Use Gradient"; }
static const char*	sKeyHorInterFiller()	{ return "HorInterFiller"; }

    
SurfaceLimitedFiller::SurfaceLimitedFiller()
    : fixedstartval_( 2000 )
    , fixedgradient_( mCast(float,SI().zDomain().userFactor()) )
    , refz_( 0 )
    , gradhorizon_( 0 )
    , refhorizon_( 0 )
    , starthorizon_( 0 )
    , gradhormid_( 0 )
    , starthormid_( 0 )
    , refhormid_( 0 )
    , usebottomval_( false  )
    , usegradient_( true )
    , usestartval_( true )
    , userefz_( true )
    , gradvertical_( true )
    , startauxdataidx_( -1 )
    , gradauxdataidx_( -1 )
{
    hors_.allowNull( true );
    faults_.allowNull( true );
}


void SurfaceLimitedFiller::initClass()
{
    SeparString sep( 0, FactoryBase::cSeparator() );
    sep += sFactoryKeyword();
    sep += sKeyHorInterFiller();

    SurfaceLimitedFiller::factory().addCreator( createInstance, sep.buf(), 
						sFactoryDisplayName() );
}


SurfaceLimitedFiller::~SurfaceLimitedFiller()
{
    releaseData();
}
 

void SurfaceLimitedFiller::releaseData()
{
    Step::releaseData();
    
    deepUnRef( hors_ );
    deepErase( faults_ );
    
    if ( starthorizon_ ) starthorizon_->unRef();
    if ( refhorizon_ ) refhorizon_->unRef();
    if ( gradhorizon_ ) gradhorizon_->unRef();

    starthorizon_ = 0;
    refhorizon_ = 0;
    gradhorizon_ = 0;
}    


const MultiID* SurfaceLimitedFiller::getSurfaceID( int idx ) const
{ return surfacelist_.validIdx(idx) ? &surfacelist_[idx] : 0; }


const MultiID* SurfaceLimitedFiller::getStartValueHorizonID() const
{ return starthormid_ && !starthormid_.isEmpty() ? &starthormid_ : 0; }


const MultiID* SurfaceLimitedFiller::getGradientHorizonID() const
{ return gradhormid_ && !gradhormid_.isEmpty() ? &gradhormid_ : 0; }


const MultiID* SurfaceLimitedFiller::getRefHorizonID() const
{ return refhormid_ && !refhormid_.isEmpty() ? &refhormid_ : 0; }


#define mRetSetBinID( targetmid ) \
    if ( mid->isEmpty() ) \
	return false; \
    targetmid = *mid; \
    return true 


bool SurfaceLimitedFiller::setStartValueHorizon( const MultiID* mid )
{ mRetSetBinID( starthormid_ ); }


bool SurfaceLimitedFiller::setGradientHorizon( const MultiID* mid )
{ mRetSetBinID( gradhormid_ ); }


bool SurfaceLimitedFiller::setRefHorizon( const MultiID* mid )
{ mRetSetBinID( refhormid_ ); }


bool SurfaceLimitedFiller::setSurfaces( const TypeSet<MultiID>& hids,
				      const TypeSet<char>& geofillside )
{
    deepUnRef( hors_ );
    side_.erase();
    surfacelist_.erase();

    if ( !hids.size() )
	return true;

    for ( int idx=0; idx<hids.size(); idx++ )
    {
	PtrMan<IOObj> ioobj = IOM().get( hids[idx] );
	if ( !ioobj )	
	{
	    errmsg_ = "Object does not exist, could not find entry for ID: ";
	    errmsg_ += hids[idx];
	    return false;
	}

	surfacelist_ += hids[idx];
	side_ += geofillside[idx];
    }

    return side_.size();
}    


EM::Horizon* SurfaceLimitedFiller::loadHorizon( const MultiID& mid ) const
{
    RefMan<EM::EMObject> emobj = EM::EMM().loadIfNotFullyLoaded( mid );
    mDynamicCastGet( EM::Horizon*, newhor, emobj.ptr() );
    if ( !newhor ) return 0;

    newhor->ref();
    return newhor;
}


#define mSetDataHorizon( mid, hor3d, auxdataidx ) \
    if ( mid.isEmpty() ) return false; \
    EM::Horizon* hor = loadHorizon( mid ); \
    if ( !hor ) return false; \
    mDynamicCastGet( EM::Horizon3D*, newhor, hor ); \
    if ( !newhor ) \
    { \
	hor->unRef(); \
	return false; \
    } \
    hor3d = newhor; \
    EM::SurfaceAuxData surfad( *hor3d ); \
    Executor* loader = surfad.auxDataLoader( auxdataidx ); \
    if ( !loader || !loader->execute() ) \
    { \
	hor3d->unRef(); \
	return false; \
    } \




bool SurfaceLimitedFiller::prepareComp( int )
{
    MouseCursorChanger cursorlock( MouseCursor::Wait );

    if ( !usestartval_ )
    {
	mSetDataHorizon( starthormid_, starthorizon_, startauxdataidx_ );
    }

    if ( !usegradient_ )
    {
	mSetDataHorizon( gradhormid_, gradhorizon_, gradauxdataidx_ );
    }

    if ( !userefz_ )
    {
	if ( refhormid_.isEmpty() ) 
	    return false; 
    
	EM::Horizon* hor = loadHorizon( refhormid_ ); 
    	if ( !hor ) return false;

	refhorizon_ = hor;
    }
    
    for ( int idx=0; idx<surfacelist_.size(); idx++ )
    {
	RefMan<EM::EMObject> emobj =
	    EM::EMM().loadIfNotFullyLoaded( surfacelist_[idx] );
	mDynamicCastGet( EM::Horizon*, newhor, emobj.ptr() );
	if ( newhor )
	{
	    newhor->ref();
	    hors_ += newhor;
	    faults_ += 0;
	}
	else
	{
	    mDynamicCastGet( EM::Fault3D*, newft, emobj.ptr() );
	    faults_ += newft ? newft->geometry().sectionGeometry(0) : 0;
	    hors_ += 0;
	}
    }
    
    if ( !output_ || !output_->nrCubes() )
	return false;

    return isOK(); 
}


bool SurfaceLimitedFiller::computeBinID( const BinID& bid, int )
{
    const Array3D<float>* inputarr = 
	input_ && input_->nrCubes() ? &input_->getCube( 0 ) : 0;

    const StepInterval<int> outputinlrg( output_->inlsampling_.start,
	    output_->inlsampling_.atIndex( output_->getInlSz()-1 ),
	    output_->inlsampling_.step );

    if ( !outputinlrg.includes( bid.inl(), false ) ||
         (bid.inl()-outputinlrg.start)%outputinlrg.step )
	return false;

    const StepInterval<int> outputcrlrg( output_->crlsampling_.start,
	    output_->crlsampling_.atIndex( output_->getCrlSz()-1 ),
	    output_->crlsampling_.step );

    if ( !outputcrlrg.includes( bid.crl(), false ) ||
         (bid.crl()-outputcrlrg.start)%outputcrlrg.step )
	return false;

    StepInterval<int> inputinlrg;
    if ( inputarr )
    {
	inputinlrg = input_->inlsampling_.interval( input_->getInlSz() );
	if ( !inputinlrg.includes( bid.inl(), false ) ||
	     (bid.inl()-inputinlrg.start)%inputinlrg.step )
	    inputarr = 0;
    }

    StepInterval<int> inputcrlrg;
    if ( inputarr )
    {
	inputcrlrg = input_->crlsampling_.interval( input_->getCrlSz() );
	if ( !inputcrlrg.includes( bid.crl(), false ) ||
	     (bid.crl()-inputcrlrg.start)%inputcrlrg.step )
	    inputarr = 0;
    }

    const od_int64 bidsq = bid.toInt64();
    const double fixedz = userefz_ ? refz_ : 
	refhorizon_->getPos( refhorizon_->sectionID(0), bidsq ).z;

    double val0 = fixedstartval_;
    if ( !usestartval_ )
    {	
	EM::PosID pid(starthorizon_->id(), starthorizon_->sectionID(0), bidsq);
	val0 = starthorizon_->auxdata.getAuxDataVal( 0, pid );
    }

    TypeSet<double> horz;
    bool allhordefined = true;
    for ( int idy=0; idy<hors_.size(); idy++ )
    {
	if ( !hors_[idy] ) continue;

	horz += hors_[idy]->getPos(hors_[idy]->sectionID(0),bidsq).z;
	if ( mIsUdf(horz[idy]) )
	{
	    allhordefined = false;
	    break;
	}
    }

    double gradient = fixedgradient_; //gradvertical_==true case done
    if ( !usegradient_ )
    {
	EM::PosID pid( gradhorizon_->id(), gradhorizon_->sectionID(0), bidsq );
	gradient = gradhorizon_->auxdata.getAuxDataVal( 0, pid );
    }
    else if ( usebottomval_ )
    {
	const StepInterval<float>& zrg = SI().zRange( true );
	const double topdepth = horz.size() > 0 && !mIsUdf(horz[0]) ? 
				horz[0] : zrg.start;
	const double bottomdepth = horz.size() > 1 && !mIsUdf(horz[1]) ? 
				   horz[1] : zrg.stop;
	const double depth = bottomdepth - topdepth;
	gradient = valrange_ / depth;
    }
    
    const int inputinlidx = inputarr ? inputinlrg.nearestIndex( bid.inl() ) : -1;
    const int inputcrlidx = inputarr ? inputcrlrg.nearestIndex( bid.crl() ) : -1;
    const int outputinlidx = outputinlrg.nearestIndex( bid.inl() );
    const int outputcrlidx = outputcrlrg.nearestIndex( bid.crl() );
    const int outputmaxidx = output_->getZSz()-1;
    const bool initok = !mIsUdf(val0) && !mIsUdf(gradient) && !mIsUdf(fixedz);

    for ( int idx=outputmaxidx; idx>=0; idx-- )
    {
	const double curz = ( output_->z0_ + idx ) * output_->zstep_;
	bool cancalculate = allhordefined;
	if ( allhordefined )
	{
	    for ( int idy=0; idy<horz.size(); idy++ )
	    {	
		if ( (horz[idy]>curz && side_[idy]==mBelow) || 
		     (horz[idy]<curz && side_[idy]==mAbove) )
		{
		    cancalculate = false;
		    break;
		}
	    }
	}

    	double value = mUdf(double);
	if ( cancalculate && initok ) 
	    value = val0 + ( curz - fixedz ) * gradient;
	else if ( inputarr )
	    value = (float) inputarr->get( inputinlidx, inputcrlidx, idx );
	
	output_->getCube(0).set( outputinlidx, outputcrlidx, idx, 
				 (float) value );
    } 

    return true;
}


void SurfaceLimitedFiller::fillPar( IOPar& pars ) const
{
    Step::fillPar( pars );

    pars.set( sKeyNrSurfaces(), side_.size() );
    for ( int idx=0; idx<side_.size(); idx++ )
    {
	BufferString midkey = sKeySurfaceID();
	midkey += idx;
	pars.set( midkey, surfacelist_[idx] );
	
	BufferString sidekey = sKeySurfaceFillSide();
	sidekey += idx;
	pars.set( sidekey, side_[idx] );
    }

    pars.setYN( sKeyUseStartValue(), usestartval_ );
    if ( usestartval_ )
	pars.set( sKeyStartValue(), fixedstartval_ );
    else 
    {
	pars.set( sKeyStartValHorID(), starthormid_ );
	pars.set( sKeyStartAuxDataID(), startauxdataidx_ );
    }
    
    pars.setYN( sKeyUseGradValue(), usegradient_ );
    pars.setYN( sKeyGradType(), gradvertical_ );
    if ( usegradient_ )
	pars.set( sKeyGradValue(), fixedgradient_ );
    else 
    {
	pars.set( sKeyGradHorID(), gradhormid_ );
	pars.set( sKeyGradAuxDataID(), gradauxdataidx_ );
    }
    
    pars.setYN( sKeyUseRefZ(), userefz_ );
    if ( userefz_ )
	pars.set( sKeyRefZ(), refz_ );
    else 
	pars.set( sKeyRefHorID(), refhormid_ );
}


bool SurfaceLimitedFiller::useHorInterFillerPar( const IOPar& pars )
{
    MouseCursorChanger cursorlock( MouseCursor::Wait );
    if ( !Step::usePar( pars ) )
	return false;

    usestartval_ = true; usegradient_ = true;
    userefz_ = true; refz_ = 0;

    float topvalue = mUdf(float);
    if ( pars.get(sKeyTopValue(),topvalue) && !mIsUdf(topvalue) )
	fixedstartval_ = topvalue;

    bool usegradient = true; float gradient = mUdf(float);
    pars.getYN( sKeyUseGradient(), usegradient );
    if (usegradient && pars.get(sKeyGradient(),gradient) && !mIsUdf(gradient))
	fixedgradient_ = gradient;

    TypeSet<MultiID> horids;
    MultiID tophorid;
    if ( pars.get(sKeyTopHorID(), tophorid) )
    {
	horids += tophorid;
	userefz_ = false;
	if ( !setRefHorizon( &tophorid ) )
	    return false;
    }

    MultiID bottomhorid;
    if ( pars.get(sKeyBotHorID(), bottomhorid) )
	horids += bottomhorid;

    float bottomvalue = mUdf(float);
    if ( !usegradient && pars.get(sKeyBotValue(),bottomvalue) && 
	 !mIsUdf(bottomvalue) )
    {
	valrange_ = bottomvalue - topvalue;
	const StepInterval<float>& zrange = SI().zRange(true);
	fixedgradient_ = mCast(float,valrange_/(zrange.stop-zrange.start));
	if ( horids.size() > 0 )
	    usebottomval_ = true;
    }

    TypeSet<char> sides;
    for ( int idx=0; idx<horids.size(); idx++ )
	sides += mCast(char,idx?mAbove:mBelow);

    if ( !setSurfaces(horids, sides) )
	return false;

    return true;
}


bool SurfaceLimitedFiller::usePar( const IOPar& pars )
{
    BufferString type;
    pars.get( sKey::Type(), type );
    if ( type == sKeyHorInterFiller() )
	return useHorInterFillerPar( pars );

    MouseCursorChanger cursorlock( MouseCursor::Wait );
    if ( !Step::usePar( pars ) )
	return false;

    int nrhors = 0;
    pars.get( sKeyNrSurfaces(), nrhors );
    TypeSet<MultiID> horbds;
    TypeSet<char> sides;
    for ( int idx=0; idx<nrhors; idx++ )
    {
	BufferString midkey = sKeySurfaceID();
	midkey += idx;

	MultiID mid;
	pars.get( midkey, mid );
	horbds += mid;

	BufferString sidekey = sKeySurfaceFillSide();
	sidekey += idx;
	
	int side;
	pars.get( sidekey, side );
	sides += mCast( char , side ); 
    }

    if ( !setSurfaces(horbds, sides) )
	return false;

    pars.getYN( sKeyUseStartValue(), usestartval_ );
    if ( usestartval_ )
	pars.get( sKeyStartValue(), fixedstartval_ );
    else
    {
	MultiID mid;
	if ( !pars.get(sKeyStartValHorID(),mid) )
	    return false;

	if ( !setStartValueHorizon( &mid ) )
	    return false;

	if ( !pars.get(sKeyStartAuxDataID(),startauxdataidx_) )
	    return false;
    }
    
    pars.getYN( sKeyGradType(), gradvertical_ );
    pars.getYN( sKeyUseGradValue(), usegradient_ );
    if ( usegradient_ )
	pars.get( sKeyGradValue(), fixedgradient_ );
    else
    {
	MultiID mid;
	if ( !pars.get(sKeyGradHorID(),mid) )
	    return false;

	if ( !setGradientHorizon( &mid ) )
	    return false;

	if ( !pars.get(sKeyGradAuxDataID(),gradauxdataidx_) )
	    return false;
    }
    
    pars.getYN( sKeyUseRefZ(), userefz_ );
    if ( userefz_ )
	pars.get( sKeyRefZ(), refz_ );
    else
    {
	MultiID mid;
	if ( !pars.get(sKeyRefHorID(),mid) )
	    return false;

	if ( !setRefHorizon( &mid ) )
	    return false;
    }
    
    return true;
}


bool SurfaceLimitedFiller::isOK() const
{
    if ( usegradient_ && mIsUdf(fixedgradient_) )
	return false;

    if ( !usegradient_ && !gradhorizon_->auxdata.nrAuxData() )
	return false;

    if ( usestartval_ && mIsUdf(fixedstartval_) )
	return false;

    if ( !usestartval_ && !starthorizon_->auxdata.nrAuxData() )
	return false;

    if ( userefz_ && mIsUdf(refz_) )
	return false;

    if ( !userefz_ && !refhorizon_ )
	return false;
    
    return true;
}

}; //namespace

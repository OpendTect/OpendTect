/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
___________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "faultadjuster.h"

#include "attribsel.h"
#include "cubicbeziercurve.h"
#include "emfault3d.h"
#include "iopar.h"
#include "parametricsurface.h"
#include "survinfo.h"


namespace MPE 
{

const char* FaultAdjuster::sKeyTrackMax() { return "Track Maximum"; }


FaultAdjuster::FaultAdjuster( EM::Fault3D& flt, const EM::SectionID& sid )
    : SectionAdjuster(sid)
    , fault_(flt)
    , attribsel( *new Attrib::SelSpec )
{
    //computers_ += new AttribPositionScoreComputer();
}


FaultAdjuster::~FaultAdjuster()
{
    delete &attribsel; 
}



void FaultAdjuster::reset()
{}


int FaultAdjuster::nextStep()
{
    /*
    mDynamicCastGet(const AttribPositionScoreComputer*,attr,getComputer(0));
    const PositionScoreComputerAttribData* attrdata = attr->getAttribData();
    for ( int idx=0; idx<pids_.size(); idx++ )
    {
	EM::SubID subid = pids_[idx];
	const Coord3& pos = fault_.getPos( sectionid_, subid );

	TypeSet<BinID> targetbids;
	getTargetPositions( subid, pidsrc_.size()?&pidsrc_[idx]:0, targetbids );
	if ( targetbids.isEmpty() ) continue;

	float minmaxval = attr->trackHigh() ? -mUndefValue : mUndefValue;
	int minmaxidx = -1;
	for ( int targetidx=0; targetidx<targetbids.size(); targetidx++ )
	{
	    float val = attrdata->getValueByZ( 0, targetbids[targetidx], pos.z);
	    
	    if ( (attr->trackHigh() && val > minmaxval) || 
		 (!attr->trackHigh() && val < minmaxval) )   
	    {
		minmaxval = val;
		minmaxidx = targetidx;
	    }
	}

	RowCol rc( subid );
	Coord crd = SI().transform( targetbids[minmaxidx] );
	fault_.setPos( sectionid_, rc.toInt64(), Coord3(crd,pos.z), true );
    }
    */

    return 0;
}


void FaultAdjuster::getNeededAttribs(
	ObjectSet<const Attrib::SelSpec>& specs ) const
{
    for ( int idx=specs.size()-1; idx>=0; idx-- )
    {
	if ( *specs[idx]==attribsel )
	    return;
    }

    specs += &attribsel;
}


CubeSampling FaultAdjuster::getAttribCube( const Attrib::SelSpec& spec ) const
{
    if ( spec!=attribsel )
	return SectionAdjuster::getAttribCube( spec );

    //TODO: make real impl
    return SectionAdjuster::getAttribCube( spec );
}


int FaultAdjuster::getNrAttributes() const { return 1; }


const Attrib::SelSpec* FaultAdjuster::getAttributeSel( int idx ) const
{ return idx ? 0 : &attribsel; }


void FaultAdjuster::setAttributeSel( int idx, const Attrib::SelSpec& spec )
{
    if ( idx ) return;

    attribsel = spec;
}


void FaultAdjuster::fillPar( IOPar& iopar ) const
{
    SectionAdjuster::fillPar( iopar );
    IOPar adjpar;
    attribsel.fillPar( adjpar );
    adjpar.setYN( sKeyTrackMax(), trackmaximum );
    iopar.mergeComp( adjpar, sKeyAdjuster() );
}


bool FaultAdjuster::usePar( const IOPar& iopar )
{
    PtrMan<IOPar> subsel = iopar.subselect( sKeyAdjuster() );
    return
	subsel &&
	SectionAdjuster::usePar( iopar ) &&
	attribsel.usePar( *subsel ) &&
	subsel->getYN( sKeyTrackMax(), trackmaximum );
}
	

void FaultAdjuster::prepareCalc( EM::SubID subid )
{
    /*
    EM::PosID posid( fault_.id(), sectionid_, subid );
    for ( int idx=0; idx<nrComputers(); idx++ )
    {
	getComputer(idx)->setPosID( posid, EM::PosID(0,0,0) );
	//getComputer(idx)->prepareCalc( direction );
    }
    */
}


#define mGetNormal( tg ) \
    const double abs = Math::Sqrt( (double)(tg.inl*tg.inl + tg.crl*tg.crl) ); \
    BinID normal( fabs(tg.crl/abs) >= M_SQRT1_2 ? 1 : 0, \
	    	  fabs(tg.inl/abs) >= M_SQRT1_2 ? 1 : 0 );


void FaultAdjuster::getTargetPositions( EM::SubID target, const EM::SubID* src,
					TypeSet<BinID>& targetpos ) const
{
    RowCol rc = RowCol::fromInt64( target );
    const Coord3& pos = fault_.getPos( sectionid_, target );
    const BinID bid = SI().transform( pos );
    targetpos += bid;

    if ( !src ) return;
    
    const RowCol srcrc = RowCol::fromInt64( *src );

    if ( srcrc.col==rc.col ) // tracking up/down
    {
	const Geometry::ParametricSurface* psurf = 0;
	    				//fault_.geometry().sectionGeometry(0);
	if ( !psurf ) return;
	Geometry::ParametricCurve* pcurv = psurf->createRowCurve( rc.row );
	mDynamicCastGet(Geometry::CubicBezierCurve*,bcurv,pcurv)
	if ( !bcurv ) return;

	Coord3 tangent = bcurv->getTangent( rc.col, true );
	BinID bnorm = SI().transform( tangent ) - SI().transform( Coord(0,0) );
	mGetNormal( bnorm );

	const BinID step( SI().inlStep(), SI().crlStep() );
	targetpos += bid + normal * step;
	targetpos += bid + normal * -step;
    }
    else
    {
	//Untested code
	const Coord3& srcpos = fault_.getPos( sectionid_, *src );
	const BinID srcbid = SI().transform( srcpos );

	const BinID step( abs(bid.crl-srcbid.crl) ? SI().inlStep() : 0,
			  abs(bid.inl-srcbid.inl) ? SI().crlStep() : 0 );
	const int nrsteps = 1;
	for ( int idx=0; idx<nrsteps; idx++ )
	{
	    const BinID hash = step*(idx+1);
	    targetpos += (bid+hash);
	    targetpos += (bid-hash);
	}
    }
}


float FaultAdjuster::computeScore( const Coord3& pos )
{
    /*
    float totalweight = 0;
    float weightedscore = 0;
    for ( int idx=0; idx<nrComputers(); idx++ )
    {
	float score = getComputer(idx)->computeScore( pos );
	float weight = getComputer(idx)->getWeight();
	weightedscore += score*weight;
	totalweight += weight;
    }

    return totalweight ? weightedscore/totalweight : 0;
    */
    return 0;
}

};  //namespace

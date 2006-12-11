/*+
 ________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          August 2004
 RCS:           $Id: visseis2ddisplay.cc,v 1.10 2006-12-11 19:40:50 cvskris Exp $
 ________________________________________________________________________

-*/


#include "visseis2ddisplay.h"

#include "vistristripset.h"
#include "viscoord.h"
#include "vistexturecoords.h"
#include "vistransform.h"
#include "viscolortab.h"
#include "visdataman.h"
#include "vismultitexture2.h"
#include "vistext.h"
//#include "vismaterial.h"
//#include "viscolortab.h"

#include "arrayndimpl.h"
#include "attribdataholder.h"
//#include "colortab.h"
#include "idxable.h"
#include "iopar.h"
#include "segposinfo.h"
//#include "seis2dline.h"
#include "seisinfo.h"
//#include "survinfo.h"

//For parsing old pars
#include "attribsel.h"
#include "vistexture2.h"

mCreateFactoryEntry( visSurvey::Seis2DDisplay );

using namespace Attrib;

namespace visSurvey
{

const char* Seis2DDisplay::sKeyLineSetID()	{ return "LineSet ID"; }
const char* Seis2DDisplay::sKeyTrcNrRange()	{ return "Trc Nr Range"; }
const char* Seis2DDisplay::sKeyZRange()		{ return "Z Range"; }
const char* Seis2DDisplay::sKeyShowLineName()	{ return "Show linename"; }
const char* Seis2DDisplay::sKeyTextureID()	{ return "Texture ID"; }


Seis2DDisplay::Seis2DDisplay()
    : transformation_(0)
    , linename_(0)
    , geometry_( *new PosInfo::Line2DData )
    , geomchanged_(this)
    , maxtrcnrrg_( INT_MAX, INT_MIN )
    , zrg_(-1,-1)
    , trcnrrg_(-1,-1)
{
    geometry_.zrg.start = geometry_.zrg.stop = mUdf(float);
    cache_.allowNull();
}


Seis2DDisplay::~Seis2DDisplay()
{
    if ( linename_ ) removeChild( linename_->getInventorNode() );

    for ( int idx=0; idx<planes_.size(); idx++ )
    {
	removeChild( planes_[idx]->getInventorNode() );
	planes_[idx]->unRef();
    }

    delete &geometry_;

    if ( transformation_ ) transformation_->unRef();
    deepUnRef( cache_ );
}


void Seis2DDisplay::setLineName( const char* lnm )
{
    setName( lnm );
    if ( linename_ )
	linename_->setText( lnm );
}


void Seis2DDisplay::setGeometry( const PosInfo::Line2DData& geometry )
{
    geometry_ = geometry;
    const TypeSet<PosInfo::Line2DPos>& linepositions = geometry.posns;
    maxtrcnrrg_.start = INT_MAX; maxtrcnrrg_.stop = INT_MIN;

    for ( int idx=linepositions.size()-1; idx>=0; idx-- )
	maxtrcnrrg_.include( linepositions[idx].nr, false );

    if ( zrg_.start==-1 )
    {
	zrg_.start = 0;
	zrg_.stop = geometry_.zrg.nrSteps();
	trcnrrg_ = maxtrcnrrg_;
    }

    updateVizPath();
    geomchanged_.trigger();
}


const StepInterval<float>& Seis2DDisplay::getMaxZRange() const
{ return geometry_.zrg; }


bool Seis2DDisplay::setZRange( const Interval<int>& nzrg )
{
    if ( mIsUdf(geometry_.zrg.start) )
    {
	pErrMsg("Geometry not set");
	return false;
    }

    const Interval<int> zrg( mMAX( nzrg.start, 0 ),
	    		     mMIN( nzrg.stop, geometry_.zrg.nrSteps()) );

    if ( !zrg.width() || zrg_==zrg )
	return false;

    const bool isbigger = !zrg_.includes( zrg.start, false ) ||
			  !zrg_.includes( zrg.stop, false );

    zrg_ = zrg;

    updateVizPath();
    return !isbigger;
}


const Interval<int>& Seis2DDisplay::getZRange() const
{ return zrg_; }


bool Seis2DDisplay::setTraceNrRange( const Interval<int>& trcrg )
{
    if ( maxtrcnrrg_.isRev() )
    {
	pErrMsg("Geometry not set");
	return false;
    }

    const Interval<int> rg( maxtrcnrrg_.limitValue( trcrg.start ),
			    maxtrcnrrg_.limitValue( trcrg.stop ) );

    if ( !rg.width() || trcnrrg_==rg )
	return false;

    const bool isbigger = !trcnrrg_.includes( rg.start, false ) ||
			  !trcnrrg_.includes( rg.stop, false );
    trcnrrg_ = rg;

    updateVizPath();

    return !isbigger;
}


const Interval<int>& Seis2DDisplay::getTraceNrRange() const
{ return trcnrrg_; }


const Interval<int>& Seis2DDisplay::getMaxTraceNrRange() const
{ return maxtrcnrrg_; }


void Seis2DDisplay::setTraceData( int attrib,
				  const Attrib::Data2DHolder& dataset )
{
    setData( attrib, dataset );
    if ( cache_[attrib] ) cache_[attrib]->unRef();

    cache_.replace( attrib, &dataset );
    cache_[attrib]->ref();
}


const Attrib::Data2DHolder* Seis2DDisplay::getCache( int attrib ) const
{ return cache_[attrib]; }


void Seis2DDisplay::setData( int attrib,
			     const Attrib::Data2DHolder& dataset )
{
    if ( dataset.isEmpty() ) return;

    const SamplingData<float>& sd = dataset.trcinfoset_[0]->sampling;

    const int nrsamp = zrg_.width()+1;

    PtrMan<Array2DImpl<float> > arr =
	new Array2DImpl<float>( nrsamp,trcnrrg_.width()+1 );

    float* arrptr = arr->getData();
    const int totalsz = arr->info().getTotalSz();
    for ( int idx=0; idx<totalsz; idx++ )
	(*arrptr++) = mUdf(float);

    for ( int dataidx=0; dataidx<dataset.size(); dataidx++ )
    {
	const int trcnr = dataset.trcinfoset_[dataidx]->nr;
	if ( !trcnrrg_.includes( trcnr ) )
	    continue;

	const int trcidx = trcnr-trcnrrg_.start;

	const DataHolder* dh = dataset.dataset_[dataidx];
	if ( !dh )
	    continue;

	const float firstz = geometry_.zrg.atIndex( zrg_.start );
	const float firstdhsamplef = sd.getIndex( firstz );
	const int firstdhsample = sd.nearestIndex( firstz );
	const bool samplebased = mIsEqual(firstdhsamplef,firstdhsample,1e-3) &&
	    			 mIsEqual(sd.step,geometry_.zrg.step,1e-3 );

	const ValueSeries<float>* dataseries =
	    dh->series(dh->validSeriesIdx()[0]);

	if ( samplebased )
	{
	    for ( int idx=0; idx<nrsamp; idx++ )
	    {
		const int sample = firstdhsample+idx;
		float val;
		if ( dh->dataPresent( sample ) )
		    val = dataseries->value( sample-dh->z0_ );
		else
		    val=mUdf(float);

		arr->set( idx, trcidx, val );
	    }
	}
	else
	{
	    pErrMsg("Not impl"); 
	}
    }

    if ( !resolution_ )
	texture_->setData( attrib, 0, arr, true );
    else
    {
	texture_->setDataOversample( attrib, 0, resolution_,
				     !isClassification( attrib ), arr, true );
    }
}



void Seis2DDisplay::updateVizPath()
{
    TypeSet<Coord> coords;
    for ( int idx=0; idx<geometry_.posns.size(); idx++ )
    {
	if ( !trcnrrg_.includes( geometry_.posns[idx].nr ) ) continue;
	coords += geometry_.posns[idx].coord;
    }

    const Interval<float> zrg( geometry_.zrg.atIndex( zrg_.start ),
			       geometry_.zrg.atIndex( zrg_.stop ) );

    const int nrcrds = coords.size();
    TypeSet<Interval<int> > stripinterval;
    if ( nrcrds>1 )
    {
	int currentstart = 0;
	for ( int idx=1; idx<coords.size(); idx++ )
	{
	    if ( coords[currentstart].sqDistance(coords[idx])>1e10 )
	    {
		stripinterval += Interval<int>(currentstart,idx);
		currentstart = idx;
	    }
	}

	if ( currentstart!=coords.size()-1 )
	    stripinterval += Interval<int>(currentstart,coords.size()-1);

	for ( int idx=0; idx<stripinterval.size(); idx++ )
	    setStrip( coords, zrg, idx, stripinterval[idx] );

	updateLineNamePos();
    }

    for ( int idx=stripinterval.size(); idx<planes_.size(); idx++ )
    {
	removeChild( planes_[idx]->getInventorNode() );
	planes_[idx]->unRef();
	planes_.remove(idx);
	idx--;
    }

    texture_->clearAll();
}


#define mAddCoords( coordidx, posidx ) \
{ \
    const float texturecoord_s = (float)(posidx)/(crds.size()-1); \
    coords->setPos( coordidx, Coord3(crds[posidx],zrg.start) ); \
    plane->setCoordIndex( coordidx, coordidx ); \
    texturecoords->setCoord( coordidx, Coord(texturecoord_s,0) ); \
    plane->setTextureCoordIndex( coordidx, coordidx ); \
    coordidx++; \
    coords->setPos( coordidx, Coord3(crds[posidx],zrg.stop) ); \
    plane->setCoordIndex( coordidx, coordidx ); \
    texturecoords->setCoord( coordidx, Coord(texturecoord_s,1) ); \
    plane->setTextureCoordIndex( coordidx, coordidx ); \
    coordidx++; \
}

void visSurvey::Seis2DDisplay::setStrip( const TypeSet<Coord>& crds,
					 const Interval<float>& zrg,
					 int stripidx,
					 const Interval<int>& crdinterval )
{
    while ( stripidx>=planes_.size() )
    {
	visBase::TriangleStripSet* plane = visBase::TriangleStripSet::create();
	plane->setNormalPerFaceBinding( true );
	plane->setDisplayTransformation( transformation_ );
	plane->setShapeType( visBase::VertexShape::cUnknownShapeType() );
	plane->setTextureCoords(visBase::TextureCoords::create());
	plane->setVertexOrdering( 
	       visBase::VertexShape::cCounterClockWiseVertexOrdering() );
	plane->setMaterial( 0 );
	planes_ += plane;
	plane->ref();
	addChild( plane->getInventorNode() );
    }

    visBase::TriangleStripSet* plane = planes_[stripidx];
    visBase::Coordinates* coords = plane->getCoordinates();
    for ( int idx=0; idx<coords->size(true); idx++ )
	coords->removePos( idx );
    visBase::TextureCoords* texturecoords = plane->getTextureCoords();
    for ( int idx=0; idx<texturecoords->size(true); idx++ )
	texturecoords->removeCoord( idx );

    Coord centercoord(0,0);
    const int nrcrds = crdinterval.width()+1;
    for ( int idx=crdinterval.start; idx<=crdinterval.stop; idx++ )
	centercoord += crds[idx]/nrcrds;

    coords->setLocalTranslation( centercoord );

    TypeSet<double> x, y;
    for ( int idx=crdinterval.start; idx<=crdinterval.stop; idx++ )
	{ x += crds[idx].x; y += crds[idx].y; }
    TypeSet<int> bpidxs;
    IdxAble::getBendPoints( x, y, x.size(), 0.5, bpidxs );

    int curknotidx=0;
    for ( int idx=0; idx<bpidxs.size(); idx++ )
	mAddCoords( curknotidx, crdinterval.start+bpidxs[idx] );

    plane->setCoordIndex( curknotidx, -1 );
    plane->removeCoordIndexAfter( curknotidx );
    plane->removeTextureCoordIndexAfter( curknotidx );
}


void Seis2DDisplay::updateLineNamePos()
{
    if ( planes_.isEmpty() || !linename_ ) return;

    visBase::Coordinates* coords = planes_[0]->getCoordinates();
    if ( !coords ) return;

    Coord3 pos = coords->getPos( 0 );
    linename_->setPosition( pos );
}


SurveyObject* Seis2DDisplay::duplicate() const
{
    Seis2DDisplay* s2dd = create();
    s2dd->setGeometry( geometry_ );
    s2dd->setZRange( zrg_ );
    s2dd->setTraceNrRange( trcnrrg_ );
    s2dd->setResolution( getResolution() );

    s2dd->setLineSetID( linesetid_ );
    s2dd->setLineName( name() );

    for ( int idx=0; idx<nrAttribs(); idx++ )
    {
	if ( idx )
	    s2dd->addAttrib();

	s2dd->setSelSpec( idx, *getSelSpec(idx) );
	s2dd->texture_->copySettingsFrom( idx,
		(const visBase::MultiTexture&) *texture_, idx );

	if ( getCache( idx ) )
	    s2dd->setData( idx, *getCache( idx ) );
    }

    return s2dd;
}


float Seis2DDisplay::calcDist( const Coord3& pos ) const
{
    Coord3 xytpos = scene_->getUTM2DisplayTransform()->transformBack( pos );

    int trcidx; float mindist;
    getNearestTrace( xytpos, trcidx, mindist );
    return mindist<0 && mIsUdf(mindist) ? mUdf(float) : sqrt( mindist );
}


void Seis2DDisplay::setDisplayTransformation( visBase::Transformation* tf )
{
    if ( transformation_ ) transformation_->unRef();
    transformation_ = tf;
    transformation_->ref();
    for ( int idx=0; idx<planes_.size(); idx++ )
	planes_[idx]->setDisplayTransformation( transformation_ );
    if ( linename_ )
	linename_->setDisplayTransformation( transformation_ );
}


visBase::Transformation* Seis2DDisplay::getDisplayTransformation()
{
    return transformation_;
}


void Seis2DDisplay::showLineName( bool yn )
{
    if ( linename_ ) linename_->turnOn( yn );
    else if ( yn )
    {
	linename_ = visBase::Text2::create();
	linename_->ref();
	updateLineNamePos();
    }
}

bool Seis2DDisplay::lineNameShown() const
{ return linename_ ? linename_->isOn() : false; }


void Seis2DDisplay::setResolution( int res )
{
    if ( res==resolution_ )
	return;

    texture_->clearAll();

    resolution_ = res;
    updateDataFromCache();
}


int Seis2DDisplay::getResolution() const
{ return resolution_; }


SurveyObject::AttribFormat Seis2DDisplay::getAttributeFormat() const
{ return SurveyObject::OtherFormat; }


void Seis2DDisplay::addCache()
{ cache_ += 0; }


void Seis2DDisplay::removeCache( int attrib )
{
    if ( cache_[attrib] ) cache_[attrib]->unRef();
    cache_.remove( attrib );
}


void Seis2DDisplay::swapCache( int a0, int a1 )
{ cache_.swap( a0, a1 ); }


void Seis2DDisplay::emptyCache( int attrib )
{
    if ( cache_[attrib] )
	cache_[attrib]->unRef();

    cache_.replace( attrib, 0 );
}


bool Seis2DDisplay::hasCache( int attrib ) const
{ return cache_[attrib] && cache_[attrib]->size(); }


void Seis2DDisplay::updateDataFromCache()
{
    for ( int idx=nrAttribs()-1; idx>=0; idx-- )
	if ( cache_[idx] ) setData( idx, *cache_[idx] );
}


void Seis2DDisplay::getMousePosInfo( const visBase::EventInfo&,
				     const Coord3& pos, BufferString& val,
				     BufferString& info ) const
{
    info = "Line: "; info += name();
    getValueString( pos, val );

    int dataidx = -1;
    float mindist;
    if ( getNearestTrace( pos, dataidx, mindist ) )
    {
	info += "   Tracenr: ";
	info += geometry_.posns[dataidx].nr;
    }
}


float Seis2DDisplay::getCacheValue( int attrib, int version,
				    const Coord3& pos ) const
{
    if ( attrib>=cache_.size() || !cache_[attrib] )
	return mUdf(float);

    int dataidx = -1;
    float mindist;
    if ( !getNearestTrace( pos, dataidx, mindist ) || 
	 dataidx>=cache_[attrib]->dataset_.size() ||
	 !cache_[attrib]->dataset_[dataidx] )
	return mUdf(float);

    const DataHolder* dh = cache_[attrib]->dataset_[dataidx];
    const int trcnr = cache_[attrib]->trcinfoset_[dataidx]->nr;
    const int sampidx =
       mNINT(pos.z/cache_[attrib]->trcinfoset_[dataidx]->sampling.step)-dh->z0_;

    if ( sampidx >= 0 && sampidx < dh->nrsamples_ )
	return dh->series(dh->validSeriesIdx()[version])->value(sampidx);

    return mUdf(float);
}


void Seis2DDisplay::snapToTracePos( Coord3& pos )
{
    int trcidx = -1;
    float mindist;
    getNearestTrace( pos, trcidx, mindist );

    if ( trcidx<0 ) return;

    const Coord& crd = geometry_.posns[trcidx].coord;
    pos.x = crd.x; pos.y = crd.y; 
}


void Seis2DDisplay::setLineSetID( const MultiID& mid )
{ linesetid_ = mid; }


const MultiID& Seis2DDisplay::lineSetID() const
{ return linesetid_; }


bool Seis2DDisplay::getNearestTrace( const Coord3& pos,
				     int& trcidx, float& mindist ) const
{
    trcidx = -1;
    mSetUdf(mindist);

    for ( int idx=geometry_.posns.size()-1; idx>=0; idx-- )
    {
	if ( !trcnrrg_.includes( geometry_.posns[idx].nr ) ) continue;

	const float dist = pos.Coord::sqDistance( geometry_.posns[idx].coord );
	if ( dist<mindist )
	{
	    mindist = dist;
	    trcidx = idx;
	}
    }

    return trcidx!=-1;
}


void Seis2DDisplay::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    visSurvey::MultiTextureSurveyObject::fillPar( par, saveids );

    par.set( sKeyLineSetID(), linesetid_ );
    par.setYN( sKeyShowLineName(), lineNameShown() );
    par.set( sKeyTrcNrRange(), trcnrrg_.start, trcnrrg_.stop );
    par.set( sKeyZRange(), zrg_.start, zrg_.stop );
}


int Seis2DDisplay::usePar( const IOPar& par )
{
    int textureid = -1;
    if ( par.get(sKeyTextureID(),textureid) )
    {
	int res =  visBase::VisualObjectImpl::usePar( par );
	if ( res != 1 ) return res;

	DataObject* text = visBase::DM().getObject( textureid );
	if ( !text ) return 0;
	if ( typeid(*text)!=typeid(visBase::Texture2) ) return -1;

	RefMan<visBase::Texture2> texture =
	    reinterpret_cast<visBase::Texture2*>(text);
	texture_->setColorTab( 0, texture->getColorTab() );
	Attrib::SelSpec as;
	as.usePar( par );
	setSelSpec( 0, as );
    }
    else //new format
    {
	int res =  visSurvey::MultiTextureSurveyObject::usePar( par );
	if ( res!=1 ) return res;

	par.get( sKeyTrcNrRange(), trcnrrg_.start, trcnrrg_.stop );
	par.get( sKeyZRange(), zrg_.start, zrg_.stop );
	bool showlinename = false;
	par.getYN( sKeyShowLineName(), showlinename );
	showLineName( showlinename );
    }

    setLineName( name() );
    par.get( sKeyLineSetID(), linesetid_ );

    return 1;
}

}; // namespace visSurvey

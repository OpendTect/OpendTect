/*+
 ________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          August 2004
 RCS:           $Id: visseis2ddisplay.cc,v 1.4 2006-06-01 07:30:15 cvskris Exp $
 ________________________________________________________________________

-*/


#include "visseis2ddisplay.h"
#include "vistristripset.h"
#include "viscoord.h"
#include "vistexturecoords.h"
#include "vistransform.h"
#include "viscolortab.h"
#include "visdataman.h"
#include "vistexture2.h"
#include "vistext.h"
#include "vismaterial.h"
#include "viscolortab.h"

#include "arrayndimpl.h"
#include "attribdataholder.h"
#include "attribsel.h"
#include "colortab.h"
#include "idxable.h"
#include "iopar.h"
#include "segposinfo.h"
#include "seis2dline.h"
#include "seisinfo.h"
#include "survinfo.h"


mCreateFactoryEntry( visSurvey::Seis2DDisplay );

using namespace Attrib;

namespace visSurvey
{

const char* Seis2DDisplay::linesetidstr = "LineSet ID";
const char* Seis2DDisplay::textureidstr = "Texture ID";


Seis2DDisplay::Seis2DDisplay()
    : VisualObjectImpl(true)
    , transformation(0)
    , texture(visBase::Texture2::create())
    , linename(0)
    , as(*new SelSpec)
    , cs(*new CubeSampling(false))
    , geomchanged(this)
    , iszlset(false)
    , cache_( 0 )
{
    texture->ref();
    addChild( texture->getInventorNode() );
    texture->turnOn(true);

    getMaterial()->setAmbience( 0.8 );
    getMaterial()->setDiffIntensity( 0.8 );
    cs.hrg.start.inl = cs.hrg.stop.inl = 0;
}


Seis2DDisplay::~Seis2DDisplay()
{
    if ( linename )
	removeChild( linename->getInventorNode() );
    for ( int idx=0; idx<planes.size(); idx++ )
    {
	removeChild( planes[idx]->getInventorNode() );
	planes[idx]->unRef();
    }

    removeChild( texture->getInventorNode() );
    texture->unRef();

    if ( transformation ) transformation->unRef();
    delete &as;
    delete &cs;

    if ( cache_ ) cache_->unRef();
    cache_ = 0;
}


void Seis2DDisplay::setLineName( const char* lnm )
{
    setName( lnm );
    if ( linename )
	linename->setText( lnm );
}


void Seis2DDisplay::setSelSpec( int attrib, const SelSpec& as_ )
{
    if ( attrib ) return;
    as = as_;
    if ( cache_ ) cache_->unRef();
    cache_ = 0;
}


const SelSpec* Seis2DDisplay::getSelSpec( int attrib ) const
{
    return !attrib ? &as : 0;
}


void Seis2DDisplay::setCubeSampling( CubeSampling cs_ )
{ cs = cs_; }


CubeSampling Seis2DDisplay::getCubeSampling() const
{ return cs; }


void Seis2DDisplay::setGeometry( const PosInfo::Line2DData& geometry, 
				 const CubeSampling* limits )
{
    const TypeSet<PosInfo::Line2DPos>& pos = geometry.posns;
    TypeSet<Coord> coords;
    for ( int idx=0; idx<pos.size(); idx++ )
    {
	if ( limits && (pos[idx].nr<limits->hrg.start.crl || 
		        pos[idx].nr > limits->hrg.stop.crl) )
	    continue;
	coords += pos[idx].coord;
    }

    Interval<float> zrg;
    assign( zrg, limits ? limits->zrg : geometry.zrg );
    setPlaneCoordinates( coords, zrg );

    if ( cs.isEmpty() )
    {
	assign( cs.zrg, geometry.zrg );
	cs.hrg.start.crl = pos[0].nr;
	cs.hrg.stop.crl = pos[pos.size()-1].nr;
    }
    else if ( limits )
    {
	cs = *limits;
	setZLimits(zrg);
	geomchanged.trigger();
    }
}


void Seis2DDisplay::clearTexture()
{
    SelSpec as_; setSelSpec( 0, as_ );
    texture->setData( 0, (visBase::Texture::DataType)0 );
}


void Seis2DDisplay::setTraceData( const Attrib::Data2DHolder& dataset )
{
    setData( dataset );
    if ( cache_ ) cache_->unRef();

    cache_ = &dataset;
    cache_->ref();
}


void Seis2DDisplay::setData( const Attrib::Data2DHolder& dataset )
{
    if ( !dataset.size() ) return;

    TypeSet<Coord> crds;
    for ( int trcinfoidx=0; trcinfoidx<dataset.size(); trcinfoidx++ )
    {
	const SeisTrcInfo* trcinfo = dataset.trcinfoset_[trcinfoidx];
	if ( !trcinfo ) continue;
	crds += trcinfo->coord;
    }

    Interval<float> zrg(0,0);
    int nrsamp;
    SamplingData<float> sd = dataset.trcinfoset_[0]->sampling;
    
    if ( isZLimitSet() )
    {
	zrg = getZLimits();
	zrg.sort();
	nrsamp = (int)( (zrg.stop-zrg.start) / sd.step );
    }
    else
    {
	nrsamp = dataset.dataset_[0]->nrsamples_;
	zrg.start = sd.start;
	zrg.stop = sd.start+sd.step*(nrsamp-1);
    }
    const int nrtrcs = dataset.size();
    setPlaneCoordinates( crds, zrg );

    float val;
    PtrMan<Array2DImpl<float> > arr = new Array2DImpl<float>(nrsamp,nrtrcs);
    for ( int dataidx=0; dataidx<dataset.size(); dataidx++ )
    {
	const DataHolder* dh = dataset.dataset_[dataidx];
	if ( !dh )
	{
	    for ( int ids=0; ids<nrsamp; ids++ )
		arr->set( ids, dataidx, mUdf(float) );
	    continue;
	}

	int csample =  mNINT( zrg.start/sd.step );
	for ( int ids=0; ids<nrsamp; ids++ )
	{
	    val = dh && dh->dataPresent(csample) ? 
		    dh->series(dh->validSeriesIdx()[0])->value(csample-dh->z0_)
		    : mUdf(float);
	    //use first valid idx: as for now we can't evaluate attributes in 2D
	    arr->set( ids, dataidx, val );
	    csample++;
	}
    }

    texture->setData( arr, (visBase::Texture::DataType)0 );
}


void Seis2DDisplay::setPlaneCoordinates( const TypeSet<Coord>& crds,
					 const Interval<float>& zrg )
{
    const int nrcrds = crds.size();
    if ( nrcrds<2 ) return;

    TypeSet<Interval<int> > stripinterval;
    int currentstart = 0;
    for ( int idx=1; idx<crds.size(); idx++ )
    {
	if ( crds[currentstart].sqDistance(crds[idx])>1e10 )
	{
	    stripinterval += Interval<int>(currentstart,idx);
	    currentstart = idx;
	}
    }

    if ( currentstart!=crds.size()-1 )
	stripinterval += Interval<int>(currentstart,crds.size()-1);

    for ( int idx=0; idx<stripinterval.size(); idx++ )
	setStrip( crds, zrg, idx, stripinterval[idx] );

    for ( int idx=stripinterval.size(); idx<planes.size(); idx++ )
    {
	removeChild( planes[idx]->getInventorNode() );
	planes[idx]->unRef();
	planes.remove(idx);
	idx--;
    }
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
    while ( stripidx>=planes.size() )
    {
	visBase::TriangleStripSet* plane = visBase::TriangleStripSet::create();
	plane->setNormalPerFaceBinding( true );
	plane->setDisplayTransformation( transformation );
	plane->setShapeType( 0 );
	plane->setTextureCoords(visBase::TextureCoords::create());
	plane->setVertexOrdering( 1 );
	plane->setMaterial( 0 );
	planes += plane;
	plane->ref();
	addChild( plane->getInventorNode() );
    }

    visBase::TriangleStripSet* plane = planes[stripidx];
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
    for ( int idx=crdinterval.start; idx<crdinterval.stop; idx++ )
	{ x += crds[idx].x; y += crds[idx].y; }
    TypeSet<int> bpidxs;
    IdxAble::getBendPoints( x, y, crds.size(), 0.5, bpidxs );

    int curknotidx=0;
    for ( int idx=0; idx<bpidxs.size(); idx++ )
	mAddCoords( curknotidx, bpidxs[idx] );

    plane->setCoordIndex( curknotidx, -1 );
    plane->removeCoordIndexAfter( curknotidx );
    plane->removeTextureCoordIndexAfter( curknotidx );

    addLineName();
}


void Seis2DDisplay::addLineName()
{
    if ( !planes.size() ) return;
    visBase::Coordinates* coords = planes[0]->getCoordinates();
    if ( !coords ) return;
    Coord3 pos = coords->getPos( 0 );

    if ( !linename )
    {
	linename = visBase::Text2::create();
	insertChild( 0, linename->getInventorNode() );
	linename->turnOn( true );
	linename->setText( name() );
	linename->setJustification( visBase::Text::Right );
	if ( transformation )
	    linename->setDisplayTransformation( transformation );
    }

    linename->setPosition( pos );
}


void Seis2DDisplay::getColTabDef( ColorTable& coltab, Interval<float>& scale,
				  float& cliprate ) const
{
    visBase::VisColorTab& ct = texture->getColorTab();
    coltab = ct.colorSeq().colors();
    scale = ct.getInterval();
    cliprate = ct.clipRate();
}


void Seis2DDisplay::setColTabDef( const ColorTable& coltab,
				  const Interval<float>& scale,
				  float cliprate )
{
    visBase::VisColorTab& ct = texture->getColorTab();
    const bool equalcolorseq = ct.colorSeq().colors() == coltab;
    if ( !equalcolorseq )
	ct.colorSeq().loadFromStorage( coltab.name() );

    if ( !mIsUdf(scale.start) && !mIsUdf(scale.stop) )
	ct.scaleTo( scale );
    if ( !mIsUdf(cliprate) )
	ct.setClipRate( cliprate );
}


SurveyObject* Seis2DDisplay::duplicate() const
{
    Seis2DDisplay* s2dd = create();
    s2dd->setCubeSampling( getCubeSampling() );
    s2dd->setResolution( getResolution() );
    s2dd->setLineSetID( linesetid );
    s2dd->setLineName( name() );

    const int ctid = s2dd->getColTabID(0);
    visBase::DataObject* obj = ctid>=0 ? visBase::DM().getObject( ctid ) : 0;
    mDynamicCastGet(visBase::VisColorTab*,nct,obj);
    if ( nct )
    {
	visBase::VisColorTab& ct = texture->getColorTab();
	const char* ctnm = ct.colorSeq().colors().name();
	nct->colorSeq().loadFromStorage( ctnm );
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
    if ( transformation ) transformation->unRef();
    transformation = tf;
    transformation->ref();
    for ( int idx=0; idx<planes.size(); idx++ )
	planes[idx]->setDisplayTransformation( transformation );
    if ( linename )
	linename->setDisplayTransformation( transformation );
}


visBase::Transformation* Seis2DDisplay::getDisplayTransformation()
{
    return transformation;
}


void Seis2DDisplay::showLineName( bool yn )
{ if ( linename ) linename->turnOn( yn ); }

bool Seis2DDisplay::lineNameShown() const
{ return linename ? linename->isOn() : false; }


int Seis2DDisplay::getColTabID(int attrib) const
{ 
    if ( attrib )
	return -1;

    return texture->getColorTab().id();
}


const TypeSet<float>* Seis2DDisplay::getHistogram(int attrib) const
{
    if ( attrib )
	return 0;

    return &texture->getHistogram();
}


int Seis2DDisplay::getResolution() const
{
    return texture->getResolution();
}


void Seis2DDisplay::setResolution( int res )
{
    texture->setResolution( res );
    if ( cache_ ) setData( *cache_ );
}


SurveyObject::AttribFormat Seis2DDisplay::getAttributeFormat() const
{ return SurveyObject::OtherFormat; }


void Seis2DDisplay::getMousePosInfo( const visBase::EventInfo&,
				     const Coord3& pos, BufferString& val,
				     BufferString& info ) const
{
    info = "Line: "; info += name();
    val = "undef";
    if ( !cache_ || !cache_->size() )  return;

    int dataidx = -1;
    float mindist;
    getNearestTrace( pos, dataidx, mindist );
    if ( dataidx < 0 )
	return;

    const DataHolder* dh = cache_->dataset_[dataidx];
    if ( !dh ) 
	return;

    const int trcnr = cache_->trcinfoset_[dataidx]->nr;
    const int sampidx =
	mNINT(pos.z/cache_->trcinfoset_[dataidx]->sampling.step)-dh->z0_;
    if ( sampidx >= 0 && sampidx < dh->nrsamples_ )
	val = dh->series(dh->validSeriesIdx()[0])->value(sampidx);

    //use first valid idx: as for now we can't evaluate attributes in 2D...
    
    info += "   Tracenr: "; info += trcnr;
}


void Seis2DDisplay::snapToTracePos( Coord3& pos )
{
    int trcidx = -1;
    float mindist;
    getNearestTrace( pos, trcidx, mindist );

    if ( trcidx < 0 ) return;
    const Coord& crd = cache_->trcinfoset_[trcidx]->coord;
    pos.x = crd.x; pos.y = crd.y; 
}


void Seis2DDisplay::getNearestTrace( const Coord3& pos, int& trcidx, 
				     float& mindist ) const
{
    trcidx = -1;
    mSetUdf(mindist);

    if ( cache_ )
    {
	for ( int idx=0; idx<cache_->size(); idx++ )
	{
	    const float dist =
		pos.Coord::sqDistance( cache_->trcinfoset_[idx]->coord );
	    if ( dist < mindist )
	    {
		mindist = dist;
		trcidx = idx;
	    }
	}
    }
}


void Seis2DDisplay::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    visBase::VisualObjectImpl::fillPar( par, saveids );

    par.set( linesetidstr, linesetid );
    par.set( textureidstr, texture->id() );
    if ( saveids.indexOf(texture->id())==-1 ) 
	saveids += texture->id();

    as.fillPar( par );
    cs.fillPar( par );
}


int Seis2DDisplay::usePar( const IOPar& par )
{
    int res =  visBase::VisualObjectImpl::usePar( par );
    if ( res != 1 ) return res;

    int textureid = -1;
    if ( par.get(textureidstr,textureid) )
    {
	DataObject* text = visBase::DM().getObject( textureid );
	if ( !text ) return 0;
	if ( typeid(*text) != typeid(visBase::Texture2) ) return -1;

	if ( texture )
	{
	    removeChild( texture->getInventorNode() );
	    texture->unRef();
	}

	texture = (visBase::Texture2*)text;
	texture->ref();
	insertChild( 0, texture->getInventorNode() );
    }

    setLineName( name() );

    cs.usePar( par );
    as.usePar( par );
    par.get( linesetidstr, linesetid );

    return 1;
}

}; // namespace visSurvey

/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2002
-*/

static const char* rcsID = "$Id: visplanedatadisplay.cc,v 1.67 2004-05-19 12:18:17 kristofer Exp $";

#include "visplanedatadisplay.h"

#include "attribsel.h"
#include "cubesampling.h"
#include "attribslice.h"
#include "vistexturerect.h"
#include "arrayndimpl.h"
#include "position.h"
#include "survinfo.h"
#include "visselman.h"
#include "visdataman.h"
#include "visrectangle.h"
#include "vismaterial.h"
#include "viscolortab.h"
#include "viscolorseq.h"
#include "sorting.h"
#include "vistransform.h"
#include "iopar.h"
#include "colortab.h"
#include <math.h>

mCreateFactoryEntry( visSurvey::PlaneDataDisplay );


namespace visSurvey {

const char* PlaneDataDisplay::trectstr = "Texture rectangle";

PlaneDataDisplay::PlaneDataDisplay()
    : VisualObject(true)
    , trect(0)
    , cache(0)
    , colcache(0)
    , as(*new AttribSelSpec)
    , colas(*new ColorAttribSel)
    , moving(this)
{
    setTextureRect( visBase::TextureRect::create() );

    trect->getMaterial()->setAmbience( 0.8 );
    trect->getMaterial()->setDiffIntensity( 0.8 );

    setType( Inline );

    trect->getRectangle().setSnapping( true );
    trect->useTexture( false );

//TODO: Fix! For some reason this is needed to let Texture2 work properly
    showManipulator(true);
    showManipulator(false);

    SPM().zscalechange.notify( mCB(this,PlaneDataDisplay,appVelChCB) );
}


PlaneDataDisplay::~PlaneDataDisplay()
{
    SPM().zscalechange.remove( mCB(this,PlaneDataDisplay,appVelChCB));
    trect->manipChanges()->remove( mCB(this,PlaneDataDisplay,manipChanged) );
    trect->unRef();
    delete &as;
    delete &colas;

    delete cache;
    delete colcache;
}


void PlaneDataDisplay::setTextureRect( visBase::TextureRect* tr )
{
    if ( trect )
    {
	trect->manipChanges()->remove( mCB(this,PlaneDataDisplay,manipChanged));
	trect->unRef();
    }

    trect = tr;
    trect->ref();
    trect->manipChanges()->notify( mCB(this,PlaneDataDisplay,manipChanged) );
}


void PlaneDataDisplay::setType( Type nt )
{
    type = nt;
    setGeometry( false, true );
}


void PlaneDataDisplay::setGeometry( bool manip, bool init_ )
{
    BinID startbid = SI().range().start;
    BinID stopbid = SI().range().stop;
    StepInterval<double> vrgd = SI().zRange();

    StepInterval<float> inlrg( startbid.inl,stopbid.inl,SI().inlWorkStep() );
    StepInterval<float> crlrg( startbid.crl,stopbid.crl,SI().crlWorkStep() );
    StepInterval<float> vrg( vrgd.start, vrgd.stop, vrgd.step );
    if ( init_ )
	trect->getRectangle().setOrigo( 
				Coord3(inlrg.start,crlrg.start,vrg.start) );
    setRanges( inlrg, crlrg, vrg, manip );

    resetDraggerSizes( SPM().getZScale() );
}


void PlaneDataDisplay::setRanges( const StepInterval<float>& irg,
					     const StepInterval<float>& crg,
					     const StepInterval<float>& zrg,
       					     bool manip )
{
    visBase::Rectangle& rect = trect->getRectangle();
    if ( type==Inline )
    {
	rect.setOrientation( visBase::Rectangle::YZ );
	rect.setRanges( zrg, crg, irg, manip );
    }
    else if ( type==Crossline )
    {
	rect.setOrientation( visBase::Rectangle::XZ );
	rect.setRanges( irg, zrg, crg, manip );
    }
    else
    {
	rect.setOrientation( visBase::Rectangle::XY );
	rect.setRanges( irg, crg, zrg, manip );
    }
}


void PlaneDataDisplay::setOrigo( const Coord3& pos )
{
    trect->getRectangle().setSnapping( false );
    trect->getRectangle().setOrigo( pos );
    trect->getRectangle().setSnapping( true );
}


void PlaneDataDisplay::setWidth( const Coord3& pos )
{
    if ( type==Inline )
	trect->getRectangle().setWidth( pos.z, pos.y );
    else if ( type==Crossline )
	trect->getRectangle().setWidth( pos.x, pos.z );
    else
	trect->getRectangle().setWidth( pos.x, pos.y );
}


void PlaneDataDisplay::resetDraggerSizes( float appvel )
{
    const float happvel = appvel/2;
    BinID startbid = SI().range().start;
    BinID stopbid = SI().range().stop;
    StepInterval<double> vrgd = SI().zRange();

    StepInterval<float> inlrange( startbid.inl,stopbid.inl,SI().inlWorkStep() );
    StepInterval<float> crlrange( startbid.crl,stopbid.crl,SI().crlWorkStep() );

    float inlspacing = SI().transform(BinID(0,0)).distance(
		       SI().transform( BinID(1,0)));
    float crlspacing = SI().transform(BinID(0,0)).distance(
		       SI().transform( BinID(0,1)));

    float baselength = ( crlrange.width() * crlspacing +
			 SI().zRange().width()*happvel +
			 inlrange.width() * inlspacing ) / 3;

    float draggerdiameter = 0.2 * baselength;
    float draggerlength =  baselength * 0.2;
    const float zfact = SI().zFactor();

    if ( type==Inline )
    {
	float draggertimesize = draggerdiameter / (zfact*happvel);
	float draggercrlsize = draggerdiameter / crlspacing;
	float draggerinlsize = draggerlength / inlspacing;

	trect->getRectangle().setDraggerSize( draggertimesize,
					      draggercrlsize,
				   	      draggerinlsize);
    }
    else if ( type==Crossline )
    {
	float draggertimesize = draggerdiameter / (zfact*happvel);
	float draggerinlsize = draggerdiameter / inlspacing;
	float draggercrlsize = draggerlength / crlspacing;

	trect->getRectangle().setDraggerSize( draggerinlsize,
					      draggertimesize,
				   	      draggercrlsize);
    }
    else
    {
	float draggerinlsize = draggerdiameter/ inlspacing;
	float draggercrlsize = draggerdiameter / crlspacing;
	float draggertimesize = draggerlength / (zfact*happvel);

	trect->getRectangle().setDraggerSize( draggerinlsize,
					      draggercrlsize,
				   	      draggertimesize);
    }
}


float PlaneDataDisplay::calcDist( const Coord3& pos ) const
{
    const visBase::Transformation* utm2display= SPM().getUTM2DisplayTransform();
    Coord3 xytpos = utm2display->transformBack( pos );
    Coord3 planeorigo = trect->getRectangle().origo();
    float width0 = trect->getRectangle().width( 0 );
    float width1 = trect->getRectangle().width( 1 );

    BinID binid = SI().transform( Coord( xytpos.x, xytpos.y ));
    
    BinID inlcrldist( 0, 0 );
    float zdiff = 0;

    if ( type==Inline )
    {
	inlcrldist.inl = abs(binid.inl-mNINT(planeorigo.x));
	
	if ( binid.crl<planeorigo.y )
	    inlcrldist.crl = mNINT(planeorigo.y-binid.crl);
	else if ( binid.crl>planeorigo.y+width1 )
	    inlcrldist.crl = mNINT( binid.crl-planeorigo.y-width1);

	if ( xytpos.z<planeorigo.z)
	    zdiff = planeorigo.z-xytpos.z;
	else if ( xytpos.z>planeorigo.z+width0 )
	    zdiff = xytpos.z-planeorigo.z-width0;

    }
    else if ( type==Crossline )
    {
	inlcrldist.crl = abs(binid.crl-mNINT(planeorigo.y));
	
	if ( binid.inl<planeorigo.x )
	    inlcrldist.inl = mNINT(planeorigo.x-binid.inl);
	else if ( binid.inl>planeorigo.x+width0 )
	    inlcrldist.inl = mNINT( binid.inl-planeorigo.x-width0);

	if ( xytpos.z<planeorigo.z)
	    zdiff = planeorigo.z-xytpos.z;
	else if ( xytpos.z>planeorigo.z+width1 )
	    zdiff = xytpos.z-planeorigo.z-width1;

    }
    else
    {
	if ( binid.inl<planeorigo.x )
	    inlcrldist.inl = mNINT(planeorigo.x-binid.inl);
	else if ( binid.inl>planeorigo.x+width0 )
	    inlcrldist.inl = mNINT( binid.inl-planeorigo.x-width0);

	if ( binid.crl<planeorigo.y )
	    inlcrldist.crl = mNINT(planeorigo.y-binid.crl);
	else if ( binid.crl>planeorigo.y+width1 )
	    inlcrldist.crl = mNINT( binid.crl-planeorigo.y-width1);

	zdiff = (planeorigo.z - xytpos.z) * SI().zFactor() * SPM().getZScale();
    }

    const float inldist =
	SI().transform( BinID(0,0)).distance( SI().transform(BinID(1,0)));
    const float crldist =
	SI().transform( BinID(0,0)).distance( SI().transform(BinID(0,1)));
    float inldiff = inlcrldist.inl * inldist;
    float crldiff = inlcrldist.crl * crldist;

    return sqrt( inldiff*inldiff + crldiff*crldiff + zdiff*zdiff );
}


void PlaneDataDisplay::appVelChCB( CallBacker* )
{
    resetDraggerSizes( SPM().getZScale() );
}


void PlaneDataDisplay::manipChanged( CallBacker* )
{
    moving.trigger();
}


SurveyObject* PlaneDataDisplay::duplicate() const
{
    PlaneDataDisplay* pdd = create();
    pdd->setType( type );
    pdd->setCubeSampling( getCubeSampling() );
    pdd->setResolution( getResolution() );

    int id = pdd->getColTabID();
    visBase::DataObject* obj = id>=0 ? visBase::DM().getObj( id ) : 0;
    mDynamicCastGet(visBase::VisColorTab*,nct,obj);
    if ( nct )
    {
	const char* ctnm = trect->getColorTab().colorSeq().colors().name();
	nct->colorSeq().loadFromStorage( ctnm );
    }
    return pdd;
}


void PlaneDataDisplay::showManipulator( bool yn )
{
    trect->getRectangle().showDraggers( yn );
}


bool PlaneDataDisplay::isManipulatorShown() const
{
    return trect->getRectangle().draggersShown();
}


bool PlaneDataDisplay::isManipulated() const
{ return getCubeSampling(true)!=getCubeSampling(false); }


void PlaneDataDisplay::resetManipulation()
{
    trect->getRectangle().resetManip();
}


void PlaneDataDisplay::acceptManipulation()
{
    setCubeSampling( getCubeSampling(true) );
    resetManipulation();
}


BufferString PlaneDataDisplay::getManipulationString() const
{
    BufferString res;
    if ( type == Inline )
    {
	res = "Inline: ";
	res += getCubeSampling(true).hrg.start.inl;
    }
    else if ( type == Crossline )
    {
	res = "Crossline: ";
	res += getCubeSampling(true).hrg.start.crl;
    }
    else
    {
	res = SI().zIsTime() ? "Time: " : "Depth: ";
	float val = getCubeSampling(true).zrg.start;
	res += SI().zIsTime() ? mNINT(val * 1000) : val;
    }

    return res;
}


BufferString PlaneDataDisplay::getManipulationPos() const
{
    BufferString res;
    if ( type == Inline )
	res = getCubeSampling(true).hrg.start.inl;
    else if ( type == Crossline )
	res = getCubeSampling(true).hrg.start.crl;
    else if ( type == Timeslice )
    {
	float val = getCubeSampling(true).zrg.start;
	res = SI().zIsTime() ? mNINT(val * 1000) : val;
    }

    return res;
}


int PlaneDataDisplay::nrResolutions() const
{
    return trect->getNrResolutions();
}


BufferString PlaneDataDisplay::getResolutionName( int res ) const
{
    if ( res == 1 ) return "Moderate";
    if ( res == 2 ) return "High";
    else return "Default";
}


int PlaneDataDisplay::getResolution() const
{
    return trect->getResolution();
}


void PlaneDataDisplay::setResolution( int res )
{
    trect->setResolution( res );
    if ( cache ) setData( cache );
    if ( colcache ) setData( colcache, colas.datatype );
}


const AttribSelSpec* PlaneDataDisplay::getSelSpec() const
{ return &as; }


void PlaneDataDisplay::setSelSpec( const AttribSelSpec& as_ )
{
    as = as_;
    delete cache;
    cache = 0;
    trect->useTexture( false );
    setName( as.userRef() );
}


void PlaneDataDisplay::setColorSelSpec( const ColorAttribSel& as_ )
{ colas = as_; }


const ColorAttribSel* PlaneDataDisplay::getColorSelSpec() const
{ return &colas; }


const TypeSet<float>* PlaneDataDisplay::getHistogram() const
{
    return &trect->getHistogram();
}


int PlaneDataDisplay::getColTabID() const
{
    return trect->getColorTab().id();
}


CubeSampling PlaneDataDisplay::getCubeSampling() const
{
    return getCubeSampling(true);
}


void PlaneDataDisplay::setCubeSampling( CubeSampling cs_ )
{
    Coord3 width( cs_.hrg.stop.inl - cs_.hrg.start.inl,
		  cs_.hrg.stop.crl - cs_.hrg.start.crl, 
		  cs_.zrg.stop - cs_.zrg.start );
    setWidth( width );
    Coord3 origo(cs_.hrg.start.inl,cs_.hrg.start.crl,cs_.zrg.start);
    setOrigo( origo );

    moving.trigger();
}


CubeSampling PlaneDataDisplay::getCubeSampling( bool manippos ) const
{
    visBase::Rectangle& rect = trect->getRectangle();
    CubeSampling cubesampl;
    cubesampl.hrg.start = 
	BinID( mNINT(manippos ? rect.manipOrigo().x : rect.origo().x),
	       mNINT(manippos ? rect.manipOrigo().y : rect.origo().y) );
    cubesampl.hrg.stop = cubesampl.hrg.start;
    cubesampl.hrg.step = BinID( SI().inlStep(), SI().crlStep() );

    float zrg0 = manippos ? rect.manipOrigo().z : rect.origo().z;
    cubesampl.zrg.start = (float)(int)(1000*zrg0+.5) / 1000;
    cubesampl.zrg.stop = cubesampl.zrg.start;

    if ( rect.orientation()==visBase::Rectangle::XY )
    {
	cubesampl.hrg.stop.inl += mNINT(rect.width(0,manippos));
	cubesampl.hrg.stop.crl += mNINT(rect.width(1,manippos));
    }
    else if ( rect.orientation()==visBase::Rectangle::XZ )
    {
	cubesampl.hrg.stop.inl += mNINT(rect.width(0,manippos));
	cubesampl.zrg.stop += rect.width(1,manippos);
    }
    else
    {
	cubesampl.hrg.stop.crl += mNINT(rect.width(1,manippos));
	cubesampl.zrg.stop += rect.width(0,manippos);
    }

    return cubesampl;
}


bool PlaneDataDisplay::setDataVolume( bool colordata, AttribSliceSet* sliceset )
{
    if ( colordata )
    {
	Interval<float> cliprate( colas.cliprate0, colas.cliprate1 );
	trect->setColorPars( colas.reverse, colas.useclip, 
			     colas.useclip ? cliprate : colas.range );
    }

    setData( sliceset, colordata ? colas.datatype : 0 );
    if ( colordata )
    {
	delete colcache;
	colcache = sliceset;
	return true;
    }

    delete cache;
    cache = sliceset;
    return true;
}


void PlaneDataDisplay::setData( const AttribSliceSet* sliceset, int datatype ) 
{
    if ( !sliceset )
    {
	trect->setData( 0, 0, 0 );
	return;
    }

    trect->removeAllTextures( true );
    const int nrslices = sliceset->size();
    for ( int slcidx=0; slcidx<nrslices; slcidx++ )
    {
	if ( slcidx )
	    trect->addTexture();

	const int lsz = (*sliceset)[slcidx]->info().getSize(0);
	const int zsz = (*sliceset)[slcidx]->info().getSize(1);
	const int slicesize = (*sliceset)[slcidx]->info().getTotalSz();

	if ( sliceset->direction == AttribSlice::Inl )
	{
	    PtrMan< Array2D<float> > datacube = new Array2DImpl<float>(zsz,lsz);
	    for ( int zidx=0; zidx<zsz; zidx++ )
	    {
		for ( int lidx=0; lidx<lsz; lidx++ )
		{
		    const float val = (*sliceset)[slcidx]->get( lidx, zidx );
		    datacube->set( zidx, lidx, val );
		}
	    }
	    trect->setData( datacube, slcidx, datatype );
	}
	else
	{
	    PtrMan< Array2D<float> > datacube = new Array2DImpl<float>(lsz,zsz);
	    float* data = datacube->getData();
	    memcpy( data, (*sliceset)[slcidx]->getData(), 
		    				slicesize*sizeof(float) );
	    trect->setData( datacube, slcidx, datatype );
	}
    }

    trect->finishTextures();
    trect->showTexture( 0 );
    trect->useTexture( true );
}


const AttribSliceSet* PlaneDataDisplay::getCacheVolume( bool colordata ) const
{
    return colordata ? colcache : cache;
}


int PlaneDataDisplay::nrTextures() const
{ return cache ? cache->size() : 0; }


void PlaneDataDisplay::selectTexture( int idx )
{
    if ( idx>=nrTextures() ) return;
    trect->showTexture( idx );
}


void PlaneDataDisplay::turnOn( bool yn )
{ trect->turnOn(yn); }


bool PlaneDataDisplay::isOn() const
{ return trect->isOn(); }


float PlaneDataDisplay::getValue( const Coord3& pos_ ) const
{
    if ( !cache ) return mUndefValue;
    const BinID bid = SI().transform(pos_);
    const Coord3 pos( bid.inl, bid.crl, pos_.z );

    Coord3 origo = trect->getRectangle().origo();
    Coord localpos;

    if ( type == Timeslice )
    {
	if ( fabs(pos.z-origo.z)> 1e-3 )
	    return mUndefValue;
	// x=inl y=crl
	localpos.x = pos.x-origo.x;
	localpos.y = pos.y-origo.y;
	localpos.x /= trect->getRectangle().width(0);
	localpos.y /= trect->getRectangle().width(1);

    }
    else if ( type == Crossline )
    {
	if ( fabs(pos.y-origo.y)> 1e-3 )
	    return mUndefValue;
	// x=inline y=depth
	localpos.x = pos.x-origo.x;
	localpos.y = pos.z-origo.z;

	localpos.x /= trect->getRectangle().width(0);
	localpos.y /= trect->getRectangle().width(1);
    }
    else
    {
	if ( fabs(pos.x-origo.x)> 1e-3 )
	    return mUndefValue;
	// x=crossline y=depth
	localpos.x = pos.y-origo.y;
	localpos.y = pos.z-origo.z;

	localpos.x /= trect->getRectangle().width(1);
	localpos.y /= trect->getRectangle().width(0);
    }

    if ( localpos.x > 1 || localpos.y > 1 || localpos.x < 0 || localpos.y < 0 )
	return mUndefValue;

    localpos.x *= ((*cache)[0]->info().getSize(0)-1);
    localpos.y *= ((*cache)[0]->info().getSize(1)-1);

    return (*cache)[0]->get(mNINT(localpos.x), mNINT(localpos.y));
}


void PlaneDataDisplay::setMaterial( visBase::Material* nm)
{ trect->setMaterial(nm); }


const visBase::Material* PlaneDataDisplay::getMaterial() const
{ return trect->getMaterial(); }


visBase::Material* PlaneDataDisplay::getMaterial()
{ return trect->getMaterial(); }


SoNode* PlaneDataDisplay::getInventorNode() { return trect->getInventorNode(); }


void PlaneDataDisplay::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    visBase::VisualObject::fillPar( par, saveids );
    int trectid = trect->id();
    par.set( trectstr, trectid );

    if ( saveids.indexOf( trectid )==-1 ) saveids += trectid;

    as.fillPar( par );
    colas.fillPar( par );
}


int PlaneDataDisplay::usePar( const IOPar& par )
{
    int res =  visBase::VisualObject::usePar( par );
    if ( res!=1 ) return res;

    int trectid;

    if ( !par.get( trectstr, trectid ))
	return -1;

    visBase::DataObject* dataobj = visBase::DM().getObj( trectid );
    if ( !dataobj ) return 0;

    mDynamicCastGet(visBase::TextureRect*,tr,dataobj)
    if ( !tr ) return -1;
    setTextureRect( tr );

    trect->getRectangle().setSnapping( true );
    trect->useTexture( false );

    if ( trect->getRectangle().orientation() == visBase::Rectangle::YZ )
	type = Inline;
    else if ( trect->getRectangle().orientation() == visBase::Rectangle::XZ )
	type = Crossline;
    else
	type = Timeslice;

    if ( !as.usePar(par) ) return -1;
    colas.usePar( par );

    return 1;
}

}; // namespace visSurvey

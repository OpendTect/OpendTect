/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2002
-*/

static const char* rcsID = "$Id: visplanedatadisplay.cc,v 1.53 2003-07-01 14:26:18 nanne Exp $";

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
#include "sorting.h"
#include "vistransform.h"
#include "iopar.h"
#include <math.h>

mCreateFactoryEntry( visSurvey::PlaneDataDisplay );

const char* visSurvey::PlaneDataDisplay::trectstr = "Texture rectangle";

visSurvey::PlaneDataDisplay::PlaneDataDisplay()
    : VisualObject(true)
    , trect(visBase::TextureRect::create())
    , cache(0)
    , colcache(0)
    , as(*new AttribSelSpec)
    , colas(*new ColorAttribSel)
    , cs(*new CubeSampling)
    , manipcs(*new CubeSampling)
    , moving(this)
{
    trect->ref();

    trect->getMaterial()->setAmbience( 0.8 );
    trect->getMaterial()->setDiffIntensity( 0.8 );
    trect->manipChanges()->notify( mCB(this,PlaneDataDisplay,manipChanged) );

    setType( Inline );


    trect->getRectangle().setSnapping( true );
    trect->useTexture( false );

//TODO: Fix! For some reason this is needed to let Texture2 work properly
    showDraggers(true);
    showDraggers(false);

    SPM().zscalechange.notify( mCB(this,PlaneDataDisplay,appVelChCB) );
}


visSurvey::PlaneDataDisplay::~PlaneDataDisplay()
{
    SPM().zscalechange.remove( mCB(this,PlaneDataDisplay,appVelChCB));
    trect->manipChanges()->remove( mCB(this,PlaneDataDisplay,manipChanged) );
    trect->unRef();
    delete &as;
    delete &colas;

    delete cache;
    delete colcache;
}


void visSurvey::PlaneDataDisplay::setType( Type nt )
{
    type = nt;
    setGeometry( false, true );
}


void visSurvey::PlaneDataDisplay::setGeometry( bool manip, bool init_ )
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


void visSurvey::PlaneDataDisplay::setRanges( const StepInterval<float>& irg,
					     const StepInterval<float>& crg,
					     const StepInterval<float>& zrg,
       					     bool manip )
{
    if ( type==Inline )
    {
	trect->getRectangle().setOrientation( visBase::Rectangle::YZ );
	trect->getRectangle().setRanges( zrg, crg, irg, manip );
    }
    else if ( type==Crossline )
    {
	trect->getRectangle().setOrientation( visBase::Rectangle::XZ );
	trect->getRectangle().setRanges( irg, zrg, crg, manip );
    }
    else
    {
	trect->getRectangle().setOrientation( visBase::Rectangle::XY );
	trect->getRectangle().setRanges( irg, crg, zrg, manip );
    }
}

void visSurvey::PlaneDataDisplay::setOrigo( const Coord3& pos )
{
    trect->getRectangle().setSnapping( false );
    trect->getRectangle().setOrigo( pos );
    trect->getRectangle().setSnapping( true );
}


void visSurvey::PlaneDataDisplay::setWidth( const Coord3& pos )
{
    if ( type==Inline )
	trect->getRectangle().setWidth( pos.z, pos.y );
    else if ( type==Crossline )
	trect->getRectangle().setWidth( pos.x, pos.z );
    else
	trect->getRectangle().setWidth( pos.x, pos.y );
}


void visSurvey::PlaneDataDisplay::resetDraggerSizes( float appvel )
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
    const float zfact = zFactor();

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


float visSurvey::PlaneDataDisplay::calcDist( const Coord3& pos ) const
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

	zdiff = (planeorigo.z - xytpos.z) * SPM().getZScale();
    }

    const float inldist =
	SI().transform( BinID(0,0)).distance( SI().transform(BinID(1,0)));
    const float crldist =
	SI().transform( BinID(0,0)).distance( SI().transform(BinID(0,1)));
    float inldiff = inlcrldist.inl * inldist;
    float crldiff = inlcrldist.crl * crldist;

    return sqrt( inldiff*inldiff + crldiff*crldiff + zdiff*zdiff );
}


void visSurvey::PlaneDataDisplay::appVelChCB( CallBacker* )
{
    resetDraggerSizes( SPM().getZScale() );
}


void visSurvey::PlaneDataDisplay::manipChanged( CallBacker* )
{
    moving.trigger();
}


void visSurvey::PlaneDataDisplay::showDraggers(bool yn)
{
    trect->getRectangle().displayDraggers(yn);
}


void visSurvey::PlaneDataDisplay::resetManip()
{
    trect->getRectangle().resetManip();
}


AttribSelSpec& visSurvey::PlaneDataDisplay::getAttribSelSpec()
{ return as; }

const AttribSelSpec& visSurvey::PlaneDataDisplay::getAttribSelSpec() const
{ return as; }

void visSurvey::PlaneDataDisplay::setAttribSelSpec( const AttribSelSpec& as_ )
{
    as = as_;
    delete cache;
    cache = 0;
    colas.datatype = 0;
    trect->useTexture( false );
    setName( as.userRef() );
}


ColorAttribSel& visSurvey::PlaneDataDisplay::getColorSelSpec()
{ return colas; }

const ColorAttribSel& visSurvey::PlaneDataDisplay::getColorSelSpec() const
{ return colas; }

void visSurvey::PlaneDataDisplay::setColorSelSpec( const ColorAttribSel& as_ )
{ colas = as_; }


CubeSampling& visSurvey::PlaneDataDisplay::getCubeSampling( bool manippos )
{
    CubeSampling& cubesampl = manippos ? manipcs : cs;
    cubesampl.hrg.start = BinID( mNINT(manippos
				? trect->getRectangle().manipOrigo().x
				: trect->getRectangle().origo().x),
		  	  mNINT(manippos
				? trect->getRectangle().manipOrigo().y
				: trect->getRectangle().origo().y) );

    cubesampl.hrg.stop = cubesampl.hrg.start;
    cubesampl.hrg.step = BinID( SI().inlStep(), SI().crlStep() );

    float zrg0 = manippos ? trect->getRectangle().manipOrigo().z
			  : trect->getRectangle().origo().z;
    cubesampl.zrg.start = (float)(int)(1000*zrg0+.5) / 1000;
    cubesampl.zrg.stop = cubesampl.zrg.start;

    if ( trect->getRectangle().orientation()==visBase::Rectangle::XY )
    {
	cubesampl.hrg.stop.inl += 
			mNINT(trect->getRectangle().width(0,manippos));
	cubesampl.hrg.stop.crl += 
	    		mNINT(trect->getRectangle().width(1,manippos));
    }
    else if ( trect->getRectangle().orientation()==visBase::Rectangle::XZ )
    {
	cubesampl.hrg.stop.inl += 
	    		mNINT(trect->getRectangle().width(0,manippos));
	cubesampl.zrg.stop += trect->getRectangle().width(1,manippos);
    }
    else
    {
	cubesampl.hrg.stop.crl += 
	    		mNINT(trect->getRectangle().width(1,manippos));
	cubesampl.zrg.stop += trect->getRectangle().width(0,manippos);
    }

    return cubesampl;
}


void visSurvey::PlaneDataDisplay::setCubeSampling( const CubeSampling& cs_ )
{
    Coord3 width( cs_.hrg.stop.inl - cs_.hrg.start.inl,
		  cs_.hrg.stop.crl - cs_.hrg.start.crl, 
		  cs_.zrg.stop - cs_.zrg.start );
    setWidth( width );
    Coord3 origo(cs_.hrg.start.inl,cs_.hrg.start.crl,cs_.zrg.start);
    setOrigo( origo );

    moving.trigger();
}


void visSurvey::PlaneDataDisplay::setSliceIdx( int idx )
{
    if ( !cache || idx >= cache->size() ) return;
    trect->showTexture( idx );
}


bool visSurvey::PlaneDataDisplay::putNewData( AttribSliceSet* sliceset, 
					      bool colordata )
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

    
void visSurvey::PlaneDataDisplay::setData( const AttribSliceSet* sliceset,
       					   int datatype	) 
{
    trect->clear();

    if ( !sliceset )
    {
	trect->setData( 0, 0, 0 );
	return;
    }

    const int nrslices = sliceset->size();
    for ( int slcidx=0; slcidx<nrslices; slcidx++ )
    {
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

    trect->showTexture( 0 );
    trect->useTexture( true );
}


const AttribSliceSet* visSurvey::PlaneDataDisplay::getPrevData() const
{
    return cache;
}


void visSurvey::PlaneDataDisplay::turnOn(bool n) { trect->turnOn(n); }


bool visSurvey::PlaneDataDisplay::isOn() const { return trect->isOn(); }


void visSurvey::PlaneDataDisplay::setColorTab( visBase::VisColorTab& ct )
{ trect->setColorTab( ct ); }


visBase::VisColorTab& visSurvey::PlaneDataDisplay::getColorTab()
{ return trect->getColorTab(); }


const visBase::VisColorTab& visSurvey::PlaneDataDisplay::getColorTab() const
{ return trect->getColorTab(); }


float visSurvey::PlaneDataDisplay::getValue( const Coord3& pos ) const
{
    if ( !cache ) return mUndefValue;

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


void visSurvey::PlaneDataDisplay::setMaterial( visBase::Material* nm)
{ trect->setMaterial(nm); }


const visBase::Material* visSurvey::PlaneDataDisplay::getMaterial() const
{ return trect->getMaterial(); }


visBase::Material* visSurvey::PlaneDataDisplay::getMaterial()
{ return trect->getMaterial(); }


SoNode* visSurvey::PlaneDataDisplay::getData() { return trect->getData(); }


const char* visSurvey::PlaneDataDisplay::getResName( int res ) const
{
    if ( res == 1 ) return "Moderate";
    if ( res == 2 ) return "High";
    else return "Default";
}


void visSurvey::PlaneDataDisplay::setResolution( int res )
{
    trect->setResolution( res );
    if ( cache ) setData( cache );
    if ( colcache ) setData( colcache, colas.datatype );
}


int visSurvey::PlaneDataDisplay::getResolution() const
{
    return trect->getResolution();
}

int visSurvey::PlaneDataDisplay::getNrResolutions() const
{
    return trect->getNrResolutions();
}


const TypeSet<float>& visSurvey::PlaneDataDisplay::getHistogram() const
{
    return trect->getHistogram();
}


void visSurvey::PlaneDataDisplay::fillPar( IOPar& par,
	TypeSet<int>& saveids ) const
{
    visBase::VisualObject::fillPar( par, saveids );
    int trectid = trect->id();
    par.set( trectstr, trectid );

    if ( saveids.indexOf( trectid )==-1 ) saveids += trectid;

    as.fillPar(par);
}


int visSurvey::PlaneDataDisplay::usePar( const IOPar& par )
{
    int res =  visBase::VisualObject::usePar( par );
    if ( res!=1 ) return res;

    int trectid;

    if ( !par.get( trectstr, trectid ))
	return -1;

    visBase::DataObject* dataobj = visBase::DM().getObj( trectid );
    if ( !dataobj ) return 0;

    mDynamicCastGet( visBase::TextureRect*, tr, dataobj );
    if ( !tr ) return -1;

    trect->unRef();
    trect = tr;
    trect->ref();

    trect->getRectangle().setSnapping( true );
    trect->useTexture( false );

    if ( trect->getRectangle().orientation() == visBase::Rectangle::YZ )
	type = Inline;
    else if ( trect->getRectangle().orientation() == visBase::Rectangle::XZ )
	type = Crossline;
    else
	type = Timeslice;

    if ( !as.usePar( par )) return -1;

    return 1;
}


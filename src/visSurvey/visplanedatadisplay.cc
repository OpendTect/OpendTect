/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2002
-*/

static const char* rcsID = "$Id: visplanedatadisplay.cc,v 1.72 2004-07-29 21:41:26 bert Exp $";

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
    trect->getColorTab().sequencechange.remove(
				mCB(this,PlaneDataDisplay,coltabChanged) );
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
	trect->getColorTab().sequencechange.remove(
				mCB(this,PlaneDataDisplay,coltabChanged) );
	trect->unRef();
    }

    trect = tr;
    trect->ref();
    trect->manipChanges()->notify( mCB(this,PlaneDataDisplay,manipChanged) );
    trect->getColorTab().sequencechange.notify( 
	    			mCB(this,PlaneDataDisplay,coltabChanged) );
}


void PlaneDataDisplay::setType( Type nt )
{
    type = nt;
    setGeometry( false, true );
}


void PlaneDataDisplay::setGeometry( bool manip, bool init_ )
{
    BinID startbid = SI().sampling(true).hrg.start;
    BinID stopbid = SI().sampling(true).hrg.stop;
    StepInterval<float> vrgd = SI().zRange(true);

    StepInterval<float> inlrg( startbid.inl,stopbid.inl,SI().inlStep(true) );
    StepInterval<float> crlrg( startbid.crl,stopbid.crl,SI().crlStep(true) );
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
    BinID startbid = SI().sampling(true).hrg.start;
    BinID stopbid = SI().sampling(true).hrg.stop;
    StepInterval<float> vrgd = SI().zRange(true);

    StepInterval<float> inlrange( startbid.inl,stopbid.inl,SI().inlStep(true) );
    StepInterval<float> crlrange( startbid.crl,stopbid.crl,SI().crlStep(true) );

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


float PlaneDataDisplay::maxDist() const
{
    float maxzdist = SI().zFactor()*SPM().getZScale()*SI().zRange().step / 2;
    return type == Timeslice ? maxzdist : SurveyObject::sDefMaxDist;
}


void PlaneDataDisplay::appVelChCB( CallBacker* )
{
    resetDraggerSizes( SPM().getZScale() );
}


void PlaneDataDisplay::manipChanged( CallBacker* )
{
    moving.trigger();
}


void PlaneDataDisplay::coltabChanged( CallBacker* )
{
    // Hack for correct transparency display
    bool manipshown = isManipulatorShown();
    if ( manipshown ) return;
    showManipulator( true );
    showManipulator( false );
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

	PtrMan< Array2D<float> > datacube = createArray( sliceset, slcidx );
	trect->setData( datacube, slcidx, datatype );
    }

    trect->finishTextures();
    trect->showTexture( 0 );
    trect->useTexture( true );
}


#define mInlIdx ((curinl-datacs.hrg.start.inl) / datacs.hrg.step.inl)
#define mCrlIdx ((curcrl-datacs.hrg.start.crl) / datacs.hrg.step.crl)
#define mZIdx (datacs.zrg.nearestIndex(curz))

#define mInZrg() (curz>=datacs.zrg.start-1e-4 && curz<=datacs.zrg.stop+1e-4)

Array2D<float>* PlaneDataDisplay::createArray( const AttribSliceSet* sliceset, 
					       int slcidx ) const
{
    CubeSampling cs = getCubeSampling(true);
    CubeSampling datacs = sliceset->sampling;

    const int nrinl = cs.nrInl();
    const int nrcrl = cs.nrCrl();
    const int nrz = cs.nrZ();

    Array2DImpl<float>* datacube = new Array2DImpl<float>(0,0);
    if ( sliceset->direction == AttribSlice::Hor )
    {
	datacube->setSize( nrinl, nrcrl );
	int curinl, curcrl;
	float val;
	for ( int inlidx=0; inlidx<nrinl; inlidx++ )
	{
	    curinl = cs.hrg.start.inl + inlidx*cs.hrg.step.inl;
	    for ( int crlidx=0; crlidx<nrcrl; crlidx++ )
	    {
		curcrl = cs.hrg.start.crl + crlidx*cs.hrg.step.crl;
		if ( !datacs.hrg.includes(BinID(curinl,curcrl)) )
		    val = mUndefValue;
		else
		{
		    int datainlidx = mInlIdx;
		    int datacrlidx = mCrlIdx;
		    val = (*sliceset)[slcidx]->get(datainlidx,datacrlidx);
		}

		datacube->set( inlidx, crlidx, val );
	    }
	}
    }
    else if ( sliceset->direction == AttribSlice::Crl )
    {
	datacube->setSize( nrinl, nrz );
	int curinl;
	float curz;
	float val;
	for ( int inlidx=0; inlidx<nrinl; inlidx++ )
	{
	    curinl = cs.hrg.start.inl + inlidx*cs.hrg.step.inl;
	    for ( int zidx=0; zidx<nrz; zidx++ )
	    {
		curz = cs.zrg.atIndex( zidx );
		if ( !datacs.hrg.inlOK(curinl) || !mInZrg() )
		    val = mUndefValue;
		else
		{
		    int datainlidx = mInlIdx;
		    int datazidx = mZIdx;
		    val = (*sliceset)[slcidx]->get(datainlidx,datazidx);
		}

		datacube->set( inlidx, zidx, val );
	    }
	}
    }
    else if ( sliceset->direction == AttribSlice::Inl )
    {
	datacube->setSize( nrz, nrcrl );
	int curcrl;
	float curz;
	float val;
	for ( int crlidx=0; crlidx<nrcrl; crlidx++ )
	{
	    curcrl = cs.hrg.start.crl + crlidx*cs.hrg.step.crl;
	    for ( int zidx=0; zidx<nrz; zidx++ )
	    {
		curz = cs.zrg.atIndex( zidx );
		if ( !datacs.hrg.crlOK(curcrl) || !mInZrg() )
		    val = mUndefValue;
		else
		{
		    int datacrlidx = mCrlIdx;
		    int datazidx = mZIdx;
		    val = (*sliceset)[slcidx]->get(datacrlidx,datazidx);
		}

		datacube->set( zidx, crlidx, val );
	    }
	}
    }

    return datacube;
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


#define mIsValid(idx,sz) ( idx>=0 && idx<sz )

float PlaneDataDisplay::getValue( const Coord3& pos_ ) const
{
    if ( !cache ) return mUndefValue;
    const BinID bid = SI().transform(pos_);

    int idx0, idx1, idx2;
    cache->getIdxs( bid.inl, bid.crl, pos_.z, idx0, idx1, idx2 );

    const int sz0 = cache->size();
    if ( !mIsValid(idx0,sz0) ) return mUndefValue;
    
    const int sz1 = (*cache)[idx0]->info().getSize(0);
    const int sz2 = (*cache)[idx0]->info().getSize(1);
    if ( !mIsValid(idx1,sz1) || !mIsValid(idx2,sz2) ) return mUndefValue;
    
    return (*cache)[idx0]->get( idx1, idx2 );
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

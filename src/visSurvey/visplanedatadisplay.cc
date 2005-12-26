/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2002
-*/

static const char* rcsID = "$Id: visplanedatadisplay.cc,v 1.100 2005-12-26 17:09:16 cvskris Exp $";

#include "visplanedatadisplay.h"

#include "attribsel.h"
#include "cubesampling.h"
#include "attribslice.h"
#include "attribdatacubes.h"
#include "arrayndslice.h"
#include "vistexturerect.h"
#include "arrayndimpl.h"
#include "position.h"
#include "survinfo.h"
#include "samplfunc.h"
#include "visselman.h"
#include "visdataman.h"
#include "visrectangle.h"
#include "vismaterial.h"
#include "viscolortab.h"
#include "viscolorseq.h"
#include "sorting.h"
#include "vistransform.h"
#include "vistransmgr.h"
#include "iopar.h"
#include "colortab.h"
#include "zaxistransform.h"
#include "genericnumer.h"
#include <math.h>

mCreateFactoryEntry( visSurvey::PlaneDataDisplay );


namespace visSurvey {

const char* PlaneDataDisplay::trectstr = "Texture rectangle";

PlaneDataDisplay::PlaneDataDisplay()
    : VisualObject(true)
    , trect(0)
    , cache(0)
    , colcache(0)
    , as(*new Attrib::SelSpec)
    , colas(*new Attrib::ColorSelSpec)
    , manipulating(this)
    , moving(this)
    , curicstep(SI().inlStep(),SI().crlStep())
    , curzstep(SI().zStep())
    , datatransform( 0 )
    , datatransformvoihandle( -1 )
{
    setTextureRect( visBase::TextureRect::create() );

    trect->getMaterial()->setColor( Color::White );
    trect->getMaterial()->setAmbience( 0.8 );
    trect->getMaterial()->setDiffIntensity( 0.8 );

    setType( Inline );

    trect->getRectangle().setSnapping( true );
    trect->useTexture( false );

//TODO: Fix! For some reason this is needed to let Texture2 work properly
    showManipulator(true);
    showManipulator(false);
}


PlaneDataDisplay::~PlaneDataDisplay()
{
    trect->manipChanges()->remove( mCB(this,PlaneDataDisplay,manipChanged) );
    trect->getColorTab().sequencechange.remove(
				mCB(this,PlaneDataDisplay,coltabChanged) );
    trect->unRef();
    delete &as;
    delete &colas;

    if ( cache ) cache->unRef();
    if ( colcache ) colcache->unRef();

    if ( scene_ )
	scene_->zscalechange.remove( mCB(this,PlaneDataDisplay,zScaleChanged) );

    setDataTransform( 0 );
}


void PlaneDataDisplay::setUpConnections()
{
    if ( scene_ )
    {
	scene_->zscalechange.notify( mCB(this,PlaneDataDisplay,zScaleChanged) );
	zScaleChanged(0);
    }
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
    if ( datatransform )
    {
	Interval<float> zrg = datatransform->getZInterval( false );
	vrgd.start = zrg.start;
	vrgd.stop = zrg.stop;
    }
	
    StepInterval<float> inlrg( startbid.inl,stopbid.inl,SI().inlStep() );
    StepInterval<float> crlrg( startbid.crl,stopbid.crl,SI().crlStep() );
    StepInterval<float> vrg( vrgd.start, vrgd.stop, vrgd.step );
    if ( init_ || datatransform )
	trect->getRectangle().setOrigo( 
				Coord3(inlrg.start,crlrg.start,vrg.start) );

    setRanges( inlrg, crlrg, vrg, manip );

    resetDraggerSizes( scene_ ? scene_->getZScale() : STM().defZScale() );
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

    StepInterval<float> inlrange( startbid.inl,stopbid.inl,SI().inlStep() );
    StepInterval<float> crlrange( startbid.crl,stopbid.crl,SI().crlStep() );

    float inlspacing = SI().transform(BinID(0,0)).distance(
		       SI().transform( BinID(1,0)));
    float crlspacing = SI().transform(BinID(0,0)).distance(
		       SI().transform( BinID(0,1)));

    float baselength = ( crlrange.width() * crlspacing +
			 SI().zRange(true).width()*happvel +
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
    const mVisTrans* utm2display = scene_->getUTM2DisplayTransform();
    Coord3 xytpos = utm2display->transformBack( pos );
    Coord3 planeorigo = trect->getRectangle().origo();
    float width0 = trect->getRectangle().width( 0 );
    float width1 = trect->getRectangle().width( 1 );

    BinID binid = SI().transform( Coord(xytpos.x,xytpos.y) );
    
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

	zdiff = (planeorigo.z-xytpos.z) * SI().zFactor() * scene_->getZScale();
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
    float maxzdist = SI().zFactor() * scene_->getZScale() * SI().zStep() / 2;
    return type == Timeslice ? maxzdist : SurveyObject::sDefMaxDist;
}


bool PlaneDataDisplay::setDataTransform( ZAxisTransform* zat )
{
    if ( datatransform )
    {
	if ( datatransformvoihandle!=-1 )
	    datatransform->removeVolumeOfInterest(datatransformvoihandle);
	datatransform->unRef();
	datatransform = 0;
    }

    datatransform = zat;
    datatransformvoihandle = -1;

    if ( datatransform )
    {
	datatransform->ref();
	setGeometry( false, false );
    }

    return true;
}


void PlaneDataDisplay::zScaleChanged( CallBacker* )
{
    if ( !scene_ ) return;
    resetDraggerSizes( scene_->getZScale() );
}


void PlaneDataDisplay::manipChanged( CallBacker* )
{
    manipulating.trigger();
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

    int ctid = pdd->getColTabID();
    visBase::DataObject* obj = ctid>=0 ? visBase::DM().getObject( ctid ) : 0;
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
    CubeSampling newcs = getCubeSampling(true);
    resetManipulation();
    setCubeSampling( newcs );
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


int PlaneDataDisplay::getResolution() const
{
    return trect->getResolution();
}


void PlaneDataDisplay::setResolution( int res )
{
    trect->setResolution( res );
    if ( cache ) setData( cache, 0 );
    if ( colcache ) setData( colcache, colas.datatype );
}


const Attrib::SelSpec* PlaneDataDisplay::getSelSpec() const
{ return &as; }


void PlaneDataDisplay::setSelSpec( const Attrib::SelSpec& as_ )
{
    as = as_;
    if ( cache ) cache->unRef();
    cache = 0;

    const char* usrref = as.userRef();
    if ( !usrref || !*usrref )
	trect->useTexture( false );

    setName( usrref );
}


void PlaneDataDisplay::setColorSelSpec( const Attrib::ColorSelSpec& as_ )
{ colas = as_; }


const Attrib::ColorSelSpec* PlaneDataDisplay::getColorSelSpec() const
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
    setOrigo( Coord3(cs_.hrg.start.inl,cs_.hrg.start.crl,cs_.zrg.start) );

    curicstep = cs_.hrg.step;
    curzstep = cs_.zrg.step;

    moving.trigger();

    if ( !datatransform )
	return;

    if ( datatransformvoihandle!=-1 )
	datatransform->setVolumeOfInterest( datatransformvoihandle, cs_ );
}


CubeSampling PlaneDataDisplay::getCubeSampling( bool manippos ) const
{
    visBase::Rectangle& rect = trect->getRectangle();
    CubeSampling cubesampl;
    cubesampl.hrg.start = 
	BinID( mNINT(manippos ? rect.manipOrigo().x : rect.origo().x),
	       mNINT(manippos ? rect.manipOrigo().y : rect.origo().y) );
    cubesampl.hrg.stop = cubesampl.hrg.start;
    cubesampl.hrg.step = curicstep;

    float zrg0 = manippos ? rect.manipOrigo().z : rect.origo().z;
    cubesampl.zrg.start = (float)(int)(1000*zrg0+.5) / 1000;
    cubesampl.zrg.stop = cubesampl.zrg.start;
    cubesampl.zrg.step = curzstep;

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

    if ( datatransform )
	assign( cubesampl.zrg, SI().zRange(true) );

    return cubesampl;
}


bool PlaneDataDisplay::setDataVolume( bool colordata, 
				      const Attrib::DataCubes* datacubes )
{
    if ( colordata )
    {
	Interval<float> cliprate( colas.cliprate0, colas.cliprate1 );
	trect->setColorPars( colas.reverse, colas.useclip, 
			     colas.useclip ? cliprate : colas.range );
    }

    setData( datacubes, colordata ? colas.datatype : 0 );
    if ( colordata )
    {
	if ( colcache ) colcache->unRef();
	colcache = datacubes;
	colcache->ref();
	return true;
    }

    if ( cache ) cache->unRef();
    cache = datacubes;
    cache->ref();
    return true;
}


void PlaneDataDisplay::setData( const Attrib::DataCubes* datacubes,
				int datatype )
{
    visBase::VisColorTab& coltab = trect->getColorTab();
    coltab.ref();
    trect->removeAllTextures( true );
    trect->setColorTab( coltab );
    coltab.unRef();

    if ( !datacubes )
    {
	trect->setData( 0, 0, 0 );
	trect->useTexture( false );
	return;
    }

    checkCubeSampling( datacubes->cubeSampling() );

    int unuseddim, dim0, dim1;
    if ( type==Inline )
    {
	unuseddim = Attrib::DataCubes::cInlDim();
	dim0 = Attrib::DataCubes::cZDim();
	dim1 = Attrib::DataCubes::cCrlDim();
    }
    else if ( type==Crossline )
    {
	unuseddim = Attrib::DataCubes::cCrlDim();
	dim0 = Attrib::DataCubes::cInlDim();
	dim1 = Attrib::DataCubes::cZDim();
    }
    else
    {
	unuseddim = Attrib::DataCubes::cZDim();
	dim0 = Attrib::DataCubes::cInlDim();
	dim1 = Attrib::DataCubes::cCrlDim();
    }

    const int nrcubes = datacubes->nrCubes();
    for ( int idx=0; idx<nrcubes; idx++ )
    {
	PtrMan<Array3D<float> > tmparray = 0;
	const Array3D<float>* usedarray = 0;
	if ( !datatransform )
	    usedarray = &datacubes->getCube(idx);
	else
	{
	    const CubeSampling cs = getCubeSampling();

	    if ( datatransformvoihandle==-1 )
		datatransformvoihandle = datatransform->addVolumeOfInterest(cs);
	    else
		datatransform->setVolumeOfInterest(datatransformvoihandle,cs);

	    datatransform->loadDataIfMissing( datatransformvoihandle );

	    ZAxisTransformSampler outpsampler( *datatransform, true, BinID(0,0),
		    	SamplingData<double>(cs.zrg.start, cs.zrg.step));
	    const Array3D<float>& srcarray = datacubes->getCube( idx );
	    const Array3DInfo& info = srcarray.info();
	    const int inlsz = info.getSize( Attrib::DataCubes::cInlDim() );
	    const int crlsz = info.getSize( Attrib::DataCubes::cCrlDim() );
	    const int zsz = cs.zrg.nrSteps()+1;
	    tmparray = new Array3DImpl<float>( inlsz, crlsz, zsz );
	    usedarray = tmparray;

	    for ( int inlidx=0; inlidx<inlsz; inlidx++ )
	    {
		for ( int crlidx=0; crlidx<crlsz; crlidx++ )
		{
		    const BinID bid( datacubes->inlsampling.atIndex(inlidx),
			   	     datacubes->crlsampling.atIndex(crlidx) );
		    outpsampler.setBinID( bid );
		    outpsampler.computeCache( Interval<int>(0,zsz-1) );

		    const float* inputptr = srcarray.getData() +
					    info.getMemPos(inlidx,crlidx,0);
		    float* outputptr = tmparray->getData() +
				    tmparray->info().getMemPos(inlidx,crlidx,0);

		    const SampledFunctionImpl<float,const float*>
			inputfunc( inputptr,
				   info.getSize(Attrib::DataCubes::cZDim()),
				   datacubes->z0*datacubes->zstep,
				   datacubes->zstep );

		    reSample( inputfunc, outpsampler, outputptr, zsz );
		}
	    }
	}

	if ( idx>0 ) trect->addTexture();

	Array2DSlice<float> slice(*usedarray);
	slice.setPos( unuseddim, 0 );
	slice.setDimMap( 0, dim0 );
	slice.setDimMap( 1, dim1 );

	if ( slice.init() )
	    trect->setData( &slice, idx, datatype );
	else
	{
	    trect->removeAllTextures( true );
	    pErrMsg( "Could not init slice." );
	}
    }

    trect->finishTextures();
    trect->showTexture( 0 );
    trect->useTexture( true );
}


#define mSetRg( var, ic ) cs.hrg.var.ic = datacs.hrg.var.ic
void PlaneDataDisplay::checkCubeSampling( const CubeSampling& datacs )
{
    bool setnewcs = false;
    CubeSampling cs = getCubeSampling(true);
    if ( cs.hrg.step.inl != datacs.hrg.step.inl )
    { mSetRg(start,inl); mSetRg(stop,inl); mSetRg(step,inl); setnewcs = true; }

    if ( cs.hrg.step.crl != datacs.hrg.step.crl )
    { mSetRg(start,crl); mSetRg(stop,crl); mSetRg(step,crl); setnewcs = true; }

    if ( cs.zrg.step != datacs.zrg.step )
    {  assign( cs.zrg, datacs.zrg ); setnewcs = true; } 

    if ( setnewcs )
    {
	setCubeSampling( cs );
	resetManipulation();
    }
}


const Attrib::DataCubes* PlaneDataDisplay::getCacheVolume(bool colordata) const
{
    return colordata ? colcache : cache;
}


int PlaneDataDisplay::nrTextures() const
{ return cache ? cache->nrCubes() : 0; }


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

void PlaneDataDisplay::getMousePosInfo( const visBase::EventInfo&,
					const Coord3& pos,
					float& val, 
					BufferString& info ) const
{
    info = getManipulationString();
    if ( !cache ) { val = mUdf(float); return; }
    const BinIDValue bidv( SI().transform(pos), pos.z );
    if ( !cache->getValue(trect->shownTexture(),bidv,&val,false) )
	{ val = mUdf(float); return; }

    return;
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

    visBase::DataObject* dataobj = visBase::DM().getObject( trectid );
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

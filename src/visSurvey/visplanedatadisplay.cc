/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2002
-*/

static const char* rcsID = "$Id: visplanedatadisplay.cc,v 1.105 2006-01-24 06:32:32 cvskris Exp $";

#include "visplanedatadisplay.h"

#include "attribsel.h"
#include "attribdatacubes.h"
#include "arrayndslice.h"
#include "genericnumer.h"
#include "vistexturerect.h"
#include "survinfo.h"
#include "samplfunc.h"
#include "visdataman.h"
#include "visrectangle.h"
#include "vismaterial.h"
#include "viscolortab.h"
#include "viscoord.h"
#include "visdepthtabplanedragger.h"
#include "vispickstyle.h"
#include "visfaceset.h"
#include "vismultitexture2.h"
#include "vistransform.h"
#include "iopar.h"
#include "zaxistransform.h"



mCreateFactoryEntry( visSurvey::PlaneDataDisplay );


namespace visSurvey {

DefineEnumNames(PlaneDataDisplay,Orientation,1,"Orientation")
{ "Inline", "Crossline", "Timeslice", 0 };


PlaneDataDisplay::PlaneDataDisplay()
    : VisualObjectImpl(true)
    , texture( visBase::MultiTexture2::create() )
    , rectangle( visBase::FaceSet::create() )
    , rectanglepickstyle( visBase::PickStyle::create() )
    , dragger( visBase::DepthTabPlaneDragger::create() )
    , curicstep(SI().inlStep(),SI().crlStep())
    , curzstep(SI().zStep())
    , datatransform( 0 )
    , datatransformvoihandle( -1 )
    , moving( this )
{
    cache.allowNull( true );
    dragger->ref();
    addChild( dragger->getInventorNode() );
    dragger->finished.notify( mCB( this, PlaneDataDisplay, draggerFinish ) );
    dragger->rightClicked()->notify(
	    		mCB( this, PlaneDataDisplay, draggerRightClick ) );

    rectanglepickstyle->ref();
    addChild( rectanglepickstyle->getInventorNode() );

    texture->ref();
    addChild( texture->getInventorNode() );
    texture->setTextureRenderQuality(1);

    rectangle->ref();
    addChild( rectangle->getInventorNode() );
    rectangle->setCoordIndex( 0, 0 );
    rectangle->setCoordIndex( 1, 1 );
    rectangle->setCoordIndex( 2, 2 );
    rectangle->setCoordIndex( 3, 3 );
    rectangle->setCoordIndex( 4, -1 );
    rectangle->setVertexOrdering(0);
    rectangle->setShapeType(0);

    material->setColor( Color::White );
    material->setAmbience( 0.8 );
    material->setDiffIntensity( 0.8 );


    setOrientation( Inline );
    updateRanges();

    as += new Attrib::SelSpec;
    cache += 0;
}


PlaneDataDisplay::~PlaneDataDisplay()
{
    dragger->finished.remove( mCB( this, PlaneDataDisplay, draggerFinish ) );
    dragger->rightClicked()->remove(
	    		mCB( this, PlaneDataDisplay, draggerRightClick ) );

    deepErase( as );
    deepUnRef( cache );

    setDataTransform( 0 );

    texture->unRef();
    rectangle->unRef();
    dragger->unRef();
    rectanglepickstyle->unRef();
}


void PlaneDataDisplay::setOrientation( Orientation nt )
{
    orientation = nt;

    dragger->setDim( (int) nt );
    updateRanges();
}


void PlaneDataDisplay::updateRanges()
{
    CubeSampling survey( SI().sampling(true) );
    if ( datatransform )
	assign( survey.zrg,  datatransform->getZInterval( false ) );
	
    const Interval<float> inlrg( survey.hrg.start.inl, survey.hrg.stop.inl );
    const Interval<float> crlrg( survey.hrg.start.crl, survey.hrg.stop.crl );

    dragger->setSpaceLimits( inlrg, crlrg, survey.zrg );
    dragger->setSize( Coord3(inlrg.width(), crlrg.width(), survey.zrg.width()));

    const CubeSampling cs = snapCubeSampling( survey );
    if ( cs!=getCubeSampling(false,true) )
	setCubeSampling( cs );
}


CubeSampling PlaneDataDisplay::snapCubeSampling( const CubeSampling& cs ) const
{
    CubeSampling res( cs );
    const Interval<float> inlrg( res.hrg.start.inl, res.hrg.stop.inl );
    const Interval<float> crlrg( res.hrg.start.crl, res.hrg.stop.crl );
    const Interval<float> zrg( res.zrg );

    res.snapToSurvey();

    if ( orientation==Inline )
	res.hrg.start.inl = res.hrg.stop.inl =
	    SI().inlRange(true).snap( inlrg.center() );
    else if ( orientation==Crossline )
	res.hrg.start.crl = res.hrg.stop.crl =
	    SI().crlRange(true).snap( crlrg.center() );
    else
	res.zrg.start = res.zrg.stop = SI().zRange(true).snap(zrg.center());

    return res;
}


float PlaneDataDisplay::calcDist( const Coord3& pos ) const
{
    const mVisTrans* utm2display = scene_->getUTM2DisplayTransform();
    const Coord3 xytpos = utm2display->transformBack( pos );
    const BinID binid = SI().transform( Coord(xytpos.x,xytpos.y) );

    const CubeSampling cs = getCubeSampling(false,true);
    
    BinID inlcrldist( 0, 0 );
    float zdiff = 0;

    inlcrldist.inl =
	binid.inl>=cs.hrg.start.inl && binid.inl<=cs.hrg.stop.inl 
	     ? 0
	     : mMIN( abs(binid.inl-cs.hrg.start.inl),
		     abs( binid.inl-cs.hrg.stop.inl) );
    inlcrldist.crl =
	binid.crl>=cs.hrg.start.crl && binid.crl<=cs.hrg.stop.crl 
	     ? 0
	     : mMIN( abs(binid.crl-cs.hrg.start.crl),
		     abs( binid.crl-cs.hrg.stop.crl) );
    zdiff = cs.zrg.includes(xytpos.z)
	? 0
	: mMIN(xytpos.z-cs.zrg.start,xytpos.z-cs.zrg.stop) *
	  SI().zFactor() * scene_->getZScale();

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
    return orientation==Timeslice ? maxzdist : SurveyObject::sDefMaxDist;
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
	updateRanges();
    }

    return true;
}


void PlaneDataDisplay::draggerFinish( CallBacker* cb )
{
    const CubeSampling cs = getCubeSampling(true,true);
    const CubeSampling snappedcs = snapCubeSampling( cs );

    if ( cs!=snappedcs )
	setDraggerPos( snappedcs );
}


void PlaneDataDisplay::draggerRightClick( CallBacker* cb )
{
    triggerRightClick( dragger->rightClickedEventInfo() );
}


void PlaneDataDisplay::setDraggerPos( const CubeSampling& cs )
{
    const Coord3 center( (cs.hrg.start.inl+cs.hrg.stop.inl)/2,
		         (cs.hrg.start.crl+cs.hrg.stop.crl)/2,
		         cs.zrg.center() );
    Coord3 width( cs.hrg.stop.inl-cs.hrg.start.inl,
		  cs.hrg.stop.crl-cs.hrg.start.crl, cs.zrg.width() );

    const Coord3 oldwidth = dragger->size();
    width[(int)orientation] = oldwidth[(int)orientation];

    dragger->setCenter( center );
    dragger->setSize( width );
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

/*
SurveyObject* PlaneDataDisplay::duplicate() const
{
    PlaneDataDisplay* pdd = create();
    pdd->setOrientation( orientation );
    pdd->setCubeSampling( getCubeSampling() );
    pdd->setResolution( getResolution() );

    int ctid = pdd->getColTabID();
    visBase::DataObject* obj = ctid>=0 ? visBase::DM().getObject( ctid ) : 0;
    mDynamicCastGet(visBase::VisColorTab*,nct,obj);
    if ( nct )
    {
	const char* ctnm = texture->getColorTab().colorSeq().colors().name();
	nct->colorSeq().loadFromStorage( ctnm );
    }
    return pdd;
}
*/


void PlaneDataDisplay::showManipulator( bool yn )
{
    dragger->turnOn( yn );
    rectanglepickstyle->setStyle( yn ? visBase::PickStyle::Unpickable
				     : visBase::PickStyle::Shape );
}


bool PlaneDataDisplay::isManipulatorShown() const
{
    return dragger->isOn();
}


bool PlaneDataDisplay::isManipulated() const
{ return getCubeSampling(true,true)!=getCubeSampling(false,true); }


void PlaneDataDisplay::resetManipulation()
{
    CubeSampling cs = getCubeSampling(false,true);
    setDraggerPos(cs);
}


void PlaneDataDisplay::acceptManipulation()
{
    CubeSampling cs = getCubeSampling(true,true);
    setCubeSampling(cs);
}


BufferString PlaneDataDisplay::getManipulationString() const
{
    BufferString res;
    if ( orientation==Inline )
    {
	res = "Inline: ";
	res += getCubeSampling(true,true).hrg.start.inl;
    }
    else if ( orientation==Crossline )
    {
	res = "Crossline: ";
	res += getCubeSampling(true,true).hrg.start.crl;
    }
    else
    {
	res = SI().zIsTime() ? "Time: " : "Depth: ";
	float val = getCubeSampling(true,true).zrg.start;
	res += SI().zIsTime() ? mNINT(val * 1000) : val;
    }

    return res;
}


NotifierAccess* PlaneDataDisplay::getManipulationNotifier()
{ return &dragger->motion; }

/*
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
}

*/


SurveyObject::AttribFormat PlaneDataDisplay::getAttributeFormat() const
{ return SurveyObject::Cube; }


bool PlaneDataDisplay::canHaveMultipleAttribs() const
{ return true; }


int PlaneDataDisplay::nrAttribs() const
{ return as.size(); }


bool PlaneDataDisplay::addAttrib()
{
    as += new Attrib::SelSpec;
    cache += 0;

    texture->addTexture("");

    return true;
}


bool PlaneDataDisplay::removeAttrib( int attrib )
{
    if ( as.size()<2 || attrib<0 || attrib>=as.size() )
	return false;

    delete as[attrib];
    as.remove( attrib );
    cache[attrib]->unRef();
    cache.remove( attrib );

    texture->removeTexture( attrib );

    return true;
}


bool PlaneDataDisplay::swapAttribs( int a0, int a1 )
{
    if ( a0<0 || a1<0 || a0>=as.size() || a1>=as.size() )
	return false;

    texture->swapTextures( a0, a1 );
    as.swap( a0, a1 );
    cache.swap( a0, a1 );

    return true;
}


const Attrib::SelSpec* PlaneDataDisplay::getSelSpec( int attrib ) const
{
    return attrib>=0 && attrib<as.size() ? as[attrib] : 0;
}


void PlaneDataDisplay::setSelSpec( int attrib, const Attrib::SelSpec& as_ )
{
    if ( attrib>=0 && attrib<as.size() )
    {
	*as[attrib] = as_;
    }

    if ( cache[attrib] ) cache[attrib]->unRef();
    cache.replace( attrib, 0 );

    const char* usrref = as_.userRef();
    if ( !usrref || !*usrref )
	texture->turnOn( false );
}


const TypeSet<float>* PlaneDataDisplay::getHistogram( int attrib ) const
{
    return texture->getHistogram( attrib, texture->currentVersion( attrib ) );
}


int PlaneDataDisplay::getColTabID( int attrib ) const
{
    return texture->getColorTab( attrib ).id();
}


CubeSampling PlaneDataDisplay::getCubeSampling() const
{
    return getCubeSampling(true,false);
}


void PlaneDataDisplay::setCubeSampling( CubeSampling cs )
{
    cs = snapCubeSampling( cs );

    visBase::Coordinates* coords = rectangle->getCoordinates();
    if ( orientation==Inline || orientation==Crossline )
    {
	coords->setPos( 0,
		       Coord3(cs.hrg.start.inl,cs.hrg.start.crl,cs.zrg.start));
	coords->setPos( 1,
		       Coord3(cs.hrg.start.inl,cs.hrg.start.crl,cs.zrg.stop));
	coords->setPos( 2, Coord3(cs.hrg.stop.inl,cs.hrg.stop.crl,cs.zrg.stop));
	coords->setPos( 3,
		       Coord3(cs.hrg.stop.inl,cs.hrg.stop.crl, cs.zrg.start));
    }
    else 
    {
	coords->setPos( 0,
			Coord3(cs.hrg.start.inl,cs.hrg.start.crl,cs.zrg.start));
	coords->setPos( 1,
			Coord3(cs.hrg.start.inl,cs.hrg.stop.crl,cs.zrg.stop));
	coords->setPos( 2, Coord3(cs.hrg.stop.inl,cs.hrg.stop.crl,cs.zrg.stop));
	coords->setPos( 3,
			Coord3(cs.hrg.stop.inl,cs.hrg.start.crl, cs.zrg.start));
    }

    setDraggerPos(cs);

    curicstep = cs.hrg.step;
    curzstep = cs.zrg.step;

    moving.trigger();

    if ( !datatransform )
	return;

    if ( datatransformvoihandle!=-1 )
	datatransform->setVolumeOfInterest( datatransformvoihandle, cs );
}


CubeSampling PlaneDataDisplay::getCubeSampling( bool manippos,
						bool displayspace ) const
{
    CubeSampling res(false);
    if ( manippos || rectangle->getCoordinates()->size()>=4 )
    {
	Coord3 c0, c1;
	if ( manippos )
	{
	    const Coord3 center = dragger->center();
	    Coord3 halfsize = dragger->size()/2;
	    halfsize[orientation] = 0;

	    c0 = center + halfsize;
	    c1 = center - halfsize;
	}
	else
	{
	    c0 = rectangle->getCoordinates()->getPos(0);
	    c1 = rectangle->getCoordinates()->getPos(2);
	}

	res.hrg.start = res.hrg.stop = BinID(mNINT(c0.x),mNINT(c0.y) );
	res.zrg.start = res.zrg.stop = c0.z;
	res.hrg.include( BinID(mNINT(c1.x),mNINT(c1.y)) );
	res.zrg.include( c1.z );
	res.hrg.step = BinID( SI().inlStep(), SI().crlStep() );
	res.zrg.step = SI().zRange(true).step;

	if ( datatransform && !displayspace )
	    res.zrg = SI().zRange(true);
    }
    return res;
}


bool PlaneDataDisplay::setDataVolume( int attrib,
				      const Attrib::DataCubes* datacubes )
{
    if ( attrib<0 || attrib>=nrAttribs() )
	return false;

    setData( attrib, datacubes );

    if ( cache[attrib] ) cache[attrib]->unRef();
    cache.replace( attrib, datacubes );
    datacubes->ref();
    return true;
}


void PlaneDataDisplay::setData( int attrib, const Attrib::DataCubes* datacubes )
{
    if ( !datacubes )
    {
	texture->setData( attrib, 0, 0 );
	texture->turnOn( false );
	return;
    }

    //Do subselection of input if input is too big

    int unuseddim, dim0, dim1;
    if ( orientation==Inline )
    {
	unuseddim = Attrib::DataCubes::cInlDim();
	dim0 = Attrib::DataCubes::cZDim();
	dim1 = Attrib::DataCubes::cCrlDim();
    }
    else if ( orientation==Crossline )
    {
	unuseddim = Attrib::DataCubes::cCrlDim();
	dim0 = Attrib::DataCubes::cZDim();
	dim1 = Attrib::DataCubes::cInlDim();
    }
    else
    {
	unuseddim = Attrib::DataCubes::cZDim();
	dim0 = Attrib::DataCubes::cCrlDim();
	dim1 = Attrib::DataCubes::cInlDim();
    }

    const int nrcubes = datacubes->nrCubes();
    texture->setNrVersions( 0, nrcubes );
    for ( int idx=0; idx<nrcubes; idx++ )
    {
	PtrMan<Array3D<float> > tmparray = 0;
	const Array3D<float>* usedarray = 0;
	if ( !datatransform )
	    usedarray = &datacubes->getCube(idx);
	else
	{
	    const CubeSampling cs = getCubeSampling(true,false);
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

	Array2DSlice<float> slice(*usedarray);
	slice.setPos( unuseddim, 0 );
	slice.setDimMap( 0, dim0 );
	slice.setDimMap( 1, dim1 );

	if ( slice.init() )
	    texture->setData( attrib, idx, &slice );
	else
	{
	    texture->turnOn(false);
	    pErrMsg( "Could not init slice." );
	}
    }

    texture->turnOn( true );
}


const Attrib::DataCubes* PlaneDataDisplay::getCacheVolume( int attrib ) const
{
    return attrib>=0 && attrib<nrAttribs() ? cache[attrib] : 0;
}


int PlaneDataDisplay::nrTextures( int attrib ) const
{
    return getCacheVolume( attrib ) ? getCacheVolume( attrib )->nrCubes() : 0;
}


void PlaneDataDisplay::selectTexture( int attrib, int idx )
{
    if ( attrib<0 || attrib>=nrAttribs() ||
	 idx<0 || idx>=texture->nrVersions(attrib) ) return;

    texture->setCurrentVersion( attrib, idx );
}


int PlaneDataDisplay::selectedTexture( int attrib ) const
{ 
    if ( attrib<0 || attrib>=nrAttribs() ) return 0;

    return texture->currentVersion( attrib );
}

#define mIsValid(idx,sz) ( idx>=0 && idx<sz )

void PlaneDataDisplay::getMousePosInfo( const visBase::EventInfo&,
					const Coord3& pos,
					float& val, 
					BufferString& info ) const
{
    info = getManipulationString();
    if ( cache.size()<=0 || !cache[0] ) { val = mUdf(float); return; }
    const BinIDValue bidv( SI().transform(pos), pos.z );
    if ( !cache[0]->getValue(texture->currentVersion(0),bidv,&val,false) )
	{ val = mUdf(float); return; }

    return;
}


void PlaneDataDisplay::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    visBase::VisualObject::fillPar( par, saveids );
    
    par.set( sKeyOrientation(), OrientationRef(orientation) );
    getCubeSampling( false, true ).fillPar( par );

    for ( int attrib=as.size()-1; attrib>=0; attrib-- )
    {
	IOPar attribpar;
	as[attrib]->fillPar( attribpar );
	const int coltabid = getColTabID(attrib);
	attribpar.set( sKeyColTabID(), coltabid );
	if ( saveids.indexOf( coltabid )==-1 ) saveids += coltabid;

	BufferString key = sKeyAttribs();
	key += attrib;
	par.mergeComp( attribpar, key );
    }

    par.set( sKeyNrAttribs(), as.size() );
}


int PlaneDataDisplay::usePar( const IOPar& par )
{
    const int res =  visBase::VisualObject::usePar( par );
    if ( res!=1 ) return res;

    int nrattribs;
    if ( par.get( sKeyNrAttribs(), nrattribs ) ) //current format
    {
	Orientation ori;
	EnumRef oriref = OrientationRef(ori);
	if ( par.get( sKeyOrientation(), oriref ) )
	    setOrientation( ori );

	CubeSampling cs;
	if ( cs.usePar( par ) )
	    setCubeSampling( cs );

	bool firstattrib = true;
	for ( int attrib=0; attrib<nrattribs; attrib++ )
	{
	    BufferString key = sKeyAttribs();
	    key += attrib;
	    PtrMan<const IOPar> attribpar = par.subselect( key );
	    if ( !attribpar )
		continue;

	    int coltabid = -1;
	    if ( attribpar->get( sKeyColTabID(), coltabid ) )
	    {
		visBase::DataObject* dataobj= visBase::DM().getObject(coltabid);
		if ( !dataobj ) return 0;

		mDynamicCastGet( const visBase::VisColorTab*, coltab, dataobj );
		if ( !coltab ) coltabid=-1;
	    }

	    if ( !firstattrib )
		addAttrib();
	    else
		firstattrib = false;

	    const int attribnr = as.size()-1;

	    as[attribnr]->usePar( *attribpar );
	    if ( coltabid!=-1 )
	    {
		mDynamicCastGet( visBase::VisColorTab*, coltab, 
		       		 visBase::DM().getObject(coltabid) );
		texture->setColorTab( attribnr, *coltab );
	    }
	}
    }
    else
    {
	int trectid;

	if ( !par.get( sKeyTextureRect(), trectid ))
	    return -1;

	visBase::DataObject* dataobj = visBase::DM().getObject( trectid );
	if ( !dataobj ) return 0;

	mDynamicCastGet(visBase::TextureRect*,tr,dataobj)
	if ( !tr ) return -1;

	tr->ref();


	visBase::Rectangle& rect = tr->getRectangle();
	CubeSampling cubesampl;
	Orientation ori;
	cubesampl.hrg.start =
	    BinID( mNINT( rect.origo().x), mNINT( rect.origo().y) );
	cubesampl.hrg.stop = cubesampl.hrg.start;
	cubesampl.hrg.step = curicstep;

	const float zrg0 = rect.origo().z;
	cubesampl.zrg.start = (float)(int)(1000*zrg0+.5) / 1000;
	cubesampl.zrg.stop = cubesampl.zrg.start;
	cubesampl.zrg.step = curzstep;

	if ( rect.orientation()==visBase::Rectangle::XY )
	{
	    cubesampl.hrg.stop.inl += mNINT(rect.width(0,false));
	    cubesampl.hrg.stop.crl += mNINT(rect.width(1,false));
	    ori = Timeslice;
	}
	else if ( rect.orientation()==visBase::Rectangle::XZ )
	{
	    cubesampl.hrg.stop.inl += mNINT(rect.width(0,false));
	    cubesampl.zrg.stop += rect.width(1,false);
	    ori = Crossline;
	}
	else
	{
	    ori = Inline;
	    cubesampl.hrg.stop.crl += mNINT(rect.width(1,false));
	    cubesampl.zrg.stop += rect.width(0,false);
	}

	setOrientation( ori );
	setCubeSampling( cubesampl );
	tr->unRef();

	as[0]->usePar( par );
    }

    return 1;
}


}; // namespace visSurvey

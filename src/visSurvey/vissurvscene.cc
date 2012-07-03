/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Oct 1999
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: vissurvscene.cc,v 1.161 2012-07-03 08:41:52 cvskris Exp $";

#include "vissurvscene.h"

#include "basemapimpl.h"
#include "cubesampling.h"
#include "envvars.h"
#include "iopar.h"
#include "keystrs.h"
#include "separstr.h"
#include "settings.h"
#include "survinfo.h"
#include "visannot.h"
#include "visdataman.h"
#include "visevent.h"
#include "vismarker.h"
#include "vismaterial.h"
#include "vispolygonselection.h"
#include "visscenecoltab.h"
#include "vistransform.h"
#include "vistransmgr.h"
#include "vissurvobj.h"
#include "vistopbotimage.h"
#include "zaxistransform.h"
#include "zdomain.h"


mCreateFactoryEntry( visSurvey::Scene );


namespace visSurvey {

const char* Scene::sKeyShowAnnot()	{ return "Show text"; }
const char* Scene::sKeyShowScale()	{ return "Show scale"; }
const char* Scene::sKeyShowGrid()	{ return "Show grid"; }
const char* Scene::sKeyShowCube()	{ return "Show cube"; }
const char* Scene::sKeyZStretch()	{ return "ZStretch"; }
const char* Scene::sKeyZAxisTransform()	{ return "ZTransform"; }
const char* Scene::sKeyAppAllowShading(){ return "Allow shading";}
const char* Scene::sKeyTopImageID()	{ return "TopImage.ID"; }
const char* Scene::sKeyBotImageID()	{ return "BotImage.ID"; }

static const char* sKeydTectScene()	{ return "dTect.Scene."; }
static const char* sKeyShowColTab()	{ return "Show ColTab"; }


Scene::Scene()
    : inlcrl2disptransform_(0)
    , utm2disptransform_(0)
    , zscaletransform_(0)
    , annot_(0)
    , marker_(0)
    , mouseposchange(this)
    , mouseposval_(0)
    , mouseposstr_("")
    , zstretchchange(this)
    , curzstretch_(-1)
    , datatransform_( 0 )
    , appallowshad_(true)
    , userwantsshading_(true)
    , mousecursor_( 0 )
    , polyselector_( 0 )
    , coordselector_( 0 )
    , zscale_( SI().zScale() )
    , infopar_(*new IOPar)
    , basemap_( 0 )
    , basemapcursor_( 0 )
    , zdomaininfo_(new ZDomain::Info(ZDomain::SI()))
{
    events_.eventhappened.notify( mCB(this,Scene,mouseMoveCB) );
    setAmbientLight( 1 );
    init();

    if ( GetEnvVarYN("DTECT_MULTITEXTURE_NO_SHADING" ) )
	userwantsshading_ = false;

    if ( userwantsshading_ )
    {
	bool noshading = false;
	Settings::common().getYN( "dTect.No shading", noshading );
	userwantsshading_ = !noshading;
    }
}


void Scene::updateAnnotationText()
{
    if ( !annot_ )
	return;

    if ( SI().inlRange(true).width() )
    	annot_->setText( 0, "In-line" );

    if ( SI().crlRange(true).width() )
    	annot_->setText( 1, "Cross-line" );

    if ( SI().zRange(true).width() )
    	annot_->setText( 2, zDomainUserName() );

    annot_->setAnnotScale( 2,
	    zdomaininfo_ ? zdomaininfo_->userFactor() : 1 );
}


void Scene::init()
{
    annot_ = visBase::Annotation::create();

    const CubeSampling& cs = SI().sampling(true);
    createTransforms( cs.hrg );
    float zsc = STM().defZStretch();
    if ( !SI().pars().get( STM().zStretchStr(), zsc ) )
	SI().pars().get( STM().zOldStretchStr(), zsc );
    setZStretch( zsc );

    setCubeSampling( cs );
    addInlCrlZObject( annot_ );
    updateAnnotationText();

    polyselector_ = visBase::PolygonSelection::create();
    addUTMObject( polyselector_ );
    polyselector_->getMaterial()->setColor( Color(255,0,0) );
    mTryAlloc( coordselector_, visBase::PolygonCoord3Selector(*polyselector_) );

    scenecoltab_ = visBase::SceneColTab::create();
    addUTMObject( scenecoltab_ );
    scenecoltab_->turnOn( false );
    scenecoltab_->doSaveInSessions( false );

    topimg_ = visBase::TopBotImage::create();
    addUTMObject( topimg_ );
    topimg_->turnOn( false );

    botimg_ = visBase::TopBotImage::create();
    addUTMObject( botimg_ );
    botimg_->turnOn( false );

    bool doshow = true;
#define mShowAnnot(str,func) \
    doshow = true; \
    Settings::common().getYN( BufferString(sKeydTectScene(),str), doshow ); \
    func( doshow );

    mShowAnnot( sKeyShowAnnot(), showAnnotText );
    mShowAnnot( sKeyShowScale(), showAnnotScale );
    mShowAnnot( sKeyShowGrid(), showAnnotGrid );
}


Scene::~Scene()
{
    if ( basemap_ && basemapcursor_ )
    {
	basemap_->removeObject( basemapcursor_ );
	delete basemapcursor_;
	basemapcursor_ = 0;
    }

    events_.eventhappened.remove( mCB(this,Scene,mouseMoveCB) );

    if ( utm2disptransform_ ) utm2disptransform_->unRef();
    if ( datatransform_ ) datatransform_->unRef();

    for ( int idx=0; idx<size(); idx++ )
    {
	mDynamicCastGet(visBase::VisualObject*,vo,getObject(idx));
	mDynamicCastGet(SurveyObject*,so,getObject(idx));
	if ( vo ) vo->setSceneEventCatcher( 0 );

	if ( !so ) continue;

	if ( so->getMovementNotifier() )
	    so->getMovementNotifier()->remove( mCB(this,Scene,objectMoved) );
	so->setScene( 0 );
   }

    delete coordselector_;
    delete &infopar_;
    delete zdomaininfo_;
}


void Scene::createTransforms( const HorSampling& hs )
{
    if ( !zscaletransform_ )
    {
	mVisTrans* trans = STM().createZScaleTransform();
	zscaletransform_ = trans;
	visBase::DataObjectGroup::addObject( trans );
    }

    if ( !inlcrl2disptransform_ )
    {
	mVisTrans* trans = STM().createIC2DisplayTransform( hs );
	inlcrl2disptransform_ = trans;
	zscaletransform_->addObject(  trans );
    }

    if ( !utm2disptransform_ )
    {
	utm2disptransform_ = STM().createUTM2DisplayTransform( hs );
	utm2disptransform_->ref();
    }

    ObjectSet<visBase::Transformation> utm2display;
    utm2display += utm2disptransform_;
    utm2display += zscaletransform_;
    events_.setUtm2Display( utm2display );
}


bool Scene::isRightHandSystem() const
{
    return SI().isClockWise();
}


const ZDomain::Info& Scene::zDomainInfo() const
{ return *zdomaininfo_; }

void Scene::setZDomainInfo( const ZDomain::Info& zdinf )
{
    delete zdomaininfo_;
    zdomaininfo_ = new ZDomain::Info( zdinf );
    updateAnnotationText();
}


const char* Scene::zDomainKey() const
{ return zdomaininfo_->key(); }

const char* Scene::zDomainUserName() const
{ return zdomaininfo_->userName(); }

const char* Scene::zDomainUnitStr( bool withparens ) const
{ return zdomaininfo_->unitStr( withparens ); }

int Scene::zDomainUserFactor() const
{ return zdomaininfo_->userFactor(); }

const char* Scene::zDomainID() const
{ return zdomaininfo_->getID(); }


void Scene::getAllowedZDomains( BufferString& dms ) const
{
    FileMultiString fms( zDomainKey() );
    if ( datatransform_ )
	fms.add( datatransform_->toZDomainKey() );
    dms = fms.buf();
}


void Scene::setCubeSampling( const CubeSampling& cs )
{
    cs_ = cs;
    if ( !annot_ ) return;

    annot_->setCubeSampling( cs );
}


int Scene::size() const
{
    return zscaletransform_->size()+inlcrl2disptransform_->size();
}


int Scene::getFirstIdx( const visBase::DataObject* dobj ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( getObject( idx )==dobj )
	    return idx;
    }

    return -1;
}


visBase::DataObject* Scene::getObject( int idx )
{
    if ( idx<zscaletransform_->size() )
	return zscaletransform_->getObject( idx );

    return inlcrl2disptransform_->getObject( idx-zscaletransform_->size() );
}


const visBase::DataObject* Scene::getObject( int idx ) const
{
    return const_cast<Scene*>( this )->getObject( idx );
}


void Scene::addUTMObject( visBase::VisualObject* obj )
{
    obj->setDisplayTransformation( utm2disptransform_ );
    const int idx = zscaletransform_->getFirstIdx( inlcrl2disptransform_ );
    zscaletransform_->insertObject( idx, obj );
}


void Scene::addInlCrlZObject( visBase::DataObject* obj )
{
    mDynamicCastGet(SurveyObject*,so,obj);
    if ( so )
    {
	so->setInlCrlSystem( SI().get3DGeometry(true) );
    }

    inlcrl2disptransform_->addObject( obj );
}


void Scene::addObject( visBase::DataObject* obj )
{
    mDynamicCastGet(SurveyObject*,so,obj)
    mDynamicCastGet(visBase::VisualObject*,vo,obj)

    if ( so )
    {
	so->setInlCrlSystem( SI().get3DGeometry(true) );
	if ( so->getMovementNotifier() )
	    so->getMovementNotifier()->notify( mCB(this,Scene,objectMoved));

	so->setScene( this );
	STM().setCurrentScene( this );
	so->allowShading( userwantsshading_ && appallowshad_ );
    }

    if ( vo )
	vo->setSceneEventCatcher( &events_ );

    if ( so && so->isInlCrl() )
	addInlCrlZObject( obj );
    else if ( vo )
	addUTMObject( vo );

    if ( so && datatransform_ )
	so->setZAxisTransform( datatransform_,0 );

    if ( so )
	objectMoved( obj );
}


void Scene::removeObject( int idx )
{
    mousecursor_ = 0;
    DataObject* obj = getObject( idx );
    mDynamicCastGet(SurveyObject*,so,obj)
    if ( so && so->getMovementNotifier() )
	so->getMovementNotifier()->remove( mCB(this,Scene,objectMoved) );

    if ( idx<zscaletransform_->size() )
	zscaletransform_->removeObject( idx );
    else
	inlcrl2disptransform_->removeObject( idx-zscaletransform_->size() );

    if ( so )
	objectMoved(0);
}


void Scene::setZStretch( float zstretch )
{
    if ( !zscaletransform_ ) return;

    if ( mIsEqual(zstretch,curzstretch_,mDefEps) ) return;

    STM().setZScale( zscaletransform_, zstretch*zscale_ );
    curzstretch_ = zstretch;
    zstretchchange.trigger();
}


float Scene::getZStretch() const
{ return curzstretch_; }


void Scene::setZScale( float zscale )
{
    zscale_ = zscale;
    if ( zscaletransform_ )
       	STM().setZScale( zscaletransform_, curzstretch_*zscale_ );
}


float Scene::getZScale() const
{ return zscale_; }


const mVisTrans* Scene::getZScaleTransform() const
{ return zscaletransform_; }


const mVisTrans* Scene::getInlCrl2DisplayTransform() const
{ return inlcrl2disptransform_; }


const mVisTrans* Scene::getUTM2DisplayTransform() const
{ return utm2disptransform_; }


void Scene::showAnnotText( bool yn )
{ annot_->showText( yn ); }


bool Scene::isAnnotTextShown() const
{ return annot_->isTextShown(); }


void Scene::showAnnotScale( bool yn )
{ annot_->showScale( yn ); }


bool Scene::isAnnotScaleShown() const
{ return annot_->isScaleShown(); }


void Scene::showAnnotGrid( bool yn )
{ annot_->showGridLines( yn ); }


bool Scene::isAnnotGridShown() const
{ return annot_->isGridLinesShown(); }


void Scene::showAnnot( bool yn )
{ annot_->turnOn( yn ); }


bool Scene::isAnnotShown() const
{ return annot_->isOn(); }


void Scene::setAnnotText( int dim, const char* txt )
{
    if ( (dim==0 && SI().inlRange(true).width()) || 
	 (dim==1 && SI().crlRange(true).width()) ||
	 (dim==2 && SI().zRange(true).width()) )
    	annot_->setText( dim, txt );
}


void Scene::setAnnotColor( const Color& col )
{
    annot_->getMaterial()->setColor( col );
    annot_->updateTextColor( col );

    for ( int idx=0; idx<size(); idx++ )
    {
	mDynamicCastGet(SurveyObject*,so,getObject(idx));
	if ( so )
	    so->setAnnotColor( col );
    }
}


const Color& Scene::getAnnotColor() const
{
    return annot_->getColor();
}


const Selector<Coord3>* Scene::getSelector() const
{
    if ( !coordselector_ ) return 0;

    mDynamicCastGet(const visBase::PolygonCoord3Selector*,sel,coordselector_);
    if ( !sel || !sel->hasPolygon() ) return 0;

    return coordselector_;
}


Coord3 Scene::getMousePos( bool xyt ) const
{
   if ( xyt ) return xytmousepos_;
   
   Coord3 res = xytmousepos_;
   BinID binid = SI().transform( Coord(res.x,res.y) );
   res.x = binid.inl;
   res.y = binid.crl;
   return res;
}


BufferString Scene::getMousePosValue() const
{ return BufferString(mouseposval_); }


BufferString Scene::getMousePosString() const
{ return mouseposstr_; }


const MouseCursor* Scene::getMouseCursor() const
{ return mousecursor_; }


void Scene::objectMoved( CallBacker* cb )
{
    ObjectSet<const SurveyObject> activeobjects;
    int movedid = -1;
    for ( int idx=0; idx<size(); idx++ )
    {
	mDynamicCastGet(SurveyObject*,so,getObject(idx))
	if ( !so ) continue;
	if ( !so->getMovementNotifier() ) continue;

	mDynamicCastGet(visBase::VisualObject*,vo,getObject(idx))
	if ( !vo ) continue;
	if ( !vo->isOn() ) continue;

	if ( cb==vo ) movedid = vo->id();

	activeobjects += so;
    }

    for ( int idx=0; idx<size(); idx++ )
    {
	mDynamicCastGet(SurveyObject*,so,getObject(idx));
	
	if ( so ) so->otherObjectsMoved( activeobjects, movedid );
    }
}


void Scene::mouseMoveCB( CallBacker* cb )
{
    STM().setCurrentScene( this );

    mCBCapsuleUnpack(const visBase::EventInfo&,eventinfo,cb);
    if ( eventinfo.type != visBase::MouseMovement ) return;

    mouseposval_ = "";
    mouseposstr_ = "";
    xytmousepos_ = Coord3::udf();

    const int sz = eventinfo.pickedobjids.size();
    if ( sz )
    {
	xytmousepos_ = eventinfo.worldpickedpos;

	for ( int idx=0; idx<sz; idx++ )
	{
	    const DataObject* pickedobj =
			visBase::DM().getObject(eventinfo.pickedobjids[idx]);
	    mDynamicCastGet(const SurveyObject*,so,pickedobj);
	    if ( so )
	    {
		if ( !mouseposval_[0] )
		{
		    BufferString newmouseposval;
		    BufferString newstr;
		    so->getMousePosInfo( eventinfo, xytmousepos_,
			    		 newmouseposval, newstr );
		    so->getMousePosInfo( eventinfo, infopar_ );
		    if ( !newstr.isEmpty() )
			mouseposstr_ = newstr;

		    if ( newmouseposval[0] )
			mouseposval_ = newmouseposval;
		}

		mousecursor_ = so->getMouseCursor();
		break;
	    }
	}
    }

    mouseposchange.trigger();
}


void Scene::setBaseMap( BaseMap* bm )
{
    if ( basemap_ && basemapcursor_ )
    {
	basemap_->removeObject( basemapcursor_ );
	delete basemapcursor_;
	basemapcursor_ = 0; 
    }

    basemap_ = bm;
    for ( int idx=0; idx<size(); idx++ )
    {
	mDynamicCastGet(SurveyObject*,so,getObject(idx));
	if ( so ) so->setBaseMap( bm );
    }
}


BaseMap* Scene::getBaseMap()
{ return basemap_; }


void Scene::setZAxisTransform( ZAxisTransform* zat, TaskRunner* tr )
{
    if ( datatransform_==zat ) return;

    if ( datatransform_ ) datatransform_->unRef();
    datatransform_ = zat;
    if ( datatransform_ ) datatransform_->ref();

    CubeSampling cs = SI().sampling( true );
    if ( !zat )
	setZDomainInfo( ZDomain::Info(ZDomain::SI()) );
    else
    {
	const Interval<float> zrg = zat->getZInterval( false );
	if ( !zrg.isUdf() )
	{
	    cs.zrg.start = zrg.start;
	    cs.zrg.stop = zrg.stop;
	    cs.zrg.step = zat->getGoodZStep();
	}

	setZDomainInfo( zat->toZDomainInfo() );
    }

    setCubeSampling( cs );

    for ( int idx=0; idx<size(); idx++ )
    {
	mDynamicCastGet(SurveyObject*,so,getObject(idx))
	if ( !so ) continue;

	so->setZAxisTransform( zat,0 );
    }

    updateAnnotationText();
}


ZAxisTransform* Scene::getZAxisTransform()
{ return datatransform_; }


void Scene::setMarkerPos( const Coord3& coord, int sceneid )
{
    updateBaseMapCursor( coord );

    Coord3 displaypos = coord;
    if ( sceneid==id() )
	displaypos = Coord3::udf();

    if ( datatransform_ && coord.isDefined() )
    {
	BufferString linenm; int trcnr = -1;
	infopar_.get( sKey::LineKey(), linenm );
	infopar_.get( sKey::TraceNr(), trcnr );
	if ( !linenm.isEmpty() && trcnr>=0 )
	{
	    BinID bid( datatransform_->lineIndex(linenm), trcnr );
	    displaypos.z = datatransform_->transform(
		    BinIDValue(bid,coord.z) );
	}
	else
	    displaypos.z = datatransform_->transform( coord );
    }

    const bool defined = displaypos.isDefined();
    if ( !defined )
    {
	if ( marker_ )
	    marker_->turnOn( false );
	return;
    }

    if ( !marker_ )
    {
	marker_ = createMarker();
	addUTMObject( marker_ );
    }

    marker_->turnOn( true );
    marker_->setCenterPos( displaypos );
}


void Scene::updateBaseMapCursor( const Coord& coord )
{
    if ( !basemap_ )
	return;

    const bool defined = coord.isDefined();

    if ( defined && !basemapcursor_ )
    {
	basemapcursor_ = new BaseMapMarkers;
	basemapcursor_->setMarkerStyle(
		MarkerStyle2D(MarkerStyle2D::Target) );
    }

    if ( basemapcursor_ && basemapcursor_->lock_.tryLock() )
    {
	if ( defined )
	{
	    if ( basemapcursor_->positions().size() )
		basemapcursor_->positions()[0] = coord;
	    else
		basemapcursor_->positions() += coord;
	}
	else
	{
	    basemapcursor_->positions().erase();
	}

	basemapcursor_->lock_.unLock();
	basemapcursor_->updateGeometry();
    }

    if ( basemapcursor_ )
	basemap_->addObject( basemapcursor_ );
}


visBase::Marker* Scene::createMarker() const
{
    visBase::Marker* marker = visBase::Marker::create();
    marker->setType( MarkerStyle3D::Cross );
    marker->getMaterial()->setColor( cDefaultMarkerColor() );
    marker->setScreenSize( 6 );
    return marker;
}


void Scene::setMarkerSize( float nv )
{
    if ( !marker_ )
    {
	marker_ = createMarker();
	addUTMObject( marker_ );
	marker_->turnOn( false );
    }

    marker_->setScreenSize( nv );
}


float Scene::getMarkerSize() const
{
    if ( !marker_ )
	return visBase::Marker::cDefaultScreenSize();

    return marker_->getScreenSize();
}


void Scene::setMarkerColor( const Color& nc )
{
    if ( !marker_ )
    {
	marker_ = createMarker();
	addUTMObject( marker_ );
	marker_->turnOn( false );
    }

    marker_->getMaterial()->setColor( nc );
}


const Color& Scene::getMarkerColor() const
{
    if ( !marker_ )
	return cDefaultMarkerColor();

    return marker_->getMaterial()->getColor();
}


const Color& Scene::cDefaultMarkerColor()
{
    static Color res( 255,255,255 );
    return res;
}


void Scene::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    visBase::DataObject::fillPar( par, saveids );
    visBase::Scene::fillOffsetPar( par );

    int kid = 0;
    int nrkids = 0;    
    for ( ; kid<size(); kid++ )
    {
	if ( getObject(kid)==annot_ || (marker_ && getObject(kid)==marker_) ||
	     getObject(kid)==inlcrl2disptransform_ ||
	     getObject(kid)==polyselector_ ) continue;

	const visBase::DataObject* dobj = getObject( kid );
	const bool saveinsess = dobj && dobj->saveInSessions();
	if ( !saveinsess )
	    continue;

	const int objid = dobj->id();
	if ( saveids.indexOf(objid) == -1 )
	    saveids += objid;

	BufferString key = kidprefix(); key += nrkids;
	par.set( key, objid );
	nrkids++;
    }

    par.set( nokidsstr(), nrkids );
    
    par.setYN( sKeyShowAnnot(), isAnnotTextShown() );
    par.setYN( sKeyShowScale(), isAnnotScaleShown() );
    par.setYN( sKeyShowGrid(), isAnnotGridShown() );
    par.setYN( sKeyShowCube(), isAnnotShown() );
    par.set( sKeyZStretch(), curzstretch_ );
    par.setYN( sKeyAppAllowShading(), appallowshad_ );
    par.setYN( sKeyShowColTab(), scenecoltab_->isOn() );

    if ( datatransform_ )
    {
	IOPar transpar;
	transpar.set( sKey::Name(), datatransform_->factoryKeyword() );
	datatransform_->fillPar( transpar );
	par.mergeComp( transpar, sKeyZAxisTransform() );
    }
    else
    {
	zdomaininfo_->def_.set( par );
	par.mergeComp( zdomaininfo_->pars_, ZDomain::sKey() );
	cs_.fillPar( par );
	par.set( sKey::Scale(), zscale_ );
    }

    par.set( sKeyTopImageID(), topimg_->id() );
    par.set( sKeyBotImageID(), botimg_->id() );
}


void Scene::removeAll()
{
    visBase::DataObjectGroup::removeAll();
    const int idx = visBase::DataObjectGroup::getFirstIdx( zscaletransform_ );
    if ( idx!=-1 )
	visBase::DataObjectGroup::removeObject( idx );

    zscaletransform_ = 0; inlcrl2disptransform_ = 0; annot_ = 0;
    polyselector_= 0;
    delete coordselector_; coordselector_ = 0;
    curzstretch_ = -1;
}


int Scene::usePar( const IOPar& par )
{
    removeAll();
    init();

    appallowshad_ = true;
    if ( !par.getYN( sKeyAppAllowShading(), appallowshad_ ) )
	par.getYN( "Allow shading", appallowshad_ ); //Old key


    int res = visBase::Scene::usePar( par );
    if ( res!=1 ) return res;

    bool ctshown = false;
    par.getYN( sKeyShowColTab(), ctshown );
    scenecoltab_->turnOn( ctshown );

    bool txtshown = true;
    par.getYN( sKeyShowAnnot(), txtshown );
    showAnnotText( txtshown );

    bool scaleshown = true;
    par.getYN( sKeyShowScale(), scaleshown );
    showAnnotScale( scaleshown );

    bool gridshown = true;
    par.getYN( sKeyShowGrid(), gridshown );
    showAnnotGrid( gridshown );

    bool cubeshown = true;
    par.getYN( sKeyShowCube(), cubeshown );
    showAnnot( cubeshown );

    float zstretch = curzstretch_;
    if ( !par.get( sKeyZStretch(), zstretch ) )
	par.get( "ZScale", zstretch ); //Old format, can be removed in 3.6

    if ( zstretch != curzstretch_ )
	setZStretch( zstretch );
  
    ZDomain::Info zdomaininfo( ZDomain::SI() ); 
    PtrMan<IOPar> transpar = par.subselect( sKeyZAxisTransform() );
    if ( transpar )
    {
	const char* nm = transpar->find( sKey::Name() );
	RefMan<ZAxisTransform> transform =
	    ZAxisTransform::factory().create( nm );
	if ( transform && transform->usePar( *transpar ) )
	    setZAxisTransform( transform,0 );
    }
    else
    {
	CubeSampling cs; float zscale;
	if ( cs.usePar( par ) && par.get( sKey::Scale(), zscale ) )
	{
	    setCubeSampling( cs );
	    setZScale( zscale );
	    delete zdomaininfo_; zdomaininfo_ = new ZDomain::Info( par );
	}
    }

    res = getImageFromPar( par,sKeyTopImageID(), topimg_ );
    if ( res != 1 ) return res;
    res = getImageFromPar( par, sKeyBotImageID(), botimg_ );
    if ( res != 1 ) return res;

    return 1;
}


int Scene::getImageFromPar( const IOPar& par, const char* key,
			    visBase::TopBotImage*& image )
{
    int imgid;
    if ( par.get(key,imgid) )
    { 
        RefMan<DataObject> dataobj = visBase::DM().getObject( imgid );
        if ( !dataobj ) return 0;
        mDynamicCastGet(visBase::TopBotImage*,im,dataobj.ptr())
        if ( !im ) return -1;
	int objidx = getFirstIdx( image );
	if ( objidx>=0 ) removeObject( objidx ); 	
	image = im;
    }

    return 1;
}


visBase::TopBotImage* Scene::getTopBotImage( bool istop )
{ return istop ? topimg_ : botimg_; }


void Scene::savePropertySettings()
{
#define mSaveProp(str,func) \
    Settings::common().setYN( BufferString(sKeydTectScene(),str), func() );

    mSaveProp( sKeyShowAnnot(), isAnnotTextShown );
    mSaveProp( sKeyShowScale(), isAnnotScaleShown );
    mSaveProp( sKeyShowGrid(), isAnnotGridShown );
    Settings::common().write();
}


} // namespace visSurvey


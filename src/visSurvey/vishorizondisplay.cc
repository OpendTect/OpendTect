/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          May 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: vishorizondisplay.cc,v 1.73 2009-04-22 05:01:04 cvsumesh Exp $";

#include "vishorizondisplay.h"

#include "attribdatapack.h"
#include "attribsel.h"
#include "bidvsetarrayadapter.h"
#include "datapointset.h"
#include "datacoldef.h"
#include "emhorizon3d.h"
#include "emsurfaceauxdata.h"
#include "emsurfaceedgeline.h"
#include "flatposdata.h"
#include "iopar.h"
#include "isocontourtracer.h"
#include "survinfo.h"
#include "mpeengine.h"

#include "viscolortab.h"
#include "viscoord.h"
#include "visdataman.h"
#include "visdrawstyle.h"
#include "visevent.h"
#include "vishingeline.h"
#include "vismarker.h"
#include "vismaterial.h"
#include "vismpe.h"
#include "visparametricsurface.h"
#include "visplanedatadisplay.h"
#include "vispolyline.h"
#include "visrandomtrackdisplay.h"
#include "visseis2ddisplay.h"
#include "vistransform.h"
#include "zaxistransform.h"

#include <math.h>



mCreateFactoryEntry( visSurvey::HorizonDisplay );

namespace visSurvey
{

const char* HorizonDisplay::sKeyTexture()	{ return "Use texture"; }
const char* HorizonDisplay::sKeyColorTableID()	{ return "ColorTable ID"; }
const char* HorizonDisplay::sKeyShift()		{ return "Shift"; }
const char* HorizonDisplay::sKeyWireFrame()	{ return "WireFrame on"; }
const char* HorizonDisplay::sKeyResolution()	{ return "Resolution"; }
const char* HorizonDisplay::sKeyEdgeLineRadius(){ return "Edgeline radius"; }
const char* HorizonDisplay::sKeyRowRange()	{ return "Row range"; }
const char* HorizonDisplay::sKeyColRange()	{ return "Col range"; }


HorizonDisplay::HorizonDisplay()
    : parrowrg_( -1, -1, -1 )
    , parcolrg_( -1, -1, -1 )
    , curtextureidx_( 0 )
    , usestexture_( false )
    , useswireframe_( false )
    , translation_( 0 )
    , edgelineradius_( 3.5 )
    , validtexture_( false )
    , resolution_( 0 )
    , zaxistransform_( 0 )
    , maxintersectionlinethickness_( 40 )
{
    as_ += new Attrib::SelSpec;
    coltabs_ += visBase::VisColorTab::create();
    coltabs_[0]->ref();
    TypeSet<float> shift;
    shift += 0.0;
    shifts_ += shift;
    curshiftidx_ += 0;
    BufferStringSet* aatrnms = new BufferStringSet();
    aatrnms->allowNull();
    userrefs_ += aatrnms;
    enabled_ += true;
    datapackids_ += -1;
}


HorizonDisplay::~HorizonDisplay()
{
    deepErase(as_);

    setSceneEventCatcher( 0 );
    deepUnRef( coltabs_ );
    shifts_.erase();
    curshiftidx_.erase();
    deepErase( userrefs_ );

    if ( translation_ )
    {
	removeChild( translation_->getInventorNode() );
	translation_->unRef();
	translation_ = 0;
    }

    removeEMStuff();
    if ( zaxistransform_ )
    {
	for ( int idx=0; idx<intersectionlinevoi_.size(); idx++ )
	    zaxistransform_->removeVolumeOfInterest(intersectionlinevoi_[idx]);

	zaxistransform_->unRef();
    }

    DataPackMgr& dpman = DPM( DataPackMgr::FlatID() );
    for ( int idx=0; idx<datapackids_.size(); idx++ )
	dpman.release( datapackids_[idx] );
}


void HorizonDisplay::setDisplayTransformation( mVisTrans* nt )
{
    EMObjectDisplay::setDisplayTransformation( nt );

    for ( int idx=0; idx<sections_.size(); idx++ )
	sections_[idx]->setDisplayTransformation(transformation_);

    for ( int idx=0; idx<edgelinedisplays_.size(); idx++ )
	edgelinedisplays_[idx]->setDisplayTransformation(transformation_);

    for ( int idx=0; idx<intersectionlines_.size(); idx++ )
	intersectionlines_[idx]->setDisplayTransformation(transformation_);
}


bool HorizonDisplay::setDataTransform( ZAxisTransform* nz )
{
    if ( zaxistransform_ ) zaxistransform_->unRef();
    zaxistransform_ = nz;
    if ( zaxistransform_ ) zaxistransform_->ref();

    for ( int idx=0; idx<sections_.size(); idx++ )
    {
	sections_[idx]->setZAxisTransform( nz );
    }

    return true;
}


const ZAxisTransform* HorizonDisplay::getDataTransform() const
{ return zaxistransform_; }


void HorizonDisplay::setSceneEventCatcher(visBase::EventCatcher* ec)
{
    EMObjectDisplay::setSceneEventCatcher( ec );

    for ( int idx=0; idx<sections_.size(); idx++ )
	sections_[idx]->setSceneEventCatcher( ec );

    for ( int idx=0; idx<edgelinedisplays_.size(); idx++ )
	edgelinedisplays_[idx]->setSceneEventCatcher( ec );

    for ( int idx=0; idx<intersectionlines_.size(); idx++ )
	intersectionlines_[idx]->setSceneEventCatcher( ec );

}


EM::PosID HorizonDisplay::findClosestNode( const Coord3& pickedpos ) const
{
    const mVisTrans* ztrans = scene_->getZScaleTransform();
    Coord3 newpos = ztrans->transformBack( pickedpos );
    if ( transformation_ )
	newpos = transformation_->transformBack( newpos );

    const BinID pickedbid = SI().transform( newpos );
    const EM::SubID pickedsubid = pickedbid.getSerialized();
    TypeSet<EM::PosID> closestnodes;
    for ( int idx=sids_.size()-1; idx>=0; idx-- )
    {
	if ( !emobject_->isDefined( sids_[idx], pickedsubid ) )
	    continue;
	closestnodes += EM::PosID( emobject_->id(), sids_[idx], pickedsubid );
    }

    if ( closestnodes.isEmpty() ) return EM::PosID( -1, -1, -1 );

    EM::PosID closestnode = closestnodes[0];
    float mindist = mUdf(float);
    for ( int idx=0; idx<closestnodes.size(); idx++ )
    {
	const Coord3 coord = emobject_->getPos( closestnodes[idx] );
	const Coord3 displaypos = ztrans->transform(
		transformation_ ? transformation_->transform(coord) : coord );

	const float dist = displaypos.distTo( pickedpos );
	if ( !idx || dist<mindist )
	{
	    closestnode = closestnodes[idx];
	    mindist = dist;
	}
    }

    return closestnode;
}


void HorizonDisplay::edgeLineRightClickCB( CallBacker* )
{}


void HorizonDisplay::removeEMStuff()
{
    for ( int idx=0; idx<sections_.size(); idx++ )
    {
	removeChild( sections_[idx]->getInventorNode() );
	sections_[idx]->unRef();
    }

    sections_.erase();
    sids_.erase();

    while ( intersectionlines_.size() )
    {
	intersectionlines_[0]->unRef();
	intersectionlines_.remove(0);
	intersectionlineids_.remove(0);
	if ( zaxistransform_ )
	    zaxistransform_->removeVolumeOfInterest( intersectionlinevoi_[0] );
	intersectionlinevoi_.remove(0);

    }

    mDynamicCastGet( EM::Horizon3D*, emhorizon, emobject_ );
    if ( emhorizon )
	emhorizon->edgelinesets.addremovenotify.remove(
			mCB(this,HorizonDisplay,emEdgeLineChangeCB ));

    while ( !edgelinedisplays_.isEmpty() )
    {
	EdgeLineSetDisplay* elsd = edgelinedisplays_[0];
	edgelinedisplays_ -= elsd;
	elsd->rightClicked()->remove(
		 mCB(this,HorizonDisplay,edgeLineRightClickCB) );
	removeChild( elsd->getInventorNode() );
	elsd->unRef();
    }

    EMObjectDisplay::removeEMStuff();
}


bool HorizonDisplay::setEMObject( const EM::ObjectID& newid )
{
    if ( !EMObjectDisplay::setEMObject( newid ) )
	return false;

    mDynamicCastGet( EM::Horizon3D*, emhorizon, emobject_ );
    if ( emhorizon ) emhorizon->edgelinesets.addremovenotify.notify(
			    mCB(this,HorizonDisplay,emEdgeLineChangeCB ));

    return true;
}


StepInterval<int> HorizonDisplay::displayedRowRange() const
{
    mDynamicCastGet(const EM::Horizon3D*, surface, emobject_ );
    if ( !surface ) return parrowrg_;
    
    return surface->geometry().rowRange();
}


StepInterval<int> HorizonDisplay::displayedColRange() const
{
    mDynamicCastGet(const EM::Horizon3D*, surface, emobject_ );
    if ( !surface ) return parcolrg_;
    
    return surface->geometry().colRange();
}


bool HorizonDisplay::updateFromEM()
{ 
    if ( !EMObjectDisplay::updateFromEM() )
	return false;

    updateSingleColor();
    return true;
}


void HorizonDisplay::updateFromMPE()
{
    const bool hastracker = MPE::engine().getTrackerByObject(getObjectID())>=0;
	
    //if ( hastracker && !restoresessupdate_ )
    //{
	//useWireframe( true );
	//useTexture( false );
    //}

    if ( displayedRowRange().nrSteps()<=1 || displayedColRange().nrSteps()<=1 )
    {
	useWireframe( true );
	setResolution( nrResolutions()-1 );
    }

    EMObjectDisplay::updateFromMPE();
}


bool HorizonDisplay::addEdgeLineDisplay( const EM::SectionID& sid )
{
    mDynamicCastGet( EM::Horizon3D*, emhorizon, emobject_ );
    EM::EdgeLineSet* els = emhorizon
	? emhorizon->edgelinesets.getEdgeLineSet(sid,false) : 0;

    if ( els )
    {
	bool found = false;
	for ( int idx=0; idx<edgelinedisplays_.size(); idx++ )
	{
	    if ( edgelinedisplays_[idx]->getEdgeLineSet()==els )
	    {
		found = true;
		break;
	    }
	}
	
	if ( !found )
	{
	    visSurvey::EdgeLineSetDisplay* elsd =
		visSurvey::EdgeLineSetDisplay::create();
	    elsd->ref();
	    elsd->setConnect(true);
	    elsd->setEdgeLineSet(els);
	    elsd->setRadius(edgelineradius_);
	    addChild( elsd->getInventorNode() );
	    elsd->setDisplayTransformation(transformation_);
	    elsd->rightClicked()->notify(
		    mCB(this,HorizonDisplay,edgeLineRightClickCB));
	    edgelinedisplays_ += elsd;
	}
    }

    return true;
}


void HorizonDisplay::useTexture( bool yn, bool trigger )
{
    if ( yn && !validtexture_ )
    {
	for ( int idx=0; idx<nrAttribs(); idx++ )
	{
	    if ( as_[idx]->id()==Attrib::SelSpec::cNoAttrib() )
	    {
		usestexture_ = yn;
		setDepthAsAttrib(idx);
		return;
	    }
	}
    }

    usestexture_ = yn;

    updateSingleColor();
    
    if ( trigger )
	changedisplay.trigger();
}


bool HorizonDisplay::usesTexture() const
{ return usestexture_; }


bool HorizonDisplay::showingTexture() const
{ return validtexture_ && usestexture_; }


bool HorizonDisplay::hasColor() const
{ return true; }


bool HorizonDisplay::getOnlyAtSectionsDisplay() const
{ return displayonlyatsections_; }


bool HorizonDisplay::canHaveMultipleAttribs() const
{ return true; }


int HorizonDisplay::nrTextures( int attrib ) const
{
    if ( attrib<0 || attrib>=nrAttribs() || sections_.isEmpty() ) return 0;

    mDynamicCastGet(const visBase::ParametricSurface*,psurf,sections_[0]);
    return psurf ? psurf->nrVersions( attrib ) : 0;
}


void HorizonDisplay::selectTexture( int attrib, int textureidx )
{
    curtextureidx_ = textureidx;
    for ( int idx=0; idx<sections_.size(); idx++ )
    {
	mDynamicCastGet(visBase::ParametricSurface*,psurf,sections_[idx]);
	if ( psurf )
	    psurf->selectActiveVersion( attrib, textureidx );
    }

    mDynamicCastGet(EM::Horizon3D*,emsurf,emobject_)
    if ( !emsurf || emsurf->auxdata.nrAuxData() == 0 ) return;

    if ( textureidx >= emsurf->auxdata.nrAuxData() )
	setSelSpec( attrib,
		    Attrib::SelSpec(0,Attrib::SelSpec::cAttribNotSel()) );
    else
    {
	BufferString attrnm = userrefs_[attrib]->get( textureidx );
	const float shift = (shifts_[attrib])[ textureidx ];
	curshiftidx_[attrib] = textureidx;
	emsurf->geometry().setShift( shift );
	Coord3 tranl = getTranslation();
	tranl.z = shift;
	setTranslation( tranl );
	setSelSpec( attrib,
		    Attrib::SelSpec(attrnm,Attrib::SelSpec::cOtherAttrib()) );
    }
}


int HorizonDisplay::selectedTexture( int attrib ) const
{
    if ( attrib<0 || attrib>=nrAttribs() || sections_.isEmpty() ) return 0;

    mDynamicCastGet(const visBase::ParametricSurface*,psurf,sections_[0]);
    return psurf ? psurf->activeVersion( attrib ) : 0;
}


SurveyObject::AttribFormat HorizonDisplay::getAttributeFormat( int ) const
{
    if ( sections_.isEmpty() ) return SurveyObject::None;
    mDynamicCastGet(const visBase::ParametricSurface*,ps,sections_[0]);
    return ps ? SurveyObject::RandomPos : SurveyObject::None;
}


int HorizonDisplay::nrAttribs() const
{ return as_.size(); }


bool HorizonDisplay::canAddAttrib( int nr ) const
{
    mDynamicCastGet(const visBase::ParametricSurface*,psurf,sections_[0]);
    const int maxnr = psurf ? psurf->maxNrTextures() : 0;
    if ( !maxnr ) return true;

    return nrAttribs()+nr<=maxnr;
}


bool HorizonDisplay::addAttrib()
{
    as_ += new Attrib::SelSpec;
    coltabs_ += visBase::VisColorTab::create();
    const int curattrib = coltabs_.size()-1;
    coltabs_[curattrib]->ref();
    coltabs_[curattrib]->setAutoScale( true );
    coltabs_[curattrib]->setClipRate( 0.025 );
    coltabs_[curattrib]->setSymMidval( mUdf(float) );
    TypeSet<float> shift;
    shift += 0.0;
    shifts_ += shift;
    curshiftidx_ += 0;
    BufferStringSet* aatrnms = new BufferStringSet();
    aatrnms->allowNull();
    userrefs_ += aatrnms;
    enabled_ += true;
    datapackids_ += -1;

    for ( int idx=0; idx<sections_.size(); idx++ )
    {
	mDynamicCastGet(visBase::ParametricSurface*,psurf,sections_[idx]);
	if ( psurf )
	{
	    psurf->addTexture();
	    psurf->setColorTab( curattrib, *coltabs_[curattrib] );
	}
    }

    return true;
}


bool HorizonDisplay::removeAttrib( int attrib )
{
    for ( int idx=0; idx<sections_.size(); idx++ )
    {
	mDynamicCastGet(visBase::ParametricSurface*,psurf,sections_[idx]);
	if ( psurf )
	    psurf->removeTexture( attrib );
    }

    coltabs_[attrib]->unRef();
    coltabs_.remove( attrib );
    shifts_.remove( attrib );
    curshiftidx_.remove( attrib );
    userrefs_.remove( attrib );
    enabled_.remove( attrib );
    DPM( DataPackMgr::FlatID() ).release( datapackids_[attrib] );
    datapackids_.remove( attrib );

    delete as_[attrib];
    as_.remove( attrib );
    return true;
}


bool HorizonDisplay::swapAttribs( int a0, int a1 )
{
    for ( int idx=0; idx<sections_.size(); idx++ )
    {
	mDynamicCastGet(visBase::ParametricSurface*,psurf,sections_[idx]);
	if ( psurf )
	    psurf->swapTextures( a0, a1 );
    }

    as_.swap( a0, a1 );
    coltabs_.swap( a0, a1 );
    enabled_.swap( a0, a1 );
    shifts_.swap( a0, a1 );
    curshiftidx_.swap( a0, a1 );
    userrefs_.swap( a0, a1 );
    datapackids_.swap( a0, a1 );
    return true;
}


void HorizonDisplay::setAttribTransparency( int attrib, unsigned char nt )
{
    for ( int idx=0; idx<sections_.size(); idx++ )
    {
	mDynamicCastGet(visBase::ParametricSurface*,psurf,sections_[idx]);
	if ( psurf )
	    psurf->setTextureTransparency( attrib, nt );
    }
}


unsigned char HorizonDisplay::getAttribTransparency( int attrib ) const
{
    for ( int idx=0; idx<sections_.size(); idx++ )
    {
	mDynamicCastGet(const visBase::ParametricSurface*,psurf,sections_[idx]);
	if ( psurf )
	    return psurf->getTextureTransparency( attrib );
    }

    return 0;
}


void HorizonDisplay::enableAttrib( int attribnr, bool yn )
{
    for ( int idx=0; idx<sections_.size(); idx++ )
    {
	mDynamicCastGet(visBase::ParametricSurface*,psurf,sections_[idx]);
	if ( psurf )
	    psurf->enableTexture( attribnr, yn );
    }
    
    mDynamicCastGet(EM::Horizon3D*,emsurf,emobject_);
    int attribidx = shifts_.size()-1;
    if ( attribnr != 0 )
    {
	while ( !isAttribEnabled(attribidx) && attribidx >= 0 )
	    attribidx--;
	
	const float zshift = attribidx < 0 ? 0
	    : shifts_[attribidx][(curshiftidx_[attribidx])];
	emsurf->geometry().setShift( zshift );
	Coord3 transl = getTranslation();
	transl.z = zshift;
	setTranslation( transl );
    }
}


bool HorizonDisplay::isAttribEnabled( int attribnr ) const
{
    if ( sections_.isEmpty() )
	return true;

    mDynamicCastGet(const visBase::ParametricSurface*,psurf,sections_[0]);
    return psurf ? psurf->isTextureEnabled(attribnr) : true;
}


void HorizonDisplay::setAttribShift( int attribnr, const TypeSet<float>& shifts)
{
    if ( shifts_.size()-1 < attribnr )
    {
	TypeSet<float> shft;
	shft += 0.0;
	shifts_.setSize( attribnr+1, shft );
	curshiftidx_.setSize( attribnr+1, 0 );
    }
    shifts_[ attribnr ] = shifts;
}


bool HorizonDisplay::isAngle( int attribnr ) const
{
    if ( sections_.isEmpty() )
	return false;

    mDynamicCastGet(const visBase::ParametricSurface*,psurf,sections_[0]);
    return psurf ? psurf->isTextureAngle(attribnr) : false;
}
    

void HorizonDisplay::setAngleFlag( int attribnr, bool yn )
{
    for ( int idx=sections_.size()-1; idx>=0; idx-- )
    {
	mDynamicCastGet(visBase::ParametricSurface*,psurf,sections_[idx]);
	if ( psurf ) psurf->setTextureAngleFlag( attribnr, yn );
    }
}


void HorizonDisplay::allowShading( bool yn )
{
    for ( int idx=sections_.size()-1; idx>=0; idx-- )
    {
	mDynamicCastGet(visBase::ParametricSurface*,psurf,sections_[idx]);
	if ( psurf ) psurf->allowShading( yn );
    }
}


const Attrib::SelSpec* HorizonDisplay::getSelSpec( int attrib ) const
{ return as_[attrib]; }


void HorizonDisplay::setSelSpec( int attrib, const Attrib::SelSpec& as )
{
    (*as_[attrib]) = as;
}


void HorizonDisplay::setDepthAsAttrib( int attrib )
{
    as_[attrib]->set( "Depth", Attrib::SelSpec::cNoAttrib(), false, "" );
    coltabs_[attrib]->setAutoScale( true );
    coltabs_[attrib]->setClipRate( 0 );
    coltabs_[attrib]->setSymMidval( mUdf(float) );

    TypeSet<DataPointSet::DataRow> pts;
    BufferStringSet nms;
    nms.add( "Depth" );
    DataPointSet positions( pts, nms, false, true );
    getRandomPos( positions );

    if ( !positions.size() ) return;

    BinIDValueSet& bivs = positions.bivSet();
    if ( bivs.nrVals()!=2 )
	bivs.setNrVals(2);

    BinIDValueSet::Pos pos;
    while ( bivs.next(pos,true) )
    {
	float* vals = bivs.getVals(pos);
	if ( zaxistransform_ )
	{
	    vals[1] = zaxistransform_->transform(
		    BinIDValue( bivs.getBinID(pos), vals[0] ) );
	}
	else
	    vals[1] = vals[0];
    }

    createAndDispDataPack( attrib, &positions );
}


void HorizonDisplay::createAndDispDataPack( int attrib,
					    const DataPointSet* positions )
{
    BufferStringSet* attrnms = new BufferStringSet();
    for ( int idx=0; idx<positions->nrCols(); idx++ )
	attrnms->add( positions->colDef( idx ).name_ );
    userrefs_.replace( attrib, attrnms );
    bool isz = ( attrnms->size()==1 && !strcmp(attrnms->get(0).buf(),"Depth") );
    mDeclareAndTryAlloc( BIDValSetArrAdapter*, bvsarr, 
	    		 BIDValSetArrAdapter(positions->bivSet(), isz? 0 : 1) );
    const char* categorynm = isz ? "Surface Data" : "Geometry";
    mDeclareAndTryAlloc( MapDataPack*, newpack,
	    		 MapDataPack( categorynm,attrnms->get(0).buf(),bvsarr));
    StepInterval<double> inlrg( bvsarr->inlrg_.start, bvsarr->inlrg_.stop,
				SI().inlStep() );
    StepInterval<double> crlrg( bvsarr->crlrg_.start, bvsarr->crlrg_.stop,
				SI().crlStep() );
    BufferStringSet dimnames;
    dimnames.add("X").add("Y").add("In-Line").add("Cross-line");
    newpack->setPropsAndInit( inlrg, crlrg, true, &dimnames );
    DataPackMgr& dpman = DPM( DataPackMgr::FlatID() );
    dpman.add( newpack );
    setDataPackID( attrib, newpack->id() );
    setRandomPosData( attrib, positions );
}


void HorizonDisplay::getRandomPos( DataPointSet& data ) const
{
    //put all sections in the same DataPointSet: anyway we are not -yet?- able 
    //to display attributes on horizon with different value for each section
    data.bivSet().allowDuplicateBids(false);
    for ( int idx=0; idx<sections_.size(); idx++ )
    {
	mDynamicCastGet(const visBase::ParametricSurface*,psurf,sections_[idx]);
	if ( !psurf ) return;

	const float zfactor = scene_ ? scene_->getZScale() : SI().zScale();
	psurf->getDataPositions( data.bivSet(), getTranslation().z/zfactor  );
    }
    data.dataChanged();
}


void HorizonDisplay::getRandomPosCache( int attrib, DataPointSet& data ) const
{
    if ( attrib < 0 ) return;
    for ( int idx=0; idx<sections_.size(); idx++ )
    {
	mDynamicCastGet(const visBase::ParametricSurface*,psurf,sections_[idx]);
	if ( !psurf ) continue;

	data.bivSet() = *psurf->getCache( attrib );
    }
    data.dataChanged();
}


void HorizonDisplay::updateSingleColor()
{
    const bool usesinglecolor = !showingTexture();
    getMaterial()->setColor( usesinglecolor ? nontexturecol_ : Color::White() );

    for ( int idx=0; idx<sections_.size(); idx++ )
    {
	mDynamicCastGet(visBase::ParametricSurface*,psurf,sections_[idx]);
	if ( psurf )
	    psurf->useTexture( !usesinglecolor );
    }
}


void HorizonDisplay::setRandomPosData( int attrib, const DataPointSet* data )
{
    if ( !data || !data->size() )
    {
	validtexture_ = false;
	updateSingleColor();
	return;
    }

    //TODO make it compatible with multiple sections which all contain data
    //when (if) we are able to display all this info
    if ( sections_.size() )
    {
	mDynamicCastGet(visBase::ParametricSurface*,psurf,sections_[0]);
	if ( psurf )
	    psurf->setTextureData( &data->bivSet(), attrib );
    }

    for ( int idx=1; idx<sections_.size(); idx++ )
    {
	mDynamicCastGet(visBase::ParametricSurface*,psurf,sections_[idx]);
	if ( psurf ) psurf->setTextureData( 0, attrib );
    }

    validtexture_ = true;
    usestexture_ = true;
    updateSingleColor();
}


bool HorizonDisplay::hasStoredAttrib( int attrib ) const
{
    const char* userref = as_[attrib]->userRef();
    return as_[attrib]->id()==Attrib::SelSpec::cOtherAttrib() &&
	   userref && *userref;
}


bool HorizonDisplay::hasDepth( int attrib ) const
{
    return as_[attrib]->id()==Attrib::SelSpec::cNoAttrib();
}


Coord3 HorizonDisplay::getTranslation() const
{
    if ( !translation_ ) return Coord3(0,0,0);

    Coord3 shift = translation_->getTranslation();
    shift.z *= -1; 
    return shift;
}


void HorizonDisplay::setTranslation( const Coord3& nt )
{
    if ( !translation_ )
    {
	translation_ = visBase::Transformation::create();
	translation_->ref();
	insertChild( 0, translation_->getInventorNode() );
    }

    Coord3 shift( nt ); shift.z *= -1;
    translation_->setTranslation( shift );

    if ( getOnlyAtSectionsDisplay() )
	setOnlyAtSectionsDisplay( true );		/* retrigger */
}


bool HorizonDisplay::usesWireframe() const { return useswireframe_; }


void HorizonDisplay::useWireframe( bool yn )
{
    useswireframe_ = yn;

    for ( int idx=0; idx<sections_.size(); idx++ )
    {
	mDynamicCastGet(visBase::ParametricSurface*,ps,sections_[idx]);
	if ( ps ) ps->useWireframe( yn );
    }
}


void HorizonDisplay::setEdgeLineRadius(float nr)
{
    edgelineradius_ = nr;
    for ( int idx=0; idx<edgelinedisplays_.size(); idx++ )
	edgelinedisplays_[idx]->setRadius(nr);
}


float HorizonDisplay::getEdgeLineRadius() const
{ return edgelineradius_; }


void HorizonDisplay::removeSectionDisplay( const EM::SectionID& sid )
{
    const int idx = sids_.indexOf( sid );
    if ( idx<0 ) return;

    removeChild( sections_[idx]->getInventorNode() );
    sections_[idx]->unRef();
    sections_.remove( idx );
    sids_.remove( idx );
};


bool HorizonDisplay::addSection( const EM::SectionID& sid )
{
    visBase::ParametricSurface* surf = visBase::ParametricSurface::create();

    mDynamicCastGet( EM::Horizon3D*, horizon, emobject_ );
    surf->setSurface(horizon->geometry().sectionGeometry(sid), true );

    while ( surf->nrTextures()<nrAttribs() ) surf->addTexture();

    for ( int idx=0; idx<nrAttribs(); idx++ )
    {
	surf->setColorTab( idx, *coltabs_[idx] );
	surf->enableTexture( idx, enabled_[idx] );
    }

    surf->useWireframe( useswireframe_ );
    surf->setResolution( resolution_-1 );

    surf->ref();
    surf->setMaterial( 0 );
    surf->setDisplayTransformation( transformation_ );
    surf->setZAxisTransform( zaxistransform_ );
    const int index = childIndex(drawstyle_->getInventorNode());
    insertChild( index, surf->getInventorNode() );
    surf->turnOn( !displayonlyatsections_ );

    sections_ += surf;
    sids_ += sid;

    hasmoved.trigger();

    return addEdgeLineDisplay( sid );
}


void HorizonDisplay::setOnlyAtSectionsDisplay( bool yn )
{
    for ( int idx=0; idx<sections_.size(); idx++ )
	sections_[idx]->turnOn(!yn);

    EMObjectDisplay::setOnlyAtSectionsDisplay( yn );
}



void HorizonDisplay::emChangeCB( CallBacker* cb )
{
    EMObjectDisplay::emChangeCB( cb );
    mCBCapsuleUnpack(const EM::EMObjectCallbackData&,cbdata,cb);
    mDynamicCastGet(const EM::Horizon3D*,emhorizon,emobject_);
    if ( cbdata.event==EM::EMObjectCallbackData::PositionChange )
    {
	validtexture_ = false;
	updateSingleColor();

	const EM::SectionID sid = cbdata.pid0.sectionID();
	const int idx = sids_.indexOf( sid );
	if ( idx>=0 )
	{
	    mDynamicCastGet(visBase::ParametricSurface*,ps,sections_[idx]);
	    if ( ps ) ps->inValidateCache(-1);
	}
    }
    else if ( cbdata.event==EM::EMObjectCallbackData::PrefColorChange )
    {
	nontexturecol_ = emobject_->preferredColor();
	if ( !usestexture_ )
	    getMaterial()->setColor( nontexturecol_ );
    }
}


void HorizonDisplay::emEdgeLineChangeCB(CallBacker* cb)
{
    mCBCapsuleUnpack( EM::SectionID, section, cb );

    mDynamicCastGet(const EM::Horizon3D*,emhorizon,emobject_);
    if ( !emhorizon ) return;

    if ( emhorizon->edgelinesets.getEdgeLineSet( section, false ) )
	 addEdgeLineDisplay(section);
    else
    {
	for ( int idx=0; idx<edgelinedisplays_.size(); idx++ )
	{
	    if (edgelinedisplays_[idx]->getEdgeLineSet()->getSection()==section)
  	    {
		EdgeLineSetDisplay* elsd = edgelinedisplays_[idx--];
		edgelinedisplays_ -= elsd;
		elsd->rightClicked()->remove(
			 mCB( this, HorizonDisplay, edgeLineRightClickCB ));
		removeChild( elsd->getInventorNode() );
		elsd->unRef();
		break;
	    }
	}
    }
}


int HorizonDisplay::nrResolutions() const
{
    if ( sections_.isEmpty() ) return 1;

    mDynamicCastGet(const visBase::ParametricSurface*,ps,sections_[0]);
    return ps ? ps->nrResolutions()+1 : 1;
}


BufferString HorizonDisplay::getResolutionName( int res ) const
{
    BufferString str;
    if ( !res ) str = "Automatic";
    else
    {
	res = nrResolutions() - res;
	res--;
	int val = 1;
	for ( int idx=0; idx<res; idx++ )
	    val *= 2;

	if ( val==2 ) 		str = "Half";
	else if ( val==1 ) 	str = "Full";
	else 			{ str = "1 / "; str += val; }
    }

    return str;
}


int HorizonDisplay::getResolution() const
{
    if ( sections_.isEmpty() ) return 0;

    mDynamicCastGet(const visBase::ParametricSurface*,ps,sections_[0]);
    return ps ? ps->currentResolution()+1 : 0;
}


void HorizonDisplay::setResolution( int res )
{
    resolution_ = res;
    for ( int idx=0; idx<sections_.size(); idx++ )
    {
	mDynamicCastGet(visBase::ParametricSurface*,ps,sections_[idx]);
	if ( ps ) ps->setResolution( res-1 );
    }
}


int HorizonDisplay::getColTabID(int attrib) const
{
    if ( !usesTexture() || sections_.isEmpty() )
	return -1;

    mDynamicCastGet(const visBase::ParametricSurface*,psurf,sections_[0]);
    return psurf->getColTabID( attrib );
}


const ColTab::Sequence* HorizonDisplay::getColTabSequence( int attrib ) const
{
    if ( !usesTexture() || sections_.isEmpty() )
	return 0;

    mDynamicCastGet(const visBase::ParametricSurface*,psurf,sections_[0]);
    return psurf ? psurf->getColTabSequence( attrib ) : 0;
}


bool HorizonDisplay::canSetColTabSequence() const
{ return usesTexture(); }


void HorizonDisplay::setColTabSequence( int attr, const ColTab::Sequence& seq )
{
    if ( !usesTexture() || sections_.isEmpty() )
	return;

    for ( int idx=0; idx<sections_.size(); idx++ )
    {
	mDynamicCastGet(visBase::ParametricSurface*,psurf,sections_[idx]);
	if ( psurf ) psurf->setColTabSequence( attr, seq );
    }
}


void HorizonDisplay::setColTabMapperSetup( int attr,
					   const ColTab::MapperSetup& ms )
{
    if ( !usesTexture() || sections_.isEmpty() )
	return;

    for ( int idx=0; idx<sections_.size(); idx++ )
    {
	mDynamicCastGet(visBase::ParametricSurface*,psurf,sections_[idx]);
	if ( psurf ) psurf->setColTabMapperSetup( attr, ms );
    }
}


const ColTab::MapperSetup* HorizonDisplay::getColTabMapperSetup( int attr) const
{
    if ( !usesTexture() || sections_.isEmpty() )
	return 0;

    mDynamicCastGet(const visBase::ParametricSurface*,psurf,sections_[0]);
    return psurf ? psurf->getColTabMapperSetup( attr ) : 0;
}


float HorizonDisplay::calcDist( const Coord3& pickpos ) const
{
    if ( !emobject_ ) return mUdf(float);

    const mVisTrans* utm2display = scene_->getUTM2DisplayTransform();
    const Coord3 xytpos = utm2display->transformBack( pickpos );
    mDynamicCastGet(const EM::Horizon3D*,hor,emobject_)
    if ( hor )
    {
	const BinID bid = SI().transform( xytpos );
	const EM::SubID bidid = bid.getSerialized();
	TypeSet<Coord3> positions;
	for ( int idx=sids_.size()-1; idx>=0; idx-- )
	{
	    if ( !emobject_->isDefined( sids_[idx], bidid ) )
		continue;

	    positions += emobject_->getPos( sids_[idx], bidid );
	}

	float mindist = mUdf(float);
	for ( int idx=0; idx<positions.size(); idx++ )
	{
	    const float zfactor = scene_ ? scene_->getZScale(): SI().zScale();
	    const Coord3& pos = positions[idx] + getTranslation()/zfactor;
	    const float dist = fabs(xytpos.z-pos.z);
	    if ( dist < mindist ) mindist = dist;
	}

	return mindist;
    }

    return mUdf(float);
}


float HorizonDisplay::maxDist() const
{
    return SI().zRange(true).step;
}


EM::SectionID HorizonDisplay::getSectionID( int visid ) const
{
    for ( int idx=0; idx<sections_.size(); idx++ )
    {
	if ( sections_[idx]->id()==visid )
		return sids_[idx];
    }

    return -1;
}


void HorizonDisplay::getMousePosInfo( const visBase::EventInfo& eventinfo,
				       const Coord3& pos, BufferString& val,
				       BufferString& info ) const
{
    EMObjectDisplay::getMousePosInfo( eventinfo, pos, val, info );
    if ( !emobject_ || !usesTexture() ) return;

    const EM::SectionID sid =
	EMObjectDisplay::getSectionID(&eventinfo.pickedobjids);

    const int sectionidx = sids_.indexOf( sid );
    if ( sectionidx<0 ) return;

    mDynamicCastGet(const visBase::ParametricSurface*,psurf,
	    	    sections_[sectionidx]);
    if ( !psurf ) return;

    const BinID bid( SI().transform(pos) );

    for ( int idx=as_.size()-1; idx>=0; idx-- )
    {
	if ( as_[idx]->id().isUnselInvalid() )
	    return;

	if ( !psurf->isTextureEnabled(idx) ||
	       psurf->getTextureTransparency(idx)==255 )
	    continue;

	const BinIDValueSet* bidvalset = psurf->getCache( idx );
	if ( !bidvalset || bidvalset->nrVals()<2 ) continue;

	const BinIDValueSet::Pos setpos = bidvalset->findFirst( bid );
	if ( setpos.i<0 || setpos.j<0 ) continue;

	const float* vals = bidvalset->getVals( setpos );
	int curtexture = selectedTexture(idx);
	if ( curtexture>bidvalset->nrVals()-1 ) curtexture = 0;

	const float fval = vals[curtexture+1];
	bool islowest = true;
	for ( int idy=idx-1; idy>=0; idy-- )
	{
	    if ( !psurf->getCache(idy) || !isAttribEnabled(idy) ||
		  psurf->getTextureTransparency(idy)==255 )
		continue;
							                 
		islowest = false;
		break;
	}    

	if ( !islowest )
	{
	    const Color col = coltabs_[idx]->color(fval);
	    if ( col.t()==255 )
		continue;
	}
	    
	if ( !mIsUdf(fval) )
	{
	    val = fval;
	    if ( as_.size() > 1 )
	    {
		BufferString attribstr = "(";
		attribstr += as_[idx]->userRef();
		attribstr += ")";
		val.replaceAt( cValNameOffset(), attribstr );
	    }
	}

	return;
    }
}


#define mEndLine \
{ \
    if ( cii && line->getCoordIndex(cii-1)!=-1 ) \
    { \
	if ( cii==1 || line->getCoordIndex(cii-2)==-1 ) \
	    line->getCoordinates()->removePos( line->getCoordIndex(--cii) ); \
	else \
	    line->setCoordIndex(cii++,-1); \
    } \
} 


#define mTraverseLine( linetype,startbid,faststop,faststep,slowdim,fastdim ) \
\
    const StepInterval<int> inlrg = horizon->geometry().rowRange(sid);\
    const StepInterval<int> crlrg = horizon->geometry().colRange(sid); \
\
    const int target##linetype = cs.hrg.start.linetype; \
    if ( !linetype##rg.includes(target##linetype) ) \
    { \
	mEndLine; \
	continue; \
    } \
 \
    const int rgindex = linetype##rg.getIndex(target##linetype); \
    const int prev##linetype = linetype##rg.atIndex(rgindex); \
    const int next##linetype = prev##linetype<target##linetype \
	? linetype##rg.atIndex(rgindex+1) \
	: prev##linetype; \
 \
    for ( BinID bid=startbid; bid[fastdim]<=faststop; bid[fastdim]+=faststep ) \
    { \
	if ( !cs.hrg.includes(bid) ) \
	{ \
	    mEndLine; \
	    continue; \
	} \
 \
	BinID prevbid( bid ); prevbid[slowdim] = prev##linetype; \
	BinID nextbid( bid ); nextbid[slowdim] = next##linetype; \
	Coord3 prevpos(horizon->getPos(sid,prevbid.getSerialized())); \
	if ( zaxistransform_ ) prevpos.z =zaxistransform_->transform(prevpos); \
	Coord3 pos = prevpos; \
	if ( nextbid!=prevbid && prevpos.isDefined() ) \
	{ \
	    Coord3 nextpos = \
		horizon->getPos(sid,nextbid.getSerialized()); \
	    if ( zaxistransform_ ) nextpos.z = \
	    	zaxistransform_->transform(nextpos); \
	    if ( nextpos.isDefined() ) \
	    { \
		const float frac = float( target##linetype - prev##linetype ) \
				   / linetype##rg.step; \
		pos = (1-frac)*prevpos + frac*nextpos; \
	    } \
	} \
 \
	if ( !pos.isDefined() || !cs.zrg.includes(pos.z) ) \
	{ \
	    mEndLine; \
	    continue; \
	} \
 \
	line->setCoordIndex(cii++, line->getCoordinates()->addPos(pos)); \
    } \
    mEndLine;


static void drawHorizonOnRandomTrack( const TypeSet<Coord>& trclist, 
			const StepInterval<float>& zrg, 
			const EM::Horizon3D* hor, const EM::SectionID&  sid, 
			const ZAxisTransform* zaxistransform, 
			visBase::IndexedShape* line, int& cii )
{
    const StepInterval<int> inlrg = hor->geometry().rowRange( sid );
    const StepInterval<int> crlrg = hor->geometry().colRange( sid );

    int startidx; 
    int stopidx = 0; 
    int jumpstart = 0;

    while ( true )
    {
	startidx = stopidx;
	while ( startidx<trclist.size()-1 && 
		trclist[startidx]==trclist[startidx+1] ) startidx++;

	stopidx = startidx + 1;
	while ( stopidx<trclist.size()-1 && 
		trclist[stopidx]!=trclist[stopidx+1] ) stopidx++;

	if ( stopidx >= trclist.size() )
	    break;

	const Coord startcrd = SI().binID2Coord().transform(trclist[startidx]);
	const Coord stopcrd = SI().binID2Coord().transform(trclist[stopidx]);

	for ( int cidx=startidx+jumpstart; cidx<=stopidx; cidx++ )
	{
	    const int inlidx = inlrg.getIndex( trclist[cidx].x );
	    const int inl0 = inlrg.atIndex( inlidx );
	    const int inl1 = inl0<trclist[cidx].x ? 
			     inlrg.atIndex( inlidx+1 ) : inl0;

	    const int crlidx = crlrg.getIndex( trclist[cidx].y );
	    const int crl0 = crlrg.atIndex( crlidx );
	    const int crl1 = crl0<trclist[cidx].y ? 
			     crlrg.atIndex( crlidx+1 ) : crl0;

	    Coord3 p00 = hor->getPos( sid, BinID(inl0,crl0).getSerialized() );
	    Coord3 p01 = hor->getPos( sid, BinID(inl0,crl1).getSerialized() );
	    Coord3 p10 = hor->getPos( sid, BinID(inl1,crl0).getSerialized() );
	    Coord3 p11 = hor->getPos( sid, BinID(inl1,crl1).getSerialized() );

	    if ( zaxistransform )
	    {
		p00.z = zaxistransform->transform( p00 );
		p01.z = zaxistransform->transform( p01 );
		p10.z = zaxistransform->transform( p10 );
		p11.z = zaxistransform->transform( p11 );
	    }

	    if ( p00.isDefined() && p01.isDefined() && 
		 p10.isDefined() && p11.isDefined() )
	    {
		const float frac = float(cidx-startidx) / (stopidx-startidx);
		Coord3 pos = (1-frac) * Coord3(startcrd,0) +
		    		frac  * Coord3(stopcrd, 0);
	    
		const float ifrac = (trclist[cidx].x - inl0) / inlrg.step;
		const float cfrac = (trclist[cidx].y - crl0) / crlrg.step;
		pos.z = (1-ifrac)*( (1-cfrac)*p00.z + cfrac*p01.z ) +
			   ifrac *( (1-cfrac)*p10.z + cfrac*p11.z );

		if ( zrg.includes(pos.z) )
		{
		    line->setCoordIndex( cii++, 
			    		 line->getCoordinates()->addPos(pos) );
		    continue;
		}
	    }
	    mEndLine; 
	}
	
	jumpstart = 1;
    }
    mEndLine; 
}


static void drawHorizonOnTimeSlice( const CubeSampling& cs, float zfactor,
			const EM::Horizon3D* hor, const EM::SectionID&  sid, 
			const ZAxisTransform* zaxistransform, 
			visBase::IndexedShape* line, int& cii )
{
    const Array2D<float>* field = 
			hor->geometry().sectionGeometry(sid)->getArray(); 
    if ( zaxistransform )
	field = hor->createArray2D( sid, zaxistransform );

    IsoContourTracer ictracer( *field );
    ictracer.setSampling( hor->geometry().rowRange(sid),
	   		  hor->geometry().colRange(sid) );
    ictracer.selectRectROI( cs.hrg.inlRange(), cs.hrg.crlRange() );
    ObjectSet<ODPolygon<float> > isocontours;
    const float zshift = hor->geometry().getShift() / zfactor;
    ictracer.getContours( isocontours, cs.zrg.start-zshift, false );
    
    for ( int cidx=0; cidx<isocontours.size(); cidx++ )
    {
	const ODPolygon<float>& ic = *isocontours[cidx];
	for ( int vidx=0; vidx<ic.size(); vidx++ )
	{
	    const Geom::Point2D<float> vertex = ic.getVertex( vidx );
	    Coord vrtxcoord( vertex.x, vertex.y );
	    vrtxcoord = SI().binID2Coord().transform( vrtxcoord );
	    const Coord3 pos( vrtxcoord, cs.zrg.start );
	    const int posidx = line->getCoordinates()->addPos( pos );
	    line->setCoordIndex( cii++, posidx ); 
	}
	
	if ( ic.isClosed() )
	{
	    const int posidx = line->getCoordIndex( cii-ic.size() );
	    line->setCoordIndex( cii++, posidx );
	}
	line->setCoordIndex( cii++, -1 );
    }

    if ( zaxistransform ) delete field;
    deepErase( isocontours );
}


void HorizonDisplay::updateIntersectionLines(
	    const ObjectSet<const SurveyObject>& objs, int whichobj )
{
    mDynamicCastGet(const EM::Horizon3D*,horizon,emobject_);
    if ( !horizon ) return;

    TypeSet<int> linestoupdate;
    BoolTypeSet lineshouldexist( intersectionlineids_.size(), false );

    if ( displayonlyatsections_ )
    {
	for ( int idx=0; idx<objs.size(); idx++ )
	{
	    int objectid = -1;
	    mDynamicCastGet( const PlaneDataDisplay*, plane, objs[idx] );
	    if ( plane )
		objectid = plane->id();
	    
	    mDynamicCastGet( const MPEDisplay*, mped, objs[idx] );
	    if ( mped && mped->isDraggerShown() )
		objectid = mped->id();

	    mDynamicCastGet( const RandomTrackDisplay*, rtdisplay, objs[idx] );
	    if ( rtdisplay )
		objectid = rtdisplay->id();
	    
	    mDynamicCastGet( const Seis2DDisplay*, seis2ddisplay, objs[idx] );
	    if ( seis2ddisplay )
		objectid = seis2ddisplay->id();

	    if ( objectid==-1 )
		continue;

	    const int idy = intersectionlineids_.indexOf(objectid);
	    if ( idy==-1 )
	    {
		linestoupdate += objectid;
		lineshouldexist += true;
	    }
	    else
	    {
		if ( ( whichobj==objectid || whichobj==id() ) && 
		     linestoupdate.indexOf(whichobj)==-1 )
		{
		    linestoupdate += objectid;
		}

		lineshouldexist[idy] = true;
	    }
	}
    }

    for ( int idx=0; idx<intersectionlineids_.size(); idx++ )
    {
	if ( !lineshouldexist[idx] )
	{
	    removeChild( intersectionlines_[idx]->getInventorNode() );
	    intersectionlines_[idx]->unRef();

	    lineshouldexist.remove(idx);
	    intersectionlines_.remove(idx);
	    intersectionlineids_.remove(idx);
	    if ( zaxistransform_ )
	    {
		zaxistransform_->removeVolumeOfInterest(
			intersectionlinevoi_[idx] );
	    }

	    intersectionlinevoi_.remove(idx);
	    idx--;
	}
    }

    for ( int idx=0; idx<linestoupdate.size(); idx++ )
    {
	CubeSampling cs(false);
	mDynamicCastGet( PlaneDataDisplay*, plane,
			 visBase::DM().getObject(linestoupdate[idx]) );
	if ( plane )
	    cs = plane->getCubeSampling(true,true,-1);

	mDynamicCastGet( const MPEDisplay*, mped,
			 visBase::DM().getObject(linestoupdate[idx]) );
	if ( mped )
	    mped->getPlanePosition(cs);

	TypeSet<Coord> trclist;
	mDynamicCastGet( const RandomTrackDisplay*, rtdisplay,
			 visBase::DM().getObject(linestoupdate[idx]) );
	if ( rtdisplay )
	{
	    cs.zrg.setFrom( rtdisplay->getDataTraceRange() );
	    cs.zrg.step = SI().zStep();
	    TypeSet<BinID> tracebids;
	    rtdisplay->getDataTraceBids( tracebids );
	    for ( int bidx=0; bidx<tracebids.size(); bidx++ )
	    {
		cs.hrg.include( tracebids[bidx] );
		trclist += Coord( tracebids[bidx].inl, tracebids[bidx].crl );
	    }
	}

	mDynamicCastGet( const Seis2DDisplay*, seis2ddisplay,
			 visBase::DM().getObject(linestoupdate[idx]) );
	if ( seis2ddisplay )
	{
	    cs.zrg.setFrom( seis2ddisplay->getZRange(false) );
	    cs.zrg.step = SI().zStep();
	    const Interval<int>& trcnrrg = seis2ddisplay->getTraceNrRange();
	    for ( int trcnr=trcnrrg.start; trcnr<=trcnrrg.stop; trcnr++ )
	    {
		Coord crd = seis2ddisplay->getCoord( trcnr );
		crd = SI().binID2Coord().transformBackNoSnap( crd );
		if ( mIsUdf(crd.x) || mIsUdf(crd.y) )
		    continue;

		cs.hrg.include( BinID((int) floor(crd.x),(int) floor(crd.y)) );
		cs.hrg.include( BinID((int)  ceil(crd.x),(int)  ceil(crd.y)) );
		trclist += crd; trclist += crd;
	    }
	}
	
	int lineidx = intersectionlineids_.indexOf(linestoupdate[idx]);
	if ( lineidx==-1 )
	{
	    lineidx = intersectionlineids_.size();
	    intersectionlineids_ += linestoupdate[idx];
	    intersectionlinevoi_ += -2;

	    const bool do3d = lineStyle()->type_==LineStyle::Solid;

	    visBase::IndexedShape* newline = do3d
		? (visBase::IndexedShape*) visBase::IndexedPolyLine3D::create()
		: (visBase::IndexedShape*) visBase::IndexedPolyLine::create();

	    newline->ref();

	    if ( do3d )
	    {
		const float radius = ((float) lineStyle()->width_) / 2;
		((visBase::IndexedPolyLine3D* ) newline)->setRadius( radius,
		    true, maxintersectionlinethickness_ );
	    }

	    newline->setRightHandSystem( righthandsystem_ );
	    newline->setDisplayTransformation(transformation_);
	    intersectionlines_ += newline;
	    addChild( newline->getInventorNode() );
	}

	if ( zaxistransform_ )
	{
	    if ( intersectionlinevoi_[lineidx]==-2 )
	    {
		intersectionlinevoi_[lineidx] =
		    zaxistransform_->addVolumeOfInterest(cs,true);
	    }
	    else
	    {
		zaxistransform_->setVolumeOfInterest(
			intersectionlinevoi_[lineidx],cs,true);
	    }

	    if ( intersectionlinevoi_[lineidx]>=0 )
	    {
		zaxistransform_->loadDataIfMissing(
			intersectionlinevoi_[lineidx] );
	    }
	}
	    
	visBase::IndexedShape* line = intersectionlines_[lineidx];
	line->removeCoordIndexAfter(-1);
	line->getCoordinates()->removeAfter(-1);
	int cii = 0;

	for ( int sectionidx=0; sectionidx<horizon->nrSections(); sectionidx++ )
	{
	    const EM::SectionID sid = horizon->sectionID(sectionidx);

	    if ( rtdisplay || seis2ddisplay )
	    {
		drawHorizonOnRandomTrack( trclist, cs.zrg, horizon, sid, 
					  zaxistransform_, line, cii );
	    }
	    else if ( cs.hrg.start.inl==cs.hrg.stop.inl )
	    {
		mTraverseLine( inl, BinID(targetinl,crlrg.start),
			       crlrg.stop, crlrg.step, 0, 1 );
	    }
	    else if ( cs.hrg.start.crl==cs.hrg.stop.crl )
	    {
		mTraverseLine( crl, BinID(inlrg.start,targetcrl),
			       inlrg.stop, inlrg.step, 1, 0 );
	    }
	    else
	    {
		const float zfactor = scene_
		    ? scene_->getZScale()
		    : SI().zScale();
		drawHorizonOnTimeSlice( cs, zfactor, horizon, sid,  
				        zaxistransform_, line, cii );
	    }
	}
    }
}


void HorizonDisplay::setLineStyle( const LineStyle& lst )
{
    if ( lst==*lineStyle() )
	return;

    const bool removelines =
	(lst.type_==LineStyle::Solid) != (lineStyle()->type_==LineStyle::Solid);

    EMObjectDisplay::setLineStyle( lst );

    if ( removelines )
    {
	for ( int idx=0; idx<intersectionlines_.size(); idx++ )
	{
	    visBase::IndexedShape* newline = lst.type_==LineStyle::Solid
		? (visBase::IndexedShape*) visBase::IndexedPolyLine3D::create()
		: (visBase::IndexedShape*) visBase::IndexedPolyLine::create();
	    newline->ref();
	    newline->setRightHandSystem( righthandsystem_ );
	    newline->setDisplayTransformation(transformation_);
	    newline->copyCoordIndicesFrom( *intersectionlines_[idx] );
	    newline->getCoordinates()->copyFrom(
		    *intersectionlines_[idx]->getCoordinates() );

	    removeChild( intersectionlines_[idx]->getInventorNode() );
	    addChild( newline->getInventorNode() );

	    intersectionlines_.replace( idx, newline )->unRef();

	    if ( lst.type_==LineStyle::Solid )
	    {
		const float radius = ((float) lineStyle()->width_) / 2;
		((visBase::IndexedPolyLine3D* ) newline )->
		    setRadius( radius, true, maxintersectionlinethickness_ );
	    }
	}
    }
    else if ( lst.type_==LineStyle::Solid )
    {
	for ( int idx=0; idx<intersectionlines_.size(); idx++ )
	{
	    const float radius = ((float) lineStyle()->width_) / 2;
	    visBase::IndexedPolyLine3D* pl =
		(visBase::IndexedPolyLine3D*) intersectionlines_[idx];
	    pl->setRadius( radius, true, maxintersectionlinethickness_ );
	}
    }
}



void HorizonDisplay::updateSectionSeeds( 
	    const ObjectSet<const SurveyObject>& objs, int movedobj )
{
    bool refresh = movedobj==-1 || movedobj==id();
    TypeSet<int> planelist;

    for ( int idx=0; idx<objs.size(); idx++ )
    {
	mDynamicCastGet(const PlaneDataDisplay*,plane,objs[idx]);
	if ( plane && plane->getOrientation()!=PlaneDataDisplay::Timeslice )
	{
	    planelist += idx; 
	    if ( movedobj==plane->id() ) 
		refresh = true;
	}

	mDynamicCastGet( const MPEDisplay*, mped, objs[idx] );
	if ( mped && mped->isDraggerShown() )
	{
	    CubeSampling cs;
	    if ( mped->getPlanePosition(cs) && cs.nrZ()!=1 )
	    {
		planelist += idx; 
		if ( movedobj==mped->id() ) 
		    refresh = true;
	    }
	}
    }

    if ( !refresh ) return;

    for ( int idx=0; idx<posattribmarkers_.size(); idx++ )
    {
	visBase::DataObjectGroup* group = posattribmarkers_[idx];
	for ( int idy=0; idy<group->size(); idy++ )
	{
	    mDynamicCastGet(visBase::Marker*,marker,group->getObject(idy))
	    if ( !marker ) continue;
	
	    marker->turnOn( !displayonlyatsections_ );
	    Coord3 pos = marker->centerPos();
	    if ( transformation_ )
		pos = transformation_->transform( pos );

	    for ( int idz=0; idz<planelist.size(); idz++ )
	    {
		const float dist = objs[planelist[idz]]->calcDist(pos);
		if ( dist < objs[planelist[idz]]->maxDist() )
		{
		    marker->turnOn(true);
		    break;
		}
	    }
	}
    }
}


void HorizonDisplay::otherObjectsMoved(
	    const ObjectSet<const SurveyObject>& objs, int whichobj )
{ 
    updateIntersectionLines( objs, whichobj ); 
    updateSectionSeeds( objs, whichobj );
}


void HorizonDisplay::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    visBase::VisualObjectImpl::fillPar( par, saveids );
    EMObjectDisplay::fillPar( par, saveids );

    if ( emobject_ && !emobject_->isFullyLoaded() )
    {
	par.set( sKeyRowRange(), displayedRowRange() );
	par.set( sKeyColRange(), displayedColRange() );
    }

    par.setYN( sKeyTexture(), usesTexture() );
    par.setYN( sKeyWireFrame(), usesWireframe() );
    par.set( sKeyShift(), getTranslation().z );
    par.set( sKeyResolution(), getResolution() );

    for ( int attrib=as_.size()-1; attrib>=0; attrib-- )
    {
	IOPar attribpar;
	as_[attrib]->fillPar( attribpar );
	const int coltabid = coltabs_[attrib]->id();
	attribpar.set( sKeyColTabID(), coltabid );
	if ( saveids.indexOf( coltabid )==-1 ) saveids += coltabid;

	attribpar.setYN( sKeyIsOn(), isAttribEnabled(attrib) );

	BufferString key = sKeyAttribs();
	key += attrib;
	par.mergeComp( attribpar, key );
    }

    par.set( sKeyNrAttribs(), as_.size() );
}


int HorizonDisplay::usePar( const IOPar& par )
{
    int res = visBase::VisualObjectImpl::usePar( par );
    if ( res!=1 ) return res;

    res = EMObjectDisplay::usePar( par );
    if ( res!=1 ) return res;

    if ( scene_ )
	setDisplayTransformation( scene_->getUTM2DisplayTransform() );

    if ( !par.get(sKeyEarthModelID,parmid_) )
	return -1;

    par.get( sKeyRowRange(), parrowrg_ );
    par.get( sKeyColRange(), parcolrg_ );

    if ( !par.getYN(sKeyTexture(),usestexture_) )
	usestexture_ = true;

    bool usewireframe = false;
    par.getYN( sKeyWireFrame(), usewireframe );
    useWireframe( usewireframe );

    int resolution = 0;
    par.get( sKeyResolution(), resolution );
    setResolution( resolution );

    Coord3 shift( 0, 0, 0 );
    par.get( sKeyShift(), shift.z );
    setTranslation( shift );

    int nrattribs;
    if ( par.get(sKeyNrAttribs(),nrattribs) ) //Current format
    {
	TypeSet<int> coltabids( nrattribs, -1 );
	for ( int attrib=0; attrib<nrattribs; attrib++ )
	{
	    BufferString key = sKeyAttribs();
	    key += attrib;
	    PtrMan<const IOPar> attribpar = par.subselect( key );
	    if ( !attribpar )
		continue;

	    if ( attribpar->get(sKeyColTabID(),coltabids[attrib]) )
	    {
		visBase::DataObject* dataobj =
		    visBase::DM().getObject( coltabids[attrib] );
		if ( !dataobj ) return 0;
		mDynamicCastGet(const visBase::VisColorTab*,coltab,dataobj);
		if ( !coltab ) coltabids[attrib] = -1;
	    }
	}

	bool firstattrib = true;
	for ( int attrib=0; attrib<nrattribs; attrib++ )
	{
	    BufferString key = sKeyAttribs();
	    key += attrib;
	    PtrMan<const IOPar> attribpar = par.subselect( key );
	    if ( !attribpar )
		continue;

	    if ( !firstattrib )
		addAttrib();
	    else
		firstattrib = false;

	    const int attribnr = as_.size()-1;

	    bool ison = true;
	    attribpar->getYN( sKeyIsOn(), ison );
	    enabled_[attribnr] = ison;

	    visBase::VisColorTab* coltab = 0;
	    const int coltabid = coltabids[attribnr];
	    if ( coltabid!=-1 )
	    {
		mDynamicCastGet(visBase::VisColorTab*,ct,
				visBase::DM().getObject(coltabid))
		coltabs_[attribnr]->unRef();
		coltabs_.replace( attribnr, ct );
		coltabs_[attribnr]->ref();
		coltab = ct;
	    }

	    as_[attribnr]->usePar( *attribpar );
	}
    }
    else //old format
    {
	as_[0]->usePar( par );
	int coltabid = -1;
	par.get( sKeyColorTableID(), coltabid );
	if ( coltabid>-1 )
	{
	    DataObject* dataobj = visBase::DM().getObject( coltabid );
	    if ( !dataobj ) return 0;

	    mDynamicCastGet( visBase::VisColorTab*, coltab, dataobj );
	    if ( !coltab ) return -1;
	    if ( coltabs_[0] ) coltabs_[0]->unRef();
	    coltabs_.replace( 0, coltab );
	    coltab->ref();
	    for ( int idx=0; idx<sections_.size(); idx++ )
	    {
		mDynamicCastGet( visBase::ParametricSurface*,psurf,
				 sections_[idx]);
		if ( psurf ) psurf->setColorTab( 0, *coltab);
	    }
	}
    }

    return 1;
}


bool HorizonDisplay::setDataPackID( int attrib, DataPack::ID dpid )
{
    DataPackMgr& dpman = DPM( DataPackMgr::FlatID() );
    const DataPack* datapack = dpman.obtain( dpid );
    if ( !datapack ) return false;

    DataPack::ID oldid = datapackids_[attrib];
    datapackids_[attrib] = dpid;
    dpman.release( oldid );
    return true;
}


DataPack::ID HorizonDisplay::getDataPackID( int attrib ) const
{
    return datapackids_[attrib];
}

}; // namespace visSurvey

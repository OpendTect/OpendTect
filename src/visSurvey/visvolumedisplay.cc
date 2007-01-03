/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          August 2002
 RCS:           $Id: visvolumedisplay.cc,v 1.49 2007-01-03 18:29:06 cvskris Exp $
________________________________________________________________________

-*/


#include "visvolumedisplay.h"

#include "visboxdragger.h"
#include "viscolortab.h"
//#include "visvolobliqueslice.h"
#include "visvolorthoslice.h"
#include "visvoltexture.h"
#include "visvolren.h"
#include "vistransform.h"

#include "cubesampling.h"
#include "attribsel.h"
#include "attribdatacubes.h"
#include "arrayndimpl.h"
#include "survinfo.h"
#include "visselman.h"
#include "visdataman.h"
#include "sorting.h"
#include "iopar.h"
#include "vismaterial.h"
#include "colortab.h"
#include "zaxistransform.h"

mCreateFactoryEntry( visSurvey::VolumeDisplay );

visBase::FactoryEntry visSurvey::VolumeDisplay::oldnameentry(
			(visBase::FactPtr) visSurvey::VolumeDisplay::create,
			"VolumeRender::VolumeDisplay");

namespace visSurvey {

const char* VolumeDisplay::volumestr = "Cube ID";
const char* VolumeDisplay::volrenstr = "Volren";
const char* VolumeDisplay::inlinestr = "Inline";
const char* VolumeDisplay::crosslinestr = "Crossline";
const char* VolumeDisplay::timestr = "Time";

const char* VolumeDisplay::nrslicesstr = "Nr of slices";
const char* VolumeDisplay::slicestr = "SliceID ";
const char* VolumeDisplay::texturestr = "TextureID";



VolumeDisplay::VolumeDisplay()
    : VisualObjectImpl(true)
    , boxdragger(visBase::BoxDragger::create())
    , texture(visBase::VolumeTexture::create())
    , volren(0)
    , as(*new Attrib::SelSpec)
    , cache(0)
    , slicemoving(this)
    , voltrans( visBase::Transformation::create() )
{
    boxdragger->ref();
    addChild( boxdragger->getInventorNode() );
    boxdragger->finished.notify( mCB(this,VolumeDisplay,manipMotionFinishCB) );
    getMaterial()->setColor( Color::White );
    getMaterial()->setAmbience( 0.8 );
    getMaterial()->setDiffIntensity( 0.8 );
    voltrans->ref();
    addChild( voltrans->getInventorNode() );
    texture->ref();
    addChild( texture->getInventorNode() );

    CubeSampling cs(false); CubeSampling sics = SI().sampling(true);
    cs.hrg.start.inl = (5*sics.hrg.start.inl+3*sics.hrg.stop.inl)/8;
    cs.hrg.start.crl = (5*sics.hrg.start.crl+3*sics.hrg.stop.crl)/8;
    cs.hrg.stop.inl = (3*sics.hrg.start.inl+5*sics.hrg.stop.inl)/8;
    cs.hrg.stop.crl = (3*sics.hrg.start.crl+5*sics.hrg.stop.crl)/8;
    cs.zrg.start = ( 5*sics.zrg.start + 3*sics.zrg.stop ) / 8;
    cs.zrg.stop = ( 3*sics.zrg.start + 5*sics.zrg.stop ) / 8;
    SI().snap( cs.hrg.start, BinID(0,0) );
    SI().snap( cs.hrg.stop, BinID(0,0) );
    float z0 = SI().zRange(true).snap( cs.zrg.start ); cs.zrg.start = z0;
    float z1 = SI().zRange(true).snap( cs.zrg.stop ); cs.zrg.stop = z1;
    
    setCubeSampling( cs );
    addSlice( 0 ); addSlice( 1 ); addSlice( 2 );
    showVolRen( true ); showVolRen( false );

    setColorTab( getColorTab() );
    showManipulator( true );
    texture->turnOn( true );
}


VolumeDisplay::~VolumeDisplay()
{
    delete &as;
    if ( cache ) cache->unRef();

    if ( volren ) volren->unRef();

    TypeSet<int> children;
    getChildren( children );
    for ( int idx=0; idx<children.size(); idx++ )
	removeSlice( children[idx] );

    texture->unRef();

    boxdragger->finished.remove( mCB(this,VolumeDisplay,manipMotionFinishCB) );
    boxdragger->unRef();

    voltrans->unRef();
}


void VolumeDisplay::setUpConnections()
{
    if ( !scene_ || !scene_->getDataTransform() ) return;

    ZAxisTransform* datatransform = scene_->getDataTransform();
    Interval<float> zrg = datatransform->getZInterval( false );
    CubeSampling cs = getCubeSampling( 0 );
    cs.zrg.start = zrg.start;
    cs.zrg.stop = zrg.stop;
    setCubeSampling( cs );
}


void VolumeDisplay::getChildren( TypeSet<int>&res ) const
{
    res.erase();
    for ( int idx=0; idx<slices.size(); idx++ )
	res += slices[idx]->id();
    if ( volren ) res += volren->id();
}


void VolumeDisplay::showManipulator( bool yn )
{ boxdragger->turnOn( yn ); }


bool VolumeDisplay::isManipulatorShown() const
{ return boxdragger->isOn(); }


bool VolumeDisplay::isManipulated() const
{
    return getCubeSampling(true,0) != getCubeSampling(false,0);
}


bool VolumeDisplay::canResetManipulation() const
{ return true; }


void VolumeDisplay::resetManipulation()
{
    const Coord3 center = voltrans->getTranslation();
    const Coord3 width = voltrans->getScale();
    boxdragger->setCenter( center );
    boxdragger->setWidth( Coord3(width.x, width.y, width.z) );
}


void VolumeDisplay::acceptManipulation()
{
    setCubeSampling( getCubeSampling(true,0) );
}


int VolumeDisplay::addSlice( int dim )
{
    visBase::OrthogonalSlice* slice = visBase::OrthogonalSlice::create();
    slice->ref();
    slice->setMaterial(0);
    slice->setDim(dim);
    slice->motion.notify( mCB(this,VolumeDisplay,sliceMoving) );
    slices += slice;

    slice->setName( !dim ? inlinestr : (dim==1 ? crosslinestr : timestr) );

    addChild( slice->getInventorNode() );
    const CubeSampling cs = getCubeSampling( 0 );
    const Interval<float> defintv(-0.5,0.5);
    slice->setSpaceLimits( defintv, defintv, defintv );
    slice->setVolumeDataSize( texture->getTextureSize(0),
	    		      texture->getTextureSize(1),
			      texture->getTextureSize(2) );

    return slice->id();
}


void VolumeDisplay::removeSlice( int displayid )
{
    for ( int idx=0; idx<slices.size(); idx++ )
    {
	if ( slices[idx]->id()==displayid )
	{
	    removeChild( slices[idx]->getInventorNode() );
	    slices[idx]->motion.remove( mCB(this,VolumeDisplay,sliceMoving) );
	    slices[idx]->unRef();
	    slices.removeFast(idx);
	    return;
	}
    }
}


void VolumeDisplay::showVolRen( bool yn )
{
    if ( yn && !volren )
    {
	volren = visBase::VolrenDisplay::create();
	volren->ref();
	volren->setMaterial(0);
	addChild( volren->getInventorNode() );
	volren->setName( volrenstr );
    }

    volren->turnOn( yn );
}


bool VolumeDisplay::isVolRenShown() const
{ return volren && volren->isOn(); }


int VolumeDisplay::volRenID() const
{ return volren ? volren->id() : -1; }

    
void VolumeDisplay::setCubeSampling( const CubeSampling& cs_ )
{
    const Interval<float> xintv( cs_.hrg.start.inl, cs_.hrg.stop.inl );
    const Interval<float> yintv( cs_.hrg.start.crl, cs_.hrg.stop.crl );
    const Interval<float> zintv( cs_.zrg.start, cs_.zrg.stop );
    voltrans->setTranslation( 
	    	Coord3(xintv.center(),yintv.center(),zintv.center()) );
    voltrans->setScale( Coord3(xintv.width(),yintv.width(),zintv.width()) );
    texture->setVolumeSize( Interval<float>(-0.5,0.5),
	    		    Interval<float>(-0.5,0.5),
			    Interval<float>(-0.5,0.5) );

    for ( int idx=0; idx<slices.size(); idx++ )
	slices[idx]->setSpaceLimits( Interval<float>(-0.5,0.5), 
				     Interval<float>(-0.5,0.5),
				     Interval<float>(-0.5,0.5) );

    texture->turnOn( false );

    resetManipulation();
}


float VolumeDisplay::getValue( const Coord3& pos_ ) const
{
    if ( !cache ) return mUdf(float);
    const BinIDValue bidv( SI().transform(pos_), pos_.z );
    float val;
    if ( !cache->getValue(0,bidv,&val,false) )
	return mUdf(float);

    return val;
}


void VolumeDisplay::manipMotionFinishCB( CallBacker* )
{
    if ( scene_ && scene_->getDataTransform() )
	return;

    CubeSampling cs = getCubeSampling( true, 0 );
    SI().snap( cs.hrg.start, BinID(0,0) );
    SI().snap( cs.hrg.stop, BinID(0,0) );
    float z0 = SI().zRange(true).snap( cs.zrg.start ); cs.zrg.start = z0;
    float z1 = SI().zRange(true).snap( cs.zrg.stop ); cs.zrg.stop = z1;

    Interval<int> inlrg( cs.hrg.start.inl, cs.hrg.stop.inl );
    Interval<int> crlrg( cs.hrg.start.crl, cs.hrg.stop.crl );
    Interval<float> zrg( cs.zrg.start, cs.zrg.stop );
    SI().checkInlRange( inlrg, true );
    SI().checkCrlRange( crlrg, true );
    SI().checkZRange( zrg, true );
    if ( inlrg.start == inlrg.stop ||
	 crlrg.start == crlrg.stop ||
	 mIsEqual(zrg.start,zrg.stop,1e-8) )
    {
	resetManipulation();
	return;
    }
    else
    {
	cs.hrg.start.inl = inlrg.start; cs.hrg.stop.inl = inlrg.stop;
	cs.hrg.start.crl = crlrg.start; cs.hrg.stop.crl = crlrg.stop;
	cs.zrg.start = zrg.start; cs.zrg.stop = zrg.stop;
    }

    const Coord3 newwidth( cs.hrg.stop.inl - cs.hrg.start.inl,
			   cs.hrg.stop.crl - cs.hrg.start.crl,
			   cs.zrg.stop - cs.zrg.start );
    boxdragger->setWidth( newwidth );
    const Coord3 newcenter( (cs.hrg.stop.inl + cs.hrg.start.inl) / 2,
			    (cs.hrg.stop.crl + cs.hrg.start.crl) / 2,
			    (cs.zrg.stop + cs.zrg.start) / 2 );
    boxdragger->setCenter( newcenter );
}


BufferString VolumeDisplay::getManipulationString() const
{
    BufferString str = slicename; str += ": "; str += sliceposition;
    return str;
}


void VolumeDisplay::sliceMoving( CallBacker* cb )
{
    mDynamicCastGet( visBase::OrthogonalSlice*, slice, cb );
    if ( !slice ) return;

    slicename = slice->name();
    sliceposition = slicePosition( slice );
    slicemoving.trigger();
}


float VolumeDisplay::slicePosition( visBase::OrthogonalSlice* slice ) const
{
    if ( !slice ) return 0;
    const int dim = slice->getDim();
    float slicepositionf = slice->getPosition();

    slicepositionf *= voltrans->getScale()[dim];
    slicepositionf += voltrans->getTranslation()[dim];

    float pos;    
    if ( dim == 0 )
	pos = SI().inlRange(true).snap(slicepositionf);
    else if ( dim == 1 )
	pos = SI().crlRange(true).snap(slicepositionf);
    else
	pos = mNINT(slicepositionf*1000);

    return pos;
}


void VolumeDisplay::setColorTab( visBase::VisColorTab& ctab )
{
    texture->setColorTab( ctab );
}


int VolumeDisplay::getColTabID( int attrib ) const
{
    return attrib ? -1 : getColorTab().id();
}


const visBase::VisColorTab& VolumeDisplay::getColorTab() const
{ return texture->getColorTab(); }


visBase::VisColorTab& VolumeDisplay::getColorTab()
{ return texture->getColorTab(); }


const TypeSet<float>* VolumeDisplay::getHistogram( int attrib ) const
{ return attrib ? 0 : &texture->getHistogram(); }


visSurvey::SurveyObject::AttribFormat VolumeDisplay::getAttributeFormat() const
{ return visSurvey::SurveyObject::Cube; }


const Attrib::SelSpec* VolumeDisplay::getSelSpec( int attrib ) const
{ return attrib ? 0 : &as; }


void VolumeDisplay::setSelSpec( int attrib, const Attrib::SelSpec& as_ )
{
    if ( attrib || as==as_ ) return;
    as = as_;
    if ( cache ) cache->unRef();
    cache = 0;
    texture->turnOn( false );
}


CubeSampling VolumeDisplay::getCubeSampling( int attrib ) const
{ return getCubeSampling(true,attrib); }


bool VolumeDisplay::setDataVolume( int attrib,
				   const Attrib::DataCubes* attribdata )
{
    if ( attrib || !attribdata )
	return false;

    texture->setData( &attribdata->getCube(0) );

    setCubeSampling( attribdata->cubeSampling() );

    for ( int idx=0; idx<slices.size(); idx++ )
	slices[idx]->setVolumeDataSize( texture->getTextureSize(0),
					texture->getTextureSize(1),
					texture->getTextureSize(2) );

    texture->turnOn( true );

    if ( cache ) cache->unRef();
    cache = attribdata;
    cache->ref();
    return true;
}


const Attrib::DataCubes* VolumeDisplay::getCacheVolume( int attrib ) const
{ return attrib ? 0 : cache; }


void VolumeDisplay::getMousePosInfo( const visBase::EventInfo&,
				     const Coord3& pos, BufferString& val,
				     BufferString& info ) const
{
    info = "";
    val = "undef";
    if ( !isManipulatorShown() )
	val = getValue( pos );
}


CubeSampling VolumeDisplay::getCubeSampling( bool manippos, int attrib ) const
{
    CubeSampling res;
    if ( manippos )
    {
	Coord3 center_ = boxdragger->center();
	Coord3 width_ = boxdragger->width();

	res.hrg.start = BinID( mNINT( center_.x - width_.x / 2 ),
			      mNINT( center_.y - width_.y / 2 ) );

	res.hrg.stop = BinID( mNINT( center_.x + width_.x / 2 ),
			     mNINT( center_.y + width_.y / 2 ) );

	res.hrg.step = BinID( SI().inlStep(), SI().crlStep() );

	res.zrg.start = center_.z - width_.z / 2;
	res.zrg.stop = center_.z + width_.z / 2;
    }
    else
    {
	const Coord3 transl = voltrans->getTranslation();
	const Coord3 scale = voltrans->getScale();

	res.hrg.start = BinID( mNINT(transl.x-scale.x/2),
			       mNINT(transl.y-scale.y/2) );
	res.hrg.stop = BinID( mNINT(transl.x+scale.x/2),
			       mNINT(transl.y+scale.y/2) );
	res.hrg.step = BinID( SI().inlStep(), SI().crlStep() );

	res.zrg.start = transl.z-scale.z/2;
	res.zrg.stop = transl.z+scale.z/2;
    }
    

    return res;
}


bool VolumeDisplay::allowPicks() const
{
    return !isVolRenShown();
}


visSurvey::SurveyObject* VolumeDisplay::duplicate() const
{
    VolumeDisplay* vd = create();

//  const char* ctnm = getColorTab().colorSeq().colors().name();
//  vd->getColorTab().colorSeq().loadFromStorage( ctnm );

    TypeSet<int> children;
    vd->getChildren( children );
    for ( int idx=0; idx<children.size(); idx++ )
	vd->removeSlice( children[idx] );

    for ( int idx=0; idx<slices.size(); idx++ )
    {
	const int sliceid = vd->addSlice( slices[idx]->getDim() );
	mDynamicCastGet(visBase::OrthogonalSlice*,slice,visBase::DM().getObject(sliceid))
	slice->setSliceNr( slices[idx]->getSliceNr() );
    }

    vd->setCubeSampling( getCubeSampling(false,0) );

    return vd;
}


void VolumeDisplay::fillPar( IOPar& par, TypeSet<int>& saveids) const
{
    visBase::VisualObject::fillPar( par, saveids );
    const CubeSampling cs = getCubeSampling(false,0);
    cs.fillPar( par );

    if ( volren )
    {
	int volid = volren->id();
	par.set( volumestr, volid );
	if ( saveids.indexOf( volid )==-1 ) saveids += volid;
    }

    const int textureid = texture->id();
    par.set( texturestr, textureid );
    if ( saveids.indexOf(textureid) == -1 ) saveids += textureid;

    const int nrslices = slices.size();
    par.set( nrslicesstr, nrslices );
    for ( int idx=0; idx<nrslices; idx++ )
    {
	BufferString str( slicestr ); str += idx;
	const int sliceid = slices[idx]->id();
	par.set( str, sliceid );
	if ( saveids.indexOf(sliceid) == -1 ) saveids += sliceid;
    }

    as.fillPar(par);
}


int VolumeDisplay::usePar( const IOPar& par )
{
    int res =  visBase::VisualObject::usePar( par );
    if ( res!=1 ) return res;

    CubeSampling cs;
    if ( cs.usePar(par) )
	setCubeSampling( cs );

    if ( !as.usePar(par) ) return -1;

    int textureid;
    if ( !par.get(texturestr,textureid) ) return false;

    visBase::DataObject* dataobj = visBase::DM().getObject( textureid );
    if ( !dataobj ) return 0;
    mDynamicCastGet(visBase::VolumeTexture*,vt,dataobj)
    if ( !vt ) return -1;
    if ( texture )
    {
	if ( childIndex(texture->getInventorNode()) !=-1 )
	    removeChild(texture->getInventorNode());
	texture->unRef();
    }

    texture = vt;
    texture->ref();
    insertChild( 0, texture->getInventorNode() );

    int volid;
    if ( !par.get(volumestr,volid) ) return -1;
    dataobj = visBase::DM().getObject( volid );
    if ( !dataobj ) return 0;
    mDynamicCastGet(visBase::VolrenDisplay*,vr,dataobj)
    if ( !vr ) return -1;
    if ( volren )
    {
	if ( childIndex(volren->getInventorNode())!=-1 )
	    removeChild(volren->getInventorNode());
	volren->unRef();
    }
    volren = vr;
    volren->ref();
    addChild( volren->getInventorNode() );

    while ( slices.size() )
	removeSlice( slices[0]->id() );

    int nrslices = 0;
    par.get( nrslicesstr, nrslices );
    for ( int idx=0; idx<nrslices; idx++ )
    {
	BufferString str( slicestr ); str += idx;
	int sliceid;
	par.get( str, sliceid );
	dataobj = visBase::DM().getObject( sliceid );
	if ( !dataobj ) return 0;
	mDynamicCastGet(visBase::OrthogonalSlice*,os,dataobj)
	if ( !os ) return -1;
	os->ref();
	os->motion.notify( mCB(this,VolumeDisplay,sliceMoving) );
	slices += os;
	addChild( os->getInventorNode() );
    }

    setColorTab( getColorTab() );

    return 1;
}


}; // namespace VolumeRender

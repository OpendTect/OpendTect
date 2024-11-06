/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vissurvobj.h"

#include "attribsel.h"
#include "coltabmapper.h"
#include "coltabsequence.h"
#include "iopar.h"
#include "keystrs.h"
#include "mousecursor.h"
#include "visevent.h"
#include "visobject.h"
#include "vistexturechannel2rgba.h"

namespace visSurvey {

mImplFactory( SurveyObject, SurveyObject::factory );

SurveyObject::SurveyObject()
{
    set3DSurvGeom( SI().get3DGeometry(true).ptr() );
}


SurveyObject::~SurveyObject()
{
    deepErase( userrefs_ );
}


float SurveyObject::sDefMaxDist()	{ return 10; }


SurveyObject::AttribFormat SurveyObject::getAttributeFormat( int attrib ) const
{ return SurveyObject::None; }

int SurveyObject::nrAttribs() const
{ return getAttributeFormat()==SurveyObject::None ? 0 : 1; }

bool SurveyObject::canAddAttrib(int) const
{ return canHaveMultipleAttribs(); }

bool SurveyObject::canRemoveAttrib() const
{ return canHaveMultipleAttribs() && nrAttribs()>1; }


TrcKeyZSampling SurveyObject::getTrcKeyZSampling( bool displayspace,
							    int attrib ) const
{
    TrcKeyZSampling cs( Survey::default3DGeomID() );
    return cs;
}



void SurveyObject::setColTabMapperSetup( int, const ColTab::MapperSetup&,
					 TaskRunner*)
{}


bool SurveyObject::canHandleColTabSeqTrans(int) const
{ return true; }


const ColTab::MapperSetup* SurveyObject::getColTabMapperSetup( int, int ) const
{ return nullptr; }


void SurveyObject::setColTabSequence( int, const ColTab::Sequence&, TaskRunner*)
{}


BufferString SurveyObject::getResolutionName( int res ) const
{
    if ( res == 0 ) return "Standard";
    else if ( res == 1 ) return "Higher";
    else if ( res == 2 ) return "Highest";
    else return "?";
}


void SurveyObject::setScene( Scene* sc )
{
    scene_ = sc;
}


SceneID SurveyObject::getSceneID() const
{
    const Scene* scene = getScene();
    return scene ? scene->getID() : SceneID::udf();
}


OD::Color SurveyObject::getBackgroundColor() const
{
    const Scene* scene = getScene();
    return scene ? scene->getBackgroundColor() : OD::Color::NoColor();
}


bool SurveyObject::alreadyTransformed( int attrib ) const
{
    const Attrib::SelSpec* as = getSelSpec( attrib );
    if ( !as ) return false;

    const StringView zdomainkey = as->zDomainKey();
    return scene_ && zdomainkey == scene_->zDomainKey();
}


void SurveyObject::getLineWidthBounds( int& min, int& max )
{ min = mUdf(int); max= mUdf(int); }


void SurveyObject::set3DSurvGeom( const Survey::Geometry3D* sg )
{
    s3dgeom_ = sg;
}


const char* SurveyObject::get3DSurvGeomName() const
{
    return s3dgeom_ ? s3dgeom_->getName() : survname_.str();
}


Pos::GeomID SurveyObject::getGeomID() const
{ return s3dgeom_ ? s3dgeom_->getID() : Pos::GeomID::udf(); }

void SurveyObject::annotateNextUpdateStage( bool yn )
{
    updatestagenr_ = yn ? updatestagenr_+1 : 0;
}


int SurveyObject::getUpdateStageNr() const
{ return updatestagenr_; }


void SurveyObject::fillPar( IOPar& par ) const
{
    par.set( sKey::Type(), factoryKeyword() );

    if ( s3dgeom_ )
	par.set( sKeySurvey(), s3dgeom_->getName() );

    par.setYN( sKeyLocked(), locked_ );
    const int nrattribs = nrAttribs();
    for ( int attrib=nrattribs-1; attrib>=0; attrib-- )
    {
	IOPar attribpar;
	const TypeSet<Attrib::SelSpec>* specs = getSelSpecs( attrib );
	if( !specs || specs->isEmpty() )
	    continue;

	attribpar.set( sKeyNrVersions(), specs->size() );
	for ( int vidx=0; vidx<specs->size(); vidx++ )
	{
	    IOPar verpar;
	    (*specs)[vidx].fillPar( verpar );
	    attribpar.mergeComp( verpar, IOPar::compKey(sKey::Version(),vidx) );
	}

	if ( specs->size() > 1 )
	    attribpar.set( sKeySelTexture(), selectedTexture(attrib) );

	if ( canSetColTabSequence() && getColTabSequence( attrib ) )
	{
	    IOPar seqpar;
	    const ColTab::Sequence* seq = getColTabSequence( attrib );
	    if ( seq->isSys() )
		seqpar.set( sKey::Name(), seq->name() );
	    else
		seq->fillPar( seqpar );

	    attribpar.mergeComp( seqpar, sKeySequence() );
	}

	if ( getColTabMapperSetup( attrib ) )
	{
	    IOPar mapperpar;
	    getColTabMapperSetup( attrib )->fillPar( mapperpar );
	    attribpar.mergeComp( mapperpar, sKeyMapper() );
	}

	attribpar.set( sKeyTextTrans(), getAttribTransparency( attrib ) );
	attribpar.setYN( visBase::VisualObjectImpl::sKeyIsOn(),
			 isAttribEnabled( attrib ) );

	BufferString key = sKeyAttribs();
	key += attrib;
	par.mergeComp( attribpar, key );
    }

    par.set( sKeyNrAttribs(), nrattribs );

    if ( getChannels2RGBA() )
	par.set( sKeyTC2RGBA(), getChannels2RGBA()->getClassName() );
}


bool SurveyObject::usePar( const IOPar& par )
{
    locked_ = false;
    par.getYN( sKeyLocked(), locked_ );

    par.get( sKeySurvey(), survname_ );

    BufferString tc2rgbatype;
    if ( par.get(sKeyTC2RGBA(),tc2rgbatype) )
    {
	if ( !getChannels2RGBA() ||
	     !tc2rgbatype.isEqual(getChannels2RGBA()->getClassName()) )
	{
	    mDynamicCastGet( visBase::TextureChannel2RGBA*, tc2rgba,
		    visBase::DataManager::factory().create(tc2rgbatype) )
	    if ( tc2rgba )
		setChannels2RGBA( tc2rgba );
	}
    }

    int nrattribs = 0;
    par.get( sKeyNrAttribs(), nrattribs );
    nrattribs = mMIN( nrattribs, maxNrAttribs() );

    bool firstattrib = true;
    for ( int attrib=0; attrib<nrattribs; attrib++ )
    {
	BufferString key = sKeyAttribs();
	key += attrib;
	ConstPtrMan<IOPar> attribpar = par.subselect( key );
	if ( !attribpar )
	    continue;

	if ( !firstattrib )
	    addAttrib();
	else
	    firstattrib = false;

	const int attribnr = nrAttribs()-1;

	int nrvers = 1;
	if ( !attribpar->get(sKeyNrVersions(),nrvers) )
	{
	    Attrib::SelSpec spec;
	    spec.usePar( *attribpar );
	    setSelSpec( attribnr, spec );
	}
	else
	{
	    TypeSet<Attrib::SelSpec> specs;
	    for ( int vidx=0; vidx<nrvers; vidx++ )
	    {
		PtrMan<IOPar> verpar =
		    attribpar->subselect(IOPar::compKey(sKey::Version(),vidx) );
		if ( !verpar )
		    continue;

		Attrib::SelSpec spec;
		spec.usePar( *verpar );
		specs += spec;
	    }

	    if ( !specs.isEmpty() )
		setSelSpecs( attribnr, specs );

	    int seltexture = 0;
	    if ( specs.size()>1 && attribpar->get(sKeySelTexture(),seltexture) )
		selectTexture( attrib, seltexture );
	}

	PtrMan<IOPar> seqpar = attribpar->subselect( sKeySequence() );
	ColTab::Sequence seq;
	if ( seqpar )
	{
	    if ( !seq.usePar(*seqpar) )
	    {
		BufferString seqname;
		if ( seqpar->get( sKey::Name(), seqname ) ) //Sys
		    ColTab::SM().get( seqname.buf(), seq );
	    }
	}
	else //Needed to read horizons written in od4.0
	{
	    seq.usePar( *attribpar );
	}

	setColTabSequence( attribnr, seq, 0 );

	PtrMan<IOPar> mappar = attribpar->subselect( sKeyMapper() );
	ColTab::MapperSetup mapper;
	if ( mappar )
	{
	    mapper.usePar( *mappar );
	    setColTabMapperSetup( attribnr, mapper, nullptr );
	}
	else //Needed to read horizons written in od4.0
	{
	    mapper.usePar( *attribpar );
	}

	bool ison = true;
	attribpar->getYN( visBase::VisualObjectImpl::sKeyIsOn(), ison );
	enableAttrib( attribnr, ison );

	unsigned int trans = 0;
	attribpar->get( sKeyTextTrans(), trans );
	setAttribTransparency( attribnr, mCast(unsigned char,trans) );
    }

    return true;
}


const visBase::TextureChannel2RGBA* SurveyObject::getChannels2RGBA() const
{
    return const_cast<SurveyObject*>(this)->getChannels2RGBA();
}


void SurveyObject::getChannelName( int idx, uiString& res ) const
{
    const visBase::TextureChannel2RGBA* tc2rgba = getChannels2RGBA();
    if ( !tc2rgba )
	return;

    tc2rgba->getChannelName( idx,  res );
}


bool SurveyObject::isAnyAttribEnabled() const
{
    for ( int idx=0; idx<nrAttribs(); idx++ )
    {
	if ( isAttribEnabled(idx) )
	    return true;
    }

    return false;
}


void SurveyObject::initAdaptiveMouseCursor( CallBacker* eventcb,
					    const VisID& objid,
					    int inplanedragkeys,
					    MouseCursor& mousecursor )
{
    mousecursor.shape_ = MouseCursor::NotSet;

    if ( eventcb )
    {
	mCBCapsuleUnpack( const visBase::EventInfo&, eventinfo, eventcb );
	if ( eventinfo.pickedobjids.isPresent(objid) )
	{
	    unsigned int buttonstate = (unsigned int) eventinfo.buttonstate_;
	    buttonstate &= OD::ShiftButton | OD::ControlButton | OD::AltButton;

	    if ( eventinfo.type==visBase::Keyboard && !eventinfo.pressed )
		buttonstate -=
		       eventinfo.key_==OD::KB_Shift	? OD::ShiftButton :
		      (eventinfo.key_==OD::KB_Control	? OD::ControlButton :
		      (eventinfo.key_==OD::KB_Alt	? OD::AltButton
							: 0));

	    if ( buttonstate == inplanedragkeys )
		mousecursor.shape_ = MouseCursor::SizeAll;
	    else
		mousecursor.shape_ = MouseCursor::GreenArrow;
	}
    }
}


void SurveyObject::getMousePosInfo( const visBase::EventInfo& info,
				    IOPar& iopar ) const
{
    const Coord3 xytmousepos = info.worldpickedpos;
    if ( xytmousepos.isUdf() )
	return;

    const MultiID mid = getMultiID();
    const Survey::Geometry& geom =  Survey::Geometry::default3D();

    const Coord pos( xytmousepos.x_, xytmousepos.y_ );
    const TrcKey trck = geom.nearestTrace( pos );
    if ( !trck.isUdf() )
	iopar.set( sKey::TraceKey(), trck );
}


bool SurveyObject::setDataPackID( int /* attrib */, const DataPackID&,
				  TaskRunner* )
{
    pErrMsg( "Should not be called" );
    return false;
}


DataPackID SurveyObject::getDataPackID( int attrib ) const
{
    pErrMsg( "Should not be called" );
    ConstRefMan<DataPack> dp = getDataPack( attrib );
    return dp ? dp->id() : DataPackID::udf();
}


DataPackID SurveyObject::getDisplayedDataPackID( int attrib ) const
{
    pErrMsg( "Should not be called" );
    ConstRefMan<DataPack> dp = getDisplayedDataPack( attrib );
    return dp ? dp->id() : DataPackID::udf();
}


ConstRefMan<DataPack> SurveyObject::getDisplayedDataPack( int attrib ) const
{
    return getDataPack( attrib );
}


ConstRefMan<SeisDataPack> SurveyObject::getDisplayedSeisDataPack(
							int attrib ) const
{
    return getSeisDataPack( attrib );
}


const Attrib::SelSpec* SurveyObject::getSelSpec( int attrib, int version ) const
{
    const TypeSet<Attrib::SelSpec>* specs = getSelSpecs( attrib );
    return specs && specs->validIdx(version) ? &(*specs)[version] : nullptr;
}


bool SurveyObject::hasSelSpec( const Attrib::SelSpec& as,
				int& attrib, int& version ) const
{
    for ( int attridx=0; attridx<nrAttribs(); attridx++ )
    {
	const TypeSet<Attrib::SelSpec>* attribas = getSelSpecs( attridx );
	if ( !attribas )
	    continue;

	const int index = attribas->indexOf( as );
	if ( index < 0 )
	    continue;

	attrib = attridx;
	version = index;
	return true;
    }

    return false;
}


void SurveyObject::setSelSpec( int attrib, const Attrib::SelSpec& newselspec )
{
    setSelSpecs( attrib, TypeSet<Attrib::SelSpec>(1,newselspec) );
}


void SurveyObject::setSelSpecs(
	int attrib, const TypeSet<Attrib::SelSpec>& newselspecs )
{
    const bool hasnewspecs = !newselspecs.isEmpty();
    const Attrib::SelSpec* oldselspec = getSelSpec( attrib, 0 );
    if ( !oldselspec || !hasnewspecs || (*oldselspec)!=newselspecs[0] )
	setColTabMapperSetup( attrib, ColTab::MapperSetup(), 0 );
}


bool SurveyObject::usesTexture() const
{
    return usestexture_;
}


bool SurveyObject::showsTexture() const
{
    return canShowTexture() && usesTexture();
}

} // namespace visSurvey

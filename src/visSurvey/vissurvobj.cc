/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Apr 2002
-*/


#include "vissurvobj.h"

#include "attribsel.h"
#include "mousecursor.h"
#include "survinfo.h"
#include "coltabmapper.h"
#include "coltabseqmgr.h"
#include "visevent.h"
#include "vistexturechannel2rgba.h"
#include "visobject.h"
#include "iopar.h"
#include "keystrs.h"
#include "seisbuf.h"
#include "uistrings.h"

namespace visSurvey {

mImplClassFactory( SurveyObject, factory );

SurveyObject::SurveyObject()
    : scene_(0)
    , s3dgeom_( 0 )
    , locked_( false )
    , updatestagenr_( 0 )
    , saveinsessionsflag_( true )
{
    set3DSurvGeom( SI().get3DGeometry(OD::UsrWork) );
}


SurveyObject::~SurveyObject()
{
    deepErase(userrefs_);
    set3DSurvGeom( 0 );
}


void SurveyObject::doRef()
{
    mDynamicCastGet( visBase::DataObject*, dobj, this );
    dobj->ref();
}


void SurveyObject::doUnRef()
{
    mDynamicCastGet( visBase::DataObject*, dobj, this );
    dobj->unRef();
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


void SurveyObject::setColTabMapper( int, const ColTab::Mapper&,
					 TaskRunner*)
{}


const ColTab::Sequence& SurveyObject::getColTabSequence( int ) const
{ return *ColTab::SeqMGR().getDefault(); }


const ColTab::Mapper& SurveyObject::getColTabMapper( int ) const
{ return *new ColTab::Mapper; }


bool SurveyObject::canHandleColTabSeqTrans(int) const
{ return true; }


void SurveyObject::setColTabSequence( int, const ColTab::Sequence&, TaskRunner*)
{}


uiWord SurveyObject::getResolutionName( int res ) const
{
    if ( res == 1 )
	return uiStrings::sHigher();
    if ( res == 2 )
	return uiStrings::sHighest();

    if ( res != 0 )
	{ pErrMsg("Resolution out of range" ); }

    return uiStrings::sStandard();
}


void SurveyObject::setScene( Scene* sc )
{ scene_ = sc; }


bool SurveyObject::alreadyTransformed( int attrib ) const
{
    const Attrib::SelSpec* as = getSelSpec( attrib );
    if ( !as ) return false;

    const FixedString zdomainkey = as->zDomainKey();
    return scene_ && zdomainkey == scene_->zDomainKey();
}


void SurveyObject::getLineWidthBounds( int& min, int& max )
{ min = mUdf(int); max= mUdf(int); }


void SurveyObject::set3DSurvGeom( const SurvGeom3D* sg )
{
    if ( s3dgeom_ )
	s3dgeom_->unRef();
    s3dgeom_ = sg;
    if ( s3dgeom_ )
	s3dgeom_->ref();
}


const char* SurveyObject::get3DSurvGeomName() const
{
    return s3dgeom_ ? s3dgeom_->name() : survname_.str();
}


Pos::GeomID SurveyObject::getGeomID() const
{
    return s3dgeom_ ? s3dgeom_->geomID() : mUdfGeomID;
}

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
	par.set( sKeySurvey(), s3dgeom_->name() );

    par.setYN( sKeyLocked(), locked_ );
    const int nrattribs = nrAttribs();
    for ( int attrib=nrattribs-1; attrib>=0; attrib-- )
    {
	IOPar attribpar;
	if( !getSelSpec( attrib ) )
	    continue;

	getSelSpec( attrib )->fillPar( attribpar );

	if ( canSetColTabSequence() )
	{
	    IOPar seqpar;
	    const ColTab::Sequence& seq = getColTabSequence( attrib );
	    if ( seq.isSys() )
		seqpar.set( sKey::Name(), seq.name() );
	    else
		seq.fillPar( seqpar );

	    attribpar.mergeComp( seqpar, sKeySequence() );
	}

	IOPar mapperpar;
	getColTabMapper( attrib ).setup().fillPar( mapperpar );
	attribpar.mergeComp( mapperpar, sKeyMapper() );

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
	     FixedString(tc2rgbatype) != getChannels2RGBA()->getClassName() )
	{
	    mDynamicCastGet( visBase::TextureChannel2RGBA*, tc2rgba,
		    visBase::DataManager::factory().create(tc2rgbatype) )
	    if ( tc2rgba )
		setChannels2RGBA( tc2rgba );
	}
    }

    int nrattribs = 0;
    par.get( sKeyNrAttribs(), nrattribs );

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

	Attrib::SelSpec spec;
	spec.usePar( *attribpar );
	setSelSpec( attribnr, spec );

	PtrMan<IOPar> seqpar = attribpar->subselect( sKeySequence() );
	ConstRefMan<ColTab::Sequence> seq = ColTab::SeqMGR().getDefault();
	if ( seqpar )
	    seq = ColTab::SeqMGR().getFromPar( *seqpar );
	setColTabSequence( attribnr, *seq, 0 );

	PtrMan<IOPar> mappar = attribpar->subselect( sKeyMapper() );
	RefMan<ColTab::MapperSetup> mappersetup = new ColTab::MapperSetup;
	if ( mappar )
	    mappersetup->usePar( *mappar );
	else // horizons written in od4.0
	    mappersetup->usePar( *attribpar );
	ColTab::Mapper& mpr = const_cast<ColTab::Mapper&>(
				    getColTabMapper(attribnr) );
	mpr.setup() = *mappersetup;

	bool ison = true;
	attribpar->getYN( visBase::VisualObjectImpl::sKeyIsOn(), ison );
	enableAttrib( attribnr, ison );

	unsigned int trans = 0;
	attribpar->get( sKeyTextTrans(), trans );
	setAttribTransparency( attribnr, mCast(unsigned char,trans) );
    }

    return true;
}


const visBase::TextureChannel2RGBA*
	visSurvey::SurveyObject::getChannels2RGBA() const
{
    return const_cast<visSurvey::SurveyObject*>(this)->getChannels2RGBA();
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
					    int objid, int inplanedragkeys,
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

    const auto& geom = SurvGeom::get3D();
    const TrcKey tk( geom.nearestTracePosition(xytmousepos.getXY()) );
    if ( !tk.isUdf() )
	iopar.set( sKey::TraceKey(), tk );
}


void SurveyObject::setSelSpec( int attrib, const Attrib::SelSpec& newselspec )
{
    setSelSpecs( attrib, Attrib::SelSpecList(1,newselspec) );
}


void SurveyObject::setSelSpecs(
	int attrib, const Attrib::SelSpecList& newselspecs )
{
    const Attrib::SelSpec* oldselspec = getSelSpec( attrib );
    if ( !oldselspec || (*oldselspec)!=newselspecs[0] )
    {
	RefMan<ColTab::Mapper> mapper = new ColTab::Mapper;
	setColTabMapper( attrib, *mapper, 0 );
    }
}


} // namespace visSurvey

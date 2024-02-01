/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "emioobjinfo.h"

#include "embodytr.h"
#include "emfaultauxdata.h"
#include "emmanager.h"
#include "emmarchingcubessurface.h"
#include "empolygonbody.h"
#include "emrandomposbody.h"
#include "emsurfaceio.h"
#include "emsurfacetr.h"
#include "file.h"
#include "horizonrelation.h"
#include "iodir.h"
#include "ioman.h"
#include "iopar.h"
#include "keystrs.h"
#include "unitofmeasure.h"

namespace EM
{


IOObjInfo::IOObjInfo( const IOObj* ioobj )
    : ioobj_(ioobj ? ioobj->clone() : nullptr)
{
    init();
}


IOObjInfo::IOObjInfo( const IOObj& ioobj )
    : ioobj_(ioobj.clone())
{
    init();
}


IOObjInfo::IOObjInfo( const MultiID& id )
    : ioobj_(IOM().get(id))
{
    init();
}


IOObjInfo::IOObjInfo( const IOObjInfo& sii )
    : type_(sii.type_)
{
    ioobj_ = sii.ioobj_ ? sii.ioobj_->clone() : nullptr;
    fillZDomain();
}


void IOObjInfo::init()
{
    setType();
    fillZDomain();
}


void IOObjInfo::fillZDomain()
{
    if ( !ioobj_ )
    {
	zinfo_ = new ZDomain::Info( SI().zDomainInfo() );
	return;
    }

    zinfo_ = new ZDomain::Info( ioobj_->pars() );
}


void IOObjInfo::setType()
{
    type_ = ioobj_ ? objectTypeOfIOObjGroup( ioobj_->group() )
		   : EM::ObjectType::Unknown;
}


IOObjInfo::~IOObjInfo()
{
    delete ioobj_;
    delete reader_;
}


IOObjInfo& IOObjInfo::operator =( const IOObjInfo& sii )
{
    if ( &sii != this )
    {
	delete ioobj_;
	ioobj_ = sii.ioobj_ ? sii.ioobj_->clone() : nullptr;
	type_ = sii.type_;
	const ZDomain::Info& zinfo = sii.zDomain();
	if ( !zDomain().isCompatibleWith(zinfo) )
	{
	    delete zinfo_;
	    zinfo_ = new ZDomain::Info( zinfo );
	}
    }

    return *this;
}


bool IOObjInfo::isOK() const
{
    return ioobj_ && ioobj_->implExists(true);
}


const char* IOObjInfo::name() const
{ return ioobj_ ? ioobj_->name().str() : nullptr; }



#define mGetReader \
if ( !reader_ && ioobj_ ) \
    reader_ = new dgbSurfaceReader( *ioobj_, ioobj_->group() );

#define mGetReaderRet \
mGetReader \
if ( !reader_ ) return false;

bool IOObjInfo::getSectionIDs( TypeSet<SectionID>& secids ) const
{
    secids += EM::SectionID::def();
    return true;
}


bool IOObjInfo::getSectionNames( BufferStringSet& secnames ) const
{
    secnames.add( "[0]" );
    return true;
}


bool IOObjInfo::getAttribNames( BufferStringSet& attrnames ) const
{
    if ( !ioobj_ )
	return false;

    if ( type_== EM::ObjectType::Flt3D )
    {
	FaultAuxData fad( ioobj_->key() );
	fad.getAuxDataList( attrnames );
	return true;
    }
    else
    {
	PtrMan<Translator> trans = ioobj_->createTranslator();
	mDynamicCastGet(EMSurfaceTranslator*,str,trans.ptr());
	if ( !str || !str->startRead(*ioobj_) )
	    return false;

	const SurfaceIOData& newsd = str->selections().sd;
	attrnames.add( newsd.valnames, true );
    }

    return true;
}


Interval<float> IOObjInfo::getZRange() const
{
    if ( !ioobj_ )
	return Interval<float>::udf();

    PtrMan<Translator> trans = ioobj_->createTranslator();
    mDynamicCastGet(EMSurfaceTranslator*,str,trans.ptr());
    if ( !str || !str->startRead(*ioobj_) )
	return Interval<float>::udf();

    const SurfaceIOData& newsd = str->selections().sd;
    return newsd.zrg;
}


const ZDomain::Info& IOObjInfo::zDomain() const
{
    return *zinfo_;
}


const UnitOfMeasure* IOObjInfo::getZUoM() const
{
    return UnitOfMeasure::zUnit( zDomain() );
}


BufferString IOObjInfo::getZUnitLabel() const
{
    return zDomain().unitStr();
}


StepInterval<int> IOObjInfo::getInlRange() const
{
    if ( !ioobj_ )
	return StepInterval<int>::udf();

    PtrMan<Translator> trans = ioobj_->createTranslator();
    mDynamicCastGet(EMSurfaceTranslator*,str,trans.ptr());
    if ( !str || !str->startRead(*ioobj_) )
	return StepInterval<int>::udf();

    const SurfaceIOData& newsd = str->selections().sd;
    return newsd.rg.inlRange();
}


StepInterval<int> IOObjInfo::getCrlRange() const
{
    if ( !ioobj_ )
	return StepInterval<int>::udf();

    PtrMan<Translator> trans = ioobj_->createTranslator();
    mDynamicCastGet(EMSurfaceTranslator*,str,trans.ptr());
    if ( !str || !str->startRead(*ioobj_) )
	return StepInterval<int>::udf();

    const SurfaceIOData& newsd = str->selections().sd;
    return newsd.rg.crlRange();
}


IOPar* IOObjInfo::getPars() const
{
    mGetReader
    return reader_ && reader_->pars() ? new IOPar(*reader_->pars()) : nullptr;
}


uiString IOObjInfo::getMessage() const
{
    mGetReader;
    return reader_ ? reader_->uiMessage() : uiString::emptyString();
}


const char* IOObjInfo::timeLastModified() const
{ return timeLastModified( false ); }


const char* IOObjInfo::timeLastModified( bool iso ) const
{
    if ( !ioobj_ )
	return nullptr;

    const char* fnm = ioobj_->fullUserExpr();
    return File::timeLastModified( fnm, iso ? 0 : Time::defDateTimeFmt() );
}


bool IOObjInfo::getLineNames( BufferStringSet& linenames ) const
{
    if ( !ioobj_ )
	return false;

    PtrMan<Translator> trans = ioobj_->createTranslator();
    mDynamicCastGet(EMSurfaceTranslator*,str,trans.ptr());
    if ( !str || !str->startRead(*ioobj_) )
	return false;

    const SurfaceIOData& newsd = str->selections().sd;
    linenames = newsd.linenames;
    return true;
}


bool IOObjInfo::getGeomIDs( TypeSet<Pos::GeomID>& geomids ) const
{
    if ( !ioobj_ )
	return false;

    PtrMan<Translator> trans = ioobj_->createTranslator();
    mDynamicCastGet(EMSurfaceTranslator*,str,trans.ptr());
    if ( !str || !str->startRead(*ioobj_) )
	return false;

    const SurfaceIOData& newsd = str->selections().sd;
    geomids = newsd.geomids;
    return true;
}


bool IOObjInfo::getTrcRanges( TypeSet< StepInterval<int> >& trcranges ) const
{
    if ( !ioobj_ )
	return false;

    PtrMan<Translator> trans = ioobj_->createTranslator();
    mDynamicCastGet(EMSurfaceTranslator*,str,trans.ptr());
    if ( !str || !str->startRead(*ioobj_) )
	return false;

    const SurfaceIOData& newsd = str->selections().sd;
    trcranges = newsd.trcranges;
    return true;
}


bool IOObjInfo::hasGeomIDs() const
{
    mGetReaderRet
    const IOPar* iop = reader_->pars();
    if ( !iop )
	return false;

    return reader_->pars()->hasKey(sKey::GeomID());
}


int IOObjInfo::getParsOffsetInFile() const
{
    mGetReaderRet
    return reader_->getParsOffset();
}



#define mErrRet(s) { errmsg = s; return false; }
bool IOObjInfo::getSurfaceData( SurfaceIOData& sd, uiString& errmsg ) const
{
    if ( !ioobj_ )
	mErrRet( tr("Cannot find surface in object database") )

    if ( !ioobj_->implExists(true) )
	mErrRet( tr("Cannot find file on disk") )

    if ( !isSurface() )
    {
	pErrMsg("getSurfaceData called on non-surface");
	mErrRet(
	::toUiString("Internal: Trying to get surface data from a non-surface"))
    }

    Translator* trans = ioobj_->createTranslator();
    mDynamicCastGet(EMSurfaceTranslator*,str,trans);
    PtrMan<EMSurfaceTranslator> ptrman_tr = str;
    if ( !str )
    {
	if ( !trans )
	    { pErrMsg("No Translator for IOObj" ); }
	else
	    { pErrMsg("Created Translator is not a EMSurfaceTranslator"); }
	mErrRet(
	    ::toUiString("Internal: Unknown Surface interpreter encountered"))
    }

    if ( !str->startRead(*ioobj_) )
    {
	uiString msg( str->errMsg() );
	if ( msg.isEmpty() )
	    msg = tr( "Cannot read '%1'").arg( ioobj_->name() );

	mErrRet( msg )
    }

    const SurfaceIOData& newsd = str->selections().sd;
    sd.rg = newsd.rg; sd.zrg = newsd.zrg;
    sd.sections = newsd.sections;
    sd.valnames = newsd.valnames;
    sd.valshifts_ = newsd.valshifts_;
    sd.linenames = newsd.linenames;
    sd.linesets = newsd.linesets;
    sd.trcranges = newsd.trcranges;

    return true;
}


EM::ObjectType IOObjInfo::objectTypeOfIOObjGroup( const char* grpname )
{
    EM::ObjectType type = EM::ObjectType::Unknown;
    parseEnum( grpname, type );
    return type;
}


void IOObjInfo::getIDs( EM::ObjectType reqtyp, TypeSet<MultiID>& ids )
{
    const MultiID mid ( IOObjContext::getStdDirData(IOObjContext::Surf)->id_ );
    const IODir iodir( mid );
    for ( int idx=0; idx<iodir.size(); idx++ )
    {
	const IOObj* ioobj = iodir.get( idx );
	if ( objectTypeOfIOObjGroup(ioobj->group()) == reqtyp )
	    ids += ioobj->key();
    }
}


Strat::LevelID IOObjInfo::levelID() const
{
    mGetReader
    return reader_ ? reader_->stratLevelID() : Strat::LevelID::udf();
}


void IOObjInfo::getTiedToLevelID( Strat::LevelID lvlid, TypeSet<MultiID>& ids,
				  bool is2d )
{
    ids.erase();
    TypeSet<MultiID> candidates;
    if ( is2d )
	getIDs( EM::ObjectType::Hor2D, candidates );
    else
	getIDs( EM::ObjectType::Hor3D, candidates );

    for ( int idx=0; idx<candidates.size(); idx++ )
    {
	const IOObjInfo ioobjinfo( candidates[idx] );
	if ( ioobjinfo.levelID() == lvlid )
	    ids += candidates[idx];
    }
}


bool IOObjInfo::sortHorizonsOnZValues( const TypeSet<MultiID>& list,
				       TypeSet<MultiID>& sorted )
{
    sorted.erase();
    if ( list.isEmpty() )
	return true;

    IOObjInfo info( list[0] );
    return RelationTree::sortHorizons( EM::is2DHorizon(info.type()),
							    list, sorted );
}


bool IOObjInfo::getBodyRange( TrcKeyZSampling& cs ) const
{
    if ( type_ != EM::ObjectType::Body )
	return false;

    RefMan<EMObject> emobj = EMM().loadIfNotFullyLoaded( ioobj_->key() );
    if ( !emobj )
	return false;

    mDynamicCastGet(MarchingCubesSurface*,emmcbody,emobj.ptr());
    if ( emmcbody )
	return emmcbody->getBodyRange( cs );

    mDynamicCastGet(PolygonBody*,empolybody,emobj.ptr());
    if ( empolybody )
	return empolybody->getBodyRange( cs );

    mDynamicCastGet(RandomPosBody*,emrrbody,emobj.ptr());
    if ( emrrbody )
	return emrrbody->getBodyRange( cs );

    return false;
}


int IOObjInfo::nrSticks() const
{
    if ( !ioobj_ )
	return 0;

    PtrMan<Translator> trans = ioobj_->createTranslator();
    mDynamicCastGet(EMSurfaceTranslator*,emtr,trans.ptr())
    mDynamicCastGet(dgbEMFaultStickSetTranslator*,fsstr,emtr);
    if ( emtr && !fsstr )
    {
	if ( !emtr->startRead(*ioobj_) )
	    return -1;

	const SurfaceIOData& newsd = emtr->selections().sd;
	return newsd.nrfltsticks_;
    }

    mGetReaderRet
    if ( !reader_->pars() )
	return 0;

    Interval<int> rowrange = Interval<int>::udf();
    reader_->pars()->get( "Row range", rowrange );
    return rowrange.isUdf() ? 0 : rowrange.width()+1;
}


void IOObjInfo::getHorizonNamesForLine( const Pos::GeomID& geomid,
					BufferStringSet& nms )
{
    nms.setEmpty();
    TypeSet<MultiID> keys;
    getHorizonIDsForLine( geomid, keys );
    for ( const auto& key : keys )
	nms.add( IOM().nameOf(key) );
}


void IOObjInfo::getHorizonIDsForLine( const Pos::GeomID& geomid,
				      TypeSet<MultiID>& keys )
{
    TypeSet<MultiID> allhorkeys;
    getIDs( EM::ObjectType::Hor2D, allhorkeys );

    for ( const auto& key : allhorkeys )
    {
	const EM::IOObjInfo emobjinfo( key );
	TypeSet<Pos::GeomID> geomids;
	if ( emobjinfo.getGeomIDs(geomids) && geomids.isPresent(geomid) )
	    keys += key;
    }
}

} // namespace EM

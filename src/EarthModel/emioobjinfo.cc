/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Satyaki Maitra
 * DATE     : April 2010
-*/


#include "emioobjinfo.h"

#include "dbdir.h"
#include "dbman.h"
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
#include "iopar.h"
#include "keystrs.h"
#include "uistrings.h"

mDefineEnumUtils( EM::IOObjInfo, ObjectType, "Object Type" )
{
  "Unknown",
  EMHorizon3DTranslatorGroup::sGroupName(),
  EMHorizon2DTranslatorGroup::sGroupName(),
  EMFaultStickSetTranslatorGroup::sGroupName(),
  EMFault3DTranslatorGroup::sGroupName(),
  EMBodyTranslatorGroup::sGroupName(),
  0
};

template<>
void EnumDefImpl<EM::IOObjInfo::ObjectType>::init()
{
    uistrings_ += uiStrings::sUnknown();
    uistrings_ += uiStrings::s3DHorizon();
    uistrings_ += uiStrings::s2DHorizon();
    uistrings_ += uiStrings::sFaultStickSet();
    uistrings_ += uiStrings::sFault();
    uistrings_ += uiStrings::sBody();
}

namespace EM
{

IOObjInfo::IOObjInfo( const IOObj* ioobj )
    : ioobj_(ioobj ? ioobj->clone() : 0)
    , reader_(0)
{
    setType();
}


IOObjInfo::IOObjInfo( const IOObj& ioobj )
    : ioobj_(ioobj.clone())
    , reader_(0)
{
    setType();
}


IOObjInfo::IOObjInfo( const DBKey& id )
    : ioobj_(id.getIOObj())
    , reader_(0)
{
    setType();
}


IOObjInfo::IOObjInfo( const IOObjInfo& sii )
    : type_(sii.type_)
    , reader_(0)
{
    ioobj_ = sii.ioobj_ ? sii.ioobj_->clone() : 0;
}


void IOObjInfo::setType()
{
    type_ = ioobj_ ? objectTypeOfIOObjGroup( ioobj_->group() ) : Unknown;
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
	ioobj_ = sii.ioobj_ ? sii.ioobj_->clone() : 0;
	type_ = sii.type_;
    }
    return *this;
}


bool IOObjInfo::isOK() const
{
    return ioobj_ && ioobj_->implExists(true);
}


const char* IOObjInfo::name() const
{ return ioobj_ ? ioobj_->name().str() : 0; }



#define mGetReader \
if ( !reader_ && ioobj_ ) \
    reader_ = new dgbSurfaceReader( *ioobj_, ioobj_->group() );

#define mGetReaderRet \
mGetReader \
if ( !reader_ ) return false;

bool IOObjInfo::getSectionIDs( TypeSet<SectionID>& secids ) const
{
    mGetReaderRet

    for ( int idx=0; idx<reader_->nrSections(); idx++ )
	secids += reader_->sectionID(idx);
    return true;
}


bool IOObjInfo::getAttribNames( BufferStringSet& attrnames ) const
{
    if ( !ioobj_ )
	return false;

    if ( type_==Fault )
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
	for ( auto nmbs : newsd.valnames )
	    if ( !nmbs->isEmpty() )
		attrnames.addIfNew( nmbs->str() );
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
    return reader_ && reader_->pars() ? new IOPar(*reader_->pars()) : 0;
}


uiString IOObjInfo::getMessage() const
{
    mGetReader;
    return reader_ ? reader_->message() : uiString::empty();
}


const char* IOObjInfo::timeLastModified() const
{
    if ( !ioobj_ ) return 0;

    const BufferString fnm = ioobj_->mainFileName();
    return File::timeLastModified( fnm );
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


bool IOObjInfo::getGeomIDs( GeomIDSet& geomids ) const
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
	    msg = ioobj_->phrCannotOpenObj();
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


IOObjInfo::ObjectType IOObjInfo::objectTypeOfIOObjGroup( const char* grpname )
{
    return ObjectTypeDef().parse( grpname );
}


void IOObjInfo::getIDs( IOObjInfo::ObjectType reqtyp, DBKeySet& ids )
{
    ConstRefMan<DBDir> dbdir = DBM().fetchDir( IOObjContext::Surf );
    if ( !dbdir )
	return;

    DBDirIter iter( *dbdir );
    while ( iter.next() )
    {
	const IOObj& ioobj = iter.ioObj();
	if ( objectTypeOfIOObjGroup(ioobj.group()) == reqtyp )
	    ids += ioobj.key();
    }
}


IOObjInfo::LevelID IOObjInfo::levelID() const
{
    mGetReader
    return reader_ ? reader_->stratLevelID() : LevelID::getInvalid();
}


void IOObjInfo::getTiedToLevelID( LevelID lvlid, DBKeySet& ids, bool is2d )
{
    ids.erase();
    DBKeySet candidates;
    if ( is2d )
	getIDs( Horizon2D, candidates );
    else
	getIDs( Horizon3D, candidates );

    for ( int idx=0; idx<candidates.size(); idx++ )
    {
	IOObjInfo ioobjinfo( candidates[idx] );
	if ( ioobjinfo.levelID() == lvlid )
	    ids += candidates[idx];
    }
}


bool IOObjInfo::sortHorizonsOnZValues( const DBKeySet& list, DBKeySet& sorted )
{
    sorted.erase();
    if ( list.isEmpty() )
	return true;

    IOObjInfo info( list[0] );
    return RelationTree::sortHorizons( info.is2DHorizon(), list, sorted );
}


bool IOObjInfo::getBodyRange( TrcKeyZSampling& cs ) const
{
    if ( type_ != IOObjInfo::Body )
	return false;

    SilentTaskRunnerProvider trprov;
    RefMan<Object> emobj =
		BodyMan().loadIfNotFullyLoaded( ioobj_->key(), trprov );
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
	return false;

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

} // namespace EM

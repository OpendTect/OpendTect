/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Satyaki Maitra
 * DATE     : April 2010
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "emioobjinfo.h"

#include "embodytr.h"
#include "emfaultauxdata.h"
#include "emsurfaceio.h"
#include "emsurfacetr.h"
#include "horizonrelation.h"
#include "iodir.h"
#include "ioman.h"
#include "iopar.h"
#include "keystrs.h"


namespace EM
{

DefineEnumNames( IOObjInfo, ObjectType, 0, "Object Type" )
{ 
  "Unknown",
  EMHorizon3DTranslatorGroup::keyword(),
  EMHorizon2DTranslatorGroup::keyword(),
  EMFaultStickSetTranslatorGroup::keyword(),
  EMFault3DTranslatorGroup::keyword(),
  EMBodyTranslatorGroup::sKeyword(),
  0 
};


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


IOObjInfo::IOObjInfo( const MultiID& id )
    : ioobj_(IOM().get(id))
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
{ return ioobj_ ? ioobj_->name() : 0; }



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


bool IOObjInfo::getSectionNames( BufferStringSet& secnames ) const
{
    mGetReaderRet

    for ( int idx=0; idx<reader_->nrSections(); idx++ )
	secnames.add( reader_->sectionName(idx) );
    return true;
}


bool IOObjInfo::getAttribNames( BufferStringSet& attrnames ) const
{
    if ( type_==Fault )
    {
	if ( !ioobj_ ) 
	    return false;

	FaultAuxData fad( ioobj_->key() );
	fad.getAuxDataList( attrnames );
	return true;
    }
    else
    {
     	mGetReaderRet;
    	for ( int idx=0; idx<reader_->nrAuxVals(); idx++ )
    	    attrnames.add( reader_->auxDataName(idx) );
    }

    return true;
}


Interval<float> IOObjInfo::getZRange() const
{
    mGetReader
    return reader_ ? reader_->zInterval()
		   : Interval<float>(mUdf(float),mUdf(float));
}


StepInterval<int> IOObjInfo::getInlRange() const
{
    mGetReader
    if ( !reader_ )
	return Interval<int>(mUdf(int),mUdf(int));
    return reader_->rowInterval();
}


StepInterval<int> IOObjInfo::getCrlRange() const
{
    mGetReader
    return reader_ ? reader_->colInterval()
		   : StepInterval<int>(mUdf(int),mUdf(int),mUdf(int));
}


IOPar* IOObjInfo::getPars() const
{
    mGetReader
    return reader_ && reader_->pars() ? new IOPar(*reader_->pars()) : 0;
}


uiString IOObjInfo::getMessage() const
{
    mGetReader;
    return reader_ ? reader_->uiMessage() : uiString::emptyString();
}


bool IOObjInfo::getLineNames( BufferStringSet& linenames ) const
{
    mGetReaderRet

    for ( int idx=0; idx<reader_->nrLines(); idx++ )
	linenames.add( reader_->lineName(idx) );
    return true;
}


bool IOObjInfo::getGeomIDs( TypeSet<Pos::GeomID>& geomids ) const
{
    mGetReaderRet

    for ( int idx=0; idx<reader_->nrLines(); idx++ )
	geomids.add( reader_->lineGeomID(idx) );

    return true;
}


bool IOObjInfo::getTrcRanges( TypeSet< StepInterval<int> >& trcranges ) const
{
    mGetReaderRet

    for ( int idx=0; idx<reader_->nrLines(); idx++ )
	trcranges.add( reader_->lineTrcRanges(idx) );
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
	mErrRet( "Cannot find surface in object database" )

    if ( !ioobj_->implExists(true) )
	mErrRet( "Cannot find file on disk" )

    if ( !isSurface() )
    {
	pErrMsg("getSurfaceData called on non-surface");
	mErrRet( "Internal: Trying to get surface data from a non-surface" )
    }

    Translator* trans = ioobj_->createTranslator();
    mDynamicCastGet(EMSurfaceTranslator*,str,trans);
    PtrMan<EMSurfaceTranslator> ptrman_tr = str;
    if ( !str )
    {
	if ( !tr )
	    { pErrMsg("No Translator for IOObj" ); }
	else
	    { pErrMsg("Created Translator is not a EMSurfaceTranslator"); }
	mErrRet( "Internal: Unknown Surface interpreter encountered" )
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


IOObjInfo::ObjectType IOObjInfo::objectTypeOfIOObjGroup( const char* grpname )
{
    ObjectType type;
    parseEnum( grpname, type );
    return type;
}


void IOObjInfo::getIDs( IOObjInfo::ObjectType reqtyp, TypeSet<MultiID>& ids )
{
    const MultiID mid ( IOObjContext::getStdDirData(IOObjContext::Surf)->id );
    const IODir iodir( mid );
    for ( int idx=0; idx<iodir.size(); idx++ )
    {
	const IOObj* ioobj = iodir.get( idx );
	if ( objectTypeOfIOObjGroup(ioobj->group()) == reqtyp )
	    ids += ioobj->key();
    }
}


int IOObjInfo::levelID() const
{
    mGetReader
    return reader_ ? reader_->stratLevelID() : -1;
}


void IOObjInfo::getTiedToLevelID( int lvlid, TypeSet<MultiID>& ids, bool is2d )
{
    ids.erase();
    TypeSet<MultiID> candidates;
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


bool IOObjInfo::sortHorizonsOnZValues( const TypeSet<MultiID>& list,
				       TypeSet<MultiID>& sorted )
{
    sorted.erase();
    if ( list.isEmpty() )
	return true;

    IOObjInfo info( list[0] );
    EM::RelationTree reltree( info.is2DHorizon() );
    return reltree.getSorted( list, sorted );
}


bool IOObjInfo::getBodyRange( CubeSampling& cs ) const
{
    if ( type_ != IOObjInfo::Body )
	return false;

    // TODO: implement
    pErrMsg( "TODO: implement IOObjInfo::getBodyRange" );
    mGetReader
    return reader_;
}


int IOObjInfo::nrSticks() const
{
    mGetReaderRet
    if ( !reader_->pars() )
	return 0;

    Interval<int> rowrange = Interval<int>::udf();
    reader_->pars()->get( "Row range", rowrange );
    return rowrange.isUdf() ? 0 : rowrange.width()+1;
}

} // namespace EM

/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : 7-1-1996
-*/


#include "ctxtioobj.h"
#include "iostrm.h"
#include "dbman.h"
#include "dbdir.h"
#include "iopar.h"
#include "oddirs.h"
#include "transl.h"
#include "globexpr.h"
#include "separstr.h"
#include "file.h"
#include "genc.h"
#include "filepath.h"
#include "survinfo.h"
#include "keystrs.h"
#include "uistrings.h"

mDefineEnumUtils(IOObjContext,StdSelType,"Std sel type") {

	"Seismic data",
	"Surface data",
	"Location data",
	"Feature Sets",
	"Well Info",
	"Neural Networks",
	"Miscellaneous data",
	"Attribute definitions",
	"Model data",
	"Survey Geometries",
	"None",
	0
};

template<>
void EnumDefImpl<IOObjContext::StdSelType>::init()
{
    uistrings_ += uiStrings::sSeismicData();
    uistrings_ += mEnumTr("Surface Data",0);
    uistrings_ += mEnumTr("Location Data",0);
    uistrings_ += mEnumTr("Feature Sets",0);
    uistrings_ += mEnumTr("Well Information",0);
    uistrings_ += mEnumTr("Neural Networks",0);
    uistrings_ += mEnumTr("Miscellaneous data",0);
    uistrings_ += mEnumTr("Attribute definitions",0);
    uistrings_ += mEnumTr("Model Data",0);
    uistrings_ += mEnumTr("Survey Geometries",0);
    uistrings_ += uiStrings::sNone();
}

#define mStdDirD(nr,dirnm,typ) \
    IOObjContext::StdDirData( nr, dirnm, \
	    IOObjContext::StdSelTypeDef().getKey(IOObjContext::typ) )

static const IOObjContext::StdDirData stddirdata[] = {
    mStdDirD( 100010, sSeismicSubDir(), Seis ),
    mStdDirD( 100020, sSurfaceSubDir(), Surf ),
    mStdDirD( 100030, "Locations", Loc ),
    mStdDirD( 100040, "Features", Feat ),
    mStdDirD( 100050, sWellSubDir(), WllInf ),
    mStdDirD( 100060, "NLAs", NLA ),
    mStdDirD( 100070, "Misc", Misc ),
    mStdDirD( 100080, "Attribs", Attr ),
    mStdDirD( 100090, "Models", Mdl ),
    mStdDirD( 100100, "Geometry", Geom ),
    mStdDirD( -1, "None", None ),
    mStdDirD( 0, 0, Seis )
};


IOObjContext::StdDirData::StdDirData( DBGroupNrType dirnr, const char* thedirnm,
				      const char* thedesc )
    : id_(DBDirID::get(dirnr))
    , dirnm_(thedirnm)
    , desc_(thedesc)
{
}


int IOObjContext::totalNrStdDirs() { return 10; }
const IOObjContext::StdDirData* IOObjContext::getStdDirData(
	IOObjContext::StdSelType sst )
{ return &stddirdata[(int)sst]; }


IOObjSelConstraints::IOObjSelConstraints()
    : require_(*new IOPar)
    , dontallow_(*new IOPar)
    , allownonuserselectable_(false)
    ,allowmissing_(false)
{
}


IOObjSelConstraints::IOObjSelConstraints( const IOObjSelConstraints& oth )
    : require_(*new IOPar(oth.require_))
    , dontallow_(*new IOPar(oth.dontallow_))
    , allowtransls_(oth.allowtransls_)
    , allownonuserselectable_(oth.allownonuserselectable_)
    , allowmissing_(oth.allowmissing_)
{
}


IOObjSelConstraints::~IOObjSelConstraints()
{
    delete &require_;
    delete &dontallow_;
}


IOObjSelConstraints& IOObjSelConstraints::operator =(
				const IOObjSelConstraints& oth )
{
    if ( this != &oth )
    {
	require_ = oth.require_;
	dontallow_ = oth.dontallow_;
	allowtransls_ = oth.allowtransls_;
	allownonuserselectable_ = oth.allownonuserselectable_;
	allowmissing_ = oth.allowmissing_;
    }
    return *this;
}


void IOObjSelConstraints::clear()
{
    require_.setEmpty();
    dontallow_.setEmpty();
    allowtransls_.setEmpty();
    allownonuserselectable_ = false;
    allowmissing_ = false;
}


bool IOObjSelConstraints::isAllowedTranslator( const char* trnm,
					       const char* allowtrs )
{
    if ( !allowtrs || !*allowtrs )
	return true;

    FileMultiString fms( allowtrs );
    const int sz = fms.size();
    for ( int idx=0; idx<sz; idx++ )
    {
	GlobExpr ge( fms[idx] );
	if ( ge.matches( trnm ) )
	    return true;
    }
    return false;
}


bool IOObjSelConstraints::isGood( const IOObj& ioobj, bool forread ) const
{
    if ( !allownonuserselectable_ && !ioobj.isUserSelectable(forread) )
	return false;
    else if ( !isAllowedTranslator(ioobj.translator(),allowtransls_) )
	return false;

    for ( int ireq=0; ireq<require_.size(); ireq++ )
    {
	FileMultiString fms( require_.getValue(ireq) );
	const int fmssz = fms.size();
	const char* val = ioobj.pars().find( require_.getKey(ireq) );
	const bool valisempty = !val || !*val;

	if ( fmssz == 0 && valisempty ) continue;

	const FileMultiString valfms( val );
	const int valfmssz = valfms.size();
	bool isok = false;
	for ( int ifms=0; ifms<fmssz; ifms++ )
	{
	    const BufferString fmsstr( fms[ifms] );
	    const bool fmsstrisempty = fmsstr.isEmpty();
	    if ( fmsstrisempty && valisempty )
		isok = true;
	    else if ( allowmissing_ && valisempty )
		isok = true;
	    else if ( fmsstrisempty != valisempty )
		continue;
	    else
	    {
		isok = false;
		for ( int ifms2=0; ifms2<valfmssz; ifms2++ )
		{
		    if ( fmsstr == valfms[ifms2] )
			{ isok = true; break; }
		}
	    }
	    if ( isok )
		break;
	}
	if ( !isok ) return false;
    }

    if ( dontallow_.isEmpty() )
	return true;

    for ( int ipar=0; ipar<ioobj.pars().size(); ipar++ )
    {
	const char* notallowedvals = dontallow_.find(
					ioobj.pars().getKey( ipar ) );
	if ( !notallowedvals )
	    continue;

	FileMultiString fms( notallowedvals );
	const int fmssz = fms.size();
	const char* val = ioobj.pars().getValue( ipar );
	const bool valisempty = !val || !*val;
	if ( valisempty && fmssz < 1 )
	    return false;

	const FileMultiString valfms( val );
	const int valfmssz = valfms.size();
	for ( int ifms=0; ifms<fmssz; ifms++ )
	{
	    const BufferString fmsstr( fms[ifms] );
	    for ( int ifms2=0; ifms2<valfmssz; ifms2++ )
	    {
		if ( fmsstr == valfms[ifms2] )
		    { return false; }
	    }
	}
    }

    return true;
}


IOObjContext::IOObjContext( const TranslatorGroup* trg, const char* prefname )
    : NamedObject(prefname)
    , trgroup_(trg)
    , stdseltype_(None)
    , destpolicy_(SurveyOnly)
    , forread_(true)
{
}


IOObjContext::IOObjContext( const IOObjContext& oth )
    : NamedObject(oth)
{
    *this = oth;
}


#define mCpMemb(nm) nm = oth.nm

IOObjContext& IOObjContext::operator =( const IOObjContext& oth )
{
    if ( this != &oth )
    {
	mCpMemb(stdseltype_); mCpMemb(trgroup_); mCpMemb(destpolicy_);
	mCpMemb(forread_); mCpMemb(dirid_); mCpMemb(deftransl_);
	mCpMemb(toselect_);
    }
    return *this;
}


BufferString IOObjContext::getDataDirName( StdSelType sst, bool dirnmonly )
{
    const IOObjContext::StdDirData* sdd = getStdDirData( sst );
    BufferString dirnm( sdd->dirnm_ );
    File::Path fp( GetDataDir(), sdd->dirnm_ );
    BufferString fulldirnm = fp.fullPath();
    if ( !File::exists(fulldirnm) )
    {
	// Try legacy names
	BufferString altdirnm;
	if ( sst == IOObjContext::NLA )
	    altdirnm.set( "NNs" );
	else if ( sst == IOObjContext::Surf )
	    altdirnm.set( "Grids" );
	else if ( sst == IOObjContext::Loc )
	    altdirnm.set( "Wavelets" );
	else if ( sst == IOObjContext::WllInf )
	    altdirnm.set( "Logs" );

	if ( !altdirnm.isEmpty() )
	{
	    fp.setFileName( altdirnm );
	    const BufferString fullaltdirnm = fp.fullPath();
	    if ( File::exists(fullaltdirnm) )
	    {
		dirnm = altdirnm;
		fulldirnm = fullaltdirnm;
	    }
	}
    }

    return dirnmonly ? dirnm : fulldirnm;
}


IOObjContext::DBDirID IOObjContext::getSelDirID() const
{
    DBDirID dirid = dirid_;
    if ( !dirid.isValid() )
    {
	if ( stdseltype_ == None )
	    return DBKey::getInvalid().groupID();
	dirid = getStdDirData(stdseltype_)->id_;
    }
    return dirid;
}


FixedString IOObjContext::objectTypeName() const
{
    return translatorGroupName();
}


uiString IOObjContext::uiObjectTypeName( int n ) const
{
    return translatorTypeName( n );
}


FixedString IOObjContext::translatorGroupName() const
{
    fillTrGroup();
    return trgroup_->groupName();
}


uiString IOObjContext::translatorTypeName( int n ) const
{
    fillTrGroup();
    return trgroup_->typeName( n );
}


void IOObjContext::fillTrGroup() const
{
    if ( trgroup_ )
	return;

    pErrMsg("We should never be here");

    IOObjContext& self = *const_cast<IOObjContext*>( this );

#define mCase(typ,str) \
    case IOObjContext::typ: \
	self.trgroup_ = &TranslatorGroup::getGroup( str ); \
    break

    switch ( stdseltype_ )
    {
	mCase(Surf,"Horizon");
	mCase(Loc,"PickSet Group");
	mCase(Feat,"Feature set");
	mCase(WllInf,"Well");
	mCase(Attr,"Attribute definitions");
	mCase(Misc,"Session setup");
	mCase(Mdl,"EarthModel");
	case IOObjContext::NLA:
	    self.trgroup_ = &TranslatorGroup::getGroup( "NonLinear Analysis" );
	    if ( translatorGroupName().isEmpty() )
		self.trgroup_ = &TranslatorGroup::getGroup( "Neural network" );
	default:
	    self.trgroup_ = &TranslatorGroup::getGroup( "Seismic Data" );
	break;
    }
}


bool IOObjContext::validIOObj( const IOObj& ioobj ) const
{
    fillTrGroup();

    if ( !trgroup_->objSelector(ioobj.group()) )
	return false;

    // check if the translator is present at all
    const ObjectSet<const Translator>& trs = trgroup_->templates();
    for ( int idx=0; idx<trs.size(); idx++ )
    {
	if ( trs[idx]->userName() == ioobj.translator() )
	    break;
	else if ( idx == trs.size()-1 )
	    return false;
    }

    return toselect_.isGood( ioobj, forread_ );
}


int IOObjContext::nrMatches() const
{
    return DBDirEntryList(*this).size();
}



IOStream* IOObjContext::crDefaultWriteObj( const Translator& transl,
					    const DBKey& ky ) const
{
    fillTrGroup();

    IOStream* iostrm = new IOStream( name(), ky, false );
    iostrm->setGroup( translatorGroupName() );
    iostrm->setTranslator( transl.userName() );

    const StdDirData* sdd = getStdDirData( stdseltype_ );
    const char* dirnm = sdd ? sdd->dirnm_ : 0;
    if ( dirnm )
	iostrm->setDirName( dirnm );
    iostrm->setExt( transl.defExtension() );
    iostrm->updateCreationPars();

    ConstRefMan<DBDir> dbdir = DBM().fetchDir( ky.dirID() );
    if ( !dbdir )
	return iostrm;

    dbdir->prepObj( *iostrm );
    const BufferString uniqnm( iostrm->name() );
    int ifnm = 0;
    while ( true )
    {
	iostrm->genFileName();
	if ( !File::exists(iostrm->mainFileName()) )
	    break;
	ifnm++;
	iostrm->setName( BufferString(uniqnm,ifnm) );
	dbdir->prepObj( *iostrm );
    }

    if ( destpolicy_ == PreferShared )
    {
	const BufferString subdirnm( File::Path(dbdir->dirName()).fileName() );
	File::Path fpsh( GetBaseDataDir(), subdirnm );
	const BufferString shdirnm( fpsh.fullPath() );
	const bool exists = File::exists( shdirnm );
	if ( exists && !File::isDirectory(shdirnm) )
	    return iostrm;
	else if ( !exists && !File::createDir(shdirnm) )
	    return iostrm;

	const File::Path fpold( iostrm->mainFileName() );
	File::Path fpnew( "${DTECT_DATA}", subdirnm, fpold.fileName() );
	iostrm->fileSpec().setFileName( fpnew.fullPath() );
    }

    return iostrm;
}


void CtxtIOObj::fillIfOnlyOne()
{
    ctxt_.fillTrGroup();
    ConstRefMan<DBDir> dbdir = DBM().fetchDir( ctxt_.getSelDirID() );
    if ( !dbdir )
	return;

    DBDirIter iter( *dbdir );
    DBKey dbky;
    while ( iter.next() )
    {
	if ( ctxt_.validIOObj(iter.ioObj()) )
	{
	    if ( dbky.isValid() )
		return; // more than one
	    else
		dbky = iter.key();
	}
    }

    if ( dbky.isValid() )
	setObj( dbdir->getEntry(dbky.objID()) );
}


void CtxtIOObj::fillDefault( bool oone2 )
{
    ctxt_.fillTrGroup();

    BufferString keystr( ctxt_.trgroup_->getSurveyDefaultKey(0) );

    const BufferString typestr = ctxt_.toselect_.require_.find( sKey::Type() );
    if ( !typestr.isEmpty() )
	    keystr = IOPar::compKey( keystr, typestr );

    return fillDefaultWithKey( keystr, oone2 );
}


void CtxtIOObj::fillDefaultWithKey( const char* parky, bool oone2 )
{
    const BufferString valstr = SI().getDefaultPars().find( parky );
    if ( !valstr.isEmpty() )
	setObj( DBKey(valstr).getIOObj() );

    if ( !ioobj_ && oone2 )
	fillIfOnlyOne();
}


void CtxtIOObj::setObj( IOObj* obj )
{
    if ( obj == ioobj_ )
	return;

    delete ioobj_; ioobj_ = obj;
    if ( ioobj_ && !ctxt_.hasStdSelDirID() )
	ctxt_.dirid_ = ioobj_->key().dirID();
}


void CtxtIOObj::setObj( const DBKey& id )
{
    setObj( id.getIOObj() );
}


void CtxtIOObj::setPar( IOPar* iop )
{
    if ( iop != iopar_ )
	{ delete iopar_; iopar_ = iop; }
}


void CtxtIOObj::destroyAll()
{
    deleteAndZeroPtr( ioobj_ );
    deleteAndZeroPtr( iopar_ );
}


int CtxtIOObj::fillObj( bool mktmp, int translidxfornew )
{
    const bool emptynm = ctxt_.name().isEmpty();
    if ( !ioobj_ && emptynm )
	return 0;

    if ( ioobj_ && (ctxt_.hasName(ioobj_->name()) || emptynm) )
	return 1;

    DBM().getEntry( *this, mktmp, translidxfornew );
    return ioobj_ ? 2 : 0;
}

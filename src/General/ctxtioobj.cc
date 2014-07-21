/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 7-1-1996
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "ctxtioobj.h"
#include "iostrm.h"
#include "ioman.h"
#include "iodir.h"
#include "iopar.h"
#include "oddirs.h"
#include "transl.h"
#include "globexpr.h"
#include "separstr.h"
#include "file.h"
#include "filepath.h"
#include "survinfo.h"
#include "keystrs.h"

DefineEnumNames(IOObjContext,StdSelType,1,"Std sel type") {

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

static const IOObjContext::StdDirData stddirdata[] = {
	{ "100010", "Seismics", IOObjContext::StdSelTypeNames()[0] },
	{ "100020", "Surfaces", IOObjContext::StdSelTypeNames()[1] },
	{ "100030", "Locations", IOObjContext::StdSelTypeNames()[2] },
	{ "100040", "Features", IOObjContext::StdSelTypeNames()[3] },
	{ "100050", "WellInfo", IOObjContext::StdSelTypeNames()[4] },
	{ "100060", "NLAs", IOObjContext::StdSelTypeNames()[5] },
	{ "100070", "Misc", IOObjContext::StdSelTypeNames()[6] },
	{ "100080", "Attribs", IOObjContext::StdSelTypeNames()[7] },
	{ "100090", "Models", IOObjContext::StdSelTypeNames()[8] },
	{ "100100", "Geometry", IOObjContext::StdSelTypeNames()[9] },
	{ "", "None", IOObjContext::StdSelTypeNames()[10] },
	{ 0, 0, 0 }
};

int IOObjContext::totalNrStdDirs() { return 10; }
const IOObjContext::StdDirData* IOObjContext::getStdDirData(
	IOObjContext::StdSelType sst )
{ return stddirdata + (int)sst; }


IOObjSelConstraints::IOObjSelConstraints()
    : require_(*new IOPar)
    , dontallow_(*new IOPar)
    , allownonuserselectable_(false)
{
}


IOObjSelConstraints::IOObjSelConstraints( const IOObjSelConstraints& oth )
    : require_(*new IOPar(oth.require_))
    , dontallow_(*new IOPar(oth.dontallow_))
    , allowtransls_(oth.allowtransls_)
    , allownonuserselectable_(oth.allownonuserselectable_)
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
    }
    return *this;
}


void IOObjSelConstraints::clear()
{
    require_.setEmpty();
    dontallow_.setEmpty();
    allowtransls_.setEmpty();
    allownonuserselectable_ = false;
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
	, trgroup(trg)
	, newonlevel(1)
	, stdseltype(None)
{
    multi = false;
    forread = maydooper = true;
}


IOObjContext::IOObjContext( const IOObjContext& oth )
    : NamedObject(oth.name())
{
    *this = oth;
}


#define mCpMemb(nm) nm = oth.nm

IOObjContext& IOObjContext::operator =( const IOObjContext& oth )
{
    if ( this != &oth )
    {
	mCpMemb(stdseltype); mCpMemb(trgroup); mCpMemb(newonlevel);
	mCpMemb(multi); mCpMemb(forread);
	mCpMemb(selkey); mCpMemb(maydooper); mCpMemb(deftransl);
	mCpMemb(toselect);
    }
    return *this;
}


BufferString IOObjContext::getDataDirName( StdSelType sst )
{
    const IOObjContext::StdDirData* sdd = getStdDirData( sst );
    FilePath fp( GetDataDir(), sdd->dirnm );
    BufferString dirnm = fp.fullPath();
    if ( !File::exists(dirnm) )
    {	// Try legacy names
	if ( sst == IOObjContext::NLA )
	    fp.setFileName( "NNs" );
	else if ( sst == IOObjContext::Surf )
	    fp.setFileName( "Grids" );
	else if ( sst == IOObjContext::Loc )
	    fp.setFileName( "Wavelets" );
	else if ( sst == IOObjContext::WllInf )
	    fp.setFileName( "Logs" );

	BufferString altdirnm = fp.fullPath();
	if ( File::exists(altdirnm) )
	    dirnm = altdirnm;
    }
    return dirnm;
}


MultiID IOObjContext::getSelKey() const
{
    return selkey.isEmpty()
	? MultiID( stdseltype == None ? "" : getStdDirData(stdseltype)->id )
	: selkey;
}


void IOObjContext::fillTrGroup() const
{
    if ( trgroup ) return;

    IOObjContext& self = *const_cast<IOObjContext*>( this );

#define mCase(typ,str) \
    case IOObjContext::typ: \
	self.trgroup = &TranslatorGroup::getGroup( str, true ); \
    break

    switch ( stdseltype )
    {
	mCase(Surf,"Horizon");
	mCase(Loc,"PickSet Group");
	mCase(Feat,"Feature set");
	mCase(WllInf,"Well");
	mCase(Attr,"Attribute definitions");
	mCase(Misc,"Session setup");
	mCase(Mdl,"EarthModel");
	case IOObjContext::NLA:
	    self.trgroup = &TranslatorGroup::getGroup( "NonLinear Analysis",
							true );
	    if ( trgroup->userName().isEmpty() )
		self.trgroup = &TranslatorGroup::getGroup( "Neural network",
							   true );
	default:
	    self.trgroup = &TranslatorGroup::getGroup( "Seismic Data", true );
	break;
    }
}


const char* IOObjContext::objectTypeName() const
{
    const_cast<IOObjContext*>(this)->fillTrGroup(); // just to be safe
    return trgroup->userName();
}


bool IOObjContext::validIOObj( const IOObj& ioobj ) const
{
    if ( trgroup )
    {
	if ( !trgroup->objSelector(ioobj.group()) )
	    return false;

	// check if the translator is present at all
	const ObjectSet<const Translator>& trs = trgroup->templates();
	for ( int idx=0; idx<trs.size(); idx++ )
	{
	    if ( trs[idx]->userName() == ioobj.translator() )
		break;
	    else if ( idx == trs.size()-1 )
		return false;
	}
    }

    return toselect.isGood( ioobj, forread );
}



IOStream* IOObjContext::crDefaultWriteObj( const Translator& transl,
					    const MultiID& ky ) const
{
    fillTrGroup();

    IOStream* iostrm = new IOStream( name(), ky, false );
    iostrm->setGroup( trgroup->userName() );
    iostrm->setTranslator( transl.userName() );

    const StdDirData* sdd = getStdDirData( stdseltype );
    const char* dirnm = sdd ? sdd->dirnm : 0;
    if ( dirnm )
	iostrm->setDirName( dirnm );
    iostrm->setExt( transl.defExtension() );

    IODir iodir( ky );
    iodir.ensureUniqueName( *iostrm );
    const BufferString uniqnm( iostrm->name() );
    int ifnm = 0;
    while ( true )
    {
	iostrm->genDefaultImpl();
	if ( !File::exists(iostrm->fileName()) )
	    break;
	ifnm++;
	iostrm->setName( BufferString(uniqnm,ifnm) );
	iodir.ensureUniqueName( *iostrm );
    }

    iostrm->updateCreationPars();
    return iostrm;
}


void CtxtIOObj::fillIfOnlyOne()
{
    ctxt.fillTrGroup();

    const IODir iodir( ctxt.getSelKey() );
    int ivalid = -1;
    for ( int idx=0; idx<iodir.size(); idx++ )
    {
	if ( ctxt.validIOObj(*iodir.get(idx)) )
	{
	    if ( ivalid >= 0 )
		return;
	    else
		ivalid = idx;
	}
    }

    if ( ivalid >= 0 )
	setObj( iodir.get(ivalid)->clone() );
}


void CtxtIOObj::fillDefault( bool oone2 )
{
    ctxt.fillTrGroup();

    BufferString keystr( ctxt.trgroup->getSurveyDefaultKey(0) );

    const FixedString typestr = ctxt.toselect.require_.find( sKey::Type() );
    if ( !typestr.isEmpty() )
	    keystr = IOPar::compKey( keystr, typestr );

    return fillDefaultWithKey( keystr, oone2 );
}


void CtxtIOObj::fillDefaultWithKey( const char* parky, bool oone2 )
{
    const char* kystr = SI().pars().find( parky );
    if ( kystr && *kystr )
	setObj( IOM().get(MultiID(kystr)) );

    if ( !ioobj && oone2 )
	fillIfOnlyOne();
}


void CtxtIOObj::setObj( IOObj* obj )
{
    if ( obj == ioobj ) return;

    delete ioobj; ioobj = obj;
    if ( ioobj )
	ctxt.selkey = ctxt.hasStdSelKey() ? "" : ioobj->key().upLevel().buf();
}


void CtxtIOObj::setObj( const MultiID& id )
{
    delete ioobj; ioobj = IOM().get( id );
}


void CtxtIOObj::setPar( IOPar* iop )
{
    if ( iop == iopar ) return;

    delete iopar; iopar = iop;
}


void CtxtIOObj::destroyAll()
{
    delete ioobj; ioobj = 0;
    delete iopar; iopar = 0;
}


int CtxtIOObj::fillObj( bool mktmp, int translidxfornew )
{
    const bool emptynm = ctxt.name().isEmpty();
    if ( !ioobj && emptynm )
	return 0;

    if ( ioobj && (ctxt.name() == ioobj->name() || emptynm) )
	return 1;

    IOM().getEntry( *this, mktmp, translidxfornew );
    return ioobj ? 2 : 0;
}

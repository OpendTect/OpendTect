/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 7-1-1996
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "ctxtioobj.h"
#include "ioobj.h"
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
	{ "", "None", IOObjContext::StdSelTypeNames()[9] },
	{ 0, 0, 0 }
};

int IOObjContext::totalNrStdDirs() { return 9; }
const IOObjContext::StdDirData* IOObjContext::getStdDirData(
	IOObjContext::StdSelType sst )
{ return stddirdata + (int)sst; }


IOObjSelConstraints::IOObjSelConstraints()
    : require_(*new IOPar)
    , dontallow_(*new IOPar)
    , allownonreaddefault_(false)
{
}


IOObjSelConstraints::IOObjSelConstraints( const IOObjSelConstraints& oth )
    : require_(*new IOPar(oth.require_))
    , dontallow_(*new IOPar(oth.dontallow_))
    , allowtransls_(oth.allowtransls_)
    , allownonreaddefault_(oth.allownonreaddefault_)
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
	allownonreaddefault_ = oth.allownonreaddefault_;
    }
    return *this;
}


void IOObjSelConstraints::clear()
{
    require_.setEmpty();
    dontallow_.setEmpty();
    allowtransls_.setEmpty();
    allownonreaddefault_ = false;
}


bool IOObjSelConstraints::isGood( const IOObj& ioobj ) const
{
    if ( !allownonreaddefault_ && !ioobj.isReadDefault() )
	return false;

    if ( !allowtransls_.isEmpty() )
    {
	FileMultiString fms( allowtransls_ );
	const int sz = fms.size();
	bool isok = false;
	for ( int idx=0; idx<sz; idx++ )
	{
	    GlobExpr ge( fms[idx] );
	    if ( ge.matches( ioobj.translator() ) )
		{ isok = true; break; }
	}
	if ( !isok )
	    return false;
    }

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

void IOObjContext::fillTrGroup()
{
    if ( trgroup ) return;

#define mCase(typ,str) \
    case IOObjContext::typ: \
	trgroup = &TranslatorGroup::getGroup( str, true ); \
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
	    trgroup = &TranslatorGroup::getGroup( "NonLinear Analysis", true );
	    if ( trgroup->userName().isEmpty() )
	    trgroup = &TranslatorGroup::getGroup( "Neural network", true );
	default:
	    trgroup = &TranslatorGroup::getGroup( "Seismic Data", true );
	break;
    }
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
	    else if ( idx == trs.size() - 1 )
		return false;
	}
    }

    return toselect.isGood( ioobj );
}


void CtxtIOObj::fillIfOnlyOne()
{
    ctxt.fillTrGroup();

    IOM().to( ctxt.getSelKey() );
    const IODir& iodir = *IOM().dirPtr();
    int ivalid = -1;
    for ( int idx=0; idx<iodir.size(); idx++ )
    {
	if ( ctxt.validIOObj(*iodir[idx]) )
	{
	    if ( ivalid >= 0 )
		return;
	    else
		ivalid = idx;
	}
    }

    if ( ivalid >= 0 )
	setObj( iodir[ivalid]->clone() );
}


void CtxtIOObj::fillDefault( bool oone2 )
{
    ctxt.fillTrGroup();

    BufferString keystr( ctxt.trgroup->userName() );
    if ( keystr == "Seismic Data" )
    {
	bool is3d = SI().survDataType() != SurveyInfo::Only2D;
	if ( SI().survDataType() == SurveyInfo::Both2DAnd3D
		&& ctxt.deftransl == "2D" )
	    is3d = false;
	keystr = is3d ? sKey::DefCube() : sKey::DefLineSet();
	FixedString typestr = ctxt.toselect.require_.find( sKey::Type() );
	if ( is3d && !typestr.isEmpty() )
	    keystr = IOPar::compKey(keystr,typestr);
    }
    else
    {
	if ( keystr == "Pre-Stack Seismics"
		&& SI().survDataType() != SurveyInfo::Only2D )
	    keystr = "PS3D Data Store";
	
	keystr = IOPar::compKey(sKey::Default(),keystr);
    }

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
    delete ioobj;
    delete iopar;
}


int CtxtIOObj::fillObj( bool mktmp )
{
    const bool emptynm = ctxt.name().isEmpty();
    if ( !ioobj && emptynm )
	return 0;

    if ( ioobj && (ctxt.name() == ioobj->name() || emptynm) )
	return 1;

    IOM().getEntry( *this, mktmp );
    return ioobj ? 2 : 0;
}

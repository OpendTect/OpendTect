/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 7-1-1996
-*/

static const char* rcsID = "$Id: ctxtioobj.cc,v 1.45 2010-01-19 05:44:51 cvsraman Exp $";

#include "ctxtioobj.h"
#include "ioobj.h"
#include "ioman.h"
#include "iodir.h"
#include "iopar.h"
#include "oddirs.h"
#include "transl.h"
#include "globexpr.h"
#include "separstr.h"
#include "filegen.h"
#include "filepath.h"
#include "survinfo.h"
#include "keystrs.h"

static const char* sKeySelConstr = "Selection.Constraints";

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


IOObjContext::IOObjContext( const TranslatorGroup* trg, const char* prefname )
	: NamedObject(prefname)
	, trgroup(trg)
	, parconstraints(*new IOPar)
{
    init();
}


IOObjContext::IOObjContext( const IOObjContext& rp )
	: NamedObject("")
	, parconstraints(*new IOPar)
{
    *this = rp;
}


void IOObjContext::init()
{
    newonlevel		= 1;
    multi = maychdir	=
    allownonreaddefault	= false;
    forread = maydooper =
    inctrglobexpr	= true;
    deftransl		= "";
    stdseltype		= None;
    includeconstraints	= true;
    allowcnstrsabsent	= false;
    parconstraints.clear();
}


IOObjContext::~IOObjContext()
{
    delete &parconstraints;
}


IOObjContext& IOObjContext::operator=( const IOObjContext& ct )
{
    if ( this != &ct )
    {
	setName( ct.name() );
	trgroup = ct.trgroup;
	newonlevel = ct.newonlevel;
	multi = ct.multi;
	stdseltype = ct.stdseltype;
	forread = ct.forread;
	maychdir = ct.maychdir;
	maydooper = ct.maydooper;
	selkey = ct.selkey;
	deftransl = ct.deftransl;
	trglobexpr = ct.trglobexpr;
	inctrglobexpr = ct.inctrglobexpr;
	parconstraints = ct.parconstraints;
	includeconstraints = ct.includeconstraints;
	allowcnstrsabsent = ct.allowcnstrsabsent;
	allownonreaddefault = ct.allownonreaddefault;
    }
    return *this;
}


BufferString IOObjContext::getDataDirName( StdSelType sst )
{
    const IOObjContext::StdDirData* sdd = getStdDirData( sst );
    FilePath fp( GetDataDir() ); fp.add( sdd->dirnm );
    BufferString dirnm = fp.fullPath();
    if ( !File_exists(dirnm) )
    {
	if ( sst == IOObjContext::NLA )
	    fp.setFileName( "NNs" );
	else if ( sst == IOObjContext::Surf )
	    fp.setFileName( "Grids" );
	else if ( sst == IOObjContext::Loc )
	    fp.setFileName( "Wavelets" );
	else if ( sst == IOObjContext::WllInf )
	    fp.setFileName( "Logs" );

	BufferString altdirnm = fp.fullPath();
	if ( File_exists(altdirnm) )
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


void IOObjContext::fillPar( IOPar& iopar ) const
{
    iopar.set( "Name", (const char*)name() );
    iopar.set( "Translator group", trgroup ? trgroup->userName().buf() :"" );
    iopar.set( "Data type", eString(IOObjContext::StdSelType,stdseltype) );
    iopar.set( "Level for new objects", newonlevel );
    iopar.setYN( "Multi-entries", multi );
    iopar.setYN( "Entries in other directories", maychdir );
    iopar.setYN( "Selection.For read", forread );
    iopar.setYN( "Selection.Allow operations", maydooper );
    iopar.set( "Selection.Default translator", (const char*)deftransl );
    iopar.set( "Selection.Dir key", (const char*)selkey );
    iopar.set( "Selection.Translator subsel", trglobexpr );
    iopar.setYN( "Selection.Include Translator subsel", inctrglobexpr );
    iopar.mergeComp( parconstraints, sKeySelConstr );
    iopar.setYN( IOPar::compKey(sKeySelConstr,"Include"), includeconstraints );
    iopar.setYN( IOPar::compKey(sKeySelConstr,"AllowAbsent"),allowcnstrsabsent);
    iopar.setYN( "Allow non-standard data types", allownonreaddefault );
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


void IOObjContext::usePar( const IOPar& iopar )
{
    const char* res = iopar.find( "Name" );
    if ( res ) setName( res );
    res = iopar.find( "Translator group" );
    if ( res ) trgroup = &TranslatorGroup::getGroup( res, true );
    res = iopar.find( "Data type" );
    if ( res ) stdseltype = eEnum(IOObjContext::StdSelType,res);
    fillTrGroup();

    iopar.get( "Level for new objects", newonlevel );
    iopar.getYN( "Multi-entries", multi );
    iopar.getYN( "Entries in other directories", maychdir );
    iopar.getYN( "Selection.For read", forread );
    iopar.getYN( "Selection.Allow operations", maydooper );
    iopar.getYN( "Selection.Include Translator subsel", inctrglobexpr );
    iopar.getYN( "Allow non-standard data types", allownonreaddefault );

    res = iopar.find( "Selection.Default translator" );
    if ( res ) deftransl = res;
    res = iopar.find( "Selection.Dir key" );
    if ( res ) selkey = res;
    res = iopar.find( "Selection.Translator subsel" );
    if ( res ) trglobexpr = res;

    IOPar* subiop = iopar.subselect( sKeySelConstr );
    if ( subiop )
    {
	parconstraints.merge( *subiop );
	iopar.getYN( IOPar::compKey(sKeySelConstr,"Include"),
					includeconstraints );
	iopar.getYN( IOPar::compKey(sKeySelConstr,"AllowAbsent"),
					allowcnstrsabsent);
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

    if ( !trglobexpr.isEmpty() )
    {
	FileMultiString fms( trglobexpr );
	const int sz = fms.size();
	for ( int idx=0; idx<sz; idx++ )
	{
	    GlobExpr ge( fms[idx] );
	    if ( ge.matches( ioobj.translator() ) )
	    {
		if ( inctrglobexpr )
		    break;
		else
		    return false;
	    }
	    else if ( idx == sz-1 )
		return false;
	}
    }

    if ( parconstraints.size() < 1 )
	return true;

    int nrequal = 0, nrdiff = 0;
    for ( int idx=0; idx<parconstraints.size(); idx++ )
    {
	const char* ioobjval = ioobj.pars().find( parconstraints.getKey(idx) );
	if ( !ioobjval ) continue;

	FileMultiString fms( parconstraints.getValue(idx) );
	const int nrvals = fms.size();
	bool ismatch = false;
	for ( int idy=0; idy<nrvals; idy++ )
	{
	    if ( !strcmp(ioobjval,fms[idy]) )
		{ ismatch = true; break; }
	}
	if ( ismatch )	nrequal++;
	else		nrdiff++;
    }

    if ( includeconstraints )
	return allowcnstrsabsent ? nrdiff == 0
	    			 : nrequal == parconstraints.size();
    else
	return allowcnstrsabsent ? nrequal == 0
	    			 : nrdiff == parconstraints.size();
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
	bool is3d = SI().getSurvDataType() != SurveyInfo::Only2D;
	if ( SI().getSurvDataType() == SurveyInfo::Both2DAnd3D
		&& ctxt.deftransl == "2D" )
	    is3d = false;
	keystr = is3d ? sKey::DefCube : sKey::DefLineSet;
	FixedString typestr = ctxt.parconstraints.find( sKey::Type );
	if ( is3d && !typestr.isEmpty() && ctxt.includeconstraints )
	    keystr = IOPar::compKey(keystr,typestr);
    }
    else
    {
	if ( keystr == "Pre-Stack Seismics"
		&& SI().getSurvDataType() != SurveyInfo::Only2D )
	    keystr = "PS3D Data Store";
	
	keystr = IOPar::compKey(sKey::Default,keystr);
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

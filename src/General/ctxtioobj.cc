/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 7-1-1996
-*/

static const char* rcsID = "$Id: ctxtioobj.cc,v 1.12 2002-06-20 15:59:45 bert Exp $";

#include "ctxtioobj.h"
#include "ioobj.h"
#include "ioman.h"
#include "iopar.h"
#include "transl.h"
#include "globexpr.h"

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
	{ "100010", "Seismics", IOObjContext::StdSelTypeNames[0] },
	{ "100020", "Surfaces", IOObjContext::StdSelTypeNames[1] },
	{ "100030", "Locations", IOObjContext::StdSelTypeNames[2] },
	{ "100040", "Features", IOObjContext::StdSelTypeNames[3] },
	{ "100050", "WellInfo", IOObjContext::StdSelTypeNames[4] },
	{ "100060", "NNs", IOObjContext::StdSelTypeNames[5] },
	{ "100070", "Misc", IOObjContext::StdSelTypeNames[6] },
	{ "100080", "Attribs", IOObjContext::StdSelTypeNames[7] },
	{ "100090", "Models", IOObjContext::StdSelTypeNames[8] },
	{ "", "None", IOObjContext::StdSelTypeNames[9] },
	{ 0, 0, 0 }
};

int IOObjContext::totalNrStdDirs() { return 9; }
const IOObjContext::StdDirData* IOObjContext::getStdDirData(
	IOObjContext::StdSelType sst )
{ return stddirdata + (int)sst; }


IOObjContext::IOObjContext( const Translator* trg, const char* prefname )
	: UserIDObject(prefname)
	, trgroup(trg)
{
    init();
}


IOObjContext::IOObjContext( const IOObjContext& rp )
	: UserIDObject("")
{
    *this = rp;
}


IOObjContext& IOObjContext::operator=( const IOObjContext& ct )
{
    if ( this != &ct )
    {
	setName( ct.name() );
	trgroup = ct.trgroup;
	newonlevel = ct.newonlevel;
	crlink = ct.crlink;
	needparent = ct.needparent;
	parentlevel = ct.parentlevel;
	partrgroup = ct.partrgroup;
	multi = ct.multi;
	stdseltype = ct.stdseltype;
	forread = ct.forread;
	maychdir = ct.maychdir;
	maydooper = ct.maydooper;
	parentkey = ct.parentkey;
	deftransl = ct.deftransl;
	trglobexpr = ct.trglobexpr;
	ioparkeyval[0] = ct.ioparkeyval[0];
	ioparkeyval[1] = ct.ioparkeyval[1];
	includekeyval = ct.includekeyval;
    }
    return *this;
}


void IOObjContext::fillPar( IOPar& iopar ) const
{
    iopar.set( "Name", (const char*)name() );
    iopar.set( "Translator group", trgroup ? (const char*)trgroup->name() :"" );
    iopar.set( "Data type", eString(IOObjContext::StdSelType,stdseltype) );
    iopar.set( "Level for new objects", newonlevel );
    iopar.setYN( "Create new directory", crlink );
    iopar.setYN( "Multi-entries", multi );
    iopar.setYN( "Entries in other directories", maychdir );
    iopar.setYN( "Parent needed", needparent );
    iopar.set( "Translator group parent", partrgroup
			? (const char*)partrgroup->name() : "" );
    iopar.set( "Parent level", parentlevel );
    iopar.setYN( "Selection.For read", forread );
    iopar.setYN( "Selection.Allow operations", maydooper );
    iopar.set( "Selection.Default translator", (const char*)deftransl );
    iopar.set( "Selection.Parent key", (const char*)parentkey );
    iopar.set( "Selection.Allow Translators", trglobexpr );
    iopar.set( "Selection.Property selection key", ioparkeyval[0] );
    iopar.set( "Selection.Property selection value", ioparkeyval[1] );
    iopar.setYN( "Selection.Include property", includekeyval );
}


void IOObjContext::usePar( const IOPar& iopar )
{
    const char* res = iopar.find( "Name" );
    if ( res ) setName( res );
    res = iopar.find( "Translator group" );
    if ( res ) trgroup = Translator::groups()[res];
    res = iopar.find( "Data type" );
    if ( res ) stdseltype = eEnum(IOObjContext::StdSelType,res);

    iopar.get( "Level for new objects", newonlevel );
    iopar.getYN( "Create new directory", crlink );
    iopar.getYN( "Multi-entries", multi );
    iopar.getYN( "Entries in other directories", maychdir );
    iopar.getYN( "Parent needed", needparent );

    res = iopar.find( "Translator group parent" );
    if ( res ) partrgroup = Translator::groups()[res];

    iopar.get( "Parent level", parentlevel );
    iopar.getYN( "Selection.For read", forread );
    iopar.getYN( "Selection.Allow operations", maydooper );

    res = iopar.find( "Selection.Default translator" );
    if ( res ) deftransl = res;
    res = iopar.find( "Selection.Parent key" );
    if ( res ) parentkey = res;
    res = iopar.find( "Selection.Allow Translators" );
    if ( res ) trglobexpr = res;
    res = iopar.find( "Selection.Property selection key" );
    if ( res ) ioparkeyval[0] = res;
    res = iopar.find( "Selection.Property selection value" );
    if ( res ) ioparkeyval[1] = res;
    iopar.getYN( "Selection.Include property", includekeyval );
}


bool IOObjContext::validIOObj( const IOObj& ioobj ) const
{
    if ( *((const char*)trglobexpr) )
    {
	GlobExpr ge( trglobexpr );
	if ( !ge.matches( ioobj.translator() ) )
	    return false;
    }

    const char* iopkey = ioparkeyval[0];
    if ( *iopkey )
    {
	const char* res = ioobj.pars().find( iopkey );
	if ( !res && includekeyval )
	    return false;
	if ( res && includekeyval == (ioparkeyval[1] != res) )
	    return false;
    }

    return true;
}


void CtxtIOObj::setObj( IOObj* obj )
{
    if ( obj == ioobj ) return;

    delete ioobj; ioobj = obj;
    if ( ioobj ) ctxt.parentkey = ioobj->parentKey();
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


int CtxtIOObj::fillObj( const MultiID& uid )
{
    if ( ioobj && (ctxt.name() == ioobj->name() || ctxt.name() == "") )
	return 1;
    IOM().getEntry( *this, uid );
    return ioobj ? 2 : 0;
}

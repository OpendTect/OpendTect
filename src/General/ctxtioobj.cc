/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 7-1-1996
-*/

static const char* rcsID = "$Id: ctxtioobj.cc,v 1.3 2001-04-13 11:50:04 bert Exp $";

#include "ctxtioobj.h"
#include "ioobj.h"
#include "ioman.h"
#include "iopar.h"
#include "transl.h"

DefineEnumNames(IOObjContext,StdSelType,1,"Std sel type") {

	"Seismic data",
	"Gridded data",
	"Wavelets",
	"Feature Sets",
	"Log data",
	"Neural Networks",
	"Miscellaneous data",
	"Attribute definitions",
	"None",
	0

};

static const IOObjContext::StdDirData stddirdata[] = {
	{ "100010", "Seismics", IOObjContext::StdSelTypeNames[0] },
	{ "100020", "Grids", IOObjContext::StdSelTypeNames[1] },
	{ "100030", "Wavelets", IOObjContext::StdSelTypeNames[2] },
	{ "100040", "Features", IOObjContext::StdSelTypeNames[3] },
	{ "100050", "Logs", IOObjContext::StdSelTypeNames[4] },
	{ "100060", "NNs", IOObjContext::StdSelTypeNames[5] },
	{ "100070", "Misc", IOObjContext::StdSelTypeNames[6] },
	{ "100080", "Attribs", IOObjContext::StdSelTypeNames[7] },
	{ "0", "None", IOObjContext::StdSelTypeNames[8] },
	{ 0, 0, 0 }
};

int IOObjContext::totalNrStdDirs() { return 8; }
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
}


void CtxtIOObj::setObj( IOObj* obj )
{
    if ( obj == ioobj ) return;

    delete ioobj; ioobj = obj;
    if ( ioobj ) ctxt.parentkey = ioobj->parentKey();
}


int CtxtIOObj::fillObj( const MultiID& uid )
{
    if ( ioobj && (ctxt.name() == ioobj->name() || ctxt.name() == "") )
	return 1;
    IOM().getEntry( *this, uid );
    return ioobj ? 2 : 0;
}

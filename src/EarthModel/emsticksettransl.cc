/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: emsticksettransl.cc,v 1.1 2003-09-09 16:06:12 kristofer Exp $";

#include "emsticksettransl.h"

#include "emstickset.h"
#include "emsurfaceio.h"
#include "executor.h"
#include "ptrman.h"
#include "position.h"
#include "survinfo.h"
#include "ioobj.h"
#include "geomgridsurfaceimpl.h"
#include "ascstream.h"

#include <fstream>

const char* EMStickSetTranslator::keyword = "StickSet";

const char* lmkEMStickSetTranslator::pointtypestr = "FAULT_PTYPE";
const char* lmkEMStickSetTranslator::xstr = "FAULT_X";
const char* lmkEMStickSetTranslator::ystr = "FAULT_Y";
const char* lmkEMStickSetTranslator::zstr = "FAULT_Z";
const char* lmkEMStickSetTranslator::domainstr = "FAULT_DOMAIN";
const char* lmkEMStickSetTranslator::surveystr = "FAULT_SURVEY";
const char* lmkEMStickSetTranslator::domainunitstr = "FAULT_DOMAIN_UNIT";
const char* lmkEMStickSetTranslator::distancunitestr="FAULT_DISTANCE_UNIT";
const char* lmkEMStickSetTranslator::lineidstr="FAULT_LINEID";
const char* lmkEMStickSetTranslator::tracestr="FAULT_TRACE";

const IOObjContext& EMStickSetTranslator::ioContext()
{
    static IOObjContext* ctxt = 0;

    if ( !ctxt )
    {
	ctxt = new IOObjContext( Translator::groups()[listid] );
	ctxt->crlink = false;
	ctxt->newonlevel = 1;
	ctxt->needparent = false;
	ctxt->maychdir = false;
	ctxt->stdseltype = IOObjContext::StickSet;
    }

    return *ctxt;
}


int EMStickSetTranslator::selector( const char* key )
{
    int retval = defaultSelector( EMStickSetTranslator::keyword, key );
    return retval;
}


Executor* EMStickSetTranslator::reader( EM::StickSet& hor, const IOObj* ioobj,
				     BufferString& errmsg )
{
    if ( !ioobj )
    {
	errmsg = "No object to read set in data base";
	return 0;
    }

    PtrMan<EMStickSetTranslator> tr =
	dynamic_cast<EMStickSetTranslator*>(ioobj->getTranslator());

    if ( !tr )
    {
	errmsg = "Selected Object is not an StickSet";
	return 0;
    }

    Conn* conn = ioobj->getConn( Conn::Read );
    if ( !conn )
    {
	errmsg = "Cannot open ";
	errmsg += ioobj->fullUserExpr(true);
	return 0;
    }

    const char* formatfilename = ioobj->pars()["Format file"];

    return tr->reader( hor, conn, formatfilename );
}


Executor* EMStickSetTranslator::writer( const EM::StickSet& hor, const IOObj* ioobj,
				     BufferString& errmsg )
{
    if ( !ioobj )
    {
	errmsg = "No object to store set in data base";
	return false;
    }

    PtrMan<EMStickSetTranslator> tr =
	dynamic_cast<EMStickSetTranslator*>(ioobj->getTranslator());

    if ( !tr )
    {
	errmsg = "Selected Object is not an StickSet";
	return false;
    }

    Conn* conn = ioobj->getConn( Conn::Write );
    if ( !conn )
    {
	errmsg = "Cannot open ";
	errmsg += ioobj->fullUserExpr(true);
	return false;
    }

    const char* formatfilename = ioobj->pars()["Format file"];

    return tr->writer( hor, conn, formatfilename );
}


#define mWriteFormatRecord(nm)				\
    formatfile << lmkEMStickSetTranslator::nm##str << ' ' <<	\
                  nm##interval.start << ' ' << nm##interval.stop << '\n'


lmkEMStickSetReader::lmkEMStickSetReader( EM::StickSet& stickset_, Conn* conn_,
        const char* formatfilename )
    : Executor("Reading stickset ..." )
    , stickset( stickset_ )
    , conn( conn_ )
    , lastpt( -1 )
    , useinlcrl(false)
    , xinterval(-1,-1)
    , yinterval(-1,-1)
    , zinterval(-1,-1)
    , lineidinterval( -1, -1 )
    , traceinterval( -1, -1 )
    , pointtypeinterval(-1,-1)
    , domaininterval(-1,-1)
    , domainunitinterval(-1,-1)
    , distancuniteinterval(-1,-1)
{
    if ( !formatfilename )
    {
	error = true;
	return;
    }

    ifstream formatfile(formatfilename);
    while ( formatfile )
    {
	BufferString fieldname; Interval<int> rg;

	formatfile >> fieldname >> rg.start >> rg.stop;

	if ( !formatfile ) break;
	if ( fieldname==lmkEMStickSetTranslator::xstr )
	    xinterval = rg;
	else if ( fieldname==lmkEMStickSetTranslator::ystr )
	    yinterval = rg;
	else if ( fieldname==lmkEMStickSetTranslator::zstr )
	    zinterval = rg;
	else if ( fieldname==lmkEMStickSetTranslator::pointtypestr )
	    pointtypeinterval = rg;
	else if ( fieldname==lmkEMStickSetTranslator::domainstr )
	    domaininterval = rg;
	else if ( fieldname==lmkEMStickSetTranslator::domainunitstr )
	    domainunitinterval = rg;
	else if ( fieldname==lmkEMStickSetTranslator::distancunitestr )
	    distancuniteinterval = rg;
	else if ( fieldname==lmkEMStickSetTranslator::lineidstr )
	    lineidinterval = rg;
	else if ( fieldname==lmkEMStickSetTranslator::tracestr )
	    traceinterval = rg;
    }

    if ( (  xinterval.start==-1 || xinterval.stop==-1 ||
	    yinterval.start==-1 || yinterval.stop==-1 ) &&
	    lineidinterval.start!=-1 && lineidinterval.stop!=-1 &&
	    traceinterval.start!=-1 && traceinterval.stop!=-1 )
    {
	useinlcrl = true;
    }
    else if (   xinterval.start==-1 || xinterval.stop==-1 ||
		yinterval.start==-1 || yinterval.stop==-1 ||
		zinterval.start==-1 || zinterval.stop==-1 ||
		pointtypeinterval.start==-1 || pointtypeinterval.stop==-1 )
    {
	msg = lmkEMStickSetTranslator::xstr;
	msg += ", "; 
	msg += lmkEMStickSetTranslator::ystr;
	msg += ", "; 
	msg += lmkEMStickSetTranslator::zstr;
	msg += "and ";
	msg += lmkEMStickSetTranslator::pointtypestr;
	msg += " must be provided for reading";
	error = true;
	return;
    }

    if ( !conn->forRead() || !conn->hasClass(StreamConn::classid) )
    {
	msg = "Internal error: Bad connection";
	error = true;
	return;
    }

    error = false;
}


lmkEMStickSetReader::~lmkEMStickSetReader()
{ delete conn; }


int lmkEMStickSetReader::nextStep()
{
    if ( error ) return ErrorOccurred;

    istream& strm = ((StreamConn*)conn)->iStream();
    
    char buf[] = " ";
    BufferString buffer;

    while ( strm && buf[0]!='\n' )
    {
	strm.read(buf,1);
	buffer += buf;
    }

    if ( !strm )
    {
	return Finished;
    }

    int reqlen = mMAX(pointtypeinterval.stop, zinterval.stop );
    if ( useinlcrl )
    {
	reqlen = mMAX(reqlen, lineidinterval.stop );
	reqlen = mMAX(reqlen, traceinterval.stop );
    }
    else
    {
	reqlen = mMAX(reqlen, xinterval.stop );
	reqlen = mMAX(reqlen, yinterval.stop );
    }

    reqlen = mMAX(reqlen, domaininterval.stop );
    reqlen = mMAX(reqlen, domainunitinterval.stop );
    reqlen = mMAX(reqlen, distancuniteinterval.stop );

    if ( buffer.size()<reqlen ) { return ErrorOccurred; }

    Coord3 pos;

    if ( useinlcrl )
    {
	BufferString str(&buffer[lineidinterval.start-1]);
	str[lineidinterval.width()+1] = 0;
	int inl = atoi( str );
	
	str = &buffer[traceinterval.start-1];
	str[traceinterval.width()+1] = 0;
	int crl = atoi( str );

	Coord coord = SI().transform( BinID(inl, crl ));

	pos.y = coord.x;
	pos.x = coord.y;
    }
    else
    {
	BufferString str(&buffer[xinterval.start-1]);
	str[xinterval.width()+1] = 0;
	pos.x = atof( str );

	str = &buffer[yinterval.start-1];
	str[yinterval.width()+1] = 0;
	pos.y = atof( str );
    }

    BufferString str = &buffer[zinterval.start-1];
    str[zinterval.width()+1] = 0;
    pos.z = atof( str );

    str = &buffer[pointtypeinterval.start-1];
    str[pointtypeinterval.width()+1] = 0;
    int pt = atoi( str );

    if ( domainunitinterval.start!=-1 )
    {
	str = &buffer[domainunitinterval.start-1];
	str[domainunitinterval.width()+1] = 0;
	if ( str=="ms" )
	    pos.z /= 1000;
	else
	{
	    msg = "Unknown time unit";
	    return ErrorOccurred;
	}
    }
    else
    {
	pos.z /= 1000;
    }

    if ( domaininterval.start!=-1 )
    {
	str = &buffer[domaininterval.start-1];
	str[domaininterval.width()+1] = 0;
	if ( str != "TIME" && SI().zIsTime() 
		|| str=="TIME" && !SI().zIsTime() )
	{
	    msg = "Z domain is not equal to survey domain";
	    return ErrorOccurred;
	}
    }

    if ( pt==mLMK_START_PT && lastpt!=-1 && lastpt!=mLMK_END_PT )
	return ErrorOccurred;

    if ( lastpt==mLMK_END_PT )
    {
	if ( pt!=mLMK_START_PT )
	    return ErrorOccurred;

	currentstick = stickset.addStick(false);	
	currentknot = 0;
    }

    stickset.setPos( currentstick, currentknot++, pos, false );
    lastpt = pt;

    return MoreToDo;
}


const char* lmkEMStickSetReader::message() const
{
    return *msg ? (const char*) msg : "Reading StickSet";
}



Executor* lmkEMStickSetTranslator::reader( EM::StickSet& hor, Conn* conn,
					const char* formatfilename )
{
    return new lmkEMStickSetReader( hor, conn, formatfilename );
}


Executor* lmkEMStickSetTranslator::writer( const EM::StickSet& hor, Conn* conn,
					const char* formatfilename )
{
    return 0;
}

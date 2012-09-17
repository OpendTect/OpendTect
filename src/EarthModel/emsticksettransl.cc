/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: emsticksettransl.cc,v 1.12 2010/10/14 09:58:06 cvsbert Exp $";

#include "emsticksettransl.h"

#include "emstickset.h"
#include "emsurfaceio.h"
#include "executor.h"
#include "ptrman.h"
#include "position.h"
#include "survinfo.h"
#include "ioobj.h"
#include "ioman.h"
#include "iopar.h"
#include "ascstream.h"
#include "strmprov.h"
#include <iostream>


const char* EMStickSetTranslatorGroup::keyword = "StickSet";

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


mDefSimpleTranslatorSelector(EMStickSet,keyword)
mDefSimpleTranslatorioContext(EMStickSet,Surf)


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

    BufferString formatfilename( ioobj->pars()["Format file"] );
    if ( formatfilename.isEmpty() )
    {
	const char* deffilename = ((StreamConn*) conn)->fileName();
	formatfilename = deffilename;
	for ( int idx=strlen(deffilename)-1; idx>=0; idx-- )
	{
	    if ( deffilename[idx]=='.' )
	    {
		formatfilename[idx] = 0;
		break;
	    }
	}

	formatfilename += ".fault_fmt";
	ioobj->pars().set( "Format file", formatfilename );
	IOM().commitChanges( *ioobj );
    }

    return tr->writer( hor, conn, formatfilename.buf() );
}


#define mWriteFormatRecord(nm)				\
    formatfile << lmkEMStickSetTranslator::nm##str << ' ' <<	\
                  nm##interval.start << ' ' << nm##interval.stop << '\n'


lmkEMStickSetReader::lmkEMStickSetReader( EM::StickSet& stickset_, Conn* conn_,
        const char* formatfilename )
    : Executor("Reading stickset ..." )
    , stickset( stickset_ )
    , conn( conn_ )
    , lastpt( mLMK_END_PT )
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

    StreamData formatsd = StreamProvider( formatfilename ).makeIStream();
    while ( formatsd.istrm && *formatsd.istrm )
    {
	BufferString fieldname; Interval<int> rg;

	*formatsd.istrm >> fieldname >> rg.start >> rg.stop;
	if ( !*formatsd.istrm ) break;

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
    formatsd.close();

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

    if ( !conn->forRead() || !conn->isStream() )
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
    if ( error ) return ErrorOccurred();

    std::istream& strm = ((StreamConn*)conn)->iStream();
    
    char buf[] = " ";
    BufferString buffer;

    while ( strm && buf[0]!='\n' )
    {
	strm.read(buf,1);
	buffer += buf;
    }

    if ( !strm )
    {
	return Finished();
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

    if ( buffer.size()-1<reqlen ) { return ErrorOccurred(); }

    Coord3 pos;

    if ( useinlcrl )
    {
	BufferString str(&buffer[lineidinterval.start-1]);
	str[lineidinterval.width()+1] = 0;
	int inl = toInt( str );
	
	str = &buffer[traceinterval.start-1];
	str[traceinterval.width()+1] = 0;
	int crl = toInt( str );

	Coord coord = SI().transform( BinID(inl, crl ));

	pos.y = coord.x;
	pos.x = coord.y;
    }
    else
    {
	BufferString str(&buffer[xinterval.start-1]);
	str[xinterval.width()+1] = 0;
	pos.x = toDouble( str );

	str = &buffer[yinterval.start-1];
	str[yinterval.width()+1] = 0;
	pos.y = toDouble( str );
    }

    BufferString str = &buffer[zinterval.start-1];
    str[zinterval.width()+1] = 0;
    pos.z = toDouble( str );

    str = &buffer[pointtypeinterval.start-1];
    str[pointtypeinterval.width()+1] = 0;
    int pt = toInt( str );

    if ( domainunitinterval.start!=-1 )
    {
	str = &buffer[domainunitinterval.start-1];
	str[domainunitinterval.width()+1] = 0;
	removeTrailingBlanks( str.buf() );
	
	if ( str=="ms" )
	    pos.z /= 1000;
	else
	{
	    msg = "Unknown time unit";
	    return ErrorOccurred();
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
	removeTrailingBlanks( str.buf() );
	if ( str != "TIME" && SI().zIsTime() 
		|| str=="TIME" && !SI().zIsTime() )
	{
	    msg = "Z domain is not equal to survey domain";
	    return ErrorOccurred();
	}
    }

    if ( pt==mLMK_START_PT && lastpt!=-1 && lastpt!=mLMK_END_PT )
	return ErrorOccurred();

    if ( lastpt==mLMK_END_PT )
    {
	if ( pt!=mLMK_START_PT )
	    return ErrorOccurred();

	currentstick = stickset.addStick(false);	
	currentknot = 0;
    }

    stickset.setPos( currentstick, currentknot++, pos, false );
    lastpt = pt;

    return MoreToDo();
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


Executor* lmkEMStickSetTranslator::writer( const EM::StickSet& stickset,
					Conn* conn,
					const char* formatfilename )
{
    return new lmkEMStickSetWriter( stickset, conn, formatfilename );
}


lmkEMStickSetWriter::lmkEMStickSetWriter(const EM::StickSet& stickset_,
				     Conn* conn_, const char* formatfilename )
    : Executor("StickReader")
    , conn( conn_ )
    , stickset( stickset_ )
    , currentsticknr( 0 )
    , xinterval( 1, 10 )
    , yinterval( 11, 20 )
    , zinterval( 21, 30 )
    , domaininterval( 31, 40 )
    , domainunitinterval( 41, 45 )
    , distanceunitinterval( 46, 50 )
    , pointtypeinterval( 51, 51 )
{
    StreamData formatsd = StreamProvider( formatfilename ).makeOStream();
    if ( !formatsd.usable() )
	return;

    std::ostream& formatfile = *formatsd.ostrm;
    formatfile  << lmkEMStickSetTranslator::xstr
		<< '\t' << '\t' << '\t' << xinterval.start 
		<< '\t' << xinterval.stop << '\n';
    formatfile  << lmkEMStickSetTranslator::ystr
		<< '\t' << '\t' << '\t' << yinterval.start 
		<< '\t' << yinterval.stop << '\n';
    formatfile  << lmkEMStickSetTranslator::zstr
		<< '\t' << '\t' << '\t' << zinterval.start 
		<< '\t' << zinterval.stop << '\n';
    formatfile  << lmkEMStickSetTranslator::pointtypestr
		<< '\t' << '\t' << pointtypeinterval.start 
		<< '\t' << pointtypeinterval.stop << '\n';
    formatfile  << lmkEMStickSetTranslator::domainstr
		<< '\t' << '\t' << domaininterval.start 
		<< '\t' << domaininterval.stop << '\n';
    formatfile  << lmkEMStickSetTranslator::domainunitstr
		<< '\t' << domainunitinterval.start 
		<< '\t' << domainunitinterval.stop << '\n';
    formatfile  << lmkEMStickSetTranslator::distancunitestr
		<< '\t' << distanceunitinterval.start 
		<< '\t' << distanceunitinterval.stop << '\n';

    formatsd.close();
}


lmkEMStickSetWriter::~lmkEMStickSetWriter()
{ delete conn; }



int lmkEMStickSetWriter::nextStep()
{
    if ( !stickset.nrSticks() )
	return Finished();

    const EM::StickID stickid = stickset.stickID(currentsticknr);
    std::ostream& strm = ((StreamConn*)conn)->oStream();

    if ( stickset.nrKnots(stickid)>1 )
    {
	const EM::KnotID firstknot = stickset.firstKnot(stickid);
	BufferString buffer;

	Coord3 pos = stickset.getPos( stickid, firstknot );
	fillBuffer( buffer, pos, mLMK_START_PT );
	strm << buffer << '\n';

	for ( int idx=1; idx<stickset.nrKnots(stickid)-1; idx++ )
	{
	    pos = stickset.getPos( stickid, firstknot+idx );
	    fillBuffer( buffer, pos, mLMK_INTERMED_PT );
	    strm << buffer << '\n';
	}

	pos = stickset.getPos( stickid, firstknot+stickset.nrKnots(stickid)-1);
	fillBuffer( buffer, pos, mLMK_END_PT );
	strm << buffer << '\n';
    }

    currentsticknr++;
    if ( currentsticknr>=stickset.nrSticks() )
	return Finished();

    return MoreToDo();
}


void lmkEMStickSetWriter::fillBuffer( BufferString& buffer, const Coord3& pos,
				      int pt )
{
    int maxlen = 1;
    maxlen = mMAX(maxlen, xinterval.stop );
    maxlen = mMAX(maxlen, yinterval.stop );
    maxlen = mMAX(maxlen, zinterval.stop );
    maxlen = mMAX(maxlen, domaininterval.stop );
    maxlen = mMAX(maxlen, domainunitinterval.stop );
    maxlen = mMAX(maxlen, distanceunitinterval.stop );
    maxlen = mMAX(maxlen, pointtypeinterval.stop );

    buffer.setBufSize( maxlen+1 );

    for ( int idx=0; idx<buffer.bufSize(); idx++ )
    {
	buffer[idx] = (idx==buffer.bufSize()-1) ? 0 : ' ';
    }

    BufferString tmp = pos.x;
    memcpy( &buffer[xinterval.start-1], &tmp[0],
	    mMIN(tmp.size(),xinterval.width()+1));

    tmp = pos.y;
    memcpy( &buffer[yinterval.start-1], &tmp[0],
	    mMIN(tmp.size(),yinterval.width()+1));

    tmp = pos.z*1000;
    memcpy( &buffer[zinterval.start-1], &tmp[0],
	    mMIN(tmp.size(),zinterval.width()+1));

    tmp = pt;
    memcpy( &buffer[pointtypeinterval.start-1], &tmp[0],
	    mMIN(tmp.size(),pointtypeinterval.width()+1));

    tmp = "TIME";
    memcpy( &buffer[domaininterval.start-1], &tmp[0],
	    mMIN(tmp.size(),domaininterval.width()+1));

    tmp = "ms";
    memcpy( &buffer[domainunitinterval.start-1], &tmp[0],
	    mMIN(tmp.size(),domainunitinterval.width()+1));

    tmp = "m";
    memcpy( &buffer[distanceunitinterval.start-1], &tmp[0],
	    mMIN(tmp.size(),distanceunitinterval.width()+1));

    removeTrailingBlanks( buffer.buf() );
}





/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Oct 1999
________________________________________________________________________

-*/
static const char* rcsID = "$Id: lmkemfaulttransl.cc,v 1.8 2010/10/14 09:58:06 cvsbert Exp $";

#include "lmkemfaulttransl.h"

#include "emfault3d.h"
#include "executor.h"
#include "ptrman.h"
#include "position.h"
#include "survinfo.h"
#include "ioobj.h"
#include "iopar.h"
#include "ctxtioobj.h"
#include "ascstream.h"
#include "strmprov.h"
#include <iostream>


const char* lmkEMFault3DTranslator::pointtypestr() { return "FAULT_PTYPE"; }
const char* lmkEMFault3DTranslator::xstr()	   { return "FAULT_X"; }
const char* lmkEMFault3DTranslator::ystr()	   { return "FAULT_Y"; }
const char* lmkEMFault3DTranslator::zstr()	   { return "FAULT_Z"; }
const char* lmkEMFault3DTranslator::domainstr()	   { return "FAULT_DOMAIN"; }
const char* lmkEMFault3DTranslator::surveystr()	   { return "FAULT_SURVEY"; }
const char* lmkEMFault3DTranslator::domainunitstr()
				    { return "FAULT_DOMAIN_UNIT"; }
const char* lmkEMFault3DTranslator::distancunitestr()
				    { return "FAULT_DISTANCE_UNIT"; }
const char* lmkEMFault3DTranslator::lineidstr()	{ return "FAULT_LINEID"; }
const char* lmkEMFault3DTranslator::tracestr() 	{ return "FAULT_TRACE"; }


#define mWriteFormatRecord(nm)				\
    formatfile << lmkEMFault3DTranslator::nm##str << ' ' <<	\
                  nm##interval.start << ' ' << nm##interval.stop << '\n'


lmkEMFault3DReader::lmkEMFault3DReader( EM::Fault3D& fault_, Conn* conn_,
        const char* formatfilename )
    : Executor("Reading fault ..." )
    , fault( fault_ )
    , conn( conn_ )
    , lastpt( -1 )
    , useinlcrl(false)
    , lastnode(0,0)
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

	if ( fieldname==lmkEMFault3DTranslator::xstr() )
	    xinterval = rg;
	else if ( fieldname==lmkEMFault3DTranslator::ystr() )
	    yinterval = rg;
	else if ( fieldname==lmkEMFault3DTranslator::zstr() )
	    zinterval = rg;
	else if ( fieldname==lmkEMFault3DTranslator::pointtypestr() )
	    pointtypeinterval = rg;
	else if ( fieldname==lmkEMFault3DTranslator::domainstr() )
	    domaininterval = rg;
	else if ( fieldname==lmkEMFault3DTranslator::domainunitstr() )
	    domainunitinterval = rg;
	else if ( fieldname==lmkEMFault3DTranslator::distancunitestr() )
	    distancuniteinterval = rg;
	else if ( fieldname==lmkEMFault3DTranslator::lineidstr() )
	    lineidinterval = rg;
	else if ( fieldname==lmkEMFault3DTranslator::tracestr() )
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
	msg = lmkEMFault3DTranslator::xstr();
	msg += ", "; 
	msg += lmkEMFault3DTranslator::ystr();
	msg += ", "; 
	msg += lmkEMFault3DTranslator::zstr();
	msg += "and ";
	msg += lmkEMFault3DTranslator::pointtypestr();
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


lmkEMFault3DReader::~lmkEMFault3DReader()
{ delete conn; }


int lmkEMFault3DReader::nextStep()
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

    if ( buffer.size()<reqlen ) { return ErrorOccurred(); }

    Coord3 pos;

    if ( useinlcrl )
    {
	BufferString str(&buffer[lineidinterval.start-1]);
	str[lineidinterval.width()+1] = 0;
	int inl = toInt( str.buf() );
	
	str = &buffer[traceinterval.start-1];
	str[traceinterval.width()+1] = 0;
	int crl = toInt( str.buf() );

	Coord coord = SI().transform( BinID(inl, crl ));

	pos.y = coord.x;
	pos.x = coord.y;
    }
    else
    {
	BufferString str(&buffer[xinterval.start-1]);
	str[xinterval.width()+1] = 0;
	pos.x = toDouble( str.buf() );

	str = &buffer[yinterval.start-1];
	str[yinterval.width()+1] = 0;
	pos.y = toDouble( str.buf() );
    }

    BufferString str = &buffer[zinterval.start-1];
    str[zinterval.width()+1] = 0;
    pos.z = toDouble( str.buf() );

    str = &buffer[pointtypeinterval.start-1];
    str[pointtypeinterval.width()+1] = 0;
    int pt = toInt( str.buf() );

    float zfac = 0.001;
    if ( domainunitinterval.start != -1 )
    {
	str = &buffer[domainunitinterval.start-1];
	str[domainunitinterval.width()+1] = '\0';
	if ( str == "m" )
	    zfac = 1;
	else if ( str == "ft" )
	    zfac = mFromFeetFactor;
	else if ( str != "ms" )
	    { msg = "Unknown time unit"; return ErrorOccurred(); }

	str = &buffer[domaininterval.start-1];
	str[domaininterval.width()+1] = 0;
	const bool ist = str == "TIME";
	if ( (ist && SI().zIsTime()) || (ist && !SI().zIsTime()) )
	{
	    msg = "Z domain is not equal to survey domain";
	    return ErrorOccurred();
	}
    }
    pos.z *= zfac;

    RowCol newnode( lastnode.row, lastnode.col+1 );
    if ( pt==mLMK_START_PT && lastpt!=-1 && lastpt!=mLMK_END_PT )
	return ErrorOccurred();

    if ( lastpt==mLMK_END_PT )
    {
	if ( pt!=mLMK_START_PT )
	    return ErrorOccurred();

	newnode.row++; newnode.col = 0;
	fault.geometry().insertStick( 0, newnode.row, newnode.col, pos,
			Coord3(0,1,0), false );
    }
    else
    {
	const EM::SubID subid( newnode.toInt64() );
	fault.geometry().insertKnot( 0, subid, pos, false );
    }

    lastnode = newnode;
    lastpt = pt;

    return MoreToDo();
}


const char* lmkEMFault3DReader::message() const
{
    return msg.isEmpty() ? "Reading Fault" : msg.buf();
}


lmkEMFault3DTranslator::~lmkEMFault3DTranslator()
{
}



Executor* lmkEMFault3DTranslator::reader( EM::Fault3D& hor, Conn* conn,
					const char* formatfilename )
{
    return new lmkEMFault3DReader( hor, conn, formatfilename );
}


Executor* lmkEMFault3DTranslator::writer( const EM::Fault3D& hor, Conn* conn,
					const char* formatfilename )
{
    return 0;
}

/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahk
 * DATE     : Jun 2003
___________________________________________________________________

-*/

static const char* rcsID = "$Id: emsurfauxdataio.cc,v 1.22 2005-01-17 16:27:02 nanne Exp $";

#include "emsurfauxdataio.h"

#include "ascstream.h"
#include "datainterp.h"
#include "datachar.h"
#include "emsurface.h"
#include "emsurfacegeometry.h"
#include "emsurfaceauxdata.h"
#include "geommeshsurface.h"
#include "iopar.h"
#include "survinfo.h"
#include "strmprov.h"
#include "binidselimpl.h"
#include "filegen.h"
#include <iostream>

const char* EM::dgbSurfDataWriter::attrnmstr = "Attribute";
const char* EM::dgbSurfDataWriter::infostr = "Info";
const char* EM::dgbSurfDataWriter::intdatacharstr = "Int data";
const char* EM::dgbSurfDataWriter::int64datacharstr = "Long long data";
const char* EM::dgbSurfDataWriter::floatdatacharstr = "Float data";
const char* EM::dgbSurfDataWriter::filetypestr = "Surface aux data";
const char* EM::dgbSurfDataWriter::shiftstr = "Shift";


EM::dgbSurfDataWriter::dgbSurfDataWriter( const EM::Surface& surf_,int dataidx_,
				    const BinIDSampler* sel_, bool binary_,
       				    const char* fnm_ )
    : Executor("Aux data writer")
    , stream(0)
    , chunksize(100)
    , dataidx(dataidx_)
    , surf(surf_)
    , sel(sel_)
    , sectionindex(0)
    , binary(binary_)
    , nrdone(0)
    , filename(fnm_)
{
    IOPar par( "Surface Data" );
    par.set( attrnmstr, surf.auxdata.auxDataName(dataidx) );
    par.set( shiftstr, surf.geometry.getShift() );

    if ( binary )
    {
	BufferString dc;

	int idummy;
	DataCharacteristics(idummy).toString( dc.buf() );
	par.set( intdatacharstr, dc );

	int64 lldummy;
	DataCharacteristics(lldummy).toString( dc.buf() );
	par.set( int64datacharstr, dc );

	float fdummy;
	DataCharacteristics(fdummy).toString( dc.buf() );
	par.set( floatdatacharstr, dc );
    }

    StreamData sd = StreamProvider( filename ).makeOStream();
    if ( !sd.usable() ) return;
    stream = sd.ostrm;
    if ( !(*stream) ) return;

    ascostream astream( *stream );
    astream.putHeader( filetypestr );
    par.putTo( astream );

    int nrnodes = 0;
    for ( int idx=0; idx<surf.geometry.nrSections(); idx++ )
    {
	EM::SectionID sectionid = surf.geometry.sectionID(idx);
	const Geometry::MeshSurface* meshsurf =
			surf.geometry.getSurface(sectionid);

	nrnodes += meshsurf->size();
    }

    totalnr = 100;
    chunksize = nrnodes/totalnr+1;
    if ( chunksize < 100 )
    {
	chunksize = 100;
	totalnr = nrnodes/chunksize+1;
    }
}


EM::dgbSurfDataWriter::~dgbSurfDataWriter()
{
    delete stream;
}


#define mErrRetWrite(msg) \
{ errmsg = msg; File_remove(filename.buf(),0); return ErrorOccurred; }


int EM::dgbSurfDataWriter::nextStep()
{
    EM::PosID posid( surf.id() );
    for ( int idx=0; idx<chunksize; idx++ )
    {
	while ( !subids.size() )
	{
	    if ( nrdone )
	    {
		sectionindex++;
		if ( sectionindex >= surf.geometry.nrSections() )
		    return Finished;
	    }
	    else
	    {
		if ( !writeInt(surf.geometry.nrSections()) )
		    mErrRetWrite("Error in writing data information")
	    }

	    const EM::SectionID sectionid = 
					surf.geometry.sectionID( sectionindex );
	    const Geometry::MeshSurface* meshsurf = 
					surf.geometry.getSurface( sectionid );

	    const int nrnodes = meshsurf->size();
	    for ( int idy=0; idy<nrnodes; idy++ )
	    {
		const Geometry::PosID geomposid = meshsurf->getPosID(idy);
		const Coord3 coord = meshsurf->getPos( geomposid );

		const BinID bid = SI().transform(coord);
		if ( sel && sel->excludes(bid) )
		    continue;

		const RowCol emrc( bid.inl, bid.crl );
		const EM::SubID subid = surf.geometry.rowCol2SubID( emrc );
		posid.setSubID( subid );
		posid.setSectionID( sectionid );
		const float auxvalue = 
		    		surf.auxdata.getAuxDataVal(dataidx,posid);
		if ( mIsUndefined( auxvalue ) )
		    continue;

		subids += subid;
		values += auxvalue;
	    }

	    if ( !subids.size() )
		mErrRetWrite("No data available for this surface")

	    if ( !writeInt(sectionid) || !writeInt(subids.size()) )
		mErrRetWrite("Error in writing data information")
	}

	const int subidindex = subids.size()-1;
	const EM::SubID subid = subids[subidindex];
	const float auxvalue = values[subidindex];

	if ( !writeInt64(subid) || !writeFloat(auxvalue) )
	    mErrRetWrite("Error in writing datavalues")

	subids.remove( subidindex );
	values.remove( subidindex );
    }

    nrdone++;
    return MoreToDo;
}


BufferString EM::dgbSurfDataWriter::createHovName( const char* base, int idx )
{
        BufferString res( base );
	res += "^"; res += idx; res += ".hov";
	return res;
}


bool EM::dgbSurfDataWriter::writeInt( int val )
{
    if ( binary )
	stream->write( (char*) &val, sizeof(val) );
    else
	(*stream) << val << '\n' ;

    return (*stream);
}


bool EM::dgbSurfDataWriter::writeInt64( int64 val )
{
    if ( binary )
	stream->write( (char*) &val, sizeof(val) );
    else
	(*stream) << val << '\n' ;

    return (*stream);
}


bool EM::dgbSurfDataWriter::writeFloat( float val )
{
    if ( binary )
	stream->write( (char*) &val ,sizeof(val) );
    else
	(*stream) << val << '\n';

    return (*stream);
}


int EM::dgbSurfDataWriter::nrDone() const 
{ return nrdone; }


int EM::dgbSurfDataWriter::totalNr() const
{ return totalnr; }


const char* EM::dgbSurfDataWriter::message() const
{ return errmsg; }



// Reader +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

EM::dgbSurfDataReader::dgbSurfDataReader( const char* filename )
    : Executor( "Aux data reader" )
    , intinterpreter( 0 )
    , int64interpreter( 0 )
    , floatinterpreter( 0 )
    , chunksize( 100 )
    , dataidx( -1 )
    , surf( 0 )
    , sectionindex( 0 )
    , error( true )
    , nrdone(0)
    , valsleftonsection(0)
    , shift(0)
    , stream(0)
{
    StreamData sd = StreamProvider( filename ).makeIStream();
    if ( !sd.usable() )
	return;
    stream = sd.istrm;
    if ( !(*stream) )
	return;

    ascistream astream( *stream );
    if ( !astream.isOfFileType(dgbSurfDataWriter::filetypestr) )
        return;

    const IOPar par( astream );
    if ( !par.get(dgbSurfDataWriter::attrnmstr,dataname) )
	return;

    if ( !par.get(dgbSurfDataWriter::attrnmstr,datainfo) )
	return;

    par.get( dgbSurfDataWriter::shiftstr, shift );

    BufferString dc;
    if ( par.get(EM::dgbSurfDataWriter::intdatacharstr,dc) )
    {
	DataCharacteristics writtendatachar;
	writtendatachar.set( dc.buf() );
	intinterpreter = new DataInterpreter<int>( writtendatachar );

	if ( !par.get(EM::dgbSurfDataWriter::int64datacharstr,dc) )
	{
	    error = true; 
	    errmsg = "Error in reading data characteristics (int64)";
	    return;
	}
	writtendatachar.set( dc.buf() );
	int64interpreter = new DataInterpreter<int64>( writtendatachar );
					     
	if ( !par.get(EM::dgbSurfDataWriter::floatdatacharstr,dc) )
	{
	    error = true;
	    errmsg = "Error in reading data characteristics (float)"; 
	    return;
	}
	writtendatachar.set( dc.buf() );
	floatinterpreter = new DataInterpreter<float>( writtendatachar );
    }

    error = false;
}


EM::dgbSurfDataReader::~dgbSurfDataReader()
{
    delete stream;
    delete intinterpreter;
    delete int64interpreter;
    delete floatinterpreter;
}


const char* EM::dgbSurfDataReader::dataName() const
{
    return dataname[0] ? dataname.buf() : 0;
}


const char* EM::dgbSurfDataReader::dataInfo() const
{
    return datainfo[0] ? datainfo.buf() : 0;
}



void EM::dgbSurfDataReader::setSurface( EM::Surface& surf_ )
{
    surf = &surf_;
    dataidx = surf->auxdata.addAuxData( dataname );
    surf->geometry.setShift( shift );
}


#define mErrRetRead(msg) { \
    if ( msg ) errmsg = msg; \
    surf->auxdata.removeAuxData(dataidx); return ErrorOccurred; }

int EM::dgbSurfDataReader::nextStep()
{
    if ( error ) mErrRetRead(0)

    EM::PosID posid( surf->id() );
    for ( int idx=0; idx<chunksize; idx++ )
    {
	while ( !valsleftonsection )
	{
	    if ( nrdone )
	    {
		sectionindex++;
		if ( sectionindex>=nrsections )
		    return Finished;
	    }
	    else
	    {
		if ( !readInt(nrsections) )
		    mErrRetRead( "Error in reading data information" )
	    }

	    int cp;
	    if ( !readInt(cp) || !readInt(valsleftonsection) )
		mErrRetRead( "Error in reading data information" )

	    currentsection = cp;
	    totalnr = 100;
	    chunksize = valsleftonsection/totalnr+1;
	    if ( chunksize < 100 )
	    {
		chunksize = 100;
		totalnr = valsleftonsection/chunksize+1;
	    }
	}

	EM::SubID subid;
	float val;
	if ( !readInt64(subid) || !readFloat(val) )
	    mErrRetRead( "Error in reading data values" )

	posid.setSubID( subid );
	posid.setSectionID( currentsection );
	surf->auxdata.setAuxDataVal( dataidx, posid, val );

	valsleftonsection--;
    }

    nrdone++;
    return MoreToDo;
}


static int sizeofint = sizeof(int);
static int sizeofint64 = sizeof(int64);
static int sizeoffloat = sizeof(float);

#define mReadData(interpreter,size) \
    if ( interpreter ) \
    { \
	char buf[sizeof(res)]; \
	stream->read(buf,sizeof(res)); \
	res = interpreter->get(buf,0); \
    } \
    else \
	(*stream) >> res; \
\
    return (*stream);

bool EM::dgbSurfDataReader::readInt( int& res )
{ mReadData(intinterpreter,sizeofint) }

bool EM::dgbSurfDataReader::readInt64( int64& res )
{ mReadData(int64interpreter,sizeofint64) }

bool EM::dgbSurfDataReader::readFloat( float& res )
{ mReadData(floatinterpreter,sizeoffloat) }


int EM::dgbSurfDataReader::nrDone() const 
{ return nrdone; }


int EM::dgbSurfDataReader::totalNr() const
{ return totalnr; }


const char* EM::dgbSurfDataReader::message() const
{ return errmsg; }

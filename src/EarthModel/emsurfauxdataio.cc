/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahk
 * DATE     : Jun 2003
___________________________________________________________________

-*/

static const char* rcsID = "$Id: emsurfauxdataio.cc,v 1.14 2003-12-15 16:02:45 nanne Exp $";

#include "emsurfauxdataio.h"

#include "ascstream.h"
#include "datainterp.h"
#include "datachar.h"
#include "emsurface.h"
#include "geommeshsurface.h"
#include "iopar.h"
#include "survinfo.h"
#include "strmprov.h"
#include <iostream>

const char* EM::dgbSurfDataWriter::attrnmstr = "Attribute";
const char* EM::dgbSurfDataWriter::infostr = "Info";
const char* EM::dgbSurfDataWriter::intdatacharstr = "Int data";
const char* EM::dgbSurfDataWriter::longlongdatacharstr = "Long long data";
const char* EM::dgbSurfDataWriter::floatdatacharstr = "Float data";
const char* EM::dgbSurfDataWriter::filetypestr = "Surface aux data";
const char* EM::dgbSurfDataWriter::shiftstr = "Shift";


EM::dgbSurfDataWriter::dgbSurfDataWriter( const EM::Surface& surf_,int dataidx_,
				    const BinIDSampler* sel_, bool binary_,
       				    const char* filename )
    : Executor( "Aux data writer" )
    , stream( 0 )
    , chunksize( 100 )
    , dataidx( dataidx_ )
    , surf( surf_ )
    , sel( sel_ )
    , patchindex( 0 )
    , binary( binary_ )
    , nrdone(0)
{
    IOPar par( "Surface Data" );
    par.set( attrnmstr, surf.auxDataName(dataidx) );
    par.set( shiftstr, surf.getShift() );

    if ( binary )
    {
	BufferString dc;

	int idummy;
	DataCharacteristics(idummy).toString( dc.buf() );
	par.set( intdatacharstr, dc );

	long long lldummy;
	DataCharacteristics(lldummy).toString( dc.buf() );
	par.set( longlongdatacharstr, dc );

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
    for ( int idx=0; idx<surf.nrPatches(); idx++ )
    {
	EM::PatchID patchid = surf.patchID(idx);
	const Geometry::MeshSurface* meshsurf = surf.getSurface(patchid);

	nrnodes += meshsurf->size();
    }

    totalnr = nrnodes/chunksize+1;
}


EM::dgbSurfDataWriter::~dgbSurfDataWriter()
{
    delete stream;
}


int EM::dgbSurfDataWriter::nextStep()
{
    EM::PosID posid( surf.id() );
    for ( int idx=0; idx<chunksize; idx++ )
    {
	while ( !subids.size() )
	{
	    if ( nrdone )
	    {
		patchindex++;
		if ( patchindex>=surf.nrPatches() )
		    return Finished;
	    }
	    else
	    {
		if ( !writeInt(surf.nrPatches()))
		    return ErrorOccurred;
	    }


	    const EM::PatchID patchid = surf.patchID( patchindex );
	    const Geometry::MeshSurface* meshsurf = surf.getSurface(patchid);

	    const int nrnodes = meshsurf->size();
	    for ( int idy=0; idy<nrnodes; idy++ )
	    {
		const Geometry::PosID geomposid = meshsurf->getPosID(idy);
		const Coord3 coord = meshsurf->getPos( geomposid );

		const BinID bid = SI().transform(coord);
		if ( sel && sel->excludes(bid) )
		    continue;

		const RowCol emrc( bid.inl, bid.crl );
		const EM::SubID subid = surf.rowCol2SubID( emrc );
		posid.setSubID( subid );
		posid.setPatchID( patchid );
		const float auxvalue = surf.getAuxDataVal(dataidx,posid);
		if ( mIsUndefined( auxvalue ) )
		    continue;

		subids += subid;
		values += auxvalue;
	    }

	    if ( !writeInt( patchid ) || !writeInt(subids.size()))
		return ErrorOccurred;
	}

	const int subidindex = subids.size()-1;
	const EM::SubID subid = subids[subidindex];
	const float auxvalue = values[subidindex];

	if ( !writeLongLong(subid) || !writeFloat(auxvalue) )
	    return ErrorOccurred;

	subids.remove( subidindex );
	values.remove( subidindex );
    }

    nrdone++;
    return MoreToDo;
}


int EM::dgbSurfDataWriter::nrDone() const 
{
    return nrdone;
}


int EM::dgbSurfDataWriter::totalNr() const
{
    return totalnr;
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


bool EM::dgbSurfDataWriter::writeLongLong( long long val )
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


EM::dgbSurfDataReader::dgbSurfDataReader( const char* filename )
    : Executor( "Aux data reader" )
    , intinterpreter( 0 )
    , longlonginterpreter( 0 )
    , floatinterpreter( 0 )
    , chunksize( 100 )
    , dataidx( -1 )
    , surf( 0 )
    , patchindex( 0 )
    , error( true )
    , nrdone(0)
    , valsleftonpatch(0)
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

	if ( !par.get(EM::dgbSurfDataWriter::longlongdatacharstr,dc) )
	{ error = true; return; }
	writtendatachar.set( dc.buf() );
	longlonginterpreter = new DataInterpreter<long long>( writtendatachar );
					     
	if ( !par.get(EM::dgbSurfDataWriter::floatdatacharstr,dc) )
	{ error = true; return; }
	writtendatachar.set( dc.buf() );
	floatinterpreter = new DataInterpreter<float>( writtendatachar );
    }

    error = false;
}


EM::dgbSurfDataReader::~dgbSurfDataReader()
{
    delete stream;
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
    dataidx = surf->addAuxData( dataname );
    surf->setShift( shift );
}


int EM::dgbSurfDataReader::nextStep()
{
    if ( error )
	return ErrorOccurred; 

    EM::PosID posid( surf->id() );
    for ( int idx=0; idx<chunksize; idx++ )
    {
	while ( !valsleftonpatch )
	{
	    if ( nrdone )
	    {
		patchindex++;
		if ( patchindex>=nrpatches )
		    return Finished;
	    }
	    else
	    {
		if ( !readInt(nrpatches) )
		    return ErrorOccurred;
	    }

	    int cp;
	    if ( !readInt(cp) || !readInt(valsleftonpatch))
		return ErrorOccurred;

	    currentpatch = cp;
	    totalnr = valsleftonpatch/chunksize+1;
	}

	EM::SubID subid;
	float val;
	if ( !readLongLong(subid) || !readFloat(val) )
	    return ErrorOccurred;

	posid.setSubID( subid );
	posid.setPatchID( currentpatch );
	surf->setAuxDataVal( dataidx, posid, val );

	valsleftonpatch--;
    }

    nrdone++;
    return MoreToDo;
}


int EM::dgbSurfDataReader::nrDone() const 
{
    return nrdone;
}


int EM::dgbSurfDataReader::totalNr() const
{
    return totalnr;
}

bool EM::dgbSurfDataReader::readInt( int& res )
{
    if ( intinterpreter )
    {
	char buf[sizeof(res)];
	stream->read(buf,sizeof(res));
	res = intinterpreter->get(buf,0);
    }
    else
	(*stream) >> res;

    return (*stream);
}


bool EM::dgbSurfDataReader::readLongLong( long long& res )
{
    if ( longlonginterpreter )
    {
	char buf[sizeof(res)];
	stream->read(buf,sizeof(res));
	res = longlonginterpreter->get(buf,0);
    }
    else
	(*stream) >> res;

    return (*stream);
}


bool EM::dgbSurfDataReader::readFloat(float& res)
{
    if ( floatinterpreter )
    {
	char buf[sizeof(res)];
	stream->read(buf,sizeof(res));
	res = floatinterpreter->get(buf,0);
    }
    else
	(*stream) >> res;

    return (*stream);
}

/*
___________________________________________________________________

 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahk
 * DATE     : Jun 2003
___________________________________________________________________

-*/

#include "emsurfauxdataio.h"

#include "ascstream.h"
#include "datainterp.h"
#include "emsurface.h"
#include "geomgridsurface.h"
#include "iopar.h"
#include "survinfo.h"

#include <fstream>

static const char* rcsID = "$Id: emsurfauxdataio.cc,v 1.1 2003-06-17 13:39:12 kristofer Exp $";

const char* EM::dgbSurfDataWriter::attrnmstr = "Attribute";
const char* EM::dgbSurfDataWriter::binarystr = "Binary";


EM::dgbSurfDataWriter::dgbSurfDataWriter( const EM::Surface& surf_,int dataidx_,
				    const BinIDSampler* sel_, bool binary,
       				    const char* filename )
    : Executor( "Aux data writer" )
    , stream( 0 )
    , subidinterpreter( 0 )
    , datainterpreter( 0 )
    , chunksize( 100 )
    , dataidx( dataidx_ )
    , surf( surf_ )
    , sel( sel_ )
    , patchindex( 0 )
{
    IOPar par;
    par.set( attrnmstr, surf.auxDataName(dataidx) );
    par.setYN( binarystr, binary );

    if ( binary )
    {
	//TODO: Put this machine's Datachars into the par and create the
	// 	interpreters.
    }

    stream = binary ? new ofstream( filename, ios::binary )
		    : new ofstream( filename);

    if ( !(*stream) ) return;

    ascostream astream( *stream );

    par.putTo( astream );

    int nrnodes = 0;
    for ( int idx=0; idx<surf.nrPatches(); idx++ )
    {
	EM::PatchID patchid = surf.patchID(idx);
	const Geometry::GridSurface* gridsurf = surf.getSurface(patchid);

	nrnodes += gridsurf->size();
    }

    totalnr = nrnodes/chunksize+1;
}


EM::dgbSurfDataWriter::~dgbSurfDataWriter()
{
    stream->close();
    delete stream;
    delete subidinterpreter;
    delete datainterpreter;
}


int EM::dgbSurfDataWriter::nextStep()
{
    for ( int idx=0; idx<chunksize; idx++ )
    {
	if ( !subids.size() )
	{
	    if ( nrdone )
	    {
		patchindex++;
		if ( patchindex>=surf.nrPatches() )
		    return Finished;
	    }

	    const EM::PatchID patchid = surf.patchID( patchindex );
	    const Geometry::GridSurface* gridsurf = surf.getSurface(patchid);

	    const int nrnodes = gridsurf->size();
	    for ( int idy=0; idy<nrnodes; idx++ )
	    {
		EM::SubID subid = gridsurf->getPosID(idy);
		Coord3 coord = gridsurf->getPos( subid );

		if ( sel && sel->excludes(SI().transform(coord)) )
		    continue;

		const EM::PosID posid( surf.id(), patchid, subid);
		const float auxvalue = surf.getAuxDataVal(dataidx,posid);
		if ( mIsUndefined( auxvalue ) )
		    continue;

		subids += subid;
		values += auxvalue;
	    }

	    if ( subidinterpreter )
	    {
		char intbuf[sizeof(int)];
		subidinterpreter->put( intbuf, 1, (int) patchid );
		stream->write( intbuf, sizeof(int));

		subidinterpreter->put( intbuf, 1, subids.size() );
		stream->write( intbuf, sizeof(int));
	    }
	    else
	    {
		(*stream) << patchid << '\t' << subids.size()<< '\n';
	    }
	}

	const int subidindex = subids.size()-1;
	const EM::SubID subid = subids[subidindex];
	const float auxvalue = values[subidindex];

	if ( subidinterpreter )
	{
	    char intbuf[sizeof(int)];
	    subidinterpreter->put( intbuf, 1, (int) subid );
	    stream->write( intbuf, sizeof(int));

	    char floatbuf[sizeof(float)];
	    datainterpreter->put( floatbuf, 1, (float) auxvalue );
	    stream->write( floatbuf, sizeof(float));
	}
	else
	{
	    (*stream) << subid << '\t' << auxvalue << '\n';
	}

	subids.remove( subidindex );
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


EM::dgbSurfDataReader::dgbSurfDataReader( const char* filename )
    : Executor( "Aux data writer" )
    , stream( new ifstream(filename) )
    , subidinterpreter( 0 )
    , datainterpreter( 0 )
    , chunksize( 100 )
    , dataidx( -1 )
    , surf( 0 )
    , patchindex( 0 )
{
    if ( !(*stream) )
	return;

    ascistream astream( *stream );
    const IOPar par( astream );
    if ( !par.get( dgbSurfDataWriter::attrnmstr, dataname ) )
	return;

    bool binary;
    par.getYN( dgbSurfDataWriter::binarystr, binary );
}


EM::dgbSurfDataReader::~dgbSurfDataReader()
{
    stream->close();
    delete stream;
}


const char* EM::dgbSurfDataReader::dataName() const
{
    return dataname[0] ? dataname : 0;
}



void EM::dgbSurfDataReader::setSurface( EM::Surface& surf_ )
{
    surf = & surf_;
}


int EM::dgbSurfDataReader::nextStep()
{
    for ( int idx=0; idx<chunksize; idx++ )
    {
	if ( !subids.size() )
	{
	    if ( nrdone )
	    {
		patchindex++;
		if ( patchindex>=surf->nrPatches() )
		    return Finished;
	    }

	    const EM::PatchID patchid = surf->patchID( patchindex );
	    const Geometry::GridSurface* gridsurf = surf->getSurface(patchid);

	    const int nrnodes = gridsurf->size();
	    for ( int idy=0; idy<nrnodes; idx++ )
	    {
		EM::SubID subid = gridsurf->getPosID(idy);
		Coord3 coord = gridsurf->getPos( subid );

		if ( sel && sel->excludes(SI().transform(coord)) )
		    continue;

		const EM::PosID posid( surf->id(), patchid, subid);
		const float auxvalue = surf->getAuxDataVal(dataidx,posid);
		if ( mIsUndefined( auxvalue ) )
		    continue;

		subids += subid;
		values += auxvalue;
	    }

	    if ( subidinterpreter )
	    {
		char intbuf[sizeof(int)];
		subidinterpreter->put( intbuf, 1, (int) patchid );
		//stream->write( intbuf, sizeof(int));

		subidinterpreter->put( intbuf, 1, subids.size() );
		//stream->write( intbuf, sizeof(int));
	    }
	    else
	    {
		//(*stream) << patchid << '\t' << subids.size()<< '\n';
	    }
	}

	const int subidindex = subids.size()-1;
	const EM::SubID subid = subids[subidindex];
	const float auxvalue = values[subidindex];

	if ( subidinterpreter )
	{
	    char intbuf[sizeof(int)];
	    subidinterpreter->put( intbuf, 1, (int) subid );
	//    stream->write( intbuf, sizeof(int));

	    char floatbuf[sizeof(float)];
	    datainterpreter->put( floatbuf, 1, (float) auxvalue );
	 //   stream->write( floatbuf, sizeof(float));
	}
	else
	{
	  //  (*stream) << subid << '\t' << auxvalue << '\n';
	}

	subids.remove( subidindex );
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

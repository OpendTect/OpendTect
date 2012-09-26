/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Jun 2003
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "emsurfauxdataio.h"

#include "ascstream.h"
#include "cubesampling.h"
#include "datachar.h"
#include "datainterp.h"
#include "emhorizon3d.h"
#include "emsurfaceauxdata.h"
#include "emsurfacegeometry.h"
#include "file.h"
#include "iopar.h"
#include "parametricsurface.h"
#include "strmprov.h"
#include "survinfo.h"

#include <iostream>

namespace EM
{

const char* dgbSurfDataWriter::sKeyAttrName()	    { return "Attribute"; }
const char* dgbSurfDataWriter::sKeyIntDataChar()    { return "Int data"; }
const char* dgbSurfDataWriter::sKeyInt64DataChar()  { return "Long long data"; }
const char* dgbSurfDataWriter::sKeyFloatDataChar()  { return "Float data"; }
const char* dgbSurfDataWriter::sKeyFileType() 	    {return "Surface aux data";}
const char* dgbSurfDataWriter::sKeyShift() 	    { return "Shift"; }


dgbSurfDataWriter::dgbSurfDataWriter( const Horizon3D& surf,int dataidx,
				    const HorSampling* sel, bool binary,
       				    const char* filename )
    : Executor("Aux data writer")
    , stream_(0)
    , chunksize_(100)
    , dataidx_(dataidx)
    , surf_(surf)
    , sel_(sel)
    , sectionindex_(0)
    , binary_(binary)
    , nrdone_(0)
    , filename_(filename)
{
    IOPar par( "Surface Data" );
    par.set( sKeyAttrName(), surf.auxdata.auxDataName(dataidx_) );
    par.set( sKeyShift(), surf.auxdata.auxDataShift(dataidx_) );

    if ( binary_ )
    {
	BufferString dc;

	int idummy;
	DataCharacteristics(idummy).toString( dc.buf() );
	par.set( sKeyIntDataChar(), dc );

	od_int64 lldummy;
	DataCharacteristics(lldummy).toString( dc.buf() );
	par.set( sKeyInt64DataChar(), dc );

	float fdummy;
	DataCharacteristics(fdummy).toString( dc.buf() );
	par.set( sKeyFloatDataChar(), dc );
    }

    StreamData sd = StreamProvider( filename_.buf() ).makeOStream();
    if ( !sd.usable() ) return;
    stream_ = sd.ostrm;
    if ( !(*stream_) ) return;

    ascostream astream( *stream_ );
    astream.putHeader( sKeyFileType() );
    par.putTo( astream );

    int nrnodes = 0;
    for ( int idx=0; idx<surf.nrSections(); idx++ )
    {
	SectionID sectionid = surf.sectionID(idx);
	const Geometry::BinIDSurface* meshsurf =
			surf.geometry().sectionGeometry(sectionid);
	nrnodes += meshsurf->nrKnots();
    }

    totalnr_ = 100;
    chunksize_ = nrnodes/totalnr_+1;
    if ( chunksize_ < 100 )
    {
	chunksize_ = 100;
	totalnr_ = nrnodes/chunksize_+1;
    }
}


dgbSurfDataWriter::~dgbSurfDataWriter()
{
    delete stream_;
}


bool dgbSurfDataWriter::writeDummyHeader( const char* fnm, const char* attrnm )
{
    StreamData sd = StreamProvider( fnm ).makeOStream();
    if ( !sd.usable() )
	return false;

    ascostream astream( *sd.ostrm );
    astream.putHeader( dgbSurfDataWriter::sKeyFileType() );
    IOPar par( "Surface Data" );
    par.set( dgbSurfDataWriter::sKeyAttrName(), attrnm );
    par.putTo( astream );
    sd.close();
    return true;
}


#define mErrRetWrite(msg) \
{ errmsg_ = msg; File::remove(filename_.buf()); \
    return ErrorOccurred(); }


int dgbSurfDataWriter::nextStep()
{
    PosID posid( surf_.id() );
    for ( int idx=0; idx<chunksize_; idx++ )
    {
	while ( subids_.isEmpty() )
	{
	    if ( nrdone_ )
	    {
		sectionindex_++;
		if ( sectionindex_ >= surf_.nrSections() )
		    return Finished();
	    }
	    else
	    {
		if ( !writeInt(surf_.nrSections()) )
		    mErrRetWrite("Error in writing data information")
	    }

	    const SectionID sectionid = surf_.sectionID( sectionindex_ );
	    const Geometry::BinIDSurface* meshsurf = 
				surf_.geometry().sectionGeometry( sectionid );

	    const int nrnodes = meshsurf->nrKnots();
	    for ( int idy=0; idy<nrnodes; idy++ )
	    {
		const RowCol rc = meshsurf->getKnotRowCol(idy);
		const Coord3 coord = meshsurf->getKnot( rc, false );

		const BinID bid = SI().transform(coord);
		if ( sel_ && !sel_->includes(bid) )
		    continue;

		const RowCol emrc( bid.inl, bid.crl );
		const SubID subid = emrc.toInt64();
		posid.setSubID( subid );
		posid.setSectionID( sectionid );
		const float auxval =
		    surf_.auxdata.getAuxDataVal( dataidx_, posid );
		if ( mIsUdf(auxval) )
		    continue;

		subids_ += subid;
		values_ += auxval;
	    }

	    if ( subids_.isEmpty() )
		mErrRetWrite("No data available for this surface")

	    if ( !writeInt(sectionid) || !writeInt(subids_.size()) )
		mErrRetWrite("Error in writing data information")
	}

	const int subidindex = subids_.size()-1;
	const SubID subid = subids_[subidindex];
	const float auxvalue = values_[subidindex];

	if ( !writeInt64(subid) || !writeFloat(auxvalue) )
	    mErrRetWrite("Error in writing datavalues")

	subids_.remove( subidindex );
	values_.remove( subidindex );
    }

    nrdone_++;
    return MoreToDo();
}


BufferString dgbSurfDataWriter::createHovName( const char* base, int idx )
{
    BufferString res( base );
    res += "^"; res += idx; res += ".hov";
    return res;
}


bool dgbSurfDataWriter::writeInt( int val )
{
    if ( binary_ )
	stream_->write( (char*) &val, sizeof(val) );
    else
	(*stream_) << val << '\n' ;

    return (*stream_);
}


bool dgbSurfDataWriter::writeInt64( od_int64 val )
{
    if ( binary_ )
	stream_->write( (char*) &val, sizeof(val) );
    else
	(*stream_) << val << '\n' ;

    return (*stream_);
}


bool dgbSurfDataWriter::writeFloat( float val )
{
    if ( binary_ )
	stream_->write( (char*) &val ,sizeof(val) );
    else
	(*stream_) << val << '\n';

    return (*stream_);
}


od_int64 dgbSurfDataWriter::nrDone() const 
{ return nrdone_; }


od_int64 dgbSurfDataWriter::totalNr() const
{ return totalnr_; }


const char* dgbSurfDataWriter::message() const
{ return errmsg_.str(); }



// Reader +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

dgbSurfDataReader::dgbSurfDataReader( const char* filename )
    : Executor("Aux data reader")
    , intinterpreter_(0)
    , int64interpreter_(0)
    , floatinterpreter_(0)
    , chunksize_(100)
    , dataidx_(-1)
    , surf_(0)
    , sectionindex_(0)
    , error_(true)
    , nrdone_(0)
    , valsleftonsection_(0)
    , shift_(0)
    , stream_(0)
{
    StreamData sd = StreamProvider( filename ).makeIStream();
    if ( !sd.usable() )
	return;
    stream_ = sd.istrm;
    if ( !(*stream_) )
	return;

    ascistream astream( *stream_ );
    if ( !astream.isOfFileType(dgbSurfDataWriter::sKeyFileType()) )
        return;

    const IOPar par( astream );
    if ( !par.get(dgbSurfDataWriter::sKeyAttrName(),dataname_) )
	return;

    if ( !par.get(dgbSurfDataWriter::sKeyAttrName(),datainfo_) )
	return;

    par.get( dgbSurfDataWriter::sKeyShift(), shift_ );

    BufferString dc;
    if ( par.get(dgbSurfDataWriter::sKeyIntDataChar(),dc) )
    {
	DataCharacteristics writtendatachar;
	writtendatachar.set( dc.buf() );
	intinterpreter_ = new DataInterpreter<int>( writtendatachar );

	if ( !par.get(dgbSurfDataWriter::sKeyInt64DataChar(),dc) )
	{
	    error_ = true; 
	    errmsg_ = "Error in reading data characteristics (int64)";
	    return;
	}
	writtendatachar.set( dc.buf() );
	int64interpreter_ = new DataInterpreter<od_int64>( writtendatachar );
					     
	if ( !par.get(dgbSurfDataWriter::sKeyFloatDataChar(),dc) )
	{
	    error_ = true;
	    errmsg_ = "Error in reading data characteristics (float)"; 
	    return;
	}
	writtendatachar.set( dc.buf() );
	floatinterpreter_ = new DataInterpreter<float>( writtendatachar );
    }

    error_ = false;
}


dgbSurfDataReader::~dgbSurfDataReader()
{
    delete stream_;
    delete intinterpreter_;
    delete int64interpreter_;
    delete floatinterpreter_;
}


const char* dgbSurfDataReader::dataName() const
{
    return dataname_[0] ? dataname_.buf() : 0;
}


float dgbSurfDataReader::shift() const 
{ return shift_; }


const char* dgbSurfDataReader::dataInfo() const
{
    return datainfo_[0] ? datainfo_.buf() : 0;
}



void dgbSurfDataReader::setSurface( Horizon3D& surf )
{
    surf_ = &surf;
    dataidx_ = surf_->auxdata.addAuxData( dataname_.buf() );
    surf_->auxdata.setAuxDataShift( dataidx_, shift_ );
}


#define mErrRetRead(msg) { \
    if ( msg ) errmsg_ = msg; \
    surf_->auxdata.removeAuxData(dataidx_); return ErrorOccurred(); }

int dgbSurfDataReader::nextStep()
{
    if ( error_ ) mErrRetRead(0)

    PosID posid( surf_->id() );
    for ( int idx=0; idx<chunksize_; idx++ )
    {
	while ( !valsleftonsection_ )
	{
	    if ( nrdone_ )
	    {
		sectionindex_++;
		if ( sectionindex_ >= nrsections_ )
		    return Finished();
	    }
	    else
	    {
		if ( !readInt(nrsections_) )
		    mErrRetRead( "Error in reading data information" )
	    }

	    int cp;
	    if ( !readInt(cp) || !readInt(valsleftonsection_) )
		mErrRetRead( "Error in reading data information" )

	    currentsection_ = cp;
	    totalnr_ = 100;
	    chunksize_ = valsleftonsection_/totalnr_+1;
	    if ( chunksize_ < 100 )
	    {
		chunksize_ = 100;
		totalnr_ = valsleftonsection_/chunksize_+1;
	    }
	}

	SubID subid;
	float val;
	if ( !readInt64(subid) || !readFloat(val) )
	    mErrRetRead( "Error in reading data values" )

	posid.setSubID( subid );
	posid.setSectionID( currentsection_ );
	surf_->auxdata.setAuxDataVal( dataidx_, posid, val );

	valsleftonsection_--;
    }

    nrdone_++;
    return MoreToDo();
}


#define mReadData(interpreter) \
    if ( interpreter ) \
    { \
	char buf[sizeof(res)]; \
	stream_->read(buf,sizeof(res)); \
	res = interpreter->get(buf,0); \
    } \
    else \
	(*stream_) >> res; \
\
    return (*stream_);

bool dgbSurfDataReader::readInt( int& res )
{ mReadData(intinterpreter_) }

bool dgbSurfDataReader::readInt64( od_int64& res )
{ mReadData(int64interpreter_) }

bool dgbSurfDataReader::readFloat( float& res )
{ mReadData(floatinterpreter_) }


od_int64 dgbSurfDataReader::nrDone() const 
{ return nrdone_; }


od_int64 dgbSurfDataReader::totalNr() const
{ return totalnr_; }


const char* dgbSurfDataReader::message() const
{ return errmsg_.str(); }

}; //nsamespace

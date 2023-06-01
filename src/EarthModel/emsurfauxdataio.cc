/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "emsurfauxdataio.h"

#include "ascstream.h"
#include "datachar.h"
#include "datainterp.h"
#include "emhorizon3d.h"
#include "emsurfaceauxdata.h"
#include "file.h"
#include "ioman.h"
#include "iopar.h"
#include "parametricsurface.h"
#include "streamconn.h"
#include "survinfo.h"
#include "uistrings.h"


namespace EM
{

const char* dgbSurfDataWriter::sKeyAttrName()	    { return "Attribute"; }
const char* dgbSurfDataWriter::sKeyIntDataChar()    { return "Int data"; }
const char* dgbSurfDataWriter::sKeyInt64DataChar()  { return "Long long data"; }
const char* dgbSurfDataWriter::sKeyFloatDataChar()  { return "Float data"; }
const char* dgbSurfDataWriter::sKeyFileType()	    {return "Surface aux data";}
const char* dgbSurfDataWriter::sKeyShift()	    { return "Shift"; }


dgbSurfDataWriter::dgbSurfDataWriter( const Horizon3D& surf,int dataidx,
				    const TrcKeySampling* sel, bool binary,
				    const char* filename )
    : Executor("Aux data writer")
    , stream_(nullptr)
    , chunksize_(100)
    , dataidx_(dataidx)
    , surf_(surf)
    , sel_(sel)
    , sectionindex_(0)
    , binary_(binary)
    , nrdone_(0)
    , filename_(filename)
{
    const Geometry::BinIDSurface* meshsurf =
				surf.geometry().geometryElement();
    const int nrnodes = meshsurf->nrKnots();

    chunksize_ = nrnodes/100 + 1;
    if ( chunksize_ < 100 )
	chunksize_ = 100;

    totalnr_ = nrnodes;
}


dgbSurfDataWriter::~dgbSurfDataWriter()
{
    delete stream_;
}


static BufferString getFreeFileName( const IOObj& ioobj )
{
    PtrMan<StreamConn> conn =
	dynamic_cast<StreamConn*>(ioobj.getConn(Conn::Read));
    if ( !conn )
	return "";

    const int maxnrfiles = 1024; // just a big number to make this loop end
    for ( int idx=0; idx<maxnrfiles; idx++ )
    {
	BufferString fnm =
	    dgbSurfDataWriter::createHovName( conn->fileName(), idx );
	if ( !File::exists(fnm.buf()) )
	    return fnm;
    }

    return "";
}


bool dgbSurfDataWriter::writeHeader()
{
    if ( filename_.isEmpty() )
    {
	PtrMan<IOObj> ioobj = IOM().get( surf_.multiID() );
	if ( !ioobj )
	    return false;

	filename_ = getFreeFileName( *ioobj );
	if ( filename_.isEmpty() )
	    return false;
    }

    stream_ = new od_ostream( filename_ );
    if ( !stream_ || !stream_->isOK() )
    {
	deleteAndNullPtr( stream_ );
	return false;
    }

    ascostream astream( *stream_ );
    astream.putHeader( sKeyFileType() );

    IOPar par( "Surface Data" );
    par.set( sKeyAttrName(), surf_.auxdata.auxDataName(dataidx_) );
    par.set( sKeyShift(), surf_.auxdata.auxDataShift(dataidx_) );

    if ( binary_ )
    {
	BufferString dc;

	int idummy;
	DataCharacteristics(idummy).toString( dc );
	par.set( sKeyIntDataChar(), dc );

	od_int64 lldummy;
	DataCharacteristics(lldummy).toString( dc );
	par.set( sKeyInt64DataChar(), dc );

	float fdummy;
	DataCharacteristics(fdummy).toString( dc );
	par.set( sKeyFloatDataChar(), dc );
    }

    par.putTo( astream );
    return true;
}


bool dgbSurfDataWriter::writeDummyHeader( const char* fnm, const char* attrnm )
{
    od_ostream strm( fnm );
    if ( !strm.isOK() )
	return false;

    ascostream astream( strm );
    astream.putHeader( dgbSurfDataWriter::sKeyFileType() );
    IOPar par( "Surface Data" );
    par.set( dgbSurfDataWriter::sKeyAttrName(), attrnm );
    par.putTo( astream );
    return true;
}


#define mErrRetWrite(msg) \
{ errmsg_ = msg; File::remove(filename_.buf()); \
    deleteAndNullPtr( stream_ ); return ErrorOccurred(); }


int dgbSurfDataWriter::nextStep()
{
    PosID posid( surf_.id() );
    if ( !stream_ && !writeHeader() )
	return ErrorOccurred();

    for ( int idx=0; idx<chunksize_; idx++ )
    {
	while ( subids_.isEmpty() )
	{
	    if ( nrdone_ )
	    {
		sectionindex_++;
		if ( sectionindex_ >= surf_.nrSections() )
		{
		    deleteAndNullPtr( stream_ );
		    return Finished();
		}
	    }
	    else
	    {
		if ( !writeInt(surf_.nrSections()) )
		    mErrRetWrite(tr("Error in writing data information"))
	    }

	    const Geometry::BinIDSurface* meshsurf =
				surf_.geometry().geometryElement();
	    if ( !meshsurf ) continue;

	    const int nrnodes = meshsurf->nrKnots();
	    for ( int idy=0; idy<nrnodes; idy++ )
	    {
		const RowCol rc = meshsurf->getKnotRowCol(idy);
		const Coord3 coord = meshsurf->getKnot( rc, false );

		const BinID bid = SI().transform(coord);
		if ( sel_ && !sel_->includes(bid) )
		    continue;

		const RowCol emrc( bid.inl(), bid.crl() );
		const SubID subid = emrc.toInt64();
		posid.setSubID( subid );
		const float auxval =
		    surf_.auxdata.getAuxDataVal( dataidx_, posid );
		if ( mIsUdf(auxval) )
		{
		    nrdone_++;
		    continue;
		}

		subids_ += subid;
		values_ += auxval;
		nrdone_++;
	    }

	    if ( subids_.isEmpty() )
	    {
		deleteAndNullPtr( stream_ );
		return Finished();
	    }

	    if ( !writeInt(SectionID::def().asInt()) ||
		 !writeInt(subids_.size()) )
		mErrRetWrite(tr("Error in writing data information"))
	}

	const int subidindex = subids_.size()-1;
	const SubID subid = subids_[subidindex];
	const float auxvalue = values_[subidindex];

	if ( !writeInt64(subid) || !writeFloat(auxvalue) )
	    mErrRetWrite(tr("Error in writing datavalues"))

	subids_.removeSingle( subidindex );
	values_.removeSingle( subidindex );
    }

    return MoreToDo();
}


BufferString dgbSurfDataWriter::createHovName( const char* base, int idx )
{
    BufferString res( base );
    res += "^"; res += idx; res += ".hov";
    return res;
}

#define mWriteData() \
    if ( !stream_ ) return false; \
    if ( binary_ ) \
	stream_->addBin( val ); \
    else \
	stream_->add( val ).add( od_newline ); \
    return true


bool dgbSurfDataWriter::writeInt( int val )
{ mWriteData(); }


bool dgbSurfDataWriter::writeInt64( od_int64 val )
{ mWriteData(); }


bool dgbSurfDataWriter::writeFloat( float val )
{ mWriteData(); }


od_int64 dgbSurfDataWriter::nrDone() const
{ return nrdone_; }


od_int64 dgbSurfDataWriter::totalNr() const
{ return totalnr_; }


uiString dgbSurfDataWriter::uiMessage() const
{ return errmsg_; }

uiString dgbSurfDataWriter::uiNrDoneText() const
{ return tr("Positions Written"); }



// Reader +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

dgbSurfDataReader::dgbSurfDataReader( const char* filename )
    : Executor("Aux data reader")
    , filename_(filename)
{
    error_ = !readHeader();
    deleteAndNullPtr( stream_ );
}


dgbSurfDataReader::~dgbSurfDataReader()
{
    delete stream_;
    delete intinterpreter_;
    delete int64interpreter_;
    delete floatinterpreter_;
}


bool dgbSurfDataReader::readHeader()
{
    if ( !stream_ )
	stream_ = new od_istream( filename_ );

    if ( !stream_->isOK() )
	return false;

    ascistream astream( *stream_ );
    if ( !astream.isOfFileType(dgbSurfDataWriter::sKeyFileType()) )
	return false;

    const IOPar par( astream );
    if ( !par.get(dgbSurfDataWriter::sKeyAttrName(),dataname_) )
	return false;

    if ( !par.get(dgbSurfDataWriter::sKeyAttrName(),datainfo_) )
	return false;

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
	    errmsg_ = tr("Error in reading data characteristics (int64)");
	    return false;
	}
	writtendatachar.set( dc.buf() );
	int64interpreter_ = new DataInterpreter<od_int64>( writtendatachar );

	if ( !par.get(dgbSurfDataWriter::sKeyFloatDataChar(),dc) )
	{
	    error_ = true;
	    errmsg_ = tr("Error in reading data characteristics (float)");
	    return false;
	}
	writtendatachar.set( dc.buf() );
	floatinterpreter_ = new DataInterpreter<float>( writtendatachar );
    }

    return true;
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

uiString dgbSurfDataReader::sHorizonData()
{
    return tr("Horizon data");
}

#ifdef __debug__
#   define mErrRetRead(msg) { \
    if ( !msg.isEmpty() ) errmsg_ = msg; \
    deleteAndNullPtr( stream_ ); \
    surf_->auxdata.removeAuxData(dataidx_); return ErrorOccurred(); }
#else
    #define mErrRetRead(msg) { \
    deleteAndNullPtr( stream_ ); \
    surf_->auxdata.removeAuxData(dataidx_); return ErrorOccurred(); }
#endif


#ifdef __debug__
#   define mErrRetReadNoDeleteAux(msg) { \
    if ( !msg.isEmpty() ) errmsg_ = msg; \
    deleteAndNullPtr( stream_ ); \
    return ErrorOccurred(); }
#else
#define mErrRetReadNoDeleteAux(msg) { \
    deleteAndNullPtr( stream_ ); \
    return ErrorOccurred(); }
#endif


int dgbSurfDataReader::nextStep()
{
    if ( error_ )
	mErrRetRead( uiString::emptyString() )

    if ( !stream_ && !readHeader() )
	mErrRetRead( tr("Error reading file %1").arg(filename_) )

    PosID posid( surf_->id() );
    for ( int idx=0; idx<chunksize_; idx++ )
    {
	while ( !valsleftonsection_ )
	{
	    if ( nrdone_ )
	    {
		sectionindex_++;
		if ( sectionindex_ >= nrsections_ || nrsections_ < 0 )
		{
		    deleteAndNullPtr( stream_ );
		    return Finished();
		}
	    }
	    else
	    {
		readInt( nrsections_ );
		if ( stream_->atEOF() )
		{
		    deleteAndNullPtr( stream_ );
		    return Finished();
		}

		if ( nrsections_ < 0 )
		    mErrRetReadNoDeleteAux(
		    uiStrings::phrCannotRead( sHorizonData() ) )
	    }

	    int cursec = -1;
	    const bool res = !readInt(cursec) || !readInt(valsleftonsection_);
	    if ( stream_->atEOF() )
	    {
		deleteAndNullPtr( stream_ );
		return Finished();
	    }

	    if ( res || cursec<0 )
		mErrRetReadNoDeleteAux(
		uiStrings::phrCannotRead( sHorizonData() ) )

	    currentsection_ = mCast(EM::SectionID,cursec);
	    totalnr_ = 100;
	    chunksize_ = valsleftonsection_/totalnr_+1;
	    if ( chunksize_ < 100 )
	    {
		chunksize_ = mMIN(100,valsleftonsection_);
		totalnr_ = valsleftonsection_/chunksize_+1;
	    }
	}

	SubID subid;
	float val;
	if ( !readInt64(subid) || !readFloat(val) )
	    mErrRetReadNoDeleteAux( uiStrings::phrCannotRead( sHorizonData() ) )

	posid.setSubID( subid );
	posid.setSectionID( currentsection_ );
	surf_->auxdata.setAuxDataVal( dataidx_, posid, val );

	valsleftonsection_--;
    }

    nrdone_++;
    return MoreToDo();
}


#define mReadData(interpreter) \
    if ( !stream_ ) \
	return false; \
    if ( interpreter ) \
    { \
	char buf[sizeof(res)]; \
	if ( !stream_->getBin(buf,sizeof(res)) ) return false; \
	res = interpreter->get( buf, 0 ); \
    } \
    else \
    { if ( stream_->get(res).isBad() ) return false; } \
    return true;

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


uiString dgbSurfDataReader::uiMessage() const
{ return errmsg_; }

uiString dgbSurfDataReader::uiNrDoneText() const
{ return tr("Positions Read"); }

} // namespace EM

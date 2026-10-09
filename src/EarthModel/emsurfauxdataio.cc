/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "emsurfauxdataio.h"

#include "arrayndimpl.h"
#include "ascstream.h"
#include "binidsurface.h"
#include "datachar.h"
#include "datainterp.h"
#include "emhorizon3d.h"
#include "emsurfaceauxdata.h"
#include "file.h"
#include "ioman.h"
#include "iopar.h"
#include "parametricsurface.h"
#include "paralleltask.h"
#include "streamconn.h"
#include "survinfo.h"
#include "uistrings.h"

#include <istream>
#include <streambuf>


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


bool dgbSurfDataWriter::writeHeader()
{
    if ( filename_.isEmpty() )
    {
	PtrMan<IOObj> ioobj = IOM().get( surf_.multiID() );
	if ( !ioobj )
	{
	    errmsg_ = tr("Cannot write surface attribute '%1': "
			 "output database entry not found.")
			.arg( surf_.auxdata.auxDataName(dataidx_) );
	    return false;
	}

	filename_ = SurfaceAuxData::getFreeFileName( *ioobj );
	if ( filename_.isEmpty() )
	{
	    errmsg_ = tr("Cannot write surface attribute '%1': "
			 "no free attribute filename available.")
			.arg( surf_.auxdata.auxDataName(dataidx_) );
	    return false;
	}
    }

    delete stream_;
    stream_ = new od_ostream( filename_ );
    if ( !stream_ || !stream_->isOK() )
    {
	errmsg_ = tr("Cannot open surface attribute file:\n%1")
			.arg( filename_ );
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
    if ( !stream_->isOK() )
    {
	errmsg_ = tr("Cannot write surface attribute header:\n%1")
			.arg( filename_ );
	deleteAndNullPtr( stream_ );
	return false;
    }

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
    if ( !stream_ )
    {
	if ( !writeHeader() )
	    mErrRetWrite( tr("Cannot open surface attribute file for "
			     "writing") );
    }

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
		    mErrRetWrite( tr("Error writing attribute '%1' to:\n%2" )
			.arg( surf_.auxdata.auxDataName(dataidx_) )
			.arg( filename_ ))
	    }

	    const Geometry::BinIDSurface* meshsurf =
				surf_.geometry().geometryElement();
	    if ( !meshsurf )
		continue;

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
		mErrRetWrite( tr("Error writing attribute '%1' to:\n%2" )
		    .arg( surf_.auxdata.auxDataName(dataidx_) )
		    .arg( filename_ ))
	}

	const int subidindex = subids_.size()-1;
	const SubID subid = subids_[subidindex];
	const float auxvalue = values_[subidindex];

	if ( !writeInt64(subid) || !writeFloat(auxvalue) )
	    mErrRetWrite(tr( "Error writing attribute values for '%1' to:\n%2" )
		.arg( surf_.auxdata.auxDataName(dataidx_) )
		.arg( filename_ ))

	subids_.removeSingle( subidindex );
	values_.removeSingle( subidindex );
    }

    return MoreToDo();
}


BufferString dgbSurfDataWriter::createHovName( const char* base, int idx )
{
    BufferString res( base );
    res.add( "^" ).add( idx ).add( ".hov" );
    return res;
}

#define mWriteData() \
    if ( !stream_ ) \
	return false; \
    if ( binary_ ) \
	stream_->addBin( val ); \
    else \
	stream_->add( val ).add( od_newline ); \
    return stream_->isOK()


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
    return dataname_[0] ? dataname_.buf() : nullptr;
}


float dgbSurfDataReader::shift() const
{
    return shift_;
}


const char* dgbSurfDataReader::dataInfo() const
{
    return datainfo_[0] ? datainfo_.buf() : nullptr;
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
		if ( !readInt(nrsections_) )
		{
		    if ( stream_->atEOF() )
			return Finished();

		    mErrRetRead( uiStrings::phrCannotRead( sHorizonData() ) )
		}

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


od_int64 dgbSurfDataReader::bodyOffset() const
{
    od_istream strm( filename_ );
    if ( !strm.isOK() )
	return -1;

    ascistream astream( strm );
    if ( !astream.isOfFileType(dgbSurfDataWriter::sKeyFileType()) )
	return -1;

    const IOPar par( astream );
    return par.isEmpty() && !strm.isOK() ? -1 : strm.position();
}

} // namespace EM

namespace EM
{

namespace
{

class CharRangeBuf : public std::streambuf
{
public:
CharRangeBuf( const char* start, od_int64 len )
{
    char* buf = const_cast<char*>( start );
    setg( buf, buf, buf + len );
}

protected:
std::streampos seekoff( std::streamoff off, std::ios_base::seekdir way,
			std::ios_base::openmode which ) override
{
    if ( (which & std::ios_base::in) == 0 )
	return std::streampos(-1);

    char* ptr = nullptr;
    if ( way == std::ios_base::beg )
	ptr = eback() + off;
    else if ( way == std::ios_base::cur )
	ptr = gptr() + off;
    else
	ptr = egptr() + off;

    if ( ptr < eback() || ptr > egptr() )
	return std::streampos(-1);

    setg( eback(), ptr, egptr() );
    return std::streampos( ptr - eback() );
}

std::streampos seekpos( std::streampos pos,
			std::ios_base::openmode which ) override
{ return seekoff( off_type(pos), std::ios_base::beg, which ); }

};


class CharRangeStream : public std::istream
{
public:
CharRangeStream( const char* start, od_int64 len )
    : std::istream(nullptr)
    , buf_(start,len)
{ rdbuf( &buf_ ); }

private:
    CharRangeBuf	buf_;
};


class OneAuxFile
{
public:
			OneAuxFile(const char* fnm, Horizon3D& hor)
			    : probe_(fnm)
			    , hor_(hor)
			{}

			~OneAuxFile();
    bool		index();
    bool		prepare();
    bool		allocArray(const StepInterval<int>& rowrg,
				   const StepInterval<int>& colrg);
    bool		ensureText();
    bool		decode(od_int64 start,od_int64 stop);
    void		releaseCalcData();
    void		setGlobalOffset(od_int64 off)	{ globaloff_ = off; }
    od_int64		globalOffset() const		{ return globaloff_; }
    od_int64		valueCount() const		{ return nvals_; }
    int			dataIndex() const		{ return dataidx_; }
    Array2D<float>*	array()				{ return arr_; }
    uiString		message() const			{ return msg_; }

private:

    bool		slurp();
    bool		readHeaderVals(int& nrsections,int& nvals,
				       od_int64& recpos);
    bool		putValue(const BinID&,float val);

    dgbSurfDataReader	probe_;
    Horizon3D&		hor_;
    int			dataidx_	= -1;
    uiString		msg_;
    char*		text_		= nullptr;
    od_int64		filesz_		= 0;
    od_int64		recpos_		= 0;
    od_int64		globaloff_	= 0;
    int			nvals_		= 0;
    int			i64sz_		= 0;
    int			flsz_		= 0;
    bool		binary_		= false;
    StepInterval<int>	rowrg_;
    StepInterval<int>	colrg_;
    int			nrows_		= 0;
    int			nrcols_		= 0;
    Array2D<float>*	arr_		= nullptr;
    TypeSet<od_int64>	ascoffs_;
};


bool OneAuxFile::prepare()
{
    if ( !probe_.dataName() )
    {
	msg_ = probe_.errMsg().isEmpty()
	    ? uiStrings::phrCannotRead( dgbSurfDataReader::sHorizonData() )
	    : probe_.errMsg();
	return false;
    }

    dataidx_ = hor_.auxdata.addAuxData( probe_.dataName() );
    hor_.auxdata.setAuxDataShift( dataidx_, probe_.shift() );
    return true;
}


bool OneAuxFile::slurp()
{
    filesz_ = File::getFileSize( probe_.fileName() );
    if ( filesz_ < 0 )
    {
	msg_ = uiStrings::phrCannotRead( dgbSurfDataReader::sHorizonData() );
	return false;
    }

    delete [] text_;
    text_ = nullptr;
    if ( filesz_ == 0 )
	return true;

    mTryAlloc( text_, char[filesz_+1] );
    if ( !text_ )
    {
	msg_ = ::toUiString("Not enough memory to read horizon data");
	return false;
    }

    od_istream strm( probe_.fileName() );
    if ( !strm.isOK() || !strm.getBin(text_,filesz_) )
    {
	delete [] text_;
	text_ = nullptr;
	msg_ = uiStrings::phrCannotRead( dgbSurfDataReader::sHorizonData() );
	return false;
    }

    text_[filesz_] = '\0';
    return true;
}


bool OneAuxFile::readHeaderVals( int& nrsections, int& nvals, od_int64& recpos )
{
    nrsections = 0;
    nvals = 0;
    recpos = probe_.bodyOffset();
    if ( recpos < 0 || !text_ || recpos > filesz_ )
	return true;

    binary_ = probe_.isBinary();
    if ( binary_ )
    {
	const char* p = text_ + recpos;
	const char* end = text_ + filesz_;
	const DataInterpreter<int>* di = probe_.intInterpreter();
	if ( !di )
	    return false;

	const int isz = di->nrBytes();
	if ( p + 3*isz > end )
	    return true;

	nrsections = di->get( p, 0 );
	p += isz;
	p += isz;
	nvals = di->get( p, 0 );
	p += isz;
	recpos = p - text_;
	if ( nrsections != 1 )
	{
	    msg_ = ::toUiString(
			"Cannot read horizon data with more than one section");
	    return false;
	}

	i64sz_ = probe_.int64Interpreter()
			? probe_.int64Interpreter()->nrBytes() : 0;
	flsz_ = probe_.floatInterpreter()
			? probe_.floatInterpreter()->nrBytes() : 0;
	if ( i64sz_<=0 || flsz_<=0 )
	    return false;

	return true;
    }

    CharRangeStream crs( text_, filesz_ );
    od_istream strm( crs );
    if ( recpos > 0 )
	strm.setReadPosition( recpos );
    if ( !strm.isOK() )
	return false;

    if ( strm.get(nrsections).isBad() )
	return true;

    int sectionid = -1;
    if ( strm.get(sectionid).isBad() || strm.get(nvals).isBad() )
	return false;

    if ( nrsections != 1 )
    {
	msg_ = ::toUiString(
			"Cannot read horizon data with more than one section");
	return false;
    }

    ascoffs_.setSize( nvals, 0 );
    for ( int idx=0; idx<nvals; idx++ )
    {
	ascoffs_[idx] = strm.position();
	od_int64 subid;
	float val;
	if ( strm.get(subid).isBad() || strm.get(val).isBad() )
	    return false;
    }

    return true;
}


bool OneAuxFile::decode( od_int64 start, od_int64 stop )
{
    if ( start > stop )
	return true;

    if ( binary_ )
    {
	const DataInterpreter<od_int64>* i64 = probe_.int64Interpreter();
	const DataInterpreter<float>* flt = probe_.floatInterpreter();
	const int rec = i64sz_ + flsz_;
	const char* base = text_ + recpos_;
	const char* end = text_ + filesz_;
	for ( od_int64 idx=start; idx<=stop; idx++ )
	{
	    const char* p = base + idx * rec;
	    if ( p + rec > end )
		return false;

	    const od_int64 subid = i64->get( p, 0 );
	    const float val = flt->get( p + i64sz_, 0 );
	    if ( !putValue(BinID::fromInt64(subid),val) )
		return false;
	}

	return true;
    }

    CharRangeStream crs( text_, filesz_ );
    od_istream strm( crs );
    for ( od_int64 idx=start; idx<=stop; idx++ )
    {
	strm.setReadPosition( ascoffs_[(int)idx] );
	if ( !strm.isOK() )
	    return false;

	od_int64 subid;
	float val;
	if ( strm.get(subid).isBad() || strm.get(val).isBad() )
	    return false;

	if ( !putValue(BinID::fromInt64(subid),val) )
	    return false;
    }

    return true;
}


bool OneAuxFile::putValue( const BinID& bid, float val )
{
    if ( !arr_ || !arr_->getData() )
	return false;
    if ( !rowrg_.includes(bid.inl(),false) ||
	 !colrg_.includes(bid.crl(),false) )
	return true;
    if ( (bid.inl()-rowrg_.start_) % rowrg_.step_ ||
	 (bid.crl()-colrg_.start_) % colrg_.step_ )
	return true;

    const int rowidx = rowrg_.getIndex( bid.inl() );
    const int colidx = colrg_.getIndex( bid.crl() );
    if ( rowidx<0 || colidx<0 || rowidx>=nrows_ || colidx>=nrcols_ )
	return true;

    arr_->getData()[(od_int64)rowidx*nrcols_ + colidx] = val;
    return true;
}


bool OneAuxFile::index()
{
    if ( !slurp() )
	return false;

    int nrsections = 0;
    od_int64 recpos = 0;
    if ( !readHeaderVals(nrsections,nvals_,recpos) )
    {
	if ( msg_.isEmpty() )
	    msg_ = uiStrings::phrCannotRead(
				dgbSurfDataReader::sHorizonData() );
	return false;
    }

    recpos_ = recpos;
    return true;
}


bool OneAuxFile::allocArray( const StepInterval<int>& rowrg,
			    const StepInterval<int>& colrg )
{
    deleteAndNullPtr( arr_ );
    rowrg_ = rowrg;
    colrg_ = colrg;
    nrows_ = 0;
    nrcols_ = 0;
    if ( nvals_ <= 0 )
	return true;

    nrows_ = rowrg_.nrSteps() + 1;
    nrcols_ = colrg_.nrSteps() + 1;
    if ( rowrg_.step_<=0 || colrg_.step_<=0 || nrows_<=0 || nrcols_<=0 )
    {
	msg_ = uiStrings::phrCannotRead( dgbSurfDataReader::sHorizonData() );
	return false;
    }

    PtrMan<Array2D<float> > arr =
			new Array2DImpl<float>( nrows_, nrcols_ );
    if ( !arr || !arr->isOK() )
    {
	msg_ = ::toUiString("Not enough memory to read horizon data");
	return false;
    }

    arr->setAll( mUdf(float) );
    arr_ = arr.release();
    return true;
}


bool OneAuxFile::ensureText()
{
    return text_ || slurp();
}


void OneAuxFile::releaseCalcData()
{
    delete [] text_;
    text_ = nullptr;
    delete arr_;
    arr_ = nullptr;
}


OneAuxFile::~OneAuxFile()
{
    releaseCalcData();
}


class AuxGroupTask : public ParallelTask
{ mODTextTranslationClass(AuxGroupTask);
public:
    AuxGroupTask( Horizon3D& hor, const BufferStringSet& fnms )
	: ParallelTask("Surface attributes reader")
	, hor_(hor)
    {
	for ( int idx=0; idx<fnms.size(); idx++ )
	    files_ += new OneAuxFile( fnms.get(idx).buf(), hor_ );

	od_int64 offset = 0;
	prepok_ = true;
	for ( int idx=0; idx<files_.size(); idx++ )
	{
	    OneAuxFile& file = *files_[idx];
	    if ( !file.index() )
	    {
		msg_ = file.message();
		prepok_ = false;
		break;
	    }

	    file.setGlobalOffset( offset );
	    offset += file.valueCount();
	}

	total_ = offset;
	if ( prepok_ && total_>0 )
	{
	    const Geometry::BinIDSurface* geom =
				hor_.geometry().geometryElement();
	    if ( !geom || geom->isEmpty() )
	    {
		msg_ = tr("Cannot read surface attributes without "
			  "horizon geometry");
		prepok_ = false;
	    }
	    else
	    {
		rowrg_ = geom->rowRange();
		colrg_ = geom->colRange();
		if ( rowrg_.step_<=0 || colrg_.step_<=0 )
		{
		    msg_ = tr("Cannot read surface attributes without "
			      "horizon geometry");
		    prepok_ = false;
		}
	    }
	}

	if ( prepok_ )
	{
	    for ( int idx=0; idx<files_.size(); idx++ )
	    {
		if ( !files_[idx]->prepare() )
		{
		    msg_ = files_[idx]->message();
		    prepok_ = false;
		    break;
		}
	    }
	}

	if ( prepok_ && !total_ )
	{
	    for ( int idx=0; idx<files_.size(); idx++ )
		files_[idx]->releaseCalcData();
	}

	if ( prepok_ )
	    msg_ = tr("Reading surface attributes");
    }

    ~AuxGroupTask()
    {
	deepErase( files_ );
    }

    uiString uiMessage() const override
    { return msg_; }

    uiString uiNrDoneText() const override
    { return tr("Positions read"); }

    od_int64 nrIterations() const override
    { return prepok_ ? total_ : 0; }

    int minThreadSize() const override
    { return 1024; }

    bool executeParallel( bool parallel ) override
    { return prepok_ && ParallelTask::executeParallel(parallel); }

private:

    bool doPrepare( int ) override
    {
	for ( int idx=0; idx<files_.size(); idx++ )
	{
	    OneAuxFile& file = *files_[idx];
	    if ( !file.ensureText() )
	    {
		msg_ = file.message();
		return false;
	    }

	    if ( !file.allocArray(rowrg_,colrg_) )
	    {
		msg_ = file.message();
		return false;
	    }
	}

	return true;
    }


    bool doWork( od_int64 start, od_int64 stop, int ) override
    {
	const od_int64 chunksz = 100000;
	for ( int idx=0; idx<files_.size(); idx++ )
	{
	    OneAuxFile& file = *files_[idx];
	    const od_int64 nvals = file.valueCount();
	    if ( nvals <= 0 )
		continue;

	    const od_int64 file0 = file.globalOffset();
	    const od_int64 file1 = file0 + nvals - 1;
	    if ( file1<start || file0>stop )
		continue;

	    const od_int64 localstart = start>file0 ? start-file0 : 0;
	    const od_int64 localstop = stop<file1 ? stop-file0 : nvals-1;
	    for ( od_int64 pos=localstart; pos<=localstop; )
	    {
		if ( !shouldContinue() )
		    return false;

		const od_int64 substop = mMIN( localstop, pos+chunksz-1 );
		if ( !file.decode(pos,substop) )
		{
		    msg_ = file.message();
		    if ( msg_.isEmpty() )
			msg_ = uiStrings::phrCannotRead(
					dgbSurfDataReader::sHorizonData() );
		    return false;
		}

		addToNrDone( substop-pos+1 );
		pos = substop + 1;
	    }
	}

	return true;
    }


    bool doFinish( bool success ) override
    {
	bool res = success;
	if ( res )
	    res = commitArrays();

	if ( !res && msg_.isEmpty() )
	    msg_ = uiStrings::phrCannotRead(
				dgbSurfDataReader::sHorizonData() );

	for ( int idx=0; idx<files_.size(); idx++ )
	    files_[idx]->releaseCalcData();

	return res;
    }

    bool commitArrays()
    {
	TypeSet<int> dataidxs;
	ObjectSet<Array2D<float>> arrays;
	for ( int idx=0; idx<files_.size(); idx++ )
	{
	    Array2D<float>* arr = files_[idx]->array();
	    if ( !arr )
		continue;

	    dataidxs += files_[idx]->dataIndex();
	    arrays += arr;
	}

	if ( arrays.isEmpty() )
	    return true;

	if ( hor_.auxdata.setArray2Ds(dataidxs,arrays) )
	    return true;

	msg_ = uiStrings::phrCannotRead( dgbSurfDataReader::sHorizonData() );
	return false;
    }

    Horizon3D&			hor_;
    ObjectSet<OneAuxFile>	files_;
    StepInterval<int>		rowrg_;
    StepInterval<int>		colrg_;
    od_int64			total_		= 0;
    bool			prepok_		= false;
    uiString			msg_;
};

} // namespace


Executor* createAuxDataExecutor( Horizon3D& hor, const BufferStringSet& fnms )
{
    class AuxDataReadExecutor : public Executor
    {
    public:
	AuxDataReadExecutor( Horizon3D& hor3d, const BufferStringSet& names )
	    : Executor("Surface attributes reader")
	    , task_(hor3d,names)
	{}

	int nextStep() override
	{
	    if ( done_ )
		return Finished();

	    const bool ok = task_.execute();
	    done_ = true;
	    return ok ? Finished() : ErrorOccurred();
	}

	od_int64 nrDone() const override	{ return task_.nrDone(); }
	od_int64 totalNr() const override	{ return task_.totalNr(); }
	uiString uiMessage() const override	{ return task_.uiMessage(); }
	uiString uiNrDoneText() const override	{ return task_.uiNrDoneText(); }

    private:
	AuxGroupTask		task_;
	bool			done_		= false;
    };

    return new AuxDataReadExecutor( hor, fnms );
}

} // namespace EM

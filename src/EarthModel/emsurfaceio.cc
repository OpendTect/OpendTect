/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          June 2003
 RCS:           $Id: emsurfaceio.cc,v 1.58 2006-04-12 13:56:24 cvsnanne Exp $
________________________________________________________________________

-*/

#include "emsurfaceio.h"

#include "color.h"
#include "ascstream.h"
#include "datachar.h"
#include "datainterp.h"
#include "emsurface.h"
#include "emsurfacegeometry.h"
#include "emsurfaceauxdata.h"
#include "emsurfaceedgeline.h"
#include "emsurfauxdataio.h"
#include "filegen.h"
#include "parametricsurface.h"
#include "ioobj.h"
#include "iopar.h"
#include "ptrman.h"
#include "streamconn.h"
#include "survinfo.h"

#include <fstream>


namespace EM
{


const char* dgbSurfaceReader::sKeyFloatDataChar() { return "Data char"; }

const char* dgbSurfaceReader::sKeyInt16DataChar() { return "Int 16 Data char";}
const char* dgbSurfaceReader::sKeyInt32DataChar() { return "Int Data char";}
const char* dgbSurfaceReader::sKeyInt64DataChar() { return "Int 64 Data char";}
const char* dgbSurfaceReader::sKeyNrSectionsV1()  { return "Nr Subhorizons" ; }
const char* dgbSurfaceReader::sKeyNrSections()    { return "Nr Patches"; }
const char* dgbSurfaceReader::sKeyRowRange()	  { return "Row range"; }
const char* dgbSurfaceReader::sKeyColRange()	  { return "Col range"; }
const char* dgbSurfaceReader::sKeyDBInfo()	  { return "DB info"; }
const char* dgbSurfaceReader::sKeyVersion()	  { return "Format version"; }

const char* dgbSurfaceReader::sKeyTransformX()	{ return "X transform"; }
const char* dgbSurfaceReader::sKeyTransformY() 	{ return "Y transform"; }

const char* dgbSurfaceReader::sMsgParseError()  { return "Cannot parse file"; }
const char* dgbSurfaceReader::sMsgReadError()
{ return "Unexpected end of file"; }

BufferString dgbSurfaceReader::sSectionIDKey( int idx )
{ BufferString res = "Patch "; res += idx; return res;  }


BufferString dgbSurfaceReader::sSectionNameKey( int idx )
{ BufferString res = "Patch Name "; res += idx; return res;  }


dgbSurfaceReader::dgbSurfaceReader( const IOObj& ioobj,
					const char* filetype,
					Surface* surface )
    : ExecutorGroup( "Surface Reader" )
    , conn_( dynamic_cast<StreamConn*>(ioobj.getConn(Conn::Read)) )
    , surface_( surface )
    , par_( 0 )
    , setsurfacepar_( false )
    , readrowrange_( 0 )
    , readcolrange_( 0 )
    , int16interpreter_( 0 )
    , int32interpreter_( 0 )
    , int64interpreter_( 0 )
    , floatinterpreter_( 0 )
    , nrdone_( 0 )
    , readonlyz_( true )
    , sectionindex_( 0 )
    , sectionsread_( 0 )
    , oldsectionindex_( -1 )
    , isinited_( false )
    , fullyread_( true )
    , version_( 1 )
{
    sectionnames_.allowNull();

    BufferString exnm = "Reading surface '"; exnm += ioobj.name(); exnm += "'";
    setName( exnm );
    setNrDoneText( "Nr done" );
    auxdataexecs_.allowNull(true);
    if ( !conn_ || !conn_->forRead()  )
    {
	msg_ = "Cannot open input surface file";
	error_ = true;
	return;
    }

    createAuxDataReader();
    error_ = !readHeaders( filetype );
}


bool dgbSurfaceReader::readHeaders( const char* filetype )
{
    std::istream& strm = ((StreamConn*)conn_)->iStream();
    ascistream astream( strm );
    if ( !astream.isOfFileType( filetype ))
    {
	msg_ = "Invalid filetype";
	return false;
    }

    astream.next();

    IOPar par( astream );
    version_ = 1;
    par.get( sKeyVersion(), version_ );

    BufferString dc;
#define mGetDataChar( type, str, interpr ) \
    delete interpr; \
    if ( par.get(str,dc) ) \
    { \
	DataCharacteristics writtendatachar; \
	writtendatachar.set( dc.buf() ); \
	interpr = new DataInterpreter<type>( writtendatachar ); \
    } \
    else interpr = 0

    mGetDataChar( int, sKeyInt16DataChar(), int16interpreter_ );
    mGetDataChar( int, sKeyInt32DataChar(), int32interpreter_ );
    mGetDataChar( int64, sKeyInt64DataChar(), int64interpreter_ );
    mGetDataChar( float, sKeyFloatDataChar(), floatinterpreter_ );

    if ( version_==3 )
    {
	const int64 nrsectionsoffset = readInt64( strm );
	if ( !strm ) { msg_ = sMsgReadError(); return false; }

	strm.seekg( nrsectionsoffset, std::ios::beg );
	const int nrsections = readInt32( strm );
	if ( !strm ) { msg_ = sMsgReadError(); return false; }

	for ( int idx=0; idx<nrsections; idx++ )
	    sectionoffsets_ += readInt64( strm );

	for ( int idx=0; idx<nrsections; idx++ )
	    sectionids_ += readInt32( strm );

	if ( !strm ) { msg_ = sMsgReadError(); return false; }

	ascistream parstream( strm, false );
	parstream.next();
	par_ = new IOPar( parstream );
    }
    else
    {
	int nrsections;
	if ( !par.get( sKeyNrSections() , nrsections ) &&
	     !par.get( sKeyNrSectionsV1(), nrsections ) )
	{
	    msg_ = sMsgParseError();
	    return false;
	}

	for ( int idx=0; idx<nrsections; idx++ )
	{
	    int sectionid = idx;
	    BufferString key = sSectionIDKey( idx );
	    par.get(key, sectionid);
	    sectionids_+= sectionid;

	}

	par_ = new IOPar( par );
    }

    for ( int idx=0; idx<sectionids_.size(); idx++ )
    {
	const BufferString key = sSectionNameKey( idx );
	BufferString sectionname;
	par_->get(key,sectionname);
		    
	sectionnames_ += sectionname.size() ? new BufferString(sectionname) : 0;
    }

    par_->get( sKeyRowRange(), rowrange_.start, rowrange_.stop, rowrange_.step);
    par_->get( sKeyColRange(), colrange_.start, colrange_.stop, colrange_.step);

    for ( int idx=0; idx<nrSections(); idx++ )
	sectionsel_ += sectionID(idx);

    for ( int idx=0; idx<nrAuxVals(); idx++ )
	auxdatasel_ += idx;

    par_->get( sKeyDBInfo(), dbinfo_ );

    if ( version_==1 )
	return parseVersion1( *par_ );

    return true;
}
	

void dgbSurfaceReader::createAuxDataReader()
{
    int gap = 0;
    for ( int idx=0; ; idx++ )
    {
	if ( gap > 50 ) break;

	BufferString hovfnm( 
		dgbSurfDataWriter::createHovName(conn_->fileName(),idx) );
	if ( File_isEmpty(hovfnm) )
	{ gap++; continue; }

	dgbSurfDataReader* dreader = new dgbSurfDataReader( hovfnm );
	if ( dreader->dataName() )
	{
	    auxdatanames_ += new BufferString(dreader->dataName());
	    auxdataexecs_ += dreader;
	}
	else
	{
	    delete dreader;
	    break;
	}
    }
}


const char* dgbSurfaceReader::dbInfo() const
{
    return surface_ ? surface_->dbInfo() : "";
}


dgbSurfaceReader::~dgbSurfaceReader()
{
    deepErase( sectionnames_ );
    deepErase( auxdatanames_ );
    deepErase( auxdataexecs_ );

    delete par_;
    delete conn_;
    delete readrowrange_;
    delete readcolrange_;

    delete int32interpreter_;
    delete floatinterpreter_;
}


bool dgbSurfaceReader::isOK() const
{
    return !error_;
}


int dgbSurfaceReader::nrSections() const
{
    return sectionnames_.size();
}


SectionID dgbSurfaceReader::sectionID( int idx ) const
{
    return sectionids_[idx];
}


BufferString dgbSurfaceReader::sectionName( int idx ) const
{
    if ( !sectionnames_[idx] )
    {
	BufferString res = "[";
	res += sectionids_[idx];
	res += "]";
	return res;
    }

    return *sectionnames_[idx];
}


void dgbSurfaceReader::selSections(const TypeSet<SectionID>& sel)
{
    sectionsel_ = sel;
}


int dgbSurfaceReader::nrAuxVals() const
{
    return auxdatanames_.size();
}


const char* dgbSurfaceReader::auxDataName( int idx ) const
{
    return *auxdatanames_[idx];
}


void dgbSurfaceReader::selAuxData(const TypeSet<int>& sel )
{
    auxdatasel_ = sel;
}


const StepInterval<int>& dgbSurfaceReader::rowInterval() const
{
    return rowrange_;
}


const StepInterval<int>& dgbSurfaceReader::colInterval() const
{
    return colrange_;
}


void dgbSurfaceReader::setRowInterval( const StepInterval<int>& rg )
{
    if ( readrowrange_ ) delete readrowrange_;
    readrowrange_ = new StepInterval<int>(rg);
}


void dgbSurfaceReader::setColInterval( const StepInterval<int>& rg )
{
    if ( readcolrange_ ) delete readcolrange_;
    readcolrange_ = new StepInterval<int>(rg);
}


void dgbSurfaceReader::setReadOnlyZ( bool yn )
{
    readonlyz_ = yn;
}


const IOPar* dgbSurfaceReader::pars() const
{
    return par_;
}


int dgbSurfaceReader::nrDone() const
{
    return (executors.size() ? ExecutorGroup::nrDone() : 0) + nrdone_;
}


const char* dgbSurfaceReader::nrDoneText() const
{
    return "Gridlines read";
}


int dgbSurfaceReader::totalNr() const
{
    int ownres =
	(readrowrange_?readrowrange_->nrSteps():rowrange_.nrSteps()) *
	sectionsel_.size();

    if ( !ownres ) ownres = nrrows_;

    return ownres + (executors.size() ? ExecutorGroup::totalNr() : 0);
}


void dgbSurfaceReader::setGeometry()
{
    surface_->cleanUp();
    surface_->setDBInfo( dbinfo_ );
    for ( int idx=0; idx<auxdatasel_.size(); idx++ )
    {
	if ( auxdatasel_[idx]>=auxdataexecs_.size() )
	    continue;

	auxdataexecs_[auxdatasel_[idx]]->setSurface( *surface_ );

	add( auxdataexecs_[auxdatasel_[idx]] );
	auxdataexecs_.replace( auxdatasel_[idx], 0 );
    }

    for ( int idx=0; idx<sectionsel_.size(); idx++ )
    {
	const int index = sectionids_.indexOf(sectionsel_[idx]);
	if ( index<0 )
	{
	    sectionsel_.remove(idx--);
	    continue;
	}
    }

    if ( readrowrange_ )
    {
	const RowCol filestep = getFileStep();

	if ( readrowrange_->step < abs(filestep.row) )
	    readrowrange_->step = filestep.row;
	if ( readcolrange_->step < abs(filestep.col) )
	    readcolrange_->step = filestep.col;

	if ( readrowrange_->step / filestep.row < 0 )
	    readrowrange_->step *= -1;
	if ( readcolrange_->step / filestep.col < 0 )
	    readcolrange_->step *= -1;
    }


    isinited_ = true;
}


bool dgbSurfaceReader::readRowOffsets( std::istream& strm )
{
    if ( version_<=2 )
    {
	rowoffsets_.erase();
	return true;
    }

    rowoffsets_.setSize( nrrows_, 0 );
    for ( int idx=0; idx<nrrows_; idx++ )
    {
	rowoffsets_[idx] = readInt64( strm );
	if ( !strm )
	{
	    msg_ = sMsgReadError();
	    return false;
	}
    }

    return true;
}


RowCol dgbSurfaceReader::getFileStep() const
{
    if ( version_!=1 )
	return RowCol(rowrange_.step, colrange_.step);

    return convertRowCol(1,1)-convertRowCol(0,0);
}


bool dgbSurfaceReader::shouldSkipRow( int row ) const
{
    if ( version_==1 || !isBinary() )
	return false;

    if ( sectionsel_.indexOf(sectionids_[sectionindex_])==-1 )
	return true;

    if ( !readrowrange_ )
	return false;

    if ( !readrowrange_->includes( row ) )
	return true;

    return (row-readrowrange_->start)%readrowrange_->step;
}


int dgbSurfaceReader::currentRow() const
{ return firstrow_+rowindex_*rowrange_.step; }


int dgbSurfaceReader::nextStep()
{
    if ( error_ || !surface_ )
    {
	if ( !surface_ ) 
	    msg_ = "Internal: No Surface Set";

	return ErrorOccurred;
    }

    std::istream& strm = conn_->iStream();

    if ( !isinited_ )
	setGeometry();

    if ( sectionindex_>=sectionids_.size() )
    {

	int res = ExecutorGroup::nextStep();
	if ( !res && !setsurfacepar_ )
	{
	    setsurfacepar_ = true;
	    if ( !surface_->usePar(*par_) )
	    {
		msg_ = "Could not parse header";
		return ErrorOccurred;
	    }

	    surface_->setFullyLoaded( fullyread_ );

	    surface_->resetChangedFlag();
	    surface_->geometry.checkSupport(true);
	}

	return res;
    }

    if ( sectionindex_!=oldsectionindex_ )
    {
	const int res = prepareNewSection(strm);
	if ( res!=Finished )
	    return res;
    }

    while ( isBinary() && shouldSkipRow( currentRow() ) )
    {
	fullyread_ = false;
	const int res = skipRow( strm );
	if ( res==ErrorOccurred )
	    return res;
	else if ( res==Finished ) //Section change
	    return MoreToDo;
    }

    if ( !prepareRowRead(strm) )
	return ErrorOccurred;

    const SectionID sectionid = sectionids_[sectionindex_];
    const int nrcols = readInt32( strm );
    const int firstcol = nrcols ? readInt32( strm ) : 0;
    if ( !strm )
    {
	msg_ = sMsgReadError();
	return ErrorOccurred;
    }

    if ( !nrcols )
    {
	goToNextRow();
	return MoreToDo;
    }

    if ( (version_==3 && !readVersion3Row( strm, firstcol, nrcols ) ) ||
         ( version_==2 && !readVersion2Row( strm, firstcol, nrcols ) ) ||
         ( version_==1 && !readVersion1Row( strm, firstcol, nrcols ) ) )
	return ErrorOccurred;

    surface_->geometry.checkSupport(false);

    goToNextRow();
    return MoreToDo;
}


int dgbSurfaceReader::prepareNewSection( std::istream& strm )
{
    const SectionID sectionid = sectionids_[sectionindex_];
    if ( sectionsel_.indexOf(sectionid)==-1 )
    {
	sectionindex_++;
	return MoreToDo;
    }

    if ( version_==3 )
	strm.seekg( sectionoffsets_[sectionindex_], std::ios_base::beg );

    nrrows_ = readInt32( strm );
    if ( !strm )
    {
	msg_ = sMsgReadError();
	return ErrorOccurred;
    }

    if ( nrrows_ )
    {
	firstrow_ = readInt32( strm );
	if ( !strm )
	{
	    msg_ = sMsgReadError();
	    return ErrorOccurred;
	}
    }
    else
    {
	sectionindex_++;
	sectionsread_++;
	nrdone_ = sectionsread_ *
	    ( readrowrange_ ? readrowrange_->nrSteps() : rowrange_.nrSteps() );

	return MoreToDo;
    }

    if ( version_==3 && readrowrange_ )
    {
	const StepInterval<int> sectionrowrg( firstrow_,
		    firstrow_+(nrrows_-1)*rowrange_.step, rowrange_.step );
	if ( sectionrowrg.stop<readrowrange_->start ||
	     sectionrowrg.start>readrowrange_->stop )
	{
	    sectionindex_++;
	    sectionsread_++;
	    nrdone_ = sectionsread_ *
	       (readrowrange_ ? readrowrange_->nrSteps() : rowrange_.nrSteps());
	    return MoreToDo;
	}
    }

    rowindex_ = 0;

    if ( version_==3 && !readRowOffsets( strm ) )
	return ErrorOccurred;

    oldsectionindex_ = sectionindex_;
    return Finished;
}


bool dgbSurfaceReader::readVersion1Row( std::istream& strm, int firstcol,
					int nrcols )
{
    bool isrowused = false;
    const int filerow = currentRow();
    const SectionID sectionid = sectionids_[sectionindex_];
    for ( int colindex=0; colindex<nrcols; colindex++ )
    {
	const int filecol = firstcol+colindex*colrange_.step;

	RowCol surfrc = convertRowCol( filerow, filecol );
	Coord3 pos;
	if ( !readonlyz_ )
	{
	    pos.x = readFloat( strm );
	    pos.y = readFloat( strm );
	}

	pos.z = readFloat( strm );

	//Read filltype
	if ( rowindex_!=nrrows_-1 && colindex!=nrcols-1 )
	    readInt32( strm );

	if ( !strm )
	{
	    msg_ = sMsgReadError();
	    return false;
	}

	if ( readrowrange_ && (!readrowrange_->includes(surfrc.row) ||
		    ((surfrc.row-readrowrange_->start)%readrowrange_->step)))
	{
	    fullyread_ = false;
	    continue;
	}

	if ( readcolrange_ && (!readcolrange_->includes(surfrc.col) ||
		    ((surfrc.col-readcolrange_->start)%readcolrange_->step)))
	{
	    fullyread_ = false;
	    continue;
	}

	if ( !surface_->geometry.getSurface(sectionid) )
	    createSection( sectionid );

	surface_->setPos( sectionid, surfrc.getSerialized(), pos, false );

	isrowused = true;
    }

    if ( isrowused )
	nrdone_++;

    return true;
}


bool dgbSurfaceReader::readVersion2Row( std::istream& strm,
					int firstcol, int nrcols )
{
    const int filerow = currentRow();
    bool isrowused = false;
    const SectionID sectionid = sectionids_[sectionindex_];
    for ( int colindex=0; colindex<nrcols; colindex++ )
    {
	const int filecol = firstcol+colindex*colrange_.step;

	const RowCol rowcol( filerow, filecol );
	Coord3 pos;
	if ( !readonlyz_ )
	{
	    pos.x = readFloat( strm );
	    pos.y = readFloat( strm );
	}

	pos.z = readFloat( strm );
	if ( !strm )
	{
	    msg_ = sMsgReadError();
	    return false;
	}

	if ( readcolrange_ && (!readcolrange_->includes(rowcol.col) ||
		    ((rowcol.col-readcolrange_->start)%readcolrange_->step)))
	{
	    fullyread_ = false;
	    continue;
	}

	if ( !surface_->geometry.getSurface(sectionid) )
	    createSection( sectionid );

	surface_->setPos( sectionid, rowcol.getSerialized(), pos, false );

	isrowused = true;
    }

    if ( isrowused )
	nrdone_++;

    return true;
}


int dgbSurfaceReader::skipRow( std::istream& strm )
{
    if ( !isBinary() )
	return ErrorOccurred;

    if ( version_!=3 )
    {
	const int nrcols = readInt32( strm );
	if ( !strm ) return ErrorOccurred;

	int offset = 0;
	if ( nrcols )
	{
	    int nrbytespernode = (readonlyz_?1:3)*floatinterpreter_->nrBytes();
	    if ( version_==1 )
		nrbytespernode += int32interpreter_->nrBytes(); //filltype

	    offset += nrbytespernode*nrcols;

	    offset += int32interpreter_->nrBytes(); //firstcol

	    strm.seekg( offset, std::ios_base::cur );
	    if ( !strm ) return ErrorOccurred;
	}
    }

    goToNextRow();
    return sectionindex_!=oldsectionindex_ ? Finished : MoreToDo;
}


bool dgbSurfaceReader::prepareRowRead( std::istream& strm )
{
    if ( version_!=3 )
	return true;

    strm.seekg( rowoffsets_[rowindex_], std::ios_base::beg );
    return strm;
}


void dgbSurfaceReader::goToNextRow()
{

    rowindex_++;
    if ( rowindex_>=nrrows_ )
    {
	sectionindex_++;
	sectionsread_++;
	nrdone_ = sectionsread_ *
	    ( readrowrange_ ? readrowrange_->nrSteps() : rowrange_.nrSteps() );
    }
}


bool dgbSurfaceReader::readVersion3Row( std::istream& strm,
				       int firstcol, int nrcols )
{
    SamplingData<float> zsd;
    zsd.start = readFloat( strm );
    zsd.step = readFloat( strm );

    int colindex = 0;

    if ( readcolrange_ )
    {
	const StepInterval<int> colrg( firstcol,
		    firstcol+(nrcols-1)*colrange_.step, colrange_.step );
	if ( colrg.stop<readcolrange_->start ||
	     colrg.start>readcolrange_->stop )
	{
	    fullyread_ = false;
	    goToNextRow();
	    return true;
	}

	if ( int16interpreter_ )
	{
	    colindex = colrg.nearestIndex( readcolrange_->start );
	    if ( colindex<0 )
		colindex = 0;

	    if ( colindex )
	    {
		fullyread_ = false;
		strm.seekg( colindex*int16interpreter_->nrBytes(),
			std::ios_base::cur );
	    }
	}
    }

    RowCol rc( currentRow(), 0 );
    bool didread = false;
    const SectionID sectionid = sectionids_[sectionindex_];
    for ( ; colindex<nrcols; colindex++ )
    {
	rc.col = firstcol+colindex*colrange_.step;
	if ( readcolrange_ )
	{
	    if ( !readcolrange_->includes( rc.col ) )
		break;

	    if ( (rc.col-readcolrange_->start)%readcolrange_->step )
		continue;
	}

	Coord3 pos;
	if ( !readonlyz_ )
	{
	    pos.x = readFloat( strm );
	    pos.y = readFloat( strm );
	}

	const int zidx = readInt16( strm );

	if ( !strm )
	{
	    msg_ = sMsgReadError();
	    return false;
	}

	if ( zidx==65535 )
	    pos.z = mUdf(float);
	else
	    pos.z = zsd.atIndex( zidx );


	if ( !surface_->geometry.getSurface(sectionid) )
	    createSection( sectionid );

	surface_->setPos( sectionid, rc.getSerialized(), pos, false );
	didread = true;
    }

    if ( didread )
	nrdone_++;

    return true;
}


void dgbSurfaceReader::createSection( const SectionID& sectionid )
{
    const RowCol filestep = getFileStep();
    const RowCol loadedstep = readrowrange_ && readcolrange_
			? RowCol(readrowrange_->step,readcolrange_->step)
			: filestep;
    surface_->geometry.setStep( filestep, loadedstep );

    const int index = sectionids_.indexOf(sectionid);
    surface_->geometry.addSection( sectionnames_[index] ?
	    			   *sectionnames_[index] : 0, sectionid, false );
    surface_->geometry.checkSupport(false);
}


const char* dgbSurfaceReader::message() const
{
    return msg_;
}


int dgbSurfaceReader::readInt16(std::istream& strm) const
{
    if ( int16interpreter_ )
    {
	const int sz = int16interpreter_->nrBytes();
	char buf[sz];
	strm.read(buf,sz);
	return int16interpreter_->get(buf,0);
    }

    int res;
    strm >> res;
    return res;
}



int dgbSurfaceReader::readInt32(std::istream& strm) const
{
    if ( int32interpreter_ )
    {
	const int sz = int32interpreter_->nrBytes();
	char buf[sz];
	strm.read(buf,sz);
	return int32interpreter_->get(buf,0);
    }

    int res;
    strm >> res;
    return res;
}


int64 dgbSurfaceReader::readInt64(std::istream& strm) const
{
    if ( int64interpreter_ )
    {
	const int sz = int64interpreter_->nrBytes();
	char buf[sz];
	strm.read(buf,sz);
	return int64interpreter_->get(buf,0);
    }

    int res;
    strm >> res;
    return res;
}


int dgbSurfaceReader::int64Size() const
{
    return int64interpreter_ ? int64interpreter_->nrBytes() : 21;
}


bool dgbSurfaceReader::isBinary() const
{ return floatinterpreter_; }




double dgbSurfaceReader::readFloat(std::istream& strm) const
{
    if ( floatinterpreter_ )
    {
	const int sz = floatinterpreter_->nrBytes();
	char buf[sz];
	strm.read(buf,sz);
	return floatinterpreter_->get(buf,0);
    }

    double res;
    strm >> res;
    return res;
}


RowCol dgbSurfaceReader::convertRowCol( int row, int col ) const
{
    const Coord coord(conv11*row+conv12*col+conv13,
		      conv21*row+conv22*col+conv23 );
    const BinID bid = SI().transform(coord);
    return RowCol(bid.inl, bid.crl );
}


bool dgbSurfaceReader::parseVersion1( const IOPar& par ) 
{
    return par.get( sKeyTransformX(), conv11, conv12, conv13 ) &&
	   par.get( sKeyTransformY(), conv21, conv22, conv23 );
}



dgbSurfaceWriter::dgbSurfaceWriter( const IOObj* ioobj,
					const char* filetype,
					const Surface& surface,
					bool binary )
    : ExecutorGroup( "Surface Writer" )
    , conn_( 0 )
    , ioobj_( ioobj ? ioobj->clone() : 0 )
    , surface_( surface )
    , par_( *new IOPar("Surface parameters" ))
    , writerowrange_( 0 )
    , writecolrange_( 0 )
    , writtenrowrange_( INT_MAX, INT_MIN )
    , writtencolrange_( INT_MAX, INT_MIN )
    , nrdone_( 0 )
    , sectionindex_( 0 )
    , oldsectionindex_( -1 )
    , writeonlyz_( false )
    , filetype_( filetype )
    , binary_( binary )
{
    surface.ref();
    setNrDoneText( "Nr done" );
    par_.set( dgbSurfaceReader::sKeyDBInfo(), surface.dbInfo() );

    for ( int idx=0; idx<nrSections(); idx++ )
	sectionsel_ += sectionID(idx);

    for ( int idx=0; idx<nrAuxVals(); idx++ )
    {
	if ( auxDataName(idx) )
	    auxdatasel_ += idx;
    }

    rowrange_ = surface.geometry.rowRange();
    colrange_ = surface.geometry.colRange();

    surface.fillPar( par_ );
}


dgbSurfaceWriter::~dgbSurfaceWriter()
{
    std::ostream& strm = conn_->oStream();
    const int64 nrsectionsoffset = strm.tellp();
    writeInt32( strm, sectionsel_.size(), sEOL() );

    for ( int idx=0; idx<sectionoffsets_.size(); idx++ )
	writeInt64( strm, sectionoffsets_[idx], sEOL() );

    for ( int idx=0; idx<sectionsel_.size(); idx++ )
	writeInt32( strm, sectionsel_[idx], sEOL() );


    const int64 secondparoffset = strm.tellp();
    strm.seekp( nrsectionsoffsetoffset_, std::ios::beg );
    writeInt64( strm, nrsectionsoffset, sEOL() );
    strm.seekp( secondparoffset, std::ios::beg );

    if ( writtenrowrange_.width(false)>=0 )
    {
	par_.set( dgbSurfaceReader::sKeyRowRange(),
	    writtenrowrange_.start, writtenrowrange_.stop,
	    writerowrange_ ? writerowrange_->step : rowrange_.step );
    }

    if ( writtencolrange_.width(false)>=0 )
    {
	par_.set( dgbSurfaceReader::sKeyColRange(),
	    writtencolrange_.start, writtencolrange_.stop,
	    writecolrange_ ? writecolrange_->step : colrange_.step );

    }

    for ( int idx=0; idx<sectionsel_.size(); idx++ )
    {
	const BufferString sectionname = surface_.sectionName(sectionsel_[idx]);
	if ( !sectionname.size() )
	    continue;

	par_.set( dgbSurfaceReader::sSectionNameKey(idx), sectionname );
    }

    ascostream astream( strm );
    astream.newParagraph();
    par_.putTo( astream );

    surface_.unRef();
    delete &par_;
    delete conn_;
    delete writerowrange_;
    delete writecolrange_;
    delete ioobj_;
}


int dgbSurfaceWriter::nrSections() const
{
    return surface_.nrSections();
}


SectionID dgbSurfaceWriter::sectionID( int idx ) const
{
    return surface_.geometry.sectionID(idx);
}


const char* dgbSurfaceWriter::sectionName( int idx ) const
{
    return surface_.geometry.sectionName(sectionID(idx));
}


void dgbSurfaceWriter::selSections(const TypeSet<SectionID>& sel)
{
    sectionsel_ = sel;
}


int dgbSurfaceWriter::nrAuxVals() const
{
    return surface_.auxdata.nrAuxData();
}


const char* dgbSurfaceWriter::auxDataName( int idx ) const
{
    return surface_.auxdata.auxDataName(idx);
}


void dgbSurfaceWriter::selAuxData(const TypeSet<int>& sel )
{
    auxdatasel_ = sel;
}


const StepInterval<int>& dgbSurfaceWriter::rowInterval() const
{
    return rowrange_;
}


const StepInterval<int>& dgbSurfaceWriter::colInterval() const
{
    return colrange_;
}


void dgbSurfaceWriter::setRowInterval( const StepInterval<int>& rg )
{
    if ( writerowrange_ ) delete writerowrange_;
    writerowrange_ = new StepInterval<int>(rg);
}


void dgbSurfaceWriter::setColInterval( const StepInterval<int>& rg )
{
    if ( writecolrange_ ) delete writecolrange_;
    writecolrange_ = new StepInterval<int>(rg);
}


bool dgbSurfaceWriter::writeOnlyZ() const
{
    return writeonlyz_;
}


void dgbSurfaceWriter::setWriteOnlyZ(bool yn)
{
    writeonlyz_ = yn;
}


IOPar* dgbSurfaceWriter::pars()
{
    return &par_;
}


int dgbSurfaceWriter::nrDone() const
{
    return (executors.size() ? ExecutorGroup::nrDone() : 0) + nrdone_;
}


const char* dgbSurfaceWriter::nrDoneText() const
{
    return "Gridlines written";
}


int dgbSurfaceWriter::totalNr() const
{
    return (executors.size() ? ExecutorGroup::totalNr() : 0) + 
	   (writerowrange_?writerowrange_->nrSteps():rowrange_.nrSteps()) *
	   sectionsel_.size();
}


#define mSetDc( type, string ) \
{ \
    type dummy; \
    DataCharacteristics(dummy).toString( dc.buf() ); \
}\
    versionpar.set( string, dc )

int dgbSurfaceWriter::nextStep()
{
    if ( !ioobj_ ) { msg_ = "No object info"; return ErrorOccurred; }

    if ( !nrdone_ )
    {
	conn_ = dynamic_cast<StreamConn*>(ioobj_->getConn(Conn::Write));
	if ( !conn_ )
	{
	    msg_ = "Cannot open output surface file";
	    return ErrorOccurred;
	}

	IOPar versionpar("Header 1");
	versionpar.set( dgbSurfaceReader::sKeyVersion(), 3 );
	if ( binary_ )
	{
	    BufferString dc;
	    mSetDc( int32, dgbSurfaceReader::sKeyInt32DataChar() );
	    mSetDc( unsigned short, dgbSurfaceReader::sKeyInt16DataChar() );
	    mSetDc( int64, dgbSurfaceReader::sKeyInt64DataChar() );
	    mSetDc( float, dgbSurfaceReader::sKeyFloatDataChar() );
	}


	std::ostream& strm = conn_->oStream();
	ascostream astream( strm );
	astream.putHeader( filetype_ );
	versionpar.putTo( astream );
	nrsectionsoffsetoffset_ = strm.tellp();
	writeInt64( strm, 0, sEOL() );

	for ( int idx=0; idx<auxdatasel_.size(); idx++ )
	{
	    if ( auxdatasel_[idx]>=surface_.auxdata.nrAuxData() )
		continue;

	    BufferString fnm( dgbSurfDataWriter::createHovName( 
		    			conn_->fileName(),auxdatasel_[idx]) );
	    add(new dgbSurfDataWriter(surface_,auxdatasel_[idx],0,binary_,fnm));
	    // TODO:: Change binid sampler so not all values are written when
	    // there is a subselection
	}
    }

    if ( sectionindex_>=sectionsel_.size() )
    {
	const int res = ExecutorGroup::nextStep();
	if ( !res ) const_cast<Surface*>(&surface_)->resetChangedFlag();
	return res;
    }

    std::ostream& strm = conn_->oStream();

    if ( sectionindex_!=oldsectionindex_ && !writeNewSection( strm ) )
	return ErrorOccurred;

    if ( !writeRow( strm ) )
	return ErrorOccurred;
	    
    rowindex_++;
    if ( rowindex_>=nrrows_ )
    {
	strm.seekp( rowoffsettableoffset_, std::ios_base::beg );
	if ( !strm )
	{
	    msg_ = sMsgWriteError();
	    return ErrorOccurred;
	}

	for ( int idx=0; idx<nrrows_; idx++ )
	{
	    if ( !writeInt64(strm,rowoffsettable_[idx],sEOL() ) )
	    {
		msg_ = sMsgWriteError();
		return ErrorOccurred;
	    }
	}

	rowoffsettable_.erase();

	strm.seekp( 0, std::ios_base::end );
	if ( !strm )
	{
	    msg_ = sMsgWriteError();
	    return ErrorOccurred;
	}

	sectionindex_++;
	strm.flush();
	if ( !strm )
	{
	    msg_ = sMsgWriteError();
	    return ErrorOccurred;
	}
    }

    nrdone_++;
    return MoreToDo;
}


const char* dgbSurfaceWriter::message() const
{
    return msg_;
}


bool dgbSurfaceWriter::writeInt16( std::ostream& strm, unsigned short val,
				   const char* post) const
{
    if ( binary_ )
	strm.write((const char*)&val,sizeof(val));
    else
	strm << val << post;

    return strm;
}


bool dgbSurfaceWriter::writeInt32( std::ostream& strm, int32 val,
				   const char* post) const
{
    if ( binary_ )
	strm.write((const char*)&val,sizeof(val));
    else
	strm << val << post;

    return strm;
}


bool dgbSurfaceWriter::writeInt64( std::ostream& strm, int64 val,
				   const char* post) const
{
    if ( binary_ )
	strm.write((const char*)&val,sizeof(val));
    else
    {
	BufferString str;
	sprintf( str.buf(), "%020lld%s", val, post );
	strm << str.buf();
    }

    return strm;
}


bool dgbSurfaceWriter::writeNewSection( std::ostream& strm )
{
    const Geometry::ParametricSurface* gsurf =
		surface_.geometry.getSurface( sectionsel_[sectionindex_] );

    StepInterval<int> sectionrange = gsurf->rowRange();
    sectionrange.sort();
    firstrow_ = sectionrange.start;
    int lastrow = sectionrange.stop;

    if ( writerowrange_ )
    {
	if ( firstrow_>writerowrange_->stop || lastrow<writerowrange_->start)
	    nrrows_ = 0;
	else
	{
	    if ( firstrow_<writerowrange_->start )
		firstrow_ = writerowrange_->start;

	    if ( lastrow>writerowrange_->stop )
		lastrow = writerowrange_->stop;

	    firstrow_ = writerowrange_->snap( firstrow_ );
	    lastrow = writerowrange_->snap( lastrow );

	    nrrows_ = (lastrow-firstrow_)/writerowrange_->step+1;
	}
    }
    else
    {
	nrrows_ = (lastrow-firstrow_)/rowrange_.step+1;
    }

    sectionoffsets_ += strm.tellp();

    if ( !writeInt32(strm,nrrows_, nrrows_ ? sTab() : sEOL() ) )
    {
	msg_ = sMsgWriteError();
	return false;
    }

    if ( !nrrows_ )
    {
	sectionindex_++;
	nrdone_++;
	return MoreToDo;
    }

    if ( !writeInt32(strm,firstrow_,sEOL()) )
    {
	msg_ = sMsgWriteError();
	return ErrorOccurred;
    }

    rowoffsettableoffset_ = strm.tellp();
    for ( int idx=0; idx<nrrows_; idx++ )
    {
	if ( !writeInt64(strm,0,sEOL() ) )
	{
	    msg_ = sMsgWriteError();
	    return ErrorOccurred;
	}
    }

    oldsectionindex_ = sectionindex_;
    rowindex_ = 0;

    return true;
}


bool dgbSurfaceWriter::writeRow( std::ostream& strm )
{
    rowoffsettable_ += strm.tellp();
    const int row = firstrow_+rowindex_ *
		    (writerowrange_?writerowrange_->step:rowrange_.step);

    const SectionID sectionid = surface_.geometry.sectionID(sectionindex_);
    TypeSet<Coord3> colcoords;

    int firstcol = -1;
    const int nrcols =
	(writecolrange_?writecolrange_->nrSteps():colrange_.nrSteps())+1;

    for ( int colindex=0; colindex<nrcols; colindex++ )
    {
	const int col = writecolrange_ ? writecolrange_->atIndex(colindex) :
	    				colrange_.atIndex(colindex);

	const PosID posid(  surface_.id(), sectionid,
				RowCol(row,col).getSerialized() );
	const Coord3 pos = surface_.getPos(posid);

	if ( !colcoords.size() && !pos.isDefined() )
	    continue;

	if ( !colcoords.size() )
	    firstcol = col;

	colcoords += pos;
    }

    for ( int idx=colcoords.size()-1; idx>=0; idx-- )
    {
	if ( colcoords[idx].isDefined() )
	    break;

	colcoords.remove(idx);
    }

    if ( !writeInt32(strm,colcoords.size(),colcoords.size()?sTab():sEOL()) )
    {
	msg_ = sMsgWriteError();
	return false;
    }

    if ( colcoords.size() )
    {
	if ( !writeInt32(strm,firstcol,sTab()) )
	{
	    msg_ = sMsgWriteError();
	    return false;
	}

	SamplingData<float> sd;
	if ( writeonlyz_ )
	{
	    Interval<float> rg;
	    bool isset = false;
	    for ( int idx=0; idx<colcoords.size(); idx++ )
	    {
		const Coord3 pos = colcoords[idx];
		if ( Values::isUdf(pos.z) )
		    continue;

		if ( isset )
		    rg.include( pos.z );
		else
		{
		    rg.start = rg.stop = pos.z;
		    isset = true;
		}
	    }

	    sd.start = rg.start;
	    sd.step = rg.width()/65534;

	    if ( !writeFloat( strm, sd.start, sTab()) ||
		 !writeFloat( strm, sd.step, sEOLTab()) )
	    {
		msg_ = sMsgWriteError();
		return false;
	    }
	}

	for ( int idx=0; idx<colcoords.size(); idx++ )
	{
	    const Coord3 pos = colcoords[idx];
	    if ( writeonlyz_ )
	    {
		const int index = mIsUdf(pos.z)
		    ? 0xFFFF : sd.nearestIndex(pos.z);

		if ( !writeInt16( strm, index, 
			          idx!=colcoords.size()-1 ? sEOLTab() : sEOL()))
		{
		    msg_ = sMsgWriteError();
		    return false;
		}
	    }
	    else
	    {
		if ( !writeFloat( strm, pos.x, sTab()) ||
		     !writeFloat( strm, pos.y, sTab()) ||
		     !writeFloat( strm, pos.z,
			          idx!=colcoords.size()-1 ? sEOLTab() : sEOL()))
		{
		    msg_ = sMsgWriteError();
		    return false;
		}
	    }
	}

	writtenrowrange_.include( row, false );
	writtencolrange_.include( firstcol, false );
	writtencolrange_.include( firstcol+colrange_.step*(colcoords.size()-1),
				  false );
    }

    return true;
}


bool dgbSurfaceWriter::writeFloat( std::ostream& strm, float val,
				       const char* post) const
{
    if ( binary_ )
	strm.write((const char*) &val,sizeof(val));
    else
	strm << val << post;;

    return strm;
}

}; //namespace

/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "emsurfaceio.h"

#include "arrayndimpl.h"
#include "ascstream.h"
#include "binidsurface.h"
#include "datachar.h"
#include "datainterp.h"
#include "emfault3d.h"
#include "emfaultstickset.h"
#include "emhorizon2d.h"
#include "emhorizon3d.h"
#include "empolygonbody.h"
#include "emsurfaceauxdata.h"
#include "emsurfacegeometry.h"
#include "emsurfauxdataio.h"
#include "file.h"
#include "filepath.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "math2.h"
#include "od_istream.h"
#include "paralleltask.h"
#include "posinfo2dsurv.h"
#include "ptrman.h"
#include "samplingdata.h"
#include "streamconn.h"
#include "survgeom2d.h"
#include "survinfo.h"
#include "uistrings.h"
#include "unitofmeasure.h"
#include "zdomain.h"

#include <istream>
#include <limits.h>
#include <streambuf>


namespace EM
{

static uiString sMsgWriteError()
{ return uiStrings::phrCannotWrite( uiStrings::sSurface() ); }


const char* dgbSurfaceReader::sKeyFloatDataChar() { return "Data char"; }

const char* dgbSurfaceReader::sKeyInt16DataChar() { return "Int 16 Data char";}
const char* dgbSurfaceReader::sKeyInt32DataChar() { return "Int Data char";}
const char* dgbSurfaceReader::sKeyInt64DataChar() { return "Int 64 Data char";}
const char* dgbSurfaceReader::sKeyNrSectionsV1()  { return "Nr Subhorizons" ; }
const char* dgbSurfaceReader::sKeyNrSections()    { return "Nr Patches"; }
const char* dgbSurfaceReader::sKeyRowRange()	  { return "Row range"; }
const char* dgbSurfaceReader::sKeyColRange()	  { return "Col range"; }
const char* dgbSurfaceReader::sKeyZRange()	  { return "Z range"; }
const char* dgbSurfaceReader::sKeyDepthOnly()	  { return "Depth only"; }
const char* dgbSurfaceReader::sKeyDBInfo()	  { return "DB info"; }
const char* dgbSurfaceReader::sKeyVersion()	  { return "Format version"; }

const char* dgbSurfaceReader::sKeyTransformX()	{ return "X transform"; }
const char* dgbSurfaceReader::sKeyTransformY()	{ return "Y transform"; }

const char* dgbSurfaceReader::sMsgParseError()  { return "Cannot parse file"; }
uiString dgbSurfaceReader::sMsgReadError()
{ return tr("Unexpected end of file"); }
const char* dgbSurfaceReader::sKeyUndefLineSet() {return "Undfined line set"; }
const char* dgbSurfaceReader::sKeyUndefLine()	{ return "Undfined line name"; }

BufferString dgbSurfaceReader::sSectionIDKey( int idx )
{ BufferString res = "Patch "; res += idx; return res;  }


BufferString dgbSurfaceReader::sSectionNameKey( int idx )
{ BufferString res = "Patch Name "; res += idx; return res;  }

BufferString dgbSurfaceReader::sColStepKey( int idx )
{ BufferString res = "Col step "; res += idx; return res;  }


dgbSurfaceReader::dgbSurfaceReader( const IOObj& ioobj,
				    const char* filetype )
    : ExecutorGroup( "Surface Reader" )
    , filename_(ioobj.fullUserExpr(true))
{
    zinfo_ = ZDomain::Info::getFrom( ioobj.pars() );
    init( filetype, ioobj.name() );
}


dgbSurfaceReader::dgbSurfaceReader( const char* fulluserexp,
				    const char* objname,
				    const char* filetype )
    : ExecutorGroup( "Surface Reader" )
    , filename_(fulluserexp)
{
    zinfo_ = nullptr;
    init( filetype, objname );
}



void dgbSurfaceReader::init( const char* filetype, const char* objname )
{
    zrange_ = Interval<float>(mUdf(float),mUdf(float));
    linenames_.setNullAllowed();

    BufferString exnm( "Reading surface '", objname, "'" );
    setName( exnm.buf() );
    setNrDoneText( Task::uiStdNrDoneText() );
    auxdataexecs_.allowNull(true);
    StreamConn conn( filename_, Conn::Read );
    error_ = !readHeaders( conn, filetype );
    createAuxDataReader();
}


void dgbSurfaceReader::setOutput( EM::Surface& ns )
{
    surface_ = &ns;
}


void dgbSurfaceReader::setOutput( Array3D<float>& cube )
{
    cube_ = &cube;
}


bool dgbSurfaceReader::readParData( od_istream& strm, const IOPar& toppar,
					const char* horfnm )
{
    if ( version_ == 3 )
    {
	const od_stream::Pos nrsectionsoffset = readInt64( strm );
	if ( !strm.isOK() || !nrsectionsoffset )
	{ msg_ = sMsgReadError(); return false; }

	strm.setReadPosition( nrsectionsoffset );
	const int nrsections = readInt32( strm );
	if ( !strm.isOK() ) { msg_ = sMsgReadError(); return false; }

	sectionoffsets_.setEmpty();
	for ( int idx=0; idx<nrsections; idx++ )
	{
	    const od_int64 off = readInt64( strm );
	    if ( !off ) { msg_ = sMsgReadError(); return false; }
	    sectionoffsets_ += off;
	}

	sectionids_.setEmpty();
	for ( int idx=0; idx<nrsections; idx++ )
	    sectionids_ += readInt32(strm);

	if ( !strm.isOK() ) { msg_ = sMsgReadError(); return false; }

	parsoffset_ = mCast(int,strm.position());
	ascistream parstream( strm, false );
	parstream.next();
	delete par_;
	par_ = new IOPar( parstream );
    }
    else
    {
	int nrsections;
	if ( !toppar.get( sKeyNrSections() , nrsections ) &&
	     !toppar.get( sKeyNrSectionsV1(), nrsections ) )
	{
	    msg_ = uiStrings::phrCannotRead( uiStrings::sSurface() );
	    return false;
	}

	sectionids_.setEmpty();
	for ( int idx=0; idx<nrsections; idx++ )
	{
	    int sectionid = idx;
	    BufferString key = sSectionIDKey( idx );
	    toppar.get(key.buf(), sectionid);
	    sectionids_ += sectionid;
	}

	delete par_;
	par_ = new IOPar( toppar );
    }

    mergeExternalPar( horfnm );
    return true;
}


void dgbSurfaceReader::mergeExternalPar( const char* horfnm )
{
    FilePath fp( horfnm ); fp.setExtension( "par" );
    od_istream strm( fp );
    if ( !strm.isOK() )
	return;
    IOPar par;
    if ( par.read(strm,"Surface parameters") )
	const_cast<IOPar*>(par_)->merge( par );
}


int dgbSurfaceReader::scanFor2DGeom( TypeSet< StepInterval<int> >& trcranges )
{
    TypeSet<int> lineids; bool is2d = false;

    const bool haslinenames = !linenames_.isEmpty();
    geomids_.setEmpty();
    trcranges.setEmpty();
    if ( par_->hasKey(Horizon2DGeometry::sKeyNrLines()) )
    {
	is2d = true;
	int nrlines = 0;
	par_->get( Horizon2DGeometry::sKeyNrLines(), nrlines );
	for ( int idx=0; idx<nrlines; idx++ )
	{
	    BufferString key = IOPar::compKey( "Line", idx );
	    Pos::GeomID geomid;
	    if ( !par_->get(IOPar::compKey(sKey::GeomID(),idx),geomid) )
	    {
		BufferString idstr;
		if ( par_->get(IOPar::compKey(key,Horizon2DGeometry::sKeyID()),
			       idstr) )
		{
		    PosInfo::Line2DKey l2dkey; l2dkey.fromString( idstr );
		    if ( S2DPOS().curLineSetID() != l2dkey.lsID() )
			S2DPOS().setCurLineSet( l2dkey.lsID() );
		    geomid = Survey::GM().getGeomID(
				S2DPOS().getLineSet(l2dkey.lsID()),
				S2DPOS().getLineName(l2dkey.lineID()) );
		}
	    }

	    if ( !geomid.is2D() )
		continue;

	    geomids_ += geomid;
	    if ( !haslinenames )
		linenames_.add( Survey::GM().getName(geomid) );

	    StepInterval<int> trcrange;
	    par_->get( IOPar::compKey(key,Horizon2DGeometry::sKeyTrcRg()),
		       trcrange );
	    trcranges += trcrange;
	}
    }
    else if ( par_->get(Horizon2DGeometry::sKeyLineIDs(),lineids) )
    {
	is2d = true;
	if ( linenames_.size() != lineids.size() )
	    { msg_ = tr("Inconsistency in horizon file header"); return -1; }

	for ( int idx=0; idx<lineids.size(); idx++ )
	{
	    BufferString linesetkey(Horizon2DGeometry::sKeyLineSets(),idx);
	    MultiID mid;
	    par_->get( linesetkey, mid );
	    PtrMan<IOObj> ioobj = IOM().get( mid );
	    if ( !ioobj )
	    {
		lineids[idx] = mUdf(int);
		geomids_ += Pos::GeomID::udf();
		trcranges += StepInterval<int>::udf();
		continue;
	    }

	    const Pos::GeomID geomid = Survey::GM().getGeomID( ioobj->name(),
							 linenames_.get(idx) );
	    geomids_ += geomid;
	    BufferString trcrangekey(
		    Horizon2DGeometry::sKeyTraceRange(), idx );

	    StepInterval<int> trcrange( mUdf(int), mUdf(int), 1 );
	    par_->get( trcrangekey, trcrange );
	    trcranges += trcrange;
	}

	int idx = 0;
	while ( idx<lineids.size() )
	{
	    if ( mIsUdf(lineids[idx]) || !geomids_[idx].is2D() )
	    {
		lineids.removeSingle( idx );
		linenames_.removeSingle( idx );
		geomids_.removeSingle( idx );
		trcranges.removeSingle( idx );
		continue;
	    }

	    idx++;
	}
    }

    return is2d ? 1 : 0;
}


bool dgbSurfaceReader::readHeaders( StreamConn& conn, const char* filetype )
{
    od_istream& strm = conn.iStream();
    if ( !strm.isOK() )
    {
	msg_ = tr("Could not open horizon file"); strm.addErrMsgTo( msg_ );
	return false;
    }

    ascistream astream( strm );
    if ( filetype && !astream.isOfFileType(filetype) )
    {
	msg_ = tr("Horizon file has wrong file type");
	return false;
    }

    version_ = 1;
    astream.next();
    IOPar toppar( astream );
    toppar.get( sKeyVersion(), version_ );

    BufferString dc;
#define mGetDataChar( type, str, interpr ) \
    delete interpr; \
    interpr = DataInterpreter<type>::create( toppar, str, true )
    mGetDataChar( int, sKeyInt16DataChar(), int16interpreter_ );
    mGetDataChar( int, sKeyInt32DataChar(), int32interpreter_ );
    mGetDataChar( od_int64, sKeyInt64DataChar(), int64interpreter_ );
    mGetDataChar( double, sKeyFloatDataChar(), floatinterpreter_ );

    if ( !readParData(strm,toppar,filename_) )
	return false;

    par_->get( sKeyRowRange(), rowrange_ );
    par_->get( sKeyColRange(), colrange_ );
    par_->get( sKeyZRange(), zrange_ );
    linenames_.setEmpty();
    par_->get( Horizon2DGeometry::sKeyLineNames(), linenames_ );

    const ZDomain::Info* zinfo = ZDomain::Info::getFrom( *par_ );
    if ( !zinfo )
	zinfo = UnitOfMeasure::zDomain( *par_ );

    if ( zinfo && !zinfo_ )
	zinfo_ = zinfo;

    TypeSet< StepInterval<int> > trcranges;
    const int res = scanFor2DGeom( trcranges );
    if ( res < 0 )
	return false;

    const bool is2d = res;
    setLinesTrcRngs( trcranges );

    par_->get( sKeyDBInfo(), dbinfo_ );

    bodystart_ = strm.position();
    if ( version_==1 )
	return parseVersion1( *par_ );

    if ( is2d && linesets_.isEmpty() && geomids_.isEmpty() )
    {
	msg_ = tr("No geometry found for this horizon");
	return false;
    }

    return true;
}


void dgbSurfaceReader::createAuxDataReader()
{
    int gap = 0;
    for ( int idx=0; ; idx++ )
    {
	if ( gap > 50 ) break;

	BufferString hovfnm( dgbSurfDataWriter::createHovName(filename_,idx) );
	if ( File::isEmpty(hovfnm.buf()) )
	{
	    gap++;
	    continue;
	}

	dgbSurfDataReader* dreader = new dgbSurfDataReader( hovfnm.buf() );
	if ( dreader->dataName() )
	{
	    auxdatanames_ += new BufferString(dreader->dataName());
	    auxdatashifts_ += dreader->shift();
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
    linenames_.setEmpty();
    auxdatanames_.setEmpty();
    deepErase( auxdataexecs_ );

    delete par_;
    delete conn_;
    delete readrowrange_;
    delete readcolrange_;
    delete readlinenames_;
    delete linestrcrgs_;

    delete int16interpreter_;
    delete int32interpreter_;
    delete int64interpreter_;
    delete floatinterpreter_;
    if ( surface_ )
	surface_->geometry().resetChangedFlag();
}


bool dgbSurfaceReader::isOK() const
{
    return !error_;
}


int dgbSurfaceReader::nrSections() const
{
    return sectionids_.size();
}


SectionID dgbSurfaceReader::sectionID( int ) const
{
    return SectionID::def();
}


Strat::LevelID dgbSurfaceReader::stratLevelID() const
{
    Strat::LevelID ret;
    if ( pars() )
	pars()->get( sKey::StratRef(), ret );
    return ret;
}


BufferString dgbSurfaceReader::sectionName( int idx ) const
{
    BufferString res = "[";
    res += idx;
    res += "]";
    return res;
}


int dgbSurfaceReader::nrLines() const
{ return geomids_.isEmpty() ? linenames_.size() : geomids_.size(); }


BufferString dgbSurfaceReader::lineName( int idx ) const
{
    return linenames_.validIdx(idx) ? linenames_.get( idx )
				    : BufferString::empty();
}


BufferString dgbSurfaceReader::lineSet( int idx ) const
{
    return linesets_.validIdx( idx ) ? linesets_.get( idx )
				     : BufferString::empty();
}


Pos::GeomID dgbSurfaceReader::lineGeomID( int idx ) const
{
    if ( geomids_.validIdx(idx) )
	return geomids_[idx];

    return Pos::GeomID::udf();
}


void dgbSurfaceReader::selSections(const TypeSet<SectionID>& sel)
{
    sectionindex_ = 0;
    oldsectionindex_ = -1;
}


int dgbSurfaceReader::nrAuxVals() const
{
    return auxdatanames_.size();
}


const char* dgbSurfaceReader::auxDataName( int idx ) const
{
    return auxdatanames_[idx]->buf();
}


float dgbSurfaceReader::auxDataShift( int idx ) const
{ return auxdatashifts_[idx]; }


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


const Interval<float>& dgbSurfaceReader::zInterval() const
{
    return zrange_;
}


const ZDomain::Info& dgbSurfaceReader::zDomain() const
{
    return zinfo_ ? *zinfo_ : SI().zDomainInfo();
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


void dgbSurfaceReader::setLineNames( const BufferStringSet& lns )
{
    if ( readlinenames_ ) delete readlinenames_;
    readlinenames_ = new BufferStringSet( lns );
}


StepInterval<int> dgbSurfaceReader::lineTrcRanges( int idx ) const
{
    return linestrcrgs_->validIdx(idx)
		    ? (*linestrcrgs_)[idx]
		    : StepInterval<int>(mUdf(int),mUdf(int),mUdf(int));
}


void dgbSurfaceReader::setLinesTrcRngs( const TypeSet<StepInterval<int> >& rgs )
{
    if ( linestrcrgs_ ) delete linestrcrgs_;
    linestrcrgs_ = new TypeSet<StepInterval<int> >( rgs );
}


void dgbSurfaceReader::setReadOnlyZ( bool yn )
{
    readonlyz_ = yn;
}


const IOPar* dgbSurfaceReader::pars() const
{
    return par_;
}


int dgbSurfaceReader::getParsOffset() const
{ return parsoffset_; }


od_int64 dgbSurfaceReader::nrDone() const
{
    return (executors_.size() ? ExecutorGroup::nrDone() : 0) + nrdone_;
}


uiString dgbSurfaceReader::uiNrDoneText() const
{
    return tr("Gridlines read");
}


od_int64 dgbSurfaceReader::totalNr() const
{
    int ownres =
	readrowrange_ ? readrowrange_->nrSteps() : rowrange_.nrSteps();

    if ( !ownres )
	ownres = nrrows_;

    return ownres + (executors_.size() ? ExecutorGroup::totalNr() : 1);
}


void dgbSurfaceReader::prepareSurface()
{
    if ( surface_ )
    {
	surface_->removeAll();
	surface_->setDBInfo( dbinfo_.buf() );
    }

    if ( readrowrange_ )
    {
	const RowCol filestep = getFileStep();

        if ( readrowrange_->step_ < abs(filestep.row()) )
            readrowrange_->step_ = filestep.row();
        if ( readcolrange_->step_ < abs(filestep.col()) )
            readcolrange_->step_ = filestep.col();

        if ( filestep.row() && readrowrange_->step_ / filestep.row() < 0 )
            readrowrange_->step_ *= -1;
        if ( filestep.col() && readcolrange_->step_ / filestep.col() < 0 )
            readcolrange_->step_ *= -1;
    }

    mDynamicCastGet(Horizon3D*,hor,surface_.ptr());
    if ( hor )
    {
	const RowCol filestep = getFileStep();
	const RowCol loadedstep = readrowrange_ && readcolrange_
			    ? RowCol(readrowrange_->step_,readcolrange_->step_)
			    : filestep;

	hor->geometry().setStep( filestep, loadedstep );
    }
}


void dgbSurfaceReader::setGeometry()
{
    prepareSurface();
    if ( !surface_ )
	return;

    for ( int idx=0; idx<auxdatasel_.size(); idx++ )
    {
	if ( auxdatasel_[idx]>=auxdataexecs_.size() )
	    continue;

	auxdataexecs_[auxdatasel_[idx]]->setSurface(
		reinterpret_cast<Horizon3D&>(*surface_) );

	add( auxdataexecs_[auxdatasel_[idx]] );
	auxdataexecs_.replace( auxdatasel_[idx], 0 );
    }
}


bool dgbSurfaceReader::readRowOffsets( od_istream& strm )
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
	if ( !strm.isOK() || !rowoffsets_[idx] )
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
        return RowCol(rowrange_.step_, colrange_.step_);

    return convertRowCol(1,1)-convertRowCol(0,0);
}


bool dgbSurfaceReader::shouldSkipCurrentRow() const
{
    const int row = currentRow();
    if ( version_==1 || (version_==2 && !isBinary()) )
	return false;

    if ( readlinenames_ && linenames_.validIdx(rowindex_) )
    {
	const BufferString& curlinenm = linenames_.get( rowindex_ );
	return !readlinenames_->isPresent( curlinenm.buf() );
    }

    if ( !readrowrange_ )
	return false;

    if ( !readrowrange_->includes( row, false ) )
	return true;

    return (row-readrowrange_->start_)%readrowrange_->step_;
}


int dgbSurfaceReader::currentRow() const
{ return firstrow_+rowindex_*rowrange_.step_; }


bool dgbSurfaceReader::doPrepare( od_ostream* strm )
{
    isinited_ = true;
    if ( !conn_ )
    {
	conn_ = new StreamConn( filename_, Conn::Read );
	if ( !readHeaders(*conn_,nullptr) )
	    return false;
    }

    if ( surface_ )
	surface_->enableGeometryChecks( false );

    setGeometry();
    par_->getYN( sKeyDepthOnly(), readonlyz_ );

    return executors_.isEmpty() ? true : ExecutorGroup::doPrepare( strm );
}


int dgbSurfaceReader::nextStep()
{
    if ( error_ || (!surface_ && !cube_) )
    {
	if ( !surface_ && !cube_ )
	    msg_ = ::toUiString("Internal: No Output Set");

	return ErrorOccurred();
    }

    od_istream& strm = conn_->iStream();

    if ( sectionindex_ >= sectionids_.size() )
    {
	if ( !surface_ )
	    return Finished();

	int res = ExecutorGroup::nextStep();
	if ( !res && !setsurfacepar_ )
	{
	    setsurfacepar_ = true;
	    if ( !surface_->usePar(*par_) )
	    {
		msg_ = tr("Could not parse header");
		return ErrorOccurred();
	    }

	    surface_->setFullyLoaded( fullyread_ );
	    surface_->resetChangedFlag();
	    surface_->enableGeometryChecks(true);
	}

	return res;
    }

    if ( sectionindex_!=oldsectionindex_ )
    {
	const int res = prepareNewSection(strm);
	if ( res!=Finished() )
	    return res;
    }

    while ( shouldSkipCurrentRow() )
    {
	if ( rowrange_.includes(currentRow(), false) )
	    fullyread_ = false;

	const int res = skipRow( strm );
	if ( res==ErrorOccurred() )
	    return res;
	else if ( res==Finished() ) //Section change
	    return MoreToDo();
    }

    if ( !prepareRowRead(strm) )
    {
	msg_ = strm.errMsg();
	return ErrorOccurred();
    }

    int nrcols = readInt32( strm );

    int firstcol = nrcols ? readInt32( strm ) : 0;
    int noofcoltoskip = 0;

    if ( readlinenames_ && linestrcrgs_ && !linenames_.isEmpty() )
    {
	const int trcrgidx =
	    readlinenames_->indexOf( linenames_.get(rowindex_).buf() );
	int callastcols = ( firstcol - 1 ) + nrcols;

	StepInterval<int> trcrg =
	    linestrcrgs_->validIdx(trcrgidx) ? (*linestrcrgs_)[trcrgidx]
					     : StepInterval<int>(0,0,1);
	if ( trcrg.width() > 1 )
	{
            if ( firstcol < trcrg.start_ )
                noofcoltoskip = trcrg.start_ - firstcol;

            if ( trcrg.stop_ < callastcols )
                callastcols = trcrg.stop_;
	}

	nrcols = callastcols - firstcol - noofcoltoskip + 1;
    }

    if ( !strm.isOK() )
    {
	msg_ = sMsgReadError();
	return ErrorOccurred();
    }

    int colstep = colrange_.step_;
    par_->get( sColStepKey( currentRow() ).buf(), colstep );

    mDynamicCastGet(Horizon2D*,hor2d,surface_.ptr());

    if ( hor2d )
    {
	const bool validrowidx = linesets_.validIdx(rowindex_) &&
				 linenames_.validIdx(rowindex_);
	const bool validids = validrowidx &&
			linesets_.get(rowindex_)!=sKeyUndefLineSet() &&
			linenames_.get(rowindex_)!=sKeyUndefLine();
        const bool validgeomids = geomids_.validIdx(rowindex_) &&
				  geomids_[rowindex_].isValid();

	if ( (!validrowidx || !validids) && !validgeomids )
	{
	    const int res = skipRow( strm );
	    if ( res==ErrorOccurred() )
		return res;
	    else if ( res==Finished() )
		return MoreToDo();
	}

	createArray();
	if ( geomids_.validIdx(rowindex_) )
	{
	    const Pos::GeomID geomid = geomids_[rowindex_];
	    mDynamicCastGet( const Survey::Geometry2D*, geom2d,
			     Survey::GM().getGeometry(geomid) );
	    if ( !geom2d )
		return skipRow(strm) == ErrorOccurred() ?
					ErrorOccurred() : MoreToDo();

	    const int startcol = firstcol + noofcoltoskip;
	    const int stopcol = firstcol + noofcoltoskip + colstep*(nrcols - 1);
	    hor2d->geometry().geometryElement()->addUdfRow(
			    geomid, startcol, stopcol, colstep );
	}
    }

    mDynamicCastGet(Fault3D*,flt3d,surface_.ptr());
    if ( flt3d )
    {
	createArray();
	flt3d->geometry().geometryElement()->
	    addUdfRow( currentRow(), firstcol, nrcols );
    }

    mDynamicCastGet(FaultStickSet*,emfss,surface_.ptr());
    if ( emfss )
    {
	createArray();
	emfss->geometry().geometryElement()->
	    addUdfRow( currentRow(), firstcol, nrcols );
    }

    mDynamicCastGet(PolygonBody*,polygon,surface_.ptr());
    if ( polygon )
    {
	createArray();
	polygon->geometry().geometryElement()->
	    addUdfPolygon( currentRow(), firstcol, nrcols );
    }

    if ( !nrcols )
    {
	goToNextRow();
	return MoreToDo();
    }

    if ( (version_==3 && !readVersion3Row(strm, firstcol, nrcols, colstep,
					  noofcoltoskip) ) ||
         ( version_==2 && !readVersion2Row(strm, firstcol, nrcols) ) ||
         ( version_==1 && !readVersion1Row(strm, firstcol, nrcols) ) )
	return ErrorOccurred();

    goToNextRow();
    return MoreToDo();
}


bool dgbSurfaceReader::doFinish( bool success, od_ostream*  )
{
    surface_->convertZValues( surface_->surveyStorageUnit(), true );
    if ( surface_->zDomain().isDepth() )
    {
	const auto& zdom = ZDomain::Info::getFrom(
			   surface_->zDomain().key(),
			   surface_->surveyStorageUnit()->getLabel() );
	surface_->setZDomain( zdom );
    }

    surface_->resetChangedFlag();

    deleteAndNullPtr( conn_ );
    return success;
}


int dgbSurfaceReader::prepareNewSection( od_istream& strm )
{
    if ( version_==3 )
	strm.setReadPosition( sectionoffsets_[0] );

    nrrows_ = readInt32( strm );
    if ( !strm.isOK() )
    {
	msg_ = strm.errMsg();
        return ErrorOccurred();
    }

    if ( nrrows_ )
    {
	firstrow_ = readInt32( strm );
	if ( !strm.isOK() )
	{
	    msg_ = sMsgReadError();
	    return ErrorOccurred();
	}
    }
    else
    {
	sectionindex_++;
	sectionsread_++;
	nrdone_ = sectionsread_ *
	    ( readrowrange_ ? readrowrange_->nrSteps() : rowrange_.nrSteps() );

	return MoreToDo();
    }

    if ( version_==3 && readrowrange_ )
    {
	const StepInterval<int> sectionrowrg( firstrow_,
		       firstrow_+(nrrows_-1)*rowrange_.step_, rowrange_.step_ );
        if ( sectionrowrg.stop_<readrowrange_->start_ ||
             sectionrowrg.start_>readrowrange_->stop_ )
	{
	    sectionindex_++;
	    sectionsread_++;
	    nrdone_ = sectionsread_ *
	       (readrowrange_ ? readrowrange_->nrSteps() : rowrange_.nrSteps());
	    return MoreToDo();
	}
    }

    rowindex_ = 0;

    if ( version_==3 && !readRowOffsets( strm ) )
	return ErrorOccurred();

    oldsectionindex_ = sectionindex_;
    return Finished();
}


bool dgbSurfaceReader::readVersion1Row( od_istream& strm, int firstcol,
					int nrcols )
{
    bool isrowused = false;
    const int filerow = currentRow();
    for ( int colindex=0; colindex<nrcols; colindex++ )
    {
        const int filecol = firstcol+colindex*colrange_.step_;

	RowCol surfrc = convertRowCol( filerow, filecol );
	Coord3 pos;
	if ( !readonlyz_ )
	{
            pos.x_ = readDouble( strm );
            pos.y_ = readDouble( strm );
	}

        pos.z_ = readDouble( strm );

	//Read filltype
	if ( rowindex_!=nrrows_-1 && colindex!=nrcols-1 )
	    readInt32( strm );

	if ( !strm.isOK() )
	{
	    msg_ = sMsgReadError();
	    return false;
	}

	if ( readrowrange_ && (!readrowrange_->includes(surfrc.row(), false)
	    || ((surfrc.row()-readrowrange_->start_)%readrowrange_->step_)))
	{
	    fullyread_ = false;
	    continue;
	}

	if ( readcolrange_ && (!readcolrange_->includes(surfrc.col(), false)
	    || ((surfrc.col()-readcolrange_->start_)%readcolrange_->step_)))
	{
	    fullyread_ = false;
	    continue;
	}

	createArray();
	if ( arr_ )
	{
	    int i, j;
	    if ( getIndices(surfrc,i,j) )
		arr_->set( i, j, mCast(float,pos.z_) );

	}
	else
	    surface_->setPos( surfrc.toInt64(), pos, false );

	isrowused = true;
    }

    if ( isrowused )
	nrdone_++;

    return true;
}


bool dgbSurfaceReader::readVersion2Row( od_istream& strm,
					int firstcol, int nrcols )
{
    const int filerow = currentRow();
    bool isrowused = false;
    for ( int colindex=0; colindex<nrcols; colindex++ )
    {
        const int filecol = firstcol+colindex*colrange_.step_;

	const RowCol rowcol( filerow, filecol );
	Coord3 pos;
	if ( !readonlyz_ )
	{
            pos.x_ = readDouble( strm );
            pos.y_ = readDouble( strm );
	}

        pos.z_ = readDouble( strm );
	if ( !strm.isOK() )
	{
	    msg_ = sMsgReadError();
	    return false;
	}

	if ( readcolrange_ && (!readcolrange_->includes(rowcol.col(), false)
	    || ((rowcol.col()-readcolrange_->start_)%readcolrange_->step_)))
	{
	    fullyread_ = false;
	    continue;
	}

	if ( readrowrange_ && (!readrowrange_->includes(rowcol.row(), false)
	    || ((rowcol.row()-readrowrange_->start_)%readrowrange_->step_)))
	{
	    fullyread_ = false;
	    continue;
	}

	createArray();
	if ( arr_ )
	{
	    int i, j;
	    if ( getIndices(rowcol,i,j) )
                arr_->set( i, j, mCast(float,pos.z_) );
	}
	else
	    surface_->setPos( rowcol.toInt64(), pos, false );

	isrowused = true;
    }

    if ( isrowused )
	nrdone_++;

    return true;
}


int dgbSurfaceReader::skipRow( od_istream& strm )
{
    if ( version_!=3 )
    {
	if ( !isBinary() )
	{
	    msg_ = tr("Invalid file.");
	    return ErrorOccurred();
	}

	const int nrcols = readInt32( strm );
	if ( !strm.isOK() )
	{
	    msg_ = strm.errMsg();
	    return ErrorOccurred();
	}

	int offset = 0;
	if ( nrcols )
	{
	    int nrbytespernode = (readonlyz_?1:3)*floatinterpreter_->nrBytes();
	    if ( version_==1 )
		nrbytespernode += int32interpreter_->nrBytes(); //filltype

	    offset += nrbytespernode*nrcols;

	    offset += int32interpreter_->nrBytes(); //firstcol

	    strm.ignore( offset );
	    if ( !strm.isOK() )
	    {
		msg_ = strm.errMsg();
		return ErrorOccurred();
	    }
	}
    }

    goToNextRow();
    return sectionindex_!=oldsectionindex_ ? Finished() : MoreToDo();
}


bool dgbSurfaceReader::prepareRowRead( od_istream& strm )
{
    if ( version_!=3 )
	return true;

    strm.setReadPosition( rowoffsets_[rowindex_] );
    return strm.isOK();
}


void dgbSurfaceReader::goToNextRow()
{
    rowindex_++;
    if ( rowindex_>=nrrows_ )
    {
	if ( surface_ )
	{
	    Geometry::Element* secgeom = surface_->geometry().geometryElement();
	    mDynamicCastGet(Geometry::BinIDSurface*,bidsurf,secgeom)
	    if ( bidsurf )
	    {
		StepInterval<int> inlrg = readrowrange_ ? *readrowrange_
							: rowrange_;
		StepInterval<int> crlrg = readcolrange_ ? *readcolrange_
							: colrange_;
		inlrg.sort(); crlrg.sort();
		if ( arr_ )
                    bidsurf->setArray( RowCol(inlrg.start_,crlrg.start_),
				       RowCol(inlrg.step_,crlrg.step_),
				       arr_, true );
		arr_ = nullptr;
	    }

	    if ( secgeom )
		secgeom->trimUndefParts();
	}

	sectionindex_++;
	sectionsread_++;
	nrdone_ = sectionsread_ *
	    ( readrowrange_ ? readrowrange_->nrSteps() : rowrange_.nrSteps() );
    }
}


bool dgbSurfaceReader::readVersion3Row( od_istream& strm, int firstcol,
					int nrcols, int colstep, int colstoskip)
{
    SamplingData<double> zsd;
    if ( readonlyz_ )
    {
        zsd.start_ = readDouble( strm );
        zsd.step_ = readDouble( strm );
    }

    int colindex = 0;

    if ( readcolrange_ )
    {
	const StepInterval<int> colrg( firstcol, firstcol+(nrcols-1)*colstep,
				       colstep );
        if ( colrg.stop_<readcolrange_->start_ ||
             colrg.start_>readcolrange_->stop_ )
	{
	    fullyread_ = false;
	    return true;
	}

	if ( int16interpreter_ )
	{
            colindex = colrg.nearestIndex( readcolrange_->start_ );
	    if ( colindex<0 )
		colindex = 0;

	    if ( colindex )
	    {
		fullyread_ = false;
		strm.ignore( colindex*int16interpreter_->nrBytes() );
	    }
	}
    }

    if ( !strm.isOK() )
    {
	msg_ = sMsgReadError();
	return false;
    }

    RowCol rc( currentRow(), 0 );
    bool didread = false;

    mDynamicCastGet(Horizon2D*,hor2d,surface_.ptr());
    const bool hor2dok = hor2d && geomids_.validIdx(rowindex_);

    for ( ; colindex<nrcols+colstoskip; colindex++ )
    {
	rc.col() = firstcol+colindex*colstep;
	Coord3 pos;
	if ( !readonlyz_ )
	{
	    double x = readDouble( strm );
	    double y = readDouble( strm );
	    double z = readDouble( strm );

	    if ( colindex < (colstoskip-1) )
		continue;

            pos.x_ = x;
            pos.y_ = y;
            pos.z_ = z;
	}
	else
	{
	    const int zidx = readInt16( strm );

	    if ( colindex < (colstoskip-1) )
		continue;
            pos.z_ = (zidx==65535) ? mUdf(float) : zsd.atIndex( zidx );
	}

	if ( readcolrange_ )
	{
            if ( rc.col()<readcolrange_->start_ )
		continue;

            if ( rc.col()>readcolrange_->stop_ )
		break;

            if ( (rc.col()-readcolrange_->start_)%readcolrange_->step_ )
		continue;
	}

	if ( !strm.isOK() )
	{
	    msg_ = sMsgReadError();
	    return false;
	}

        if ( !Math::IsNormalNumber(pos.z_) || !Math::IsNormalNumber(pos.x_) ||
             !Math::IsNormalNumber(pos.y_) )
	    continue;

	if ( surface_ )
	{
	    createArray();
	    RowCol myrc( rc );
	    if ( hor2dok )
		myrc.row() = hor2d->geometry().geometryElement()
			->getRowIndex( geomids_[rowindex_] );

	    if ( arr_ )
	    {
		int i, j;
		if ( getIndices(myrc,i,j) )
                    arr_->set( i, j, float(pos.z_) );
	    }
	    else
		surface_->setPos( myrc.toInt64(), pos, false );
	}

	if ( cube_ )
	{
	    cube_->set( readrowrange_->nearestIndex(rc.row()),
			readcolrange_->nearestIndex(rc.col()),
                        0, float(pos.z_) );
	}

	didread = true;
    }

    if ( didread )
	nrdone_++;

    return true;
}


bool dgbSurfaceReader::createArray()
{
    mDynamicCastGet(Geometry::BinIDSurface*,bidsurf,
		    surface_->geometry().geometryElement())
    if ( !bidsurf || arr_ )
	return true;

    StepInterval<int> inlrg = readrowrange_ ? *readrowrange_ : rowrange_;
    StepInterval<int> crlrg = readcolrange_ ? *readcolrange_ : colrange_;
    inlrg.sort(); crlrg.sort();

    PtrMan<Array2D<float> > arr =
	new Array2DImpl<float>( inlrg.nrSteps()+1, crlrg.nrSteps()+1 );
    if ( !arr || !arr->isOK() )
	return false;

    arr->setAll( mUdf(float) );
    delete arr_;
    arr_ = arr.release();

    return true;
}


bool dgbSurfaceReader::getIndices( const RowCol& rc, int& i, int& j ) const
{
    StepInterval<int> inlrg = readrowrange_ ? *readrowrange_ : rowrange_;
    StepInterval<int> crlrg = readcolrange_ ? *readcolrange_ : colrange_;
    inlrg.sort(); crlrg.sort();
    if ( !inlrg.includes(rc.row(),false) || !crlrg.includes(rc.col(),false) )
	return false;

    i = inlrg.getIndex( rc.row() );
    j = crlrg.getIndex( rc.col() );
    return true;
}


uiString dgbSurfaceReader::uiMessage() const
{
    return msg_;
}


int dgbSurfaceReader::readInt16(od_istream& strm) const
{
    if ( int16interpreter_ )
    {
	const int sz = int16interpreter_->nrBytes();
	mAllocLargeVarLenArr( char, buf, sz );
	char* bufptr = buf.ptr();
	strm.getBin( bufptr, sz );
	return int16interpreter_->get( bufptr, 0 );
    }

    int res;
    strm >> res;
    return res;
}



int dgbSurfaceReader::readInt32(od_istream& strm) const
{
    if ( int32interpreter_ )
    {
	const int sz = int32interpreter_->nrBytes();
	mAllocLargeVarLenArr( char, buf, sz );
	char* bufptr = buf.ptr();
	strm.getBin( bufptr, sz );
	return int32interpreter_->get( bufptr, 0 );
    }

    int res;
    strm >> res;
    return res;
}


od_int64 dgbSurfaceReader::readInt64(od_istream& strm) const
{
    if ( int64interpreter_ )
    {
	const int sz = int64interpreter_->nrBytes();
	mAllocLargeVarLenArr( char, buf, sz );
	char* bufptr = buf.ptr();
	strm.getBin( bufptr, sz );
	return int64interpreter_->get( bufptr, 0 );
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




double dgbSurfaceReader::readDouble(od_istream& strm) const
{
    if ( floatinterpreter_ )
    {
	const int sz = floatinterpreter_->nrBytes();
	mAllocLargeVarLenArr( char, buf, sz );
	char* bufptr = buf.ptr();
	strm.getBin( bufptr, sz );
	return floatinterpreter_->get( bufptr, 0 );
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
    return RowCol(bid.inl(), bid.crl() );
}


bool dgbSurfaceReader::parseVersion1( const IOPar& par )
{
    return par.get( sKeyTransformX(), conv11, conv12, conv13 ) &&
	   par.get( sKeyTransformY(), conv21, conv22, conv23 );
}



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

} // namespace


class HorFileReader : public ParallelTask
{ mODTextTranslationClass(HorFileReader);
public:

enum Mode { V3Bin, V3Asc, OldBin, OldAsc };

HorFileReader( dgbSurfaceReader& rdr, Mode mode, bool is2d )
    : ParallelTask("Horizon Reader")
    , rdr_(rdr)
    , mode_(mode)
    , is2d_(is2d)
{
    if ( rdr_.surface_ )
    {
	msg_ = tr("Reading surface '%1'").arg( rdr_.surface_->name() );
	rdr_.surface_->enableGeometryChecks( false );
    }
    else
	msg_ = tr("Reading horizon");

    if ( rdr_.par_ )
	rdr_.par_->getYN( dgbSurfaceReader::sKeyDepthOnly(), rdr_.readonlyz_ );

    rdr_.prepareSurface();
    prepok_ = slurp() && indexRows();
    if ( prepok_ && !nrows_ )
    {
	releaseCalcData();
	if ( !loadAux() || !finishSurface() )
	    prepok_ = false;
    }
}

~HorFileReader()
{
    releaseCalcData();
    delete &rdr_;
}

uiString uiMessage() const override
{ return msg_; }

uiString uiNrDoneText() const override
{ return tr("Gridlines read"); }

od_int64 nrIterations() const override
{ return prepok_ ? nrows_ : 0; }

bool executeParallel( bool parallel ) override
{ return prepok_ && ParallelTask::executeParallel(parallel); }

int minThreadSize() const override
{ return 4; }

private:

struct RowInfo
{
    int		row		= 0;
    int		nrcols		= 0;
    int		nsamples	= 0;
    int		firstcol	= 0;
    int		colstep		= 1;
    int		colstoskip	= 0;
    od_int64	payloadoff	= 0;
    double	zstart		= 0;
    double	zstep		= 0;
    bool	skip		= false;
    bool	haszsd		= false;

    bool operator==( const RowInfo& oth ) const
    { return row==oth.row && payloadoff==oth.payloadoff; }
};

struct LineNodes
{
    TypeSet<int>	cols;
    TypeSet<od_int64>	subids;
    TypeSet<float>	z;
    TypeSet<double>	x;
    TypeSet<double>	y;
    bool		hasxy		= false;
};

bool slurp()
{
    const od_int64 filesz = File::getFileSize( rdr_.filename_.buf() );
    if ( filesz < 0 )
    {
	msg_ = tr("Cannot open horizon file");
	return false;
    }

    mTryAlloc( text_, char[filesz+1] );
    if ( !text_ )
    {
	msg_ = tr("Not enough memory to read the horizon file");
	return false;
    }

    filesz_ = filesz;
    if ( filesz == 0 )
    {
	text_[0] = '\0';
	return true;
    }

    od_istream strm( rdr_.filename_.buf() );
    if ( !strm.isOK() || !strm.getBin(text_,filesz) )
    {
	delete [] text_;
	text_ = nullptr;
	msg_ = tr("Cannot open horizon file");
	return false;
    }

    text_[filesz] = '\0';
    return true;
}

bool indexRows()
{
    CharRangeStream crs( text_, filesz_ );
    od_istream strm( crs );
    if ( rdr_.version_ == 3 )
    {
	if ( rdr_.sectionoffsets_.isEmpty() )
	{
	    msg_ = dgbSurfaceReader::sMsgReadError();
	    return false;
	}

	strm.setReadPosition( rdr_.sectionoffsets_[0] );
    }
    else
	strm.setReadPosition( rdr_.bodystart_ );

    if ( !strm.isOK() )
    {
	msg_ = dgbSurfaceReader::sMsgReadError();
	return false;
    }

    nrows_ = rdr_.readInt32( strm );
    if ( !strm.isOK() )
    {
	msg_ = dgbSurfaceReader::sMsgReadError();
	return false;
    }

    if ( !nrows_ )
	return true;

    firstrow_ = rdr_.readInt32( strm );
    if ( !strm.isOK() )
    {
	msg_ = dgbSurfaceReader::sMsgReadError();
	return false;
    }

    TypeSet<od_int64> rowoffsets;
    if ( rdr_.version_ == 3 )
    {
	rowoffsets.setSize( nrows_, 0 );
	for ( int idx=0; idx<nrows_; idx++ )
	{
	    rowoffsets[idx] = rdr_.readInt64( strm );
	    if ( !strm.isOK() || !rowoffsets[idx] )
	    {
		msg_ = dgbSurfaceReader::sMsgReadError();
		return false;
	    }
	}
    }

    rows_.setSize( nrows_ );
    for ( int idx=0; idx<nrows_; idx++ )
    {
	if ( rdr_.version_ == 3 )
	    strm.setReadPosition( rowoffsets[idx] );

	if ( !readRowHeader(strm,idx) )
	    return false;
    }

    return strm.isOK();
}

bool readRowHeader( od_istream& strm, int idx )
{
    RowInfo& info = rows_[idx];
    info.row = firstrow_ + idx * rdr_.rowrange_.step_;
    info.colstep = rdr_.colrange_.step_;
    if ( rdr_.par_ )
	rdr_.par_->get( dgbSurfaceReader::sColStepKey(info.row).buf(),
			 info.colstep );

    info.nsamples = rdr_.readInt32( strm );
    info.nrcols = info.nsamples;
    info.firstcol = info.nsamples ? rdr_.readInt32( strm ) : 0;
    if ( !strm.isOK() )
    {
	msg_ = dgbSurfaceReader::sMsgReadError();
	return false;
    }

    int colstoskip = 0;
    if ( rdr_.readlinenames_ && rdr_.linestrcrgs_ &&
	 !rdr_.linenames_.isEmpty() &&
	 rdr_.linenames_.validIdx(idx) )
    {
	const int trcrgidx = rdr_.readlinenames_->indexOf(
				rdr_.linenames_.get(idx).buf() );
	int callastcols = (info.firstcol - 1) + info.nsamples;
	StepInterval<int> trcrg = rdr_.linestrcrgs_->validIdx(trcrgidx)
		? (*rdr_.linestrcrgs_)[trcrgidx]
		: StepInterval<int>(0,0,1);
	if ( trcrg.width() > 1 )
	{
	    if ( info.firstcol < trcrg.start_ )
		colstoskip = trcrg.start_ - info.firstcol;

	    if ( trcrg.stop_ < callastcols )
		callastcols = trcrg.stop_;
	}

	info.nrcols = callastcols - info.firstcol - colstoskip + 1;
    }

    info.colstoskip = colstoskip;
    if ( mode_==V3Bin || mode_==V3Asc )
    {
	if ( rdr_.readonlyz_ && info.nsamples )
	{
	    info.zstart = rdr_.readDouble( strm );
	    info.zstep = rdr_.readDouble( strm );
	    info.haszsd = true;
	}
    }

    info.payloadoff = strm.position();
    info.skip = shouldSkip( info.row, idx );
    if ( info.skip && rdr_.rowrange_.includes(info.row,false) )
	notfull_ = 1;

    return skipPayload( strm, info, idx );
}

bool shouldSkip( int row, int idx ) const
{
    if ( rdr_.version_==1 || (rdr_.version_==2 && !rdr_.isBinary()) )
	return false;

    if ( rdr_.readlinenames_ && rdr_.linenames_.validIdx(idx) )
    {
	const BufferString& nm = rdr_.linenames_.get( idx );
	return !rdr_.readlinenames_->isPresent( nm.buf() );
    }

    if ( !rdr_.readrowrange_ )
	return false;

    if ( !rdr_.readrowrange_->includes(row,false) )
	return true;

    return (row-rdr_.readrowrange_->start_) % rdr_.readrowrange_->step_;
}

int sampleBytes() const
{
    const int dblsz = rdr_.floatinterpreter_
		? rdr_.floatinterpreter_->nrBytes() : (int)sizeof(double);
    const int i16sz = rdr_.int16interpreter_
		? rdr_.int16interpreter_->nrBytes() : (int)sizeof(short);
    if ( mode_==V3Bin && rdr_.readonlyz_ )
	return i16sz;

    if ( mode_==V3Bin )
	return 3 * dblsz;

    if ( mode_==OldBin )
	return (rdr_.readonlyz_ ? 1 : 3) * dblsz;

    return 0;
}

int nodesToConsume( const RowInfo& info ) const
{
    if ( info.skip )
	return info.nsamples;

    if ( mode_==V3Bin || mode_==V3Asc )
	return info.nrcols + info.colstoskip;

    return info.nrcols;
}

bool readFillType( int idx, int colindex, int nconsume ) const
{
    return rdr_.version_==1 && idx!=nrows_-1 && colindex!=nconsume-1;
}

bool skipPayload( od_istream& strm, const RowInfo& info, int idx )
{
    const int nconsume = nodesToConsume( info );
    if ( nconsume <= 0 )
	return true;

    if ( mode_==V3Bin || mode_==OldBin )
    {
	od_int64 nbytes = (od_int64)nconsume * sampleBytes();
	if ( rdr_.version_ == 1 )
	{
	    const int i32sz = rdr_.int32interpreter_
			? rdr_.int32interpreter_->nrBytes() : (int)sizeof(int);
	    for ( int col=0; col<nconsume; col++ )
	    {
		if ( readFillType(idx,col,nconsume) )
		    nbytes += i32sz;
	    }
	}

	if ( nbytes > 0 )
	    strm.ignore( nbytes );

	return strm.isOK();
    }

    for ( int col=0; col<nconsume; col++ )
    {
	if ( rdr_.readonlyz_ && (mode_==V3Asc) )
	    rdr_.readInt16( strm );
	else
	{
	    if ( !rdr_.readonlyz_ )
	    {
		rdr_.readDouble( strm );
		rdr_.readDouble( strm );
	    }

	    rdr_.readDouble( strm );
	}

	if ( readFillType(idx,col,nconsume) )
	    rdr_.readInt32( strm );
    }

    return strm.isOK();
}

bool allocDest()
{
    deleteAndNullPtr( arr_ );
    deepErase( lines_ );
    anyread_ = 0;
    notfull_ = 0;

    if ( !is2d_ )
    {
	mDynamicCastGet(Geometry::BinIDSurface*,bidsurf,
		rdr_.surface_ ? rdr_.surface_->geometry().geometryElement()
			       : nullptr)
	if ( !bidsurf && !rdr_.cube_ )
	    return true;

	StepInterval<int> inlrg = rdr_.readrowrange_ ? *rdr_.readrowrange_
						      : rdr_.rowrange_;
	StepInterval<int> crlrg = rdr_.readcolrange_ ? *rdr_.readcolrange_
						      : rdr_.colrange_;
	inlrg.sort();
	crlrg.sort();
	nrcrl_ = crlrg.nrSteps() + 1;
	PtrMan<Array2D<float> > arr =
		new Array2DImpl<float>(inlrg.nrSteps()+1, nrcrl_);
	if ( !arr || !arr->isOK() )
	{
	    msg_ = tr("Not enough memory to read the horizon file");
	    return false;
	}

	arr->setAll( mUdf(float) );
	arr_ = arr.release();
	inlrg_ = inlrg;
	crlrg_ = crlrg;
	if ( rdr_.version_ == 1 )
	{
	    for ( int idx=0; idx<nrows_; idx++ )
		lines_ += new LineNodes;
	}

	return true;
    }

    for ( int idx=0; idx<nrows_; idx++ )
	lines_ += new LineNodes;

    return true;
}


bool doPrepare( int ) override
{
    if ( rdr_.surface_ )
	rdr_.surface_->enableGeometryChecks( false );

    rdr_.prepareSurface();
    if ( !text_ && !slurp() )
	return false;

    return allocDest();
}


bool doWork( od_int64 start, od_int64 stop, int ) override
{
    CharRangeStream crs( text_, filesz_ );
    od_istream strm( crs );
    for ( od_int64 idx=start; idx<=stop; idx++ )
    {
	if ( !shouldContinue() )
	    return false;

	if ( !decodeRow((int)idx,strm) )
	    return false;
    }

    return true;
}


bool doFinish( bool success ) override
{
    bool res = success;
    if ( res && (!commit() || !loadAux() || !finishSurface()) )
	res = false;

    if ( !res && msg_.isEmpty() )
	msg_ = dgbSurfaceReader::sMsgReadError();

    releaseCalcData();
    return res;
}

void releaseCalcData()
{
    deleteAndNullArrPtr( text_ );
    deleteAndNullPtr( arr_ );
    deepErase( lines_ );
}


bool decodeRow( int idx, od_istream& strm )
{
    const RowInfo& info = rows_[idx];
    if ( info.skip || info.nsamples<=0 )
    {
	addToNrDone( 1 );
	return true;
    }

    if ( mode_==V3Bin || mode_==OldBin )
	return decodeBinary( idx, info );

    strm.setReadPosition( info.payloadoff );
    if ( !strm.isOK() )
	return false;

    const int nread = nodesToConsume( info );
    for ( int colindex=0; colindex<nread; colindex++ )
    {
	Coord3 pos;
	if ( mode_==V3Asc && rdr_.readonlyz_ )
	{
	    const int zidx = rdr_.readInt16( strm );
	    if ( colindex < info.colstoskip-1 )
		continue;
	    pos.z_ = zidx==65535 ? mUdf(float)
		    : SamplingData<double>(info.zstart,info.zstep)
				.atIndex(zidx);
	}
	else
	{
	    if ( !rdr_.readonlyz_ )
	    {
		pos.x_ = rdr_.readDouble( strm );
		pos.y_ = rdr_.readDouble( strm );
	    }

	    pos.z_ = rdr_.readDouble( strm );
	    if ( readFillType(idx,colindex,nread) )
		rdr_.readInt32( strm );

	    if ( colindex < info.colstoskip-1 )
		continue;
	}

	if ( !strm.isOK() )
	    return false;
	if ( !keepNode(info,colindex,pos) )
	    return false;
    }

    addToNrDone( 1 );
    return true;
}

bool decodeBinary( int idx, const RowInfo& info )
{
    const char* p = text_ + info.payloadoff;
    const char* end = text_ + filesz_;
    const int nread = nodesToConsume( info );
    const int dblsz = rdr_.floatinterpreter_
			? rdr_.floatinterpreter_->nrBytes() : 0;
    const int i16sz = rdr_.int16interpreter_
			? rdr_.int16interpreter_->nrBytes() : 0;
    const int i32sz = rdr_.int32interpreter_
			? rdr_.int32interpreter_->nrBytes() : 0;
    for ( int colindex=0; colindex<nread; colindex++ )
    {
	Coord3 pos;
	if ( (mode_==V3Bin && rdr_.readonlyz_) )
	{
	    if ( !rdr_.int16interpreter_ || p+i16sz > end )
		return false;

	    const int zidx = rdr_.int16interpreter_->get( p, 0 );
	    p += i16sz;
	    if ( colindex < info.colstoskip-1 )
		continue;

	    pos.z_ = zidx==65535 ? mUdf(float)
		    : SamplingData<double>(info.zstart,info.zstep)
				.atIndex(zidx);
	}
	else
	{
	    const int ncoord = rdr_.readonlyz_ ? 1 : 3;
	    if ( !rdr_.floatinterpreter_ || p+ncoord*dblsz > end )
		return false;

	    if ( !rdr_.readonlyz_ )
	    {
		pos.x_ = rdr_.floatinterpreter_->get( p, 0 );
		p += dblsz;
		pos.y_ = rdr_.floatinterpreter_->get( p, 0 );
		p += dblsz;
	    }

	    pos.z_ = rdr_.floatinterpreter_->get( p, 0 );
	    p += dblsz;
	    if ( readFillType(idx,colindex,nread) )
	    {
		if ( p+i32sz > end )
		    return false;

		p += i32sz;
	    }

	    if ( colindex < info.colstoskip-1 )
		continue;
	}

	if ( !keepNode(info,colindex,pos) )
	    return false;
    }

    addToNrDone( 1 );
    return true;
}

bool keepNode( const RowInfo& info, int colindex, const Coord3& pos )
{
    const int filecol = info.firstcol + colindex * info.colstep;
    RowCol rc( info.row, filecol );
    if ( rdr_.version_ == 1 )
	rc = rdr_.convertRowCol( info.row, filecol );

    if ( rdr_.readcolrange_ &&
	 (!rdr_.readcolrange_->includes(rc.col(),false) ||
	  (rc.col()-rdr_.readcolrange_->start_)%rdr_.readcolrange_->step_) )
    {
	notfull_ = 1;
	return true;
    }

    if ( rdr_.readrowrange_ &&
	 (!rdr_.readrowrange_->includes(rc.row(),false) ||
	  (rc.row()-rdr_.readrowrange_->start_)%rdr_.readrowrange_->step_) )
    {
	notfull_ = 1;
	return true;
    }

    if ( !Math::IsNormalNumber(pos.z_) || !Math::IsNormalNumber(pos.x_) ||
	 !Math::IsNormalNumber(pos.y_) )
	return true;

    if ( !is2d_ && rdr_.version_==1 )
    {
	const int rowidx = &info - rows_.arr();
	if ( !lines_.validIdx(rowidx) )
	    return true;

	LineNodes& nodes = *lines_[rowidx];
	nodes.cols += rc.col();
	nodes.subids += rc.row();
	nodes.z += (float)pos.z_;
    }
    else if ( !is2d_ && arr_ )
    {
	int i, j;
	if ( !indices(rc,i,j) )
	    return true;

	float* data = arr_->getData();
	if ( data )
	    data[(od_int64)i*nrcrl_+j] = (float)pos.z_;
	else
	    arr_->set( i, j, (float)pos.z_ );

	if ( rdr_.cube_ )
	    rdr_.cube_->set( i, j, 0, (float)pos.z_ );
    }
    else if ( is2d_ )
    {
	const int rowidx = &info - rows_.arr();
	if ( !lines_.validIdx(rowidx) )
	    return true;

	LineNodes& nodes = *lines_[rowidx];
	nodes.cols += rc.col();
	nodes.subids += rc.toInt64();
	nodes.z += (float)pos.z_;
	if ( !rdr_.readonlyz_ )
	{
	    nodes.x += pos.x_;
	    nodes.y += pos.y_;
	    nodes.hasxy = true;
	}
    }
    else if ( rdr_.cube_ )
    {
	int i, j;
	if ( indices(rc,i,j) )
	    rdr_.cube_->set( i, j, 0, (float)pos.z_ );
    }

    anyread_ = 1;
    return true;
}

bool indices( const RowCol& rc, int& i, int& j ) const
{
    if ( !inlrg_.includes(rc.row(),false) ||
	 !crlrg_.includes(rc.col(),false) )
	return false;

    if ( inlrg_.step_ && (rc.row()-inlrg_.start_)%inlrg_.step_ )
	return false;

    if ( crlrg_.step_ && (rc.col()-crlrg_.start_)%crlrg_.step_ )
	return false;

    i = inlrg_.getIndex( rc.row() );
    j = crlrg_.getIndex( rc.col() );
    return i>=0 && j>=0;
}

bool commit()
{
    if ( !anyread_.load() )
	return true;

    return is2d_ ? commit2D() : commit3D();
}

bool commit3D()
{
    mDynamicCastGet(Geometry::BinIDSurface*,bidsurf,
	    rdr_.surface_ ? rdr_.surface_->geometry().geometryElement()
			   : nullptr)
    if ( rdr_.version_==1 )
    {
	for ( int idx=0; idx<lines_.size(); idx++ )
	{
	    const LineNodes& nodes = *lines_[idx];
	    for ( int inode=0; inode<nodes.z.size(); inode++ )
	    {
		const RowCol rc( (int)nodes.subids[inode],
				 nodes.cols[inode] );
		int i, j;
		if ( !indices(rc,i,j) )
		    continue;

		if ( arr_ )
		    arr_->set( i, j, nodes.z[inode] );

		if ( rdr_.cube_ )
		    rdr_.cube_->set( i, j, 0, nodes.z[inode] );
	    }
	}
    }

    if ( !bidsurf || !arr_ )
	return true;

    const int nrrows = arr_->info().getSize( 0 );
    const int nrcols = arr_->info().getSize( 1 );
    const BinID start( inlrg_.start_, crlrg_.start_ );
    const BinID step( inlrg_.step_, crlrg_.step_ );
    Array2D<float>* depths = bidsurf->getArray();
    const StepInterval<int> rowrg = bidsurf->rowRange();
    const StepInterval<int> colrg = bidsurf->colRange();
    const bool issamelayout = depths
	&& depths->info().getSize(0)==nrrows
	&& depths->info().getSize(1)==nrcols
	&& rowrg.start_==start.inl() && rowrg.step_==step.inl()
	&& colrg.start_==start.crl() && colrg.step_==step.crl();
    if ( !issamelayout )
    {
	PtrMan<Array2D<float> > blank = new Array2DImpl<float>(nrrows,nrcols);
	if ( !blank || !blank->isOK() )
	{
	    msg_ = tr("Not enough memory to read the horizon file");
	    return false;
	}

	blank->setAll( mUdf(float) );
	bidsurf->setArray( start, step, blank.release(), true );
	depths = bidsurf->getArray();
    }

    if ( !depths )
	return false;

    const float* src = arr_->getData();
    if ( src )
	depths->setData( src );
    else
    {
	for ( int irow=0; irow<nrrows; irow++ )
	    for ( int icol=0; icol<nrcols; icol++ )
		depths->set( irow, icol, arr_->get(irow,icol) );
    }

    bidsurf->trimUndefParts();
    return true;
}

bool commit2D()
{
    mDynamicCastGet(Horizon2D*,hor2d,rdr_.surface_.ptr())
    if ( !hor2d )
	return true;

    Geometry::Horizon2DLine* geom = hor2d->geometry().geometryElement();
    if ( !geom )
	return true;

    for ( int idx=0; idx<rows_.size(); idx++ )
    {
	const RowInfo& info = rows_[idx];
	if ( info.skip || !lines_.validIdx(idx) || lines_[idx]->z.isEmpty() )
	    continue;
	if ( !rdr_.geomids_.validIdx(idx) || !rdr_.geomids_[idx].isValid() )
	    continue;

	const int startcol = info.firstcol + info.colstoskip;
	const int nr = info.nrcols>0 ? info.nrcols : 1;
	const int stopcol = info.firstcol + info.colstoskip +
			    info.colstep * (nr-1);
	geom->addUdfRow( rdr_.geomids_[idx], startcol, stopcol, info.colstep );
	const int rowidx = geom->getRowIndex( rdr_.geomids_[idx] );
	if ( rowidx < 0 )
	    continue;

	const LineNodes& nodes = *lines_[idx];
	for ( int inode=0; inode<nodes.z.size(); inode++ )
	{
	    Coord3 pos;
	    if ( nodes.hasxy )
	    {
		pos.x_ = nodes.x[inode];
		pos.y_ = nodes.y[inode];
	    }

	    pos.z_ = nodes.z[inode];
	    const RowCol rc = rdr_.version_==1
		    ? RowCol::fromInt64( nodes.subids[inode] )
		    : RowCol( rowidx, nodes.cols[inode] );
	    geom->setKnot( rc, pos );
	}
    }

    geom->trimUndefParts();
    return true;
}

bool loadAux()
{
    mDynamicCastGet(Horizon3D*,hor,rdr_.surface_.ptr())
    if ( !hor || rdr_.auxdatasel_.isEmpty() )
	return true;

    BufferStringSet fnms;
    for ( int idx=0; idx<rdr_.auxdatasel_.size(); idx++ )
    {
	const int iaux = rdr_.auxdatasel_[idx];
	if ( !rdr_.auxdataexecs_.validIdx(iaux) || !rdr_.auxdataexecs_[iaux] )
	    continue;

	fnms.add( rdr_.auxdataexecs_[iaux]->fileName() );
    }

    if ( fnms.isEmpty() )
	return true;

    PtrMan<Task> task = createAuxDataTask( *hor, fnms );
    if ( !task )
    {
	msg_ = task ? task->uiMessage()
		    : uiStrings::phrCannotRead( tr("horizon data") );
	return false;
    }

    task->setProgressMeter( progressMeter() );
    if ( !task->execute() )
    {
	msg_ = task ? task->uiMessage()
		    : uiStrings::phrCannotRead( tr("horizon data") );
	return false;
    }

    return true;
}

bool finishSurface()
{
    if ( !rdr_.surface_ )
	return true;

    if ( notfull_.load() )
	rdr_.fullyread_ = false;

    if ( rdr_.par_ && !rdr_.surface_->usePar(*rdr_.par_) )
    {
	msg_ = tr("Could not parse header");
	return false;
    }

    rdr_.surface_->setFullyLoaded( rdr_.fullyread_ );
    rdr_.surface_->enableGeometryChecks( true );
    rdr_.surface_->convertZValues( rdr_.surface_->surveyStorageUnit(), true );
    if ( rdr_.surface_->zDomain().isDepth() )
    {
	const auto& zdom = ZDomain::Info::getFrom(
			rdr_.surface_->zDomain().key(),
			rdr_.surface_->surveyStorageUnit()->getLabel() );
	rdr_.surface_->setZDomain( zdom );
    }

    rdr_.surface_->resetChangedFlag();
    return true;
}

    dgbSurfaceReader&		rdr_;
    Mode			mode_;
    bool			is2d_;
    char*			text_		= nullptr;
    od_int64			filesz_		= 0;
    bool			prepok_		= false;
    int				nrows_		= 0;
    int				firstrow_	= 0;
    int				nrcrl_		= 0;
    Array2D<float>*		arr_		= nullptr;
    StepInterval<int>		inlrg_;
    StepInterval<int>		crlrg_;
    TypeSet<RowInfo>		rows_;
    ObjectSet<LineNodes>	lines_;
    Threads::Atomic<int>	anyread_;
    Threads::Atomic<int>	notfull_;
    uiString			msg_;
};


class Hor3DV3BinReader : public HorFileReader
{
public:
    explicit Hor3DV3BinReader( dgbSurfaceReader& rdr )
	: HorFileReader(rdr, V3Bin, false) {}
};

class Hor3DV3AscReader : public HorFileReader
{
public:
    explicit Hor3DV3AscReader( dgbSurfaceReader& rdr )
	: HorFileReader(rdr, V3Asc, false) {}
};

class Hor3DOldBinReader : public HorFileReader
{
public:
    explicit Hor3DOldBinReader( dgbSurfaceReader& rdr )
	: HorFileReader(rdr, OldBin, false) {}
};

class Hor3DOldAscReader : public HorFileReader
{
public:
    explicit Hor3DOldAscReader( dgbSurfaceReader& rdr )
	: HorFileReader(rdr, OldAsc, false) {}
};

class Hor2DV3BinReader : public HorFileReader
{
public:
    explicit Hor2DV3BinReader( dgbSurfaceReader& rdr )
	: HorFileReader(rdr, V3Bin, true) {}
};

class Hor2DV3AscReader : public HorFileReader
{
public:
    explicit Hor2DV3AscReader( dgbSurfaceReader& rdr )
	: HorFileReader(rdr, V3Asc, true) {}
};

class Hor2DOldBinReader : public HorFileReader
{
public:
    explicit Hor2DOldBinReader( dgbSurfaceReader& rdr )
	: HorFileReader(rdr, OldBin, true) {}
};

class Hor2DOldAscReader : public HorFileReader
{
public:
    explicit Hor2DOldAscReader( dgbSurfaceReader& rdr )
	: HorFileReader(rdr, OldAsc, true) {}
};


class LegacySurfaceReadTask : public ParallelTask
{
public:
    explicit LegacySurfaceReadTask( dgbSurfaceReader& rdr )
	: ParallelTask(rdr.name())
	, rdr_(rdr)
    {}

    ~LegacySurfaceReadTask()			{ delete &rdr_; }
    bool executeParallel( bool ) override	{ return rdr_.execute(); }
    bool doWork( od_int64, od_int64, int ) override	{ return true; }
    od_int64 nrIterations() const override	{ return 0; }
    od_int64 nrDone() const override		{ return rdr_.nrDone(); }
    od_int64 totalNr() const override		{ return rdr_.totalNr(); }
    uiString uiMessage() const override		{ return rdr_.uiMessage(); }
    uiString uiNrDoneText() const override
    { return rdr_.uiNrDoneText(); }

    dgbSurfaceReader&	rdr_;
};


class FailedReadTask : public ParallelTask
{ mODTextTranslationClass(FailedReadTask);
public:
    FailedReadTask( dgbSurfaceReader& rdr, const uiString& msg )
	: ParallelTask(rdr.name())
	, rdr_(rdr)
	, msg_(msg)
    {}

    ~FailedReadTask()				{ delete &rdr_; }
    bool executeParallel( bool ) override	{ return false; }
    bool doWork( od_int64, od_int64, int ) override	{ return false; }
    od_int64 nrIterations() const override	{ return 0; }
    uiString uiMessage() const override		{ return msg_; }

    dgbSurfaceReader&	rdr_;
    uiString		msg_;
};


Task* dgbSurfaceReader::createReadTask()
{
    mDynamicCastGet(const Horizon2D*,hor2d,surface_.ptr())
    mDynamicCastGet(const Horizon3D*,hor3d,surface_.ptr())
    const bool horizon = hor2d || hor3d;
    if ( !horizon && !cube_ )
	return new LegacySurfaceReadTask( *this );

    if ( sectionids_.size() != 1 )
    {
	msg_ = tr("Cannot read a horizon with more than one section");
	return new FailedReadTask( *this, msg_ );
    }

    const bool bin = isBinary();
    const bool v3 = version_ == 3;
    if ( hor2d )
    {
	if ( v3 && bin )
	    return new Hor2DV3BinReader( *this );

	if ( v3 )
	    return new Hor2DV3AscReader( *this );

	if ( bin )
	    return new Hor2DOldBinReader( *this );

	return new Hor2DOldAscReader( *this );
    }

    if ( v3 && bin )
	return new Hor3DV3BinReader( *this );

    if ( v3 )
	return new Hor3DV3AscReader( *this );

    if ( bin )
	return new Hor3DOldBinReader( *this );

    return new Hor3DOldAscReader( *this );
}


// dgbSurfaceWriter

dgbSurfaceWriter::dgbSurfaceWriter( const IOObj* ioobj,
				    const char* filetype,
				    const Surface& surface,
				    bool binary )
    : ExecutorGroup( "Surface Writer" )
    , objectmid_(ioobj ? ioobj->key() : MultiID::udf() )
    , surface_(surface)
    , binary_(binary)
    , filetype_(filetype)
{
    init( ioobj ? ioobj->fullUserExpr(false) : 0 );
}


dgbSurfaceWriter::dgbSurfaceWriter( const char* fulluserexpr,
				    const char* filetype,
				    const Surface& surface,
				    bool binary )
    : ExecutorGroup( "Surface Writer" )
    , objectmid_(MultiID::udf())
    , surface_(surface)
    , binary_(binary)
    , filetype_(filetype)
{
    init( fulluserexpr );
}


void dgbSurfaceWriter::init( const char* fulluserexpr )
{
    fulluserexpr_ = fulluserexpr;
    par_ = new IOPar("Surface parameters" );
    conn_ = 0;
    writerowrange_ = 0;
    writecolrange_ = 0;
    writtenrowrange_ = Interval<int>( INT_MAX, INT_MIN );
    writtencolrange_ = Interval<int>( INT_MAX, INT_MIN );
    zrange_ = Interval<float>(Interval<float>::udf());
    nrdone_ = 0;
    sectionindex_ = 0;
    oldsectionindex_= -1;
    writeonlyz_ = false;
    nrrows_ = 0;
    shift_ = 0;
    writingfinished_ = false;
    auxwritersprepared_ = false;
    geometry_ = reinterpret_cast<const EM::RowColSurfaceGeometry*>(
							&surface_.geometry() );

    surface_.ref();
    setNrDoneText( Task::uiStdNrDoneText() );
    par_->set( dgbSurfaceReader::sKeyDBInfo(), surface_.dbInfo() );

    for ( int idx=0; idx<nrSections(); idx++ )
	sectionsel_ += sectionID( idx );

    for ( int idx=0; idx<nrAuxVals(); idx++ )
    {
	if ( auxDataName(idx) )
	    auxdatasel_ += idx;
    }

    rowrange_ = geometry_->rowRange();
    colrange_ = geometry_->colRange();

    surface_.fillPar( *par_ );
}


dgbSurfaceWriter::~dgbSurfaceWriter()
{
    if ( !writingfinished_ )
	finishWriting();

    surface_.unRef();
    delete par_;
    delete conn_;
    delete writerowrange_;
    delete writecolrange_;
}


void dgbSurfaceWriter::finishWriting()
{
    writingfinished_ = true;
    if ( !conn_ )
	return;

    od_ostream& strm = conn_->oStream();
    const od_int64 nrsectionsoffset = strm.position();
    writeInt32( strm, sectionsel_.size(), sEOL() );

    for ( int idx=0; idx<sectionoffsets_.size(); idx++ )
	writeInt64( strm, sectionoffsets_[idx], sEOL() );

    for ( int idx=0; idx<sectionsel_.size(); idx++ )
	writeInt32( strm, sectionsel_[idx].asInt(), sEOL() );


    const od_stream::Pos secondparoffset = strm.position();
    strm.setWritePosition( nrsectionsoffsetoffset_ );
    writeInt64( strm, nrsectionsoffset, sEOL() );
    strm.setWritePosition( secondparoffset );

    par_->setYN( dgbSurfaceReader::sKeyDepthOnly(), writeonlyz_ );

    const int rowrgstep = writerowrange_ ?
			      writerowrange_->step_ : rowrange_.step_;
    par_->set( dgbSurfaceReader::sKeyRowRange(),
	       writtenrowrange_.start_, writtenrowrange_.stop_, rowrgstep );

    const int colrgstep = writecolrange_ ?
			      writecolrange_->step_ : colrange_.step_;
    par_->set( dgbSurfaceReader::sKeyColRange(),
	       writtencolrange_.start_, writtencolrange_.stop_, colrgstep );

    par_->set( dgbSurfaceReader::sKeyZRange(), zrange_ );

    for (int idx=firstrow_; idx<firstrow_+rowrgstep*nrrows_; idx+=rowrgstep)
    {
	const int idxcolstep = geometry_->colRange(idx).step_;
	if ( idxcolstep && idxcolstep!=colrange_.step_ )
	    par_->set( dgbSurfaceReader::sColStepKey(idx).buf(),idxcolstep);
    }

    PtrMan<IOObj> objioobj = IOM().get( objectmid_ );
    if ( objioobj )
    {
	const auto* objzinfo = ZDomain::Info::getFrom( objioobj->pars() );
	if ( objzinfo )
	    objzinfo->fillPar( *par_ );
	else
	    surface_.zDomain().fillPar( *par_ );
    }
    else
	surface_.zDomain().fillPar( *par_ );

    ascostream astream( strm );
    astream.newParagraph();
    par_->putTo( astream );
    surface_.saveDisplayPars();
    deleteAndNullPtr( conn_ );
}


int dgbSurfaceWriter::nrSections() const
{
    return surface_.nrSections();
}


SectionID dgbSurfaceWriter::sectionID( int idx ) const
{
    return surface_.sectionID(idx);
}


const char* dgbSurfaceWriter::sectionName( int idx ) const
{
    return surface_.sectionName(sectionID(idx)).buf();
}


void dgbSurfaceWriter::selSections(const TypeSet<SectionID>& sel, bool keep )
{
    if ( keep )
    {
	for ( int idx=0; idx<sel.size(); idx++ )
	{
	    if ( !sectionsel_.isPresent(sel[idx]) )
		sectionsel_ += sel[idx];
	}
    }
    else
    {
	sectionsel_ = sel;
    }
}


int dgbSurfaceWriter::nrAuxVals() const
{
    mDynamicCastGet(const Horizon3D*,hor,&surface_);
    return hor ? hor->auxdata.nrAuxData() : 0;
}


const char* dgbSurfaceWriter::auxDataName( int idx ) const
{
    mDynamicCastGet(const Horizon3D*,hor,&surface_);
    return hor ? hor->auxdata.auxDataName(idx) : nullptr;
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
    delete writerowrange_;
    writerowrange_ = new StepInterval<int>( rg );
}


void dgbSurfaceWriter::setColInterval( const StepInterval<int>& rg )
{
    delete writecolrange_;
    writecolrange_ = new StepInterval<int>( rg );
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
    return par_;
}


od_int64 dgbSurfaceWriter::nrDone() const
{
    return (executors_.size() ? ExecutorGroup::nrDone() : 0) + nrdone_;
}


uiString dgbSurfaceWriter::uiNrDoneText() const
{
    return tr("Gridlines written");
}


od_int64 dgbSurfaceWriter::totalNr() const
{
    return (executors_.size() ? ExecutorGroup::totalNr() : 1) +
	   (writerowrange_?writerowrange_->nrSteps():rowrange_.nrSteps()) *
	   sectionsel_.size();
}


#define mSetDc( type, string ) \
{ \
    type dummy; \
    DataCharacteristics(dummy).toString( dc ); \
}\
    versionpar.set( string, dc )

int dgbSurfaceWriter::nextStep()
{
    if ( !nrdone_ )
    {
	conn_ = !fulluserexpr_.isEmpty() ?
		    new StreamConn(fulluserexpr_,Conn::Write) : 0;
	if ( !conn_ )
	{
	    msg_ = tr("Cannot open output surface file");
	    return ErrorOccurred();
	}

	od_ostream& strm = conn_->oStream();
	if ( !strm.isOK() )
	{
	    msg_ = tr("Cannot open output surface file");
	    strm.addErrMsgTo( msg_ );
	    delete conn_; conn_ = 0;
	    return ErrorOccurred();
	}

	IOPar versionpar("Header 1");
	versionpar.set( dgbSurfaceReader::sKeyVersion(), 3 );
	if ( binary_ )
	{
	    BufferString dc;
	    mSetDc( od_int32, dgbSurfaceReader::sKeyInt32DataChar() );
	    mSetDc( unsigned short, dgbSurfaceReader::sKeyInt16DataChar() );
	    mSetDc( od_int64, dgbSurfaceReader::sKeyInt64DataChar() );
	    mSetDc( double, dgbSurfaceReader::sKeyFloatDataChar() );
	}

	ascostream astream( strm );
	astream.putHeader( filetype_.buf() );
	versionpar.putTo( astream );
	nrsectionsoffsetoffset_ = strm.position();
	writeInt64( strm, 0, sEOL() );

	mDynamicCastGet(const Horizon3D*,hor,&surface_);
	for ( int idx=0; idx<auxdatasel_.size(); idx++ )
	{
	    const int dataidx = auxdatasel_[idx];
	    if ( dataidx<0 || dataidx>=hor->auxdata.nrAuxData() )
		continue;

	    BufferString fnm = hor->auxdata.getFileName( fulluserexpr_,
							auxDataName(dataidx) );
	    if ( fnm.isEmpty() )
	    {
		PtrMan<IOObj> ioobj = IOM().get( objectmid_ );
		if ( !ioobj )
		    ioobj = IOM().get( surface_.multiID() );

		if ( !ioobj )
		{
		    msg_ = tr("Cannot write surface attribute '%1': "
			      "output database entry not found.")
			    .arg( auxDataName(dataidx) );
		    return ErrorOccurred();
		}
	    }

	    add( new dgbSurfDataWriter(*hor,dataidx,0,binary_,fnm.buf()) );
	    // TODO:: Change binid sampler so not all values are written when
	    // there is a subselection
	}
    }

    if ( sectionindex_>=sectionsel_.size() )
    {
	if ( !auxwritersprepared_ && !executors_.isEmpty() )
	{
	    auxwritersprepared_ = true;
	    if ( !ExecutorGroup::doPrepare(nullptr) )
	    {
		msg_ = ExecutorGroup::uiMessage();
		if ( msg_.isEmpty() )
		    msg_ = tr("Cannot prepare surface attribute writer");

		return ErrorOccurred();
	    }
	}

	const int res = ExecutorGroup::nextStep();
	if ( !res && objectmid_==surface_.multiID() )
	    const_cast<Surface*>(&surface_)->resetChangedFlag();

	if ( res == Finished() )
	    finishWriting();

	return res;
    }

    od_ostream& strm = conn_->oStream();

    if ( sectionindex_!=oldsectionindex_ && !writeNewSection( strm ) )
	return ErrorOccurred();

    if ( nrrows_ && !writeRow( strm ) )
	return ErrorOccurred();

    rowindex_++;
    if ( rowindex_>=nrrows_ )
    {
	strm.setWritePosition( rowoffsettableoffset_ );
	if ( !strm.isOK() )
	{
	    msg_ = sMsgWriteError();
	    return ErrorOccurred();
	}

	for ( int idx=0; idx<nrrows_; idx++ )
	{
	    if ( !writeInt64(strm,rowoffsettable_[idx],sEOL() ) )
	    {
		msg_ = sMsgWriteError();
		return ErrorOccurred();
	    }
	}

	rowoffsettable_.erase();

	strm.setWritePosition( 0, od_stream::End );
	if ( !strm.isOK() )
	{
	    msg_ = sMsgWriteError();
	    return ErrorOccurred();
	}

	sectionindex_++;
	strm.flush();
	if ( !strm.isOK() )
	{
	    msg_ = sMsgWriteError();
	    return ErrorOccurred();
	}
    }

    nrdone_++;
    return MoreToDo();
}


uiString dgbSurfaceWriter::uiMessage() const
{
    if ( executors_.size() > 0 )
    {
	const uiString grpmsg = ExecutorGroup::uiMessage();
	if ( !grpmsg.isEmpty() )
	    return grpmsg;
    }

    return msg_;
}


bool dgbSurfaceWriter::writeInt16( od_ostream& strm, unsigned short val,
				   const char* post) const
{
    if ( binary_ )
	strm.addBin( val );
    else
	strm << val << post;

    return strm.isOK();
}


bool dgbSurfaceWriter::writeInt32( od_ostream& strm, od_int32 val,
				   const char* post) const
{
    if ( binary_ )
	strm.addBin( val );
    else
	strm << val << post;

    return strm.isOK();
}


bool dgbSurfaceWriter::writeInt64( od_ostream& strm, od_int64 val,
				   const char* post) const
{
    if ( binary_ )
	strm.addBin( val );
    else
    {
	BufferString valstr; valstr = val;
	int len = valstr.size();
	for ( int idx=20; idx>len; idx-- )
	    strm << '0';
	strm << valstr << post;
    }

    return strm.isOK();
}


bool dgbSurfaceWriter::writeNewSection( od_ostream& strm )
{
    rowindex_ = 0;
    rowoffsettableoffset_ = 0;

    mDynamicCastGet(const Geometry::RowColSurface*,gsurf,
		    surface_.geometryElement())

    if ( !gsurf || gsurf->isEmpty() )
    {
	nrrows_ = 0;
    }
    else
    {
	StepInterval<int> sectionrange = gsurf->rowRange();
	sectionrange.sort();
        firstrow_ = sectionrange.start_;
        int lastrow = sectionrange.stop_;

	if ( writerowrange_ )
	{
	    if ( firstrow_>writerowrange_->stop_
		|| lastrow<writerowrange_->start_)
		nrrows_ = 0;
	    else
	    {
                if ( firstrow_<writerowrange_->start_ )
                    firstrow_ = writerowrange_->start_;

                if ( lastrow>writerowrange_->stop_ )
                    lastrow = writerowrange_->stop_;

		firstrow_ = writerowrange_->snap( firstrow_ );
		lastrow = writerowrange_->snap( lastrow );

                nrrows_ = (lastrow-firstrow_)/writerowrange_->step_+1;
	    }
	}
	else
	{
	    nrrows_ = sectionrange.nrSteps()+1;
	}
    }

    sectionoffsets_ += strm.position();

    if ( !writeInt32(strm,nrrows_, nrrows_ ? sTab() : sEOL() ) )
    {
	msg_ = sMsgWriteError();
	return false;
    }

    if ( !nrrows_ )
    {
	sectionindex_++;
	nrdone_++;
	return MoreToDo();
    }

    if ( !writeInt32(strm,firstrow_,sEOL()) )
    {
	msg_ = sMsgWriteError();
	return ErrorOccurred();
    }

    rowoffsettableoffset_ = strm.position();
    for ( int idx=0; idx<nrrows_; idx++ )
    {
	if ( !writeInt64(strm,0,sEOL() ) )
	{
	    msg_ = sMsgWriteError();
	    return ErrorOccurred();
	}
    }

    oldsectionindex_ = sectionindex_;
    const BufferString sectionname = "[0]";
    par_->set( dgbSurfaceReader::sSectionNameKey(0).buf(), sectionname );
    return true;
}


void dgbSurfaceWriter::setShift( float s )
{
    shift_ = s;
}


bool dgbSurfaceWriter::writeRow( od_ostream& strm )
{
    if ( !colrange_.step_ || !rowrange_.step_ )
    {
	msg_ = tr("Cannot write surface: inline/crossline step is not set.");
	pErrMsg("Steps not set");
	return false;
    }

    rowoffsettable_ += strm.position();
    const int row = firstrow_ + rowindex_ *
                    (writerowrange_ ? writerowrange_->step_ : rowrange_.step_);

    const StepInterval<int> colrange = geometry_->colRange( row );

    TypeSet<Coord3> colcoords;

    int firstcol = -1;
    const int nrcols =
	(writecolrange_ ? writecolrange_->nrSteps() : colrange.nrSteps()) + 1;

    const auto* objzunit = surface_.zUnit();
    const auto* datasetzunit = objzunit;
    PtrMan<IOObj> objioobj = IOM().get( objectmid_ );
    if ( objioobj )
    {
	const auto* objzinfo = ZDomain::Info::getFrom( objioobj->pars() );
	if ( objzinfo )
	    datasetzunit = UnitOfMeasure::zUnit( *objzinfo );
    }

    mDynamicCastGet(const Horizon*,hor,&surface_)
    for ( int colindex=0; colindex<nrcols; colindex++ )
    {
	const int col = writecolrange_ ? writecolrange_->atIndex(colindex) :
					colrange.atIndex(colindex);

	const PosID posid(  surface_.id(), RowCol(row,col) );
	Coord3 pos = surface_.getPos( posid );
	if ( hor && pos.isDefined() )
            pos.z_ += shift_;

	if ( colcoords.isEmpty() && !pos.isDefined() )
	    continue;

        if ( !mIsUdf(pos.z_) )
            zrange_.include( (float) pos.z_, false );

	if ( colcoords.isEmpty() )
	    firstcol = col;

	convValue( pos.z_, objzunit, datasetzunit );
	colcoords += pos;
    }

    for ( int idx=colcoords.size()-1; idx>=0; idx-- )
    {
	if ( colcoords[idx].isDefined() )
	    break;

	colcoords.removeSingle(idx);
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

	SamplingData<double> sd;
	if ( writeonlyz_ )
	{
	    Interval<double> rg;
	    bool isset = false;
	    for ( int idx=0; idx<colcoords.size(); idx++ )
	    {
		const Coord3 pos = colcoords[idx];
                if ( Values::isUdf(pos.z_) )
		    continue;

		if ( isset )
                    rg.include( pos.z_ );
		else
		{
                    rg.start_ = rg.stop_ = pos.z_;
		    isset = true;
		}
	    }

            sd.start_ = rg.start_;
            sd.step_ = rg.width()/65534;

            if ( !writeDouble( strm, sd.start_, sTab()) ||
                 !writeDouble( strm, sd.step_, sEOLTab()) )
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
                const int index = mIsUdf(pos.z_)
                                  ? 0xFFFF : sd.nearestIndex(pos.z_);

		if ( !writeInt16( strm, mCast(unsigned short,index),
			          idx!=colcoords.size()-1 ? sEOLTab() : sEOL()))
		{
		    msg_ = sMsgWriteError();
		    return false;
		}
	    }
	    else
	    {
                if ( !writeDouble( strm, pos.x_, sTab()) ||
                     !writeDouble( strm, pos.y_, sTab()) ||
                     !writeDouble( strm, pos.z_,
			          idx!=colcoords.size()-1 ? sEOLTab() : sEOL()))
		{
		    msg_ = sMsgWriteError();
		    return false;
		}
	    }
	}

	writtenrowrange_.include( row, false );
	writtencolrange_.include( firstcol, false );

	const int lastcol = firstcol + (colcoords.size()-1) *
			    (writecolrange_ ? writecolrange_->step_
					    : colrange.step_);
	writtencolrange_.include( lastcol, false );
    }

    return true;
}


bool dgbSurfaceWriter::writeDouble( od_ostream& strm, double val,
				       const char* post) const
{
    if ( binary_ )
	strm.addBin( val );
    else
	strm << ::toString(val) << post;

    return strm.isOK();
}

} // namespace EM

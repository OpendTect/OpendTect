/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          June 2003
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "emsurfaceio.h"

#include "arrayndimpl.h"
#include "binidsurface.h"
#include "color.h"
#include "ascstream.h"
#include "datachar.h"
#include "datainterp.h"
#include "emfaultstickset.h"
#include "emfault3d.h"
#include "emhorizon2d.h"
#include "emhorizon3d.h"
#include "empolygonbody.h"
#include "emsurfacegeometry.h"
#include "emsurfaceauxdata.h"
#include "emsurfaceedgeline.h"
#include "emsurfauxdataio.h"
#include "file.h"
#include "parametricsurface.h"
#include "ioobj.h"
#include "ioman.h"
#include "iopar.h"
#include "ptrman.h"
#include "separstr.h"
#include "streamconn.h"
#include "survinfo.h"
#include "surv2dgeom.h"
#include "strmoper.h"
#include "strmprov.h"
#include "filepath.h"

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
const char* dgbSurfaceReader::sKeyZRange()	  { return "Z range"; }
const char* dgbSurfaceReader::sKeyDepthOnly()	  { return "Depth only"; }
const char* dgbSurfaceReader::sKeyDBInfo()	  { return "DB info"; }
const char* dgbSurfaceReader::sKeyVersion()	  { return "Format version"; }

const char* dgbSurfaceReader::sKeyTransformX()	{ return "X transform"; }
const char* dgbSurfaceReader::sKeyTransformY() 	{ return "Y transform"; }

const char* dgbSurfaceReader::sMsgParseError()  { return "Cannot parse file"; }
const char* dgbSurfaceReader::sMsgReadError()
{ return "Unexpected end of file"; }
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
{
    init( ioobj.fullUserExpr(true), ioobj.name() );
    error_ = !readHeaders( filetype );
}


dgbSurfaceReader::dgbSurfaceReader( const char* fulluserexp,
				    const char* objname,
				    const char* filetype )
    : ExecutorGroup( "Surface Reader" )
{
    init( fulluserexp, objname );
    error_ = !readHeaders( filetype );
}



void dgbSurfaceReader::init( const char* fullexpr, const char* objname )
{
    conn_ = new StreamConn( fullexpr, Conn::Read );
    cube_ = 0;
    surface_ = 0;
    par_ = 0;
    setsurfacepar_ = false;
    readrowrange_ = 0;
    readcolrange_ = 0;
    zrange_ = Interval<float>(mUdf(float),mUdf(float));
    int16interpreter_ = 0;
    int32interpreter_ = 0;
    int64interpreter_ = 0;
    floatinterpreter_ = 0;
    nrdone_ = 0;
    readonlyz_ =true;
    sectionindex_ = 0;
    sectionsread_ = 0;
    oldsectionindex_ = -1; 
    isinited_ = false;
    fullyread_ = true;
    version_ = 1;
    readlinenames_ = 0;
    linestrcrgs_ = 0;		       
    nrrows_ = 0;

    sectionnames_.allowNull();
    linenames_.allowNull();

    BufferString exnm( "Reading surface '", objname, "'" );
    setName( exnm.buf() );
    setNrDoneText( "Nr done" );
    auxdataexecs_.allowNull(true);
    if ( !conn_ || !conn_->forRead()  )
    {
	msg_ = "Cannot open input surface file";
	error_ = true;
	return;
    }

    createAuxDataReader();
}


void dgbSurfaceReader::setOutput( EM::Surface& ns )
{
    if ( surface_ ) surface_->unRef();
    surface_ = &ns;
    surface_->ref();
}


void dgbSurfaceReader::setOutput( Array3D<float>& cube )
{
    cube_ = &cube;
}


bool dgbSurfaceReader::readParData( std::istream& strm, const IOPar& toppar,
					const char* horfnm )
{
    if ( version_ == 3 )
    {
	const od_int64 nrsectionsoffset = readInt64( strm );
	if ( !strm || !nrsectionsoffset )
	{ msg_ = sMsgReadError(); return false; }

	StrmOper::seek( strm, nrsectionsoffset, std::ios::beg );
	const int nrsections = readInt32( strm );
	if ( !strm ) { msg_ = sMsgReadError(); return false; }

	for ( int idx=0; idx<nrsections; idx++ )
	{
	    const od_int64 off = readInt64( strm );
	    if ( !off ) { msg_ = sMsgReadError(); return false; }
	    sectionoffsets_ += off;
	}

	for ( int idx=0; idx<nrsections; idx++ )
	    sectionids_ += mCast(EM::SectionID,readInt32(strm));

	if ( !strm ) { msg_ = sMsgReadError(); return false; }

	ascistream parstream( strm, false );
	parstream.next();
	par_ = new IOPar( parstream );
    }
    else
    {
	int nrsections;
	if ( !toppar.get( sKeyNrSections() , nrsections ) &&
	     !toppar.get( sKeyNrSectionsV1(), nrsections ) )
	{
	    msg_ = sMsgParseError();
	    return false;
	}

	for ( int idx=0; idx<nrsections; idx++ )
	{
	    int sectionid = idx;
	    BufferString key = sSectionIDKey( idx );
	    toppar.get(key.buf(), sectionid);
	    sectionids_ += (EM::SectionID)sectionid;

	}

	par_ = new IOPar( toppar );
    }

    mergeExternalPar( horfnm );
    return true;
}


void dgbSurfaceReader::mergeExternalPar( const char* horfnm )
{
    FilePath fp( horfnm ); fp.setExtension( "par" );
    StreamData sd( StreamProvider(fp.fullPath()).makeIStream() );
    if ( !sd.usable() ) return;
    IOPar par;
    if ( par.read(*sd.istrm,"Surface parameters") )
	const_cast<IOPar*>(par_)->merge( par );
    sd.close();
}


int dgbSurfaceReader::scanFor2DGeom( TypeSet< StepInterval<int> >& trcranges )
{
    TypeSet<int> lineids; bool is2d = false;

    if ( par_->find(sKey::GeomID()) )
    {
	is2d = true;
	int nrlines = 0;
	par_->get( Horizon2DGeometry::sKeyNrLines(), nrlines );
	
	for ( int idx=0; idx<nrlines; idx++ )
	{
	    int geomid;
	    BufferString key = IOPar::compKey( sKey::GeomID(),idx );
	    par_->get( key, geomid );
	    geomids_.add( geomid );
	    StepInterval<int> trcrange( mUdf(int), mUdf(int), 1 );
	    par_->get( IOPar::compKey(key,Horizon2DGeometry::sKeyTrcRg()), 
								    trcrange);
	    trcranges += trcrange;
	}
    }
    else if ( par_->find( Horizon2DGeometry::sKeyNrLines()) )
    {
	is2d = true;
	int nrlines = 0;
	par_->get( Horizon2DGeometry::sKeyNrLines(), nrlines );
	
	for ( int idx=0; idx<nrlines; idx++ )
	{
#ifdef mNew2DGeometryImpl

	    BufferString key = IOPar::compKey( "Line", idx );

	    BufferString idstr;
	    par_->get( IOPar::compKey(key,Horizon2DGeometry::sKeyID()), idstr );
	    PosInfo::GeomID oldgeomid; oldgeomid.fromString( idstr );
	    S2DPOS().setCurLineSet( oldgeomid.lsid_ );
	    BufferString lnm = S2DPOS().getLineName( oldgeomid.lineid_ );
	    int geomid = Survey::GM().getGeomID( lnm.buf() );
	    if ( geomid < 0 )
	    {
		lnm = Survey::Geometry2D::makeUniqueLineName( S2DPOS().
			curLineSet(),S2DPOS().getLineName(oldgeomid.lineid_) );
		geomid = Survey::GM().getGeomID( lnm.buf() );
	    }

	    geomids_ += geomid;
	    StepInterval<int> trcrange( mUdf(int), mUdf(int), 1 );
	    par_->get( IOPar::compKey(key,Horizon2DGeometry::sKeyTrcRg()),
								    trcrange );
	    trcranges += trcrange;
#else
	    SeparString linekey( "Line", '.' );
	    linekey.add( idx );

	    PosInfo::GeomID geomid;
	    SeparString lineidkey( linekey.buf(), '.' );
	    lineidkey.add( Horizon2DGeometry::sKeyID() );
	    BufferString geomidstr;
	    if ( !par_->get(lineidkey.buf(),geomidstr) )
		continue;

	    geomid.fromString( geomidstr );
	    S2DPOS().setCurLineSet( geomid.lsid_ );
	    linesets_.add(
		    S2DPOS().hasLineSet(geomid.lsid_)
		    ? S2DPOS().getLineSet(geomid.lsid_) : sKeyUndefLineSet() );
	    linenames_.add(S2DPOS().hasLine(geomid.lineid_,geomid.lsid_)
		    ? S2DPOS().getLineName(geomid.lineid_) : sKeyUndefLine() );
	    
	    SeparString linetrcrgkey( linekey.buf(), '.' );
	    linetrcrgkey.add( Horizon2DGeometry::sKeyTrcRg() );
	    StepInterval<int> trcrange( mUdf(int), mUdf(int), 1 );
	    par_->get( linetrcrgkey, trcrange );
	    trcranges += trcrange;
#endif
	}
    }
    else if ( par_->get(Horizon2DGeometry::sKeyLineIDs(),lineids) )
    {
	is2d = true;
	if ( linenames_.size() != lineids.size() )
	    { msg_ = "Inconsistency in horizon file header"; return -1; }

	for ( int idx=0; idx<lineids.size(); idx++ )
	{
	    BufferString linesetkey(Horizon2DGeometry::sKeyLineSets(),idx);
	    if ( !par_->hasKey(linesetkey) )
		continue;
	    MultiID mid;
	    par_->get( linesetkey, mid );
	    PtrMan<IOObj> ioobj = IOM().get( mid );
	    linesets_.add( ioobj ? ioobj->name() : "" );
	    if ( !ioobj ) lineids[idx] = mUdf(int);
	    BufferString trcrangekey( 
		    Horizon2DGeometry::sKeyTraceRange(), idx );

	    StepInterval<int> trcrange( mUdf(int), mUdf(int), 1 );
	    par_->get( trcrangekey, trcrange );
	    trcranges += trcrange;
	}

	int idx = 0;
	while ( idx<lineids.size() )
	{
	    if ( mIsUdf(lineids[idx]) )
	    {
		lineids.removeSingle( idx );
		linenames_.removeSingle( idx );
		continue;
	    }
	    idx++;
	}
    }
    return is2d ? 1 : 0;
}


bool dgbSurfaceReader::readHeaders( const char* filetype )
{
    StreamConn& sconn( *((StreamConn*)conn_) );
    std::istream& strm = sconn.iStream();
    ascistream astream( strm );
    if ( !astream.isOfFileType( filetype ))
	{ msg_ = "Invalid filetype"; return false; }

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

    if ( !readParData(strm,toppar,sconn.fileName()) )
	return false;

    for ( int idx=0; idx<sectionids_.size(); idx++ )
    {
	const BufferString key = sSectionNameKey( idx );
	BufferString sectionname;
	par_->get(key.buf(),sectionname);
		    
	sectionnames_ += sectionname.size() ? new BufferString(sectionname) : 0;
    }

    par_->get( sKeyRowRange(), rowrange_ );
    par_->get( sKeyColRange(), colrange_ );
    par_->get( sKeyZRange(), zrange_ );
    par_->get( Horizon2DGeometry::sKeyLineNames(), linenames_ );

    TypeSet< StepInterval<int> > trcranges;
    const int res = scanFor2DGeom( trcranges );
    if ( res < 0 ) return false;
    const bool is2d = res;
    setLinesTrcRngs( trcranges );

    for ( int idx=0; idx<nrSections(); idx++ )
	sectionsel_ += sectionID(idx);
    for ( int idx=0; idx<nrAuxVals(); idx++ )
	auxdatasel_ += idx;

    par_->get( sKeyDBInfo(), dbinfo_ );

    if ( version_==1 )
	return parseVersion1( *par_ );

#ifdef mNew2DGeometryImpl
    if ( is2d && geomids_.isEmpty() )
#else
    if ( is2d && linesets_.isEmpty() )
#endif
    {
	msg_ = "No geometry found for this horizon";
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

	BufferString hovfnm( 
		dgbSurfDataWriter::createHovName(conn_->fileName(),idx) );
	if ( File::isEmpty(hovfnm.buf()) )
	{ gap++; continue; }

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
    deepErase( sectionnames_ );
    deepErase( linenames_ );
    deepErase( auxdatanames_ );
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
    {
	surface_->geometry().resetChangedFlag();
	surface_->unRef();
    }
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


int dgbSurfaceReader::stratLevelID() const
{
    int ret = -1;
    if ( pars() )
	pars()->get( sKey::StratRef(), ret );
    return ret;
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


int dgbSurfaceReader::nrLines() const
{ 
#ifdef mNew2DGeometryImpl
    return geomids_.size();
#else
    return linenames_.size(); 
#endif
}


BufferString dgbSurfaceReader::lineName( int idx ) const
{ return linenames_.get( idx ); }


BufferString dgbSurfaceReader::lineSet( int idx ) const
{ return linesets_.validIdx( idx ) ? linesets_.get( idx ) : BufferString(""); }


TraceID::GeomID dgbSurfaceReader::lineGeomID( int idx ) const
{ return geomids_.validIdx( idx ) ? geomids_[idx] : -1; }



void dgbSurfaceReader::selSections(const TypeSet<SectionID>& sel)
{
    sectionsel_ = sel;
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


od_int64 dgbSurfaceReader::nrDone() const
{
    return (executors_.size() ? ExecutorGroup::nrDone() : 0) + nrdone_;
}


const char* dgbSurfaceReader::nrDoneText() const
{
    return "Gridlines read";
}


od_int64 dgbSurfaceReader::totalNr() const
{
    int ownres =
	(readrowrange_?readrowrange_->nrSteps():rowrange_.nrSteps()) *
	sectionsel_.size();

    if ( !ownres ) ownres = nrrows_;

    return ownres + (executors_.size() ? ExecutorGroup::totalNr() : 0);
}


void dgbSurfaceReader::setGeometry()
{
    if ( surface_ )
    {
	surface_->removeAll();
	surface_->setDBInfo( dbinfo_.buf() );
	for ( int idx=0; idx<auxdatasel_.size(); idx++ )
	{
	    if ( auxdatasel_[idx]>=auxdataexecs_.size() )
		continue;

	    auxdataexecs_[auxdatasel_[idx]]->setSurface(
		    reinterpret_cast<Horizon3D&>(*surface_));

	    add( auxdataexecs_[auxdatasel_[idx]] );
	    auxdataexecs_.replace( auxdatasel_[idx], 0 );
	}
    }

    if ( readrowrange_ )
    {
	const RowCol filestep = getFileStep();

	if ( readrowrange_->step < abs(filestep.row) )
	    readrowrange_->step = filestep.row;
	if ( readcolrange_->step < abs(filestep.col) )
	    readcolrange_->step = filestep.col;

	if ( filestep.row && readrowrange_->step / filestep.row < 0 )
	    readrowrange_->step *= -1;
	if ( filestep.col && readcolrange_->step / filestep.col < 0 )
	    readcolrange_->step *= -1;
    }

    mDynamicCastGet(Horizon3D*,hor,surface_);
    if ( hor )
    {
	const RowCol filestep = getFileStep();
	const RowCol loadedstep = readrowrange_ && readcolrange_
	    ? RowCol(readrowrange_->step,readcolrange_->step)
	    : filestep;
   
	hor->geometry().setStep( filestep, loadedstep );
    }
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
	if ( !strm || !rowoffsets_[idx] )
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
    if ( version_==1 || (version_==2 && !isBinary()) )
	return false;

    if ( !sectionsel_.isPresent(sectionids_[sectionindex_]) )
	return true;

    if ( !readrowrange_ )
	return false;

    if ( !readrowrange_->includes( row, false ) )
	return true;

    return (row-readrowrange_->start)%readrowrange_->step;
}


int dgbSurfaceReader::currentRow() const
{ return firstrow_+rowindex_*rowrange_.step; }


int dgbSurfaceReader::nextStep()
{
    if ( error_ || (!surface_ && !cube_) )
    {
	if ( !surface_ && !cube_ ) 
	    msg_ = "Internal: No Output Set";

	return ErrorOccurred();
    }

    std::istream& strm = conn_->iStream();

    if ( !isinited_ )
    {
	isinited_ = true;

	if ( surface_ ) surface_->enableGeometryChecks( false );
	setGeometry();
	par_->getYN( sKeyDepthOnly(), readonlyz_ );
    }

    if ( sectionindex_>=sectionids_.size() )
    {
	if ( !surface_ ) return Finished();

	int res = ExecutorGroup::nextStep();
	if ( !res && !setsurfacepar_ )
	{
	    setsurfacepar_ = true;
	    if ( !surface_->usePar(*par_) )
	    {
		msg_ = "Could not parse header";
		return ErrorOccurred();
	    }

	    surface_->setFullyLoaded( fullyread_ );

	    surface_->resetChangedFlag();
	    surface_->enableGeometryChecks(true);
	    
	    if ( !surface_->nrSections() )
		surface_->geometry().addSection( "", false );
	}

	return res;
    }

    if ( sectionindex_!=oldsectionindex_ )
    {
	const int res = prepareNewSection(strm);
	if ( res!=Finished() )
	    return res;
    }

    while ( shouldSkipRow( currentRow() ) )
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
	return ErrorOccurred();

    const SectionID sectionid = sectionids_[sectionindex_];
    
    int nrcols = readInt32( strm );
    int firstcol = nrcols ? readInt32( strm ) : 0;
    int noofcoltoskip = 0;
    
    BufferStringSet lines;
    if ( par_->hasKey(Horizon2DGeometry::sKeyLineNames()) )
	par_->get( Horizon2DGeometry::sKeyLineNames(),lines );
    else if ( par_->find(sKey::GeomID()) )
    {
	int nrlines = 0;
	par_->get( Horizon2DGeometry::sKeyNrLines(), nrlines );
	for ( int idx=0; idx<nrlines; idx++ )
	{
	    BufferString key = IOPar::compKey( sKey::GeomID(), idx );
	    int geomid;
	    par_->get( key, geomid );
	    BufferString lnm = Survey::GM().getName( geomid );
	    if ( lnm.isEmpty() )
		sKeyUndefLine();
	    else
		lines.add( lnm );
	}
    }
    else if ( par_->find(Horizon2DGeometry::sKeyNrLines()) )
    {
	int nrlines = 0;
	par_->get( Horizon2DGeometry::sKeyNrLines(), nrlines );
	for ( int idx=0; idx<nrlines; idx++ )
	{
	    BufferString key = IOPar::compKey( "Line", idx );
	    BufferString idstr;
	    par_->get( IOPar::compKey(key,Horizon2DGeometry::sKeyID()), idstr );
	    PosInfo::GeomID geomid; geomid.fromString( idstr );
	    lines.add( S2DPOS().hasLine(geomid.lineid_,geomid.lsid_) ?
		S2DPOS().getLineName(geomid.lineid_) : sKeyUndefLine() );
	}
    }

    if ( readlinenames_ && linestrcrgs_ && !lines.isEmpty() )
    {
	const int trcrgidx =
	    readlinenames_->indexOf( lines.get(rowindex_).buf() );
	int callastcols = ( firstcol - 1 ) + nrcols;

	StepInterval<int> trcrg =
	    linestrcrgs_->validIdx(trcrgidx) ? (*linestrcrgs_)[trcrgidx]
	    				     : StepInterval<int>(0,0,1);
	if ( trcrg.width() > 1 )
	{
	    if ( firstcol < trcrg.start )
		noofcoltoskip = trcrg.start - firstcol;

	    if ( trcrg.stop < callastcols )
		callastcols = trcrg.stop;
	}

	nrcols = callastcols - firstcol - noofcoltoskip + 1;
    }
    if ( !strm )
    {
	msg_ = sMsgReadError();
	return ErrorOccurred();
    }

    int colstep = colrange_.step;
    par_->get( sColStepKey( currentRow() ).buf(), colstep );

    mDynamicCastGet(Horizon2D*,hor2d,surface_);

    if ( hor2d )
    {
#ifdef mNew2DGeometryImpl
	if ( (!geomids_.validIdx(rowindex_)) || (geomids_[rowindex_]<0) )
#else
	if ( (!linesets_.validIdx(rowindex_) || !linenames_.validIdx(rowindex_))
	  || (linesets_.get(rowindex_)==sKeyUndefLineSet() ||
	      linenames_.get(rowindex_)==sKeyUndefLine()) )
#endif
	{
	    const int res = skipRow( strm );
	    if ( res==ErrorOccurred() )
		return res;
	    else if ( res==Finished() ) 
		return MoreToDo();
	}

	if ( !hor2d->sectionGeometry(sectionid) )
	    createSection( sectionid );

#ifdef mNew2DGeometryImpl
	hor2d->geometry().sectionGeometry( sectionid )->addUdfRow
				(geomids_[rowindex_],firstcol+noofcoltoskip,
				    firstcol+nrcols+noofcoltoskip-1, colstep );
#else
	PosInfo::GeomID geomid = S2DPOS().getGeomID(linesets_[rowindex_]->buf(),
	    					 linenames_[rowindex_]->buf() );
	if ( !geomid.isOK() )
	     return ErrorOccurred();

	hor2d->geometry().sectionGeometry( sectionid )->addUdfRow(
		geomid, firstcol+noofcoltoskip,
		firstcol+nrcols+noofcoltoskip-1, colstep );
#endif
    }

    mDynamicCastGet(Fault3D*,flt3d,surface_);
    if ( flt3d )
    {
	if ( !flt3d->sectionGeometry(sectionid) )
	    createSection( sectionid );
	flt3d->geometry().sectionGeometry( sectionid )->
	    addUdfRow( currentRow(), firstcol, nrcols );
    }

    mDynamicCastGet(FaultStickSet*,emfss,surface_);
    if ( emfss )
    {
	if ( !emfss->sectionGeometry(sectionid) )
	    createSection( sectionid );
	emfss->geometry().sectionGeometry( sectionid )->
	    addUdfRow( currentRow(), firstcol, nrcols );
    }

    mDynamicCastGet(PolygonBody*,polygon,surface_);
    if ( polygon )
    {
	if ( !polygon->sectionGeometry(sectionid) )
	    createSection( sectionid );
	polygon->geometry().sectionGeometry( sectionid )->
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


int dgbSurfaceReader::prepareNewSection( std::istream& strm )
{
    const SectionID sectionid = sectionids_[sectionindex_];
    if ( version_==3 && !sectionsel_.isPresent(sectionid) )
    {
	sectionindex_++;
	return MoreToDo();
    }

    if ( version_==3 )
	StrmOper::seek( strm, sectionoffsets_[sectionindex_], 
			std::ios_base::beg );

    nrrows_ = readInt32( strm );
    if ( !strm )
    {
	msg_ = sMsgReadError();
	return ErrorOccurred();
    }

    if ( nrrows_ )
    {
	firstrow_ = readInt32( strm );
	if ( !strm )
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
		    firstrow_+(nrrows_-1)*rowrange_.step, rowrange_.step );
	if ( sectionrowrg.stop<readrowrange_->start ||
	     sectionrowrg.start>readrowrange_->stop )
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
	    pos.x = readDouble( strm );
	    pos.y = readDouble( strm );
	}

	pos.z = readDouble( strm );

	//Read filltype
	if ( rowindex_!=nrrows_-1 && colindex!=nrcols-1 )
	    readInt32( strm );

	if ( !strm )
	{
	    msg_ = sMsgReadError();
	    return false;
	}

	if ( readrowrange_ && (!readrowrange_->includes(surfrc.row, false) ||
		    ((surfrc.row-readrowrange_->start)%readrowrange_->step)))
	{
	    fullyread_ = false;
	    continue;
	}

	if ( readcolrange_ && (!readcolrange_->includes(surfrc.col, false) ||
		    ((surfrc.col-readcolrange_->start)%readcolrange_->step)))
	{
	    fullyread_ = false;
	    continue;
	}

	if ( !surface_->sectionGeometry(sectionid) )
	    createSection( sectionid );

	surface_->setPos( sectionid, surfrc.toInt64(), pos, false );

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
	    pos.x = readDouble( strm );
	    pos.y = readDouble( strm );
	}

	pos.z = readDouble( strm );
	if ( !strm )
	{
	    msg_ = sMsgReadError();
	    return false;
	}

	if ( readcolrange_ && (!readcolrange_->includes(rowcol.col, false) ||
		    ((rowcol.col-readcolrange_->start)%readcolrange_->step)))
	{
	    fullyread_ = false;
	    continue;
	}

	if ( readrowrange_ && (!readrowrange_->includes(rowcol.row, false) ||
		    ((rowcol.row-readrowrange_->start)%readrowrange_->step)))
	{
	    fullyread_ = false;
	    continue;
	}

	if ( !surface_->sectionGeometry(sectionid) )
	    createSection( sectionid );

	surface_->setPos( sectionid, rowcol.toInt64(), pos, false );

	isrowused = true;
    }

    if ( isrowused )
	nrdone_++;

    return true;
}


int dgbSurfaceReader::skipRow( std::istream& strm )
{
    if ( version_!=3 )
    {
	if ( !isBinary() ) return ErrorOccurred();

	const int nrcols = readInt32( strm );
	if ( !strm ) return ErrorOccurred();

	int offset = 0;
	if ( nrcols )
	{
	    int nrbytespernode = (readonlyz_?1:3)*floatinterpreter_->nrBytes();
	    if ( version_==1 )
		nrbytespernode += int32interpreter_->nrBytes(); //filltype

	    offset += nrbytespernode*nrcols;

	    offset += int32interpreter_->nrBytes(); //firstcol

	    StrmOper::seek( strm, offset, std::ios_base::cur );
	    if ( !strm ) return ErrorOccurred();
	}
    }

    goToNextRow();
    return sectionindex_!=oldsectionindex_ ? Finished() : MoreToDo();
}


bool dgbSurfaceReader::prepareRowRead( std::istream& strm )
{
    if ( version_!=3 )
	return true;

    StrmOper::seek( strm, rowoffsets_[rowindex_], std::ios_base::beg );
    return strm;
}


void dgbSurfaceReader::goToNextRow()
{
    rowindex_++;
    if ( rowindex_>=nrrows_ )
    {
	if ( surface_ )
	{
	    const SectionID sid = sectionids_[sectionindex_];
	    if ( surface_->geometry().sectionGeometry( sid ) )
		surface_->geometry().sectionGeometry( sid )->trimUndefParts();
	}

	sectionindex_++;
	sectionsread_++;
	nrdone_ = sectionsread_ *
	    ( readrowrange_ ? readrowrange_->nrSteps() : rowrange_.nrSteps() );
    }
}


bool dgbSurfaceReader::readVersion3Row( std::istream& strm, int firstcol, 
					int nrcols, int colstep, int colstoskip)
{
    SamplingData<double> zsd;
    if ( readonlyz_ )
    {
	zsd.start = readDouble( strm );
	zsd.step = readDouble( strm );
    }

    int colindex = 0;

    if ( readcolrange_ )
    {
	const StepInterval<int> colrg( firstcol, firstcol+(nrcols-1)*colstep, 
				       colstep );
	if ( colrg.stop<readcolrange_->start ||
	     colrg.start>readcolrange_->stop )
	{
	    fullyread_ = false;
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
		StrmOper::seek( strm, colindex*int16interpreter_->nrBytes(),
				std::ios_base::cur );
	    }
	}
    }

    RowCol rc( currentRow(), 0 );
    bool didread = false;
    const SectionID sectionid = sectionids_[sectionindex_];
    const int cubezidx = sectionsel_.indexOf( sectionid );

    for ( ; colindex<nrcols+colstoskip; colindex++ )
    {
	rc.col = firstcol+colindex*colstep;
	Coord3 pos;
	if ( !readonlyz_ )
	{
	    double x = readDouble( strm );
	    double y = readDouble( strm );
	    double z = readDouble( strm );

	    if ( colindex < (colstoskip-1) )
		continue;

	    pos.x = x;
	    pos.y = y;
	    pos.z = z;
	}
	else
	{
	    const int zidx = readInt16( strm );

	    if ( colindex < (colstoskip-1) )
		continue;
	    pos.z = (zidx==65535) ? mUdf(float) : zsd.atIndex( zidx );
	}

	if ( readcolrange_ )
	{
	    if ( rc.col<readcolrange_->start )
		continue;

	    if ( rc.col>readcolrange_->stop )
		break;

	    if ( (rc.col-readcolrange_->start)%readcolrange_->step )
		continue;
	}

	if ( !strm )
	{
	    msg_ = sMsgReadError();
	    return false;
	}

	if ( !Math::IsNormalNumber(pos.z) || !Math::IsNormalNumber(pos.x) ||
	     !Math::IsNormalNumber(pos.y) )
	    continue;

	if ( surface_ )
	{
	    if ( !surface_->sectionGeometry(sectionid) )
		createSection( sectionid );

	    surface_->setPos( sectionid, rc.toInt64(), pos, false );
	}

	if ( cube_ )
	{
	    cube_->set( readrowrange_->nearestIndex(rc.row),
			readcolrange_->nearestIndex(rc.col),
			cubezidx, (float) pos.z );
	}

	didread = true;
    }

    if ( didread )
	nrdone_++;

    return true;
}


void dgbSurfaceReader::createSection( const SectionID& sectionid )
{
    const int index = sectionids_.indexOf(sectionid);
    surface_->geometry().addSection( sectionName(index), sectionid, false );
    mDynamicCastGet( Geometry::BinIDSurface*,bidsurf,
	    	     surface_->geometry().sectionGeometry(sectionid) );
    if ( !bidsurf )
	return;

    StepInterval<int> inlrg = readrowrange_ ? *readrowrange_ : rowrange_;
    StepInterval<int> crlrg = readcolrange_ ? *readcolrange_ : colrange_;
    inlrg.sort(); crlrg.sort();

    mDeclareAndTryAlloc( Array2D<float>*, arr,
	    Array2DImpl<float>(inlrg.nrSteps()+1, crlrg.nrSteps()+1) );
    arr->setAll( mUdf(float) );

    bidsurf->setArray( RowCol( inlrg.start, crlrg.start),
	    	       RowCol( inlrg.step, crlrg.step ), arr, true );
}


const char* dgbSurfaceReader::message() const
{
    return msg_.buf();
}


int dgbSurfaceReader::readInt16(std::istream& strm) const
{
    if ( int16interpreter_ )
    {
	const int sz = int16interpreter_->nrBytes();
	ArrPtrMan<char> buf = new char [sz];
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
	ArrPtrMan<char> buf = new char [sz];
	strm.read(buf,sz);
	return int32interpreter_->get(buf,0);
    }

    int res;
    strm >> res;
    return res;
}


od_int64 dgbSurfaceReader::readInt64(std::istream& strm) const
{
    if ( int64interpreter_ )
    {
	const int sz = int64interpreter_->nrBytes();
	ArrPtrMan<char> buf = new char [sz];
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




double dgbSurfaceReader::readDouble(std::istream& strm) const
{
    if ( floatinterpreter_ )
    {
	const int sz = floatinterpreter_->nrBytes();
	ArrPtrMan<char> buf = new char [sz];
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
    , surface_(surface)
    , filetype_(filetype)
    , binary_(binary)
    , objectmid_(ioobj ? ioobj->key() : MultiID::udf() )
{ 
    init( ioobj ? ioobj->fullUserExpr(false) : 0 );
}


dgbSurfaceWriter::dgbSurfaceWriter( const char* fulluserexpr,
				    const char* filetype,
				    const Surface& surface,
				    bool binary )
    : ExecutorGroup( "Surface Writer" )
    , surface_(surface)
    , filetype_(filetype)
    , binary_(binary)
    , objectmid_(MultiID::udf())
{ 
    init( fulluserexpr );
}


void dgbSurfaceWriter::init( const char* fulluserexpr )
{
    fulluserexpr_ = fulluserexpr;
    par_ = new IOPar("Surface parameters" );
    writerowrange_ = 0;
    writecolrange_ = 0;
    writtenrowrange_ = Interval<int>( INT_MAX, INT_MIN );
    writtencolrange_ = Interval<int>( INT_MAX, INT_MIN );
    zrange_ = Interval<float>(SI().zRange(false).stop,SI().zRange(false).start);
    nrdone_ = 0;
    sectionindex_ = 0;
    oldsectionindex_= -1;
    writeonlyz_ = false;
    nrrows_ = 0;
    shift_ = 0;
    writingfinished_ = false;
    geometry_ = reinterpret_cast<const EM::RowColSurfaceGeometry*>(
	     						&surface_.geometry() );
    surface_.ref();
    setNrDoneText( "Nr done" );
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
    if ( conn_ )
    {
	std::ostream& strm = conn_->oStream();
	const od_int64 nrsectionsoffset = strm.tellp();
	writeInt32( strm, sectionsel_.size(), sEOL() );

	for ( int idx=0; idx<sectionoffsets_.size(); idx++ )
	    writeInt64( strm, sectionoffsets_[idx], sEOL() );

	for ( int idx=0; idx<sectionsel_.size(); idx++ )
	    writeInt32( strm, sectionsel_[idx], sEOL() );


	const od_int64 secondparoffset = strm.tellp();
	strm.seekp( nrsectionsoffsetoffset_, std::ios::beg );
	writeInt64( strm, nrsectionsoffset, sEOL() );
	strm.seekp( secondparoffset, std::ios::beg );

	par_->setYN( dgbSurfaceReader::sKeyDepthOnly(), writeonlyz_ );

	const int rowrgstep = writerowrange_ ? 
			      writerowrange_->step : rowrange_.step;
	par_->set( dgbSurfaceReader::sKeyRowRange(),
		  writtenrowrange_.start, writtenrowrange_.stop, rowrgstep );

	const int colrgstep = writecolrange_ ? 
			      writecolrange_->step : colrange_.step;
	par_->set( dgbSurfaceReader::sKeyColRange(),
		  writtencolrange_.start, writtencolrange_.stop, colrgstep );
	
	par_->set( dgbSurfaceReader::sKeyZRange(), zrange_ );
			      
	for (int idx=firstrow_; idx<firstrow_+rowrgstep*nrrows_; idx+=rowrgstep)
	{
	    const int idxcolstep = geometry_->colRange(idx).step;
	    if ( idxcolstep && idxcolstep!=colrange_.step )
		par_->set( dgbSurfaceReader::sColStepKey(idx).buf(),idxcolstep);
	}

	ascostream astream( strm );
	astream.newParagraph();
	par_->putTo( astream );
    }

    writingfinished_ = true;
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
    return hor ? hor->auxdata.auxDataName(idx) : 0;
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
    return par_;
}


od_int64 dgbSurfaceWriter::nrDone() const
{
    return (executors_.size() ? ExecutorGroup::nrDone() : 0) + nrdone_;
}


const char* dgbSurfaceWriter::nrDoneText() const
{
    return "Gridlines written";
}


od_int64 dgbSurfaceWriter::totalNr() const
{
    return (executors_.size() ? ExecutorGroup::totalNr() : 0) + 
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
    if ( !nrdone_ )
    {
	conn_ = fulluserexpr_ ? new StreamConn(fulluserexpr_,Conn::Write) : 0;
	if ( !conn_ )
	{
	    msg_ = "Cannot open output surface file";
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


	std::ostream& strm = conn_->oStream();
	ascostream astream( strm );
	astream.putHeader( filetype_.buf() );
	versionpar.putTo( astream );
	nrsectionsoffsetoffset_ = strm.tellp();
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
		for ( int count=0; true; count++ )
		{
		    fnm = dgbSurfDataWriter::createHovName( conn_->fileName(),
							    count );
		    if ( !File::exists(fnm.buf()) )
			break;
		}
	    }
	    add(new dgbSurfDataWriter(*hor,dataidx,0,binary_,fnm.buf()));
	    // TODO:: Change binid sampler so not all values are written when
	    // there is a subselection
	}
    }

    if ( sectionindex_>=sectionsel_.size() )
    {
	const int res = ExecutorGroup::nextStep();
	if ( !res && objectmid_==surface_.multiID() ) 
	    const_cast<Surface*>(&surface_)->resetChangedFlag();
	if ( res == Finished() )
	    finishWriting();

	return res;
    }

    std::ostream& strm = conn_->oStream();

    if ( sectionindex_!=oldsectionindex_ && !writeNewSection( strm ) )
	return ErrorOccurred();

    if ( nrrows_ && !writeRow( strm ) )
	return ErrorOccurred();
	    
    rowindex_++;
    if ( rowindex_>=nrrows_ )
    {
	strm.seekp( rowoffsettableoffset_, std::ios_base::beg );
	if ( !strm )
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

	strm.seekp( 0, std::ios_base::end );
	if ( !strm )
	{
	    msg_ = sMsgWriteError();
	    return ErrorOccurred();
	}

	sectionindex_++;
	strm.flush();
	if ( !strm )
	{
	    msg_ = sMsgWriteError();
	    return ErrorOccurred();
	}
    }

    nrdone_++;
    return MoreToDo();
}


const char* dgbSurfaceWriter::message() const
{
    return msg_.buf();
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


bool dgbSurfaceWriter::writeInt32( std::ostream& strm, od_int32 val,
				   const char* post) const
{
    if ( binary_ )
	strm.write((const char*)&val,sizeof(val));
    else
	strm << val << post;

    return strm;
}


bool dgbSurfaceWriter::writeInt64( std::ostream& strm, od_int64 val,
				   const char* post) const
{
    if ( binary_ )
	strm.write((const char*)&val,sizeof(val));
    else
    {
	BufferString valstr;
	getStringFromInt64(val,valstr.buf());
	int len = valstr.size();
	for ( int idx=20; idx>len; idx-- )
	    strm << '0';
	strm << valstr << post;
    }

    return strm;
}


bool dgbSurfaceWriter::writeNewSection( std::ostream& strm )
{
    rowindex_ = 0;
    rowoffsettableoffset_ = 0;

    mDynamicCastGet( const Geometry::RowColSurface*, gsurf,
	    	     surface_.sectionGeometry( sectionsel_[sectionindex_] ) );

    if ( !gsurf || gsurf->isEmpty() )
    {
	nrrows_ = 0;
    }
    else
    {
	StepInterval<int> sectionrange = gsurf->rowRange();
	sectionrange.sort();
	firstrow_ = sectionrange.start;
	int lastrow = sectionrange.stop;

	if ( writerowrange_ )
	{
	    if (firstrow_>writerowrange_->stop || lastrow<writerowrange_->start)
	    {
		nrrows_ = 0;
	    }
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
	    nrrows_ = sectionrange.nrSteps()+1;
	}
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
	return MoreToDo();
    }

    if ( !writeInt32(strm,firstrow_,sEOL()) )
    {
	msg_ = sMsgWriteError();
	return ErrorOccurred();
    }

    rowoffsettableoffset_ = strm.tellp();
    for ( int idx=0; idx<nrrows_; idx++ )
    {
	if ( !writeInt64(strm,0,sEOL() ) )
	{
	    msg_ = sMsgWriteError();
	    return ErrorOccurred();
	}
    }

    oldsectionindex_ = sectionindex_;
    const BufferString sectionname =
	surface_.sectionName( sectionsel_[sectionindex_] );

    if ( sectionname.size() )
    {
	par_->set( dgbSurfaceReader::sSectionNameKey(sectionindex_).buf(),
		  sectionname );
    }

    return true;
}


void dgbSurfaceWriter::setShift( float s )
{ shift_ = s; }


bool dgbSurfaceWriter::writeRow( std::ostream& strm )
{
    if ( !colrange_.step || !rowrange_.step )
	pErrMsg("Steps not set");

    rowoffsettable_ += strm.tellp();
    const int row = firstrow_ + rowindex_ *
		    (writerowrange_ ? writerowrange_->step : rowrange_.step);

    const StepInterval<int> colrange = geometry_->colRange( row );

    const SectionID sectionid = sectionsel_[sectionindex_];
    TypeSet<Coord3> colcoords;

    int firstcol = -1;
    const int nrcols =
	(writecolrange_ ? writecolrange_->nrSteps() : colrange.nrSteps()) + 1;

    mDynamicCastGet(const Horizon3D*,hor3d,&surface_)
    for ( int colindex=0; colindex<nrcols; colindex++ )
    {
	const int col = writecolrange_ ? writecolrange_->atIndex(colindex) :
	    				colrange.atIndex(colindex);

	const PosID posid(  surface_.id(), sectionid,
				RowCol(row,col).toInt64() );
	Coord3 pos = surface_.getPos( posid );
	if ( hor3d && pos.isDefined() )
	    pos.z += shift_;

	if ( colcoords.isEmpty() && !pos.isDefined() )
	    continue;

	if ( !mIsUdf(pos.z) )
	    zrange_.include( (float) pos.z, false );

	if ( colcoords.isEmpty() )
	    firstcol = col;

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

	    if ( !writeDouble( strm, sd.start, sTab()) ||
		 !writeDouble( strm, sd.step, sEOLTab()) )
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

		if ( !writeInt16( strm, mCast(unsigned short,index), 
			          idx!=colcoords.size()-1 ? sEOLTab() : sEOL()))
		{
		    msg_ = sMsgWriteError();
		    return false;
		}
	    }
	    else
	    {
		if ( !writeDouble( strm, pos.x, sTab()) ||
		     !writeDouble( strm, pos.y, sTab()) ||
		     !writeDouble( strm, pos.z,
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
		    (writecolrange_ ? writecolrange_->step : colrange.step);
	writtencolrange_.include( lastcol, false );
    }

    return true;
}


bool dgbSurfaceWriter::writeDouble( std::ostream& strm, double val,
				       const char* post) const
{
    if ( binary_ )
	strm.write((const char*) &val,sizeof(val));
    else
    {
	char str[255];
	getStringFromDouble("%.17g", val, str );
	strm << str << post;
    }

    return strm;
}

}; //namespace

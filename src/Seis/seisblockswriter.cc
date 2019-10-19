/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Apr 2017
________________________________________________________________________

-*/

#include "seisblockswriter.h"
#include "seisblocksbackend.h"
#include "seismemblocks.h"

#include "arrayndimpl.h"
#include "ascstream.h"
#include "datachar.h"
#include "executor.h"
#include "file.h"
#include "horsubsel.h"
#include "keystrs.h"
#include "odmemory.h"
#include "posidxpairdataset.h"
#include "paralleltask.h"
#include "cubedata.h"
#include "scaler.h"
#include "seistrc.h"
#include "seisbuf.h"
#include "separstr.h"
#include "survgeom3d.h"
#include "survinfo.h" // for survey name and zInFeet
#include "uistrings.h"
#include "zdomain.h"


namespace Seis
{
namespace Blocks
{

class StepFinder
{
public:

		StepFinder(Writer&);

    bool	isFinished() const	{ return tbuf_.isEmpty(); }
    void	finish(uiRetVal&);
    void	addTrace(const SeisTrc&,uiRetVal&);

    Writer&		wrr_;
    Pos::IdxPairDataSet positions_;
    SeisTrcBuf		tbuf_;

};

} // namespace Blocks

} // namespace Seis


Seis::Blocks::StepFinder::StepFinder( Writer& wrr )
    : wrr_(wrr)
    , positions_(0,false)
    , tbuf_(false)
{
}


// Algo: first collect at least 2000 traces. Once 3 inls and 3 crls found,
// finish the procedure.

void Seis::Blocks::StepFinder::addTrace( const SeisTrc& trc, uiRetVal& uirv )
{
    const BinID bid( trc.info().binID() );
    if ( positions_.includes(bid) )
	return;
    tbuf_.add( new SeisTrc(trc) );
    positions_.add( bid );
    if ( positions_.totalSize() % 2000 )
	return;
    const int nrinls = positions_.nrFirst();
    if ( nrinls < 3 )
	return;

    bool found3crls = false;
    for ( int iinl=0; iinl<nrinls; iinl++ )
    {
	if ( positions_.nrSecondAtIdx(iinl) > 2 )
	    { found3crls = true; break; }
    }
    if ( !found3crls )
	return;

    finish( uirv );
}


// Algo: if enough inls/crls found - or at the end, finish() will be called.
// If at least one step was seen, the default (SI or provided) is replaced.
// Now add the collected traces to the writer so it can start off making
// the appropriate blocks.
// The writer will know the detection is finished because the tbuf is empty.

void Seis::Blocks::StepFinder::finish( uiRetVal& uirv )
{
    Pos::IdxPairDataSet::SPos spos;
    int inlstate = -1, crlstate = -1;
	// initialising these four because of alarmist code analysers
    int previnl = -1, prevcrl = -1;
    int inlstep = 1, crlstep = 1;
    while ( positions_.next(spos) )
    {
	const BinID bid( positions_.getIdxPair(spos) );
	if ( inlstate < 0 )
	{
	    previnl = bid.inl();
	    prevcrl = bid.crl();
	    inlstate = crlstate = 0;
	}
	const int inldiff = bid.inl() - previnl;
	if ( inldiff )
	{
	    previnl = bid.inl();
	    if ( inlstate < 1 )
	    {
		inlstep = inldiff;
		inlstate = 1;
	    }
	    else if ( inldiff < inlstep )
		inlstep = inldiff;
	    continue;
	}
	const int crldiff = bid.crl() - prevcrl;
	if ( crldiff < 1 )
	    continue;
	prevcrl = bid.crl();
	if ( crlstate < 1 )
	{
	    crlstep = crldiff;
	    crlstate = 1;
	}
	else if ( crldiff < crlstep )
	    crlstep = crldiff;
    }

    CubeHorSubSel chss;
    if ( inlstate > 0 )
    {
	chss.inlSubSel().setOutputStep( inlstep, previnl );
	wrr_.hgeom_.inlRange() = chss.inlRange();
    }
    if ( crlstate > 0 )
    {
	chss.crlSubSel().setOutputStep( crlstep, prevcrl );
	wrr_.hgeom_.crlRange() = chss.crlRange();
    }

    for ( int idx=0; idx<tbuf_.size(); idx++ )
    {
	SeisTrc* trc = tbuf_.get( idx );
	wrr_.doAdd( *trc, uirv );
	if ( uirv.isError() )
	    break;
    }

    tbuf_.deepErase(); // finished!
}



Seis::Blocks::Writer::Writer( const HGeom* hgeom )
    : specifieddatarep_(OD::AutoDataRep)
    , nrcomps_(1)
    , isfinished_(false)
    , stepfinder_(0)
    , interp_(new DataInterp(DataCharacteristics()))
    , backend_(0)
{
    if ( hgeom )
	hgeom_ = *hgeom;
    else
	hgeom_ = SurvGeom::get3D();
    zgeom_ = hgeom_.zRange();
}


Seis::Blocks::Writer::~Writer()
{
    if ( !isfinished_ )
    {
	Task* task = finisher();
	if ( task )
	{
	    task->execute();
	    delete task;
	}
    }

    delete backend_;
    deepErase( zevalpositions_ );
    delete interp_;
    delete stepfinder_;
}


void Seis::Blocks::Writer::setEmpty()
{
    clearColumns();
    deepErase( zevalpositions_ );
}


void Seis::Blocks::Writer::setBasePath( const File::Path& fp )
{
    if ( fp.fullPath() != basepath_.fullPath() )
    {
	basepath_ = fp;
	needreset_ = true;
    }
}


void Seis::Blocks::Writer::setFullPath( const char* nm )
{
    File::Path fp( nm );
    fp.setExtension( 0 );
    setBasePath( fp );
}


void Seis::Blocks::Writer::setFileNameBase( const char* nm )
{
    File::Path fp( basepath_ );
    fp.setFileName( nm );
    setBasePath( fp );
}


void Seis::Blocks::Writer::setDataRep( OD::DataRepType rep )
{
    if ( specifieddatarep_ != rep )
    {
	specifieddatarep_ = rep;
	needreset_ = true;
    }
}


void Seis::Blocks::Writer::setCubeName( const char* nm )
{
    cubename_ = nm;
}


void Seis::Blocks::Writer::setZDomain( const ZDomain::Def& def )
{
    zdomain_ = def;
}


void Seis::Blocks::Writer::addComponentName( const char* nm )
{
    compnms_.add( nm );
}


void Seis::Blocks::Writer::addAuxInfo( const char* key, const IOPar& iop )
{
    BufferString auxkey( key );
    if ( auxkey.isEmpty() )
	{ pErrMsg( "Refusing to add section without key" ); return; }

    IOPar* toadd = new IOPar( iop );
    toadd->setName( BufferString(sKeySectionPre(),auxkey) );
    auxiops_ += toadd;
}


void Seis::Blocks::Writer::setScaler( const LinScaler* newscaler )
{
    if ( (!scaler_ && !newscaler) )
	return;

    delete scaler_;
    scaler_ = newscaler ? newscaler->clone() : 0;
    needreset_ = true;
}


void Seis::Blocks::Writer::resetZ()
{
    const float eps = Seis::cDefSampleSnapDist();
    deepErase( zevalpositions_ );
    const int nrglobzidxs = Block::globIdx4Z(zgeom_,zgeom_.stop,dims_.z()) + 1;

    for ( idx_type globzidx=0; globzidx<nrglobzidxs; globzidx++ )
    {
	ZEvalPosSet* posset = new ZEvalPosSet;
	for ( idx_type loczidx=0; loczidx<dims_.z(); loczidx++ )
	{
	    const float z = Block::z4Idxs( zgeom_, dims_.z(),
					   globzidx, loczidx );
	    if ( z > zgeom_.start-eps && z < zgeom_.stop+eps )
		*posset += ZEvalPos( loczidx, z );
	}
	if ( posset->isEmpty() )
	    delete posset;
	else
	    zevalpositions_ += posset;
    }

    if ( zevalpositions_.isEmpty() )
    {
	pErrMsg("Huh");
	ZEvalPosSet* posset = new ZEvalPosSet;
	*posset += ZEvalPos( 0, zgeom_.start );
	 zevalpositions_ += posset;
    }
}


int Seis::Blocks::Writer::traceSize() const
{
    if ( zevalpositions_.isEmpty() )
	return 0;

    return (zevalpositions_.size()-1) * dims_.z()
	 + zevalpositions_.last()->size();
}


uiRetVal Seis::Blocks::Writer::add( const SeisTrc& trc )
{
    uiRetVal uirv;
    Threads::Locker locker( accesslock_ );

    if ( needreset_ )
    {
	needreset_ = false;
	isfinished_ = false;
	setEmpty();
	if ( trc.isEmpty() )
	{
	    uirv.add( tr("No data in input trace") );
	    return uirv;
	}

	datarep_ = specifieddatarep_;
	if ( datarep_ == OD::AutoDataRep )
	    datarep_ = trc.data().getInterpreter()->dataChar().userType();
	delete interp_;
	interp_ = DataInterp::create( DataCharacteristics(datarep_), true );

	zgeom_.start = trc.startPos();
	zgeom_.stop = trc.endPos();
	zgeom_.step = trc.stepPos();
	nrcomps_ = trc.nrComponents();
	resetZ();

	delete stepfinder_;
	stepfinder_ = new StepFinder( *const_cast<Writer*>(this) );
    }

    if ( stepfinder_ )
    {
	stepfinder_->addTrace( trc, uirv );
	if ( stepfinder_->isFinished() )
	    { delete stepfinder_; stepfinder_ = 0; }
	return uirv;
    }

    doAdd( trc, uirv );
    return uirv;
}


void Seis::Blocks::Writer::doAdd( const SeisTrc& trc, uiRetVal& uirv )
{
    const BinID bid = trc.info().binID();
    const HGlobIdx globidx( Block::globIdx4Inl(hgeom_,bid.inl(),dims_.inl()),
			    Block::globIdx4Crl(hgeom_,bid.crl(),dims_.crl()) );

    MemBlockColumn* column = getColumn( globidx );
    if ( !column )
    {
	uirv.set( tr("Memory needed for writing process unavailable.") );
	setEmpty();
	return;
    }
    else if ( isCompleted(*column) )
	return; // this check is absolutely necessary to for MT writing

    const HLocIdx locidx( Block::locIdx4Inl(hgeom_,bid.inl(),dims_.inl()),
			  Block::locIdx4Crl(hgeom_,bid.crl(),dims_.crl()) );

    for ( int icomp=0; icomp<nrcomps_; icomp++ )
    {
	MemBlockColumn::BlockSet& blockset = *column->blocksets_[icomp];
	for ( int iblock=0; iblock<blockset.size(); iblock++ )
	{
	    const ZEvalPosSet& posset = *zevalpositions_[iblock];
	    MemBlock& block = *blockset[iblock];
	    add2Block( block, posset, locidx, trc, icomp );
	}
    }

    bool& visited = column->visited_[locidx.inl()][locidx.crl()];
    if ( !visited )
    {
	column->nruniquevisits_++;
	visited = true;
    }

    if ( isCompleted(*column) )
	writeColumn( *column, uirv );
}


void Seis::Blocks::Writer::add2Block( MemBlock& block,
			const ZEvalPosSet& zevals, const HLocIdx& hlocidx,
			const SeisTrc& trc, int icomp )
{
    if ( block.isRetired() )
	return; // new visit of already written. Won't do, but no error.

    LocIdx locidx( hlocidx.inl(), hlocidx.crl(), 0 );
    for ( int idx=0; idx<zevals.size(); idx++ )
    {
	const ZEvalPos& evalpos = zevals[idx];
	locidx.z() = evalpos.first;
	float val2set = trc.getValue( evalpos.second, icomp );
	if ( scaler_ )
	    val2set = (float)scaler_->scale( val2set );
	block.setValue( locidx, val2set );
    }
}


Seis::Blocks::MemBlockColumn*
Seis::Blocks::Writer::mkNewColumn( const HGlobIdx& hglobidx )
{
    MemBlockColumn* column = new MemBlockColumn( hglobidx, dims_, nrcomps_ );

    const int nrglobzidxs = zevalpositions_.size();
    for ( idx_type globzidx=0; globzidx<nrglobzidxs; globzidx++ )
    {
	const ZEvalPosSet& evalposs = *zevalpositions_[globzidx];
	GlobIdx globidx( hglobidx.inl(), hglobidx.crl(), globzidx );
	Dimensions dims( dims_ );
	if ( globzidx == nrglobzidxs-1 )
	    dims.z() = (size_type)evalposs.size();

	for ( int icomp=0; icomp<nrcomps_; icomp++ )
	{
	    MemBlock* block = new MemBlock( globidx, dims, *interp_ );
	    if ( block->isRetired() ) // ouch
		{ delete column; return 0; }
	    block->zero();
	    *column->blocksets_[icomp] += block;
	}
    }

    return column;
}


Seis::Blocks::MemBlockColumn*
Seis::Blocks::Writer::getColumn( const HGlobIdx& globidx )
{
    Column* column = findColumn( globidx );
    if ( !column )
    {
	column = mkNewColumn( globidx );
	addColumn( column );
    }
    return (MemBlockColumn*)column;
}


bool Seis::Blocks::Writer::isCompleted( const MemBlockColumn& column ) const
{
    return column.nruniquevisits_ == ((int)dims_.inl()) * dims_.crl();
}


namespace Seis
{
namespace Blocks
{

class ColumnWriter : public Executor
{ mODTextTranslationClass(Seis::Blocks::ColumnWriter)
public:

ColumnWriter( WriteBackEnd& be, MemBlockColumn& colmn )
    : Executor("Column Writer")
    , backend_(be)
    , column_(colmn)
    , nrblocks_(be.wrr_.nrColumnBlocks())
    , iblock_(0)
{
    column_.getDefArea( start_, dims_ );
    backend_.setColumnInfo( column_, start_, dims_, uirv_ );
}

virtual od_int64 nrDone() const { return iblock_; }
virtual od_int64 totalNr() const { return nrblocks_; }
virtual uiString nrDoneText() const { return tr("Blocks written"); }
virtual uiString message() const
{
    if ( uirv_.isError() )
	return uirv_;
    return tr("Writing Traces");
}


int finish()
{
    column_.retire();
    return uirv_.isError() ? ErrorOccurred() : Finished();
}

virtual int nextStep()
{
    if ( uirv_.isError() || iblock_ >= nrblocks_ )
	return finish();

    for ( int icomp=0; icomp<backend_.wrr_.nrcomps_; icomp++ )
    {
	MemBlockColumn::BlockSet& blockset = *column_.blocksets_[icomp];
	MemBlock& block = *blockset[iblock_];
	backend_.putBlock( icomp, block, start_, dims_, uirv_ );
	if ( !uirv_.isOK() )
	    return finish();
    }

    iblock_++;
    return MoreToDo();
}

    WriteBackEnd&	backend_;
    MemBlockColumn&	column_;
    HDimensions		dims_;
    HLocIdx		start_;
    const int		nrblocks_;
    int			iblock_;
    uiRetVal		uirv_;

};

} // namespace Blocks
} // namespace Seis


void Seis::Blocks::Writer::writeColumn( MemBlockColumn& column, uiRetVal& uirv )
{
    if ( !backend_ )
    {
	if ( usehdf_ )
	    backend_ = new HDF5WriteBackEnd( *this, uirv );
	else
	    backend_ = new StreamWriteBackEnd( *this, uirv );
	if ( !uirv.isOK() )
	    return;
    }

    ColumnWriter wrr( *backend_, column );
    if ( !wrr.execute() )
	uirv = wrr.uirv_;
}


void Seis::Blocks::Writer::writeInfoFiles( uiRetVal& uirv )
{
    const BufferString ovvwfnm( overviewFileName() );
    if ( File::exists(ovvwfnm) )
	File::remove( ovvwfnm );

    od_ostream infostrm( infoFileName() );
    if ( infostrm.isBad() )
    {
	uirv.add( uiStrings::phrCannotOpenForWrite( infostrm.fileName() ) );
	return;
    }

    if ( !writeInfoFileData(infostrm) )
    {
	uirv.add( uiStrings::phrCannotWrite( toUiString(infostrm.fileName()) ));
	return;
    }

    od_ostream ovvwstrm( ovvwfnm );
    if ( ovvwstrm.isBad() )
	{ ErrMsg( uiStrings::phrCannotOpenForWrite(ovvwfnm) ); return; }
    if ( !writeOverviewFileData(ovvwstrm) )
	File::remove( ovvwfnm );
}


bool Seis::Blocks::Writer::writeInfoFileData( od_ostream& strm )
{
    ascostream ascostrm( strm );
    if ( !ascostrm.putHeader(sKeyFileType()) )
	return false;

    Interval<idx_type> globinlidxrg, globcrlidxrg;
    Interval<double> xrg, yrg;
    scanPositions( globinlidxrg, globcrlidxrg, xrg, yrg );
		    // also fills cubedata_

    gensectioniop_.set( sKeyFmtVersion(), version_ );
    gensectioniop_.set( sKeySurveyName(), SI().name() );
    gensectioniop_.set( sKeyCubeName(), cubename_ );
    DataCharacteristics::putUserTypeToPar( gensectioniop_, datarep_ );
    if ( scaler_ )
    {
	// write the scaler needed to reconstruct the org values
	LinScaler* invscaler = scaler_->inverse();
	invscaler->put( gensectioniop_ );
	delete invscaler;
    }
    gensectioniop_.set( sKey::XRange(), xrg );
    gensectioniop_.set( sKey::YRange(), yrg );
    gensectioniop_.set( sKey::InlRange(), finalinlrg_ );
    gensectioniop_.set( sKey::CrlRange(), finalcrlrg_ );
    gensectioniop_.set( sKey::ZRange(), zgeom_ );
    zdomain_.set( gensectioniop_ );
    if ( zdomain_.isDepth() && SI().zInFeet() )
	gensectioniop_.setYN( sKeyDepthInFeet(), true );

    FileMultiString fms;
    for ( int icomp=0; icomp<nrcomps_; icomp++ )
    {
	BufferString nm;
	if ( icomp < compnms_.size() )
	    nm = compnms_.get( icomp );
	else
	    nm.set( "Component " ).add( icomp+1 );
	fms += nm;
    }
    gensectioniop_.set( sKeyComponents(), fms );
    if ( datatype_ != UnknownData )
	gensectioniop_.set( sKeyDataType(), nameOf(datatype_) );

    hgeom_.putMapInfo( gensectioniop_ );
    gensectioniop_.set( sKeyDimensions(), dims_.inl(), dims_.crl(), dims_.z() );
    gensectioniop_.set( sKeyGlobInlRg(), globinlidxrg );
    gensectioniop_.set( sKeyGlobCrlRg(), globcrlidxrg );

    gensectioniop_.putTo( ascostrm );
    if ( !strm.isOK() )
	return false;

    Pos::IdxPairDataSet::SPos spos;
    while ( columns_.next(spos) )
    {
	const MemBlockColumn& column = *(MemBlockColumn*)columns_.getObj(spos);
	if ( column.nruniquevisits_ > 0 )
	{
	    const HGlobIdx& gidx = column.globIdx();
	    BufferString ky;
	    ky.add( gidx.inl() ).add( '.' ).add( gidx.crl() );
	    fileidsectioniop_.add( ky, column.fileid_ );
	}
    }
    fileidsectioniop_.setName( usehdf_ ? sKeyFileIDSection()
				       : sKeyOffSection() );
    fileidsectioniop_.putTo( ascostrm );
    if ( !strm.isOK() )
	return false;

    for ( int idx=0; idx<auxiops_.size(); idx++ )
    {
	auxiops_[idx]->putTo( ascostrm );
	if ( !strm.isOK() )
	    return false;
    }

    strm << sKeyPosSection() << od_endl;
    return cubedata_.write( strm, true );
}


#define mGetNrInlCrlAndRg() \
    const int stepinl = hgeom_.inlRange().step; \
    const int stepcrl = hgeom_.crlRange().step; \
    const StepInterval<int> inlrg( finalinlrg_.start, finalinlrg_.stop, \
				   stepinl ); \
    const StepInterval<int> crlrg( finalcrlrg_.start, finalcrlrg_.stop, \
				   stepcrl ); \
    const int nrinl = inlrg.nrSteps() + 1; \
    const int nrcrl = crlrg.nrSteps() + 1

bool Seis::Blocks::Writer::writeOverviewFileData( od_ostream& strm )
{
    ascostream astrm( strm );
    if ( !astrm.putHeader("Cube Overview") )
	return false;

    mGetNrInlCrlAndRg();

    Array2DImpl<float> data( nrinl, nrcrl );
    MemSetter<float> memsetter( data.getData(), mUdf(float),
				(od_int64)data.totalSize() );
    memsetter.execute();

    Pos::IdxPairDataSet::SPos spos;
    while ( columns_.next(spos) )
    {
	const MemBlockColumn& column = *(MemBlockColumn*)columns_.getObj(spos);
	const Dimensions bdims( column.dims() );
	const BinID start(
	    Block::startInl4GlobIdx(hgeom_,column.globIdx().inl(),bdims.inl()),
	    Block::startCrl4GlobIdx(hgeom_,column.globIdx().crl(),bdims.crl()));
	const MemColumnSummary& summary = column.summary_;

	for ( idx_type iinl=0; iinl<bdims.inl(); iinl++ )
	{
	    const int inl = start.inl() + inlrg.step * iinl;
	    const int ia2dinl = inlrg.nearestIndex( inl );
	    if ( ia2dinl < 0 || ia2dinl >= nrinl )
		continue;
	    for ( idx_type icrl=0; icrl<bdims.crl(); icrl++ )
	    {
		const int crl = start.crl() + crlrg.step * icrl;
		const int ia2dcrl = crlrg.nearestIndex( crl );
		if ( ia2dcrl < 0 || ia2dcrl >= nrcrl )
		    continue;
		data.set( ia2dinl, ia2dcrl, summary.vals_[iinl][icrl] );
	    }
	}
    }

    return writeFullSummary( astrm, data );
}


bool Seis::Blocks::Writer::writeFullSummary( ascostream& astrm,
				const Array2D<float>& data ) const
{
    mGetNrInlCrlAndRg();
    const Coord origin(
	    SI().transform(BinID(finalinlrg_.start,finalcrlrg_.start)) );
    const Coord endinl0(
	    SI().transform(BinID(finalinlrg_.start,finalcrlrg_.stop)) );
    const Coord endcrl0(
	    SI().transform(BinID(finalinlrg_.stop,finalcrlrg_.start)) );
    const double fullinldist = origin.distTo<double>( endinl0 );
    const double fullcrldist = origin.distTo<double>( endcrl0 );

    IOPar iop;
    iop.set( sKey::InlRange(), inlrg );
    iop.set( sKey::CrlRange(), crlrg );
    hgeom_.putMapInfo( iop );
    iop.putTo( astrm, false );

    const int smallestdim = nrinl > nrcrl ? nrcrl : nrinl;
    TypeSet<int_twins> levelnrblocks;
    int nodesperblock = smallestdim / 2;
    int nrlevels = 0;
    while ( nodesperblock > 1 )
    {
	const float finlnrnodes = ((float)nrinl) / nodesperblock;
	const float fcrlnrnodes = ((float)nrcrl) / nodesperblock;
	const int_twins nrblocks( mNINT32(finlnrnodes), mNINT32(fcrlnrnodes) );
	levelnrblocks += nrblocks;
	const Coord blockdist( fullcrldist/nrblocks.first(),
				fullinldist/nrblocks.second() );
	const double avglvlblcksz = (blockdist.x_ + blockdist.y_) * .5;
	astrm.put( IOPar::compKey(sKey::Level(),nrlevels), avglvlblcksz );
	nodesperblock /= 2; nrlevels++;
    }
    astrm.newParagraph();

    for ( int ilvl=0; ilvl<nrlevels; ilvl++ )
    {
	writeLevelSummary( astrm.stream(), data, levelnrblocks[ilvl] );
	astrm.newParagraph();
	if ( !astrm.isOK() )
	    return false;
    }

    return true;
}


void Seis::Blocks::Writer::writeLevelSummary( od_ostream& strm,
				const Array2D<float>& data,
				int_twins nrblocks ) const
{
    mGetNrInlCrlAndRg();
    const float_twins fcellsz( ((float)nrinl) / nrblocks.first(),
			     ((float)nrcrl) / nrblocks.second() );
    const float_twins fhcellsz( fcellsz.first() * .5f, fcellsz.second() * .5f );
    const int_twins cellsz( mNINT32(fcellsz.first()),
			    mNINT32(fcellsz.second()) );
    const int_twins hcellsz( mNINT32(fhcellsz.first()),
			    mNINT32(fhcellsz.second()) );

    for ( int cix=0; ; cix++ )
    {
	const float fcenterx = fhcellsz.first() + cix * fcellsz.first();
	int_twins center( mNINT32(fcenterx), 0 );
	if ( center.first() >= nrinl )
	    break;

	for ( int ciy=0; ; ciy++ )
	{
	    const float fcentery = fhcellsz.second() + ciy * fcellsz.second();
	    center.second() = mNINT32( fcentery );
	    if ( center.second() > nrcrl )
		break;

	    float sumv = 0.f; int nrv = 0;
	    for ( int ix=center.first()-hcellsz.first();
		      ix<=center.first()+hcellsz.first(); ix++ )
	    {
		if ( ix < 0 )
		    continue;
		if ( ix >= nrinl )
		    break;
		for ( int iy=center.second()-hcellsz.second();
			  iy<=center.second()+hcellsz.second(); iy++ )
		{
		    if ( iy < 0 )
			continue;
		    if ( iy >= nrcrl )
			break;
		    const float v = data.get( ix, iy );
		    if ( !mIsUdf(v) )
			{ sumv += v; nrv++; }
		}
	    }
	    if ( nrv > 0 )
	    {
		const BinID cbid( finalinlrg_.atIndex(center.first(),stepinl),
				  finalcrlrg_.atIndex(center.second(),stepcrl));
		const Coord centercoord( SI().transform(cbid) );
		strm << centercoord.x_ << '\t' << centercoord.y_ << '\t'
				<< sumv / nrv << '\n';
	    }
	}
    }
}


void Seis::Blocks::Writer::scanPositions(
	Interval<idx_type>& globinlrg, Interval<idx_type>& globcrlrg,
	Interval<double>& xrg, Interval<double>& yrg )
{
    Pos::IdxPairDataSet sortedpositions( 0, false );
    Pos::IdxPairDataSet::SPos spos;
    bool first = true;
    while ( columns_.next(spos) )
    {
	const MemBlockColumn& column = *(MemBlockColumn*)columns_.getObj(spos);
	if ( column.nruniquevisits_ < 1 )
	    continue;

	const HGlobIdx& globidx = column.globIdx();
	if ( first )
	{
	    globinlrg.start = globinlrg.stop = globidx.inl();
	    globcrlrg.start = globcrlrg.stop = globidx.crl();
	}
	else
	{
	    globinlrg.include( globidx.inl(), false );
	    globcrlrg.include( globidx.crl(), false );
	}

	for ( idx_type iinl=0; iinl<dims_.inl(); iinl++ )
	{
	    for ( idx_type icrl=0; icrl<dims_.crl(); icrl++ )
	    {
		if ( !column.visited_[iinl][icrl] )
		    continue;
		const int inl = Block::inl4Idxs( hgeom_, dims_.inl(),
						 globidx.inl(), iinl );
		const int crl = Block::crl4Idxs( hgeom_, dims_.crl(),
						 globidx.crl(), icrl );
		const Coord coord = SI().transform( BinID(inl,crl) );
		if ( first )
		{
		    finalinlrg_.start = finalinlrg_.stop = inl;
		    finalcrlrg_.start = finalcrlrg_.stop = crl;
		    xrg.start = xrg.stop = coord.x_;
		    yrg.start = yrg.stop = coord.y_;
		}
		else
		{
		    finalinlrg_.include( inl, false );
		    finalcrlrg_.include( crl, false );
		    xrg.include( coord.x_, false );
		    yrg.include( coord.y_, false );
		}

		sortedpositions.add( BinID(inl,crl) );
	    }
	}

	first = false;
    }

    PosInfo::LineCollDataFiller lcdf( cubedata_ );
    spos.reset();
    while ( sortedpositions.next(spos) )
    {
	const Pos::IdxPair ip( sortedpositions.getIdxPair( spos ) );
	lcdf.add( BinID(ip.inl(),ip.crl()) );
    }
}


namespace Seis
{
namespace Blocks
{

class WriterFinisher : public Executor
{ mODTextTranslationClass(Seis::Blocks::WriterFinisher)
public:

WriterFinisher( Writer& wrr )
    : Executor("Seis Blocks Writer finisher")
    , wrr_(wrr)
    , colidx_(0)
{
    if ( wrr_.stepfinder_ )
    {
	wrr_.stepfinder_->finish( uirv_ );
	if ( uirv_.isError() )
	    return;
    }

    Pos::IdxPairDataSet::SPos spos;
    while ( wrr_.columns_.next(spos) )
    {
	MemBlockColumn* column = (MemBlockColumn*)wrr_.columns_.getObj( spos );
	if ( column->nruniquevisits_ < 1 )
	    column->retire();
	else if ( !column->isRetired() )
	    towrite_ += column;
    }
}

virtual od_int64 nrDone() const
{
    return colidx_;
}

virtual od_int64 totalNr() const
{
    return towrite_.size();
}

virtual uiString nrDoneText() const
{
   return tr("Column files written");
}

virtual uiString message() const
{
    if ( uirv_.isOK() )
       return tr("Writing edge blocks");
    return uirv_;
}

virtual int nextStep()
{
    if ( uirv_.isError() )
	return ErrorOccurred();

    if ( !towrite_.validIdx(colidx_) )
	return wrapUp();

    wrr_.writeColumn( *towrite_[colidx_], uirv_ );
    if ( uirv_.isError() )
	return ErrorOccurred();

    colidx_++;
    return MoreToDo();
}

int wrapUp()
{
    wrr_.writeInfoFiles( uirv_ );
    if ( wrr_.backend_ )
    {
	wrr_.backend_->close( uirv_ );
	delete wrr_.backend_; wrr_.backend_ = 0;
    }
    wrr_.isfinished_ = true;
    return uirv_.isOK() ? Finished() : ErrorOccurred();
}

    int			colidx_;
    uiRetVal		uirv_;
    Writer&		wrr_;
    ObjectSet<MemBlockColumn> towrite_;

};

} // namespace Blocks
} // namespace Seis


Task* Seis::Blocks::Writer::finisher()
{
    return new WriterFinisher( *this );
}

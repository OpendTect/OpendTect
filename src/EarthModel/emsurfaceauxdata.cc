/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "emsurfaceauxdata.h"

#include "arrayndimpl.h"
#include "binidsurface.h"
#include "binidvalset.h"
#include "datacoldef.h"
#include "datapointset.h"
#include "emhorizon3d.h"
#include "emsurfacegeometry.h"
#include "emsurfacetr.h"
#include "emsurfauxdataio.h"
#include "file.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "paralleltask.h"
#include "posfilter.h"
#include "posvecdataset.h"
#include "ptrman.h"
#include "survinfo.h"
#include "trckey.h"
#include "trckeyzsampling.h"
#include "uistrings.h"
#include "varlenarray.h"

namespace EM
{

SurfaceAuxData::SurfaceAuxData( Horizon3D& horizon )
    : horizon_(horizon)
    , changed_(false)
{
    auxdatanames_.allowNull(true);
    auxdatainfo_.allowNull(true);
    auxdata_.allowNull(true);
}


SurfaceAuxData::~SurfaceAuxData()
{
    removeAll();
}


void SurfaceAuxData::removeAll()
{
    auxdatanames_.setEmpty();
    auxdatainfo_.setEmpty();
    auxdatashift_.erase();
    auxdatatypes_.erase();

    deepErase( auxdata_ );
    changed_ = true;
}


bool SurfaceAuxData::validIdx( int idx ) const
{
    return auxdatanames_.validIdx( idx );
}


int SurfaceAuxData::nrAuxData() const
{ return auxdatanames_.size(); }


const char* SurfaceAuxData::auxDataName( int dataidx ) const
{
    if ( nrAuxData() && auxdatanames_[dataidx] )
	return auxdatanames_[dataidx]->buf();

    return nullptr;
}


void SurfaceAuxData::setAuxDataType( int dataidx, AuxDataType type )
{
    if ( auxdatatypes_.validIdx(dataidx) )
	auxdatatypes_[dataidx] = type;
}


SurfaceAuxData::AuxDataType SurfaceAuxData::getAuxDataType( int dataidx ) const
{
    return auxdatatypes_.validIdx(dataidx) ? auxdatatypes_[dataidx] : NoType;
}


float SurfaceAuxData::auxDataShift( int dataidx ) const
{ return auxdatashift_[dataidx]; }


void SurfaceAuxData::setAuxDataName( int dataidx, const char* name )
{
    if ( auxdatanames_[dataidx] )
	auxdatanames_.replace( dataidx, new BufferString(name) );
}


void SurfaceAuxData::setAuxDataShift( int dataidx, float shift )
{
    if ( auxdatanames_[dataidx] )
	auxdatashift_[dataidx] = shift;
}


bool SurfaceAuxData::hasAuxDataName( const char* nm ) const
{ return auxDataIndex(nm) >= 0; }


int SurfaceAuxData::auxDataIndex( const char* nm ) const
{
    for ( int idx=0; idx<auxdatanames_.size(); idx++ )
	if ( auxdatanames_[idx] && auxdatanames_.get(idx) == nm )
	    return idx;

    return -1;
}


int SurfaceAuxData::addAuxData( const char* name )
{
    auxdatanames_.add( name );
    auxdatashift_ += 0.0;
    auxdatatypes_ += NoType;

    for ( int idx=0; idx<auxdata_.size(); idx++ )
    {
	if ( auxdata_[idx] )
	    auxdata_[idx]->setNrVals( nrAuxData(), true );
    }

    changed_ = true;
    return auxdatanames_.size()-1;
}


void SurfaceAuxData::addAuxData( const BufferStringSet& name,
						const BinIDValueSet& data )
{
    for ( const auto* nm : name )
	addAuxData( nm->buf() );

    if ( !auxdata_.isEmpty() )
    {
	const int idx = auxdata_.size()-1;
	auxdata_.replace( idx, new BinIDValueSet(data) );
    }
}


void SurfaceAuxData::removeAuxData( int dataidx )
{
    auxdatanames_.replace( dataidx, "" );
    auxdatashift_[dataidx] = 0.0;
    auxdatatypes_[dataidx] = NoType;

    for ( int idx=0; idx<auxdata_.size(); idx++ )
    {
	if ( auxdata_[idx] )
	    auxdata_[idx]->removeVal( dataidx );
    }

    changed_ = true;
}


float SurfaceAuxData::getAuxDataVal( int dataidx, const PosID& posid ) const
{
    return getAuxDataVal( dataidx, posid.getRowCol() );
}


float SurfaceAuxData::getAuxDataVal( int dataidx, const TrcKey& tk ) const
{
    return getAuxDataVal( dataidx, tk.position() );
}


float SurfaceAuxData::getAuxDataVal( int dataidx, const BinID& bid ) const
{
    if ( !auxdatanames_.validIdx(dataidx) ||
	 !auxdata_.validIdx(0) || !auxdata_[0] )
	return mUdf(float);

    if ( mIsUdf(horizon_.getZ(bid)) )
	return mUdf(float);

    const BinIDValueSet::SPos pos = auxdata_[0]->find( bid );
    if ( !pos.isValid() )
	return mUdf(float);

    return auxdata_[0]->getVals( pos )[dataidx];
}


void SurfaceAuxData::setAuxDataVal( int dataidx, const PosID& posid, float val )
{
    setAuxDataVal( dataidx, posid, val, false );
}


void SurfaceAuxData::setAuxDataVal( int dataidx, const PosID& posid, float val,
				    bool onlynewpos )
{
    const TrcKey tk = TrcKey( BinID::fromInt64( posid.subID() ) );
    if ( !auxdatanames_.validIdx(dataidx) ||
	tk.isUdf() ||
	horizon_.isNodeLocked(tk) )
	return;

    const int sectionidx = horizon_.sectionIndex( posid.sectionID() );
    if ( sectionidx < 0 )
	return;

    if ( !auxdata_.validIdx(sectionidx) )
    {
	for ( int idx=auxdata_.size(); idx<horizon_.nrSections(); idx++ )
	    auxdata_ += nullptr;
    }

    if ( !auxdata_[sectionidx] )
	auxdata_.replace( sectionidx, new BinIDValueSet( nrAuxData(), false ) );

    if ( dataidx >= auxdata_[sectionidx]->nrVals() )
	auxdata_[sectionidx]->setNrVals( dataidx + 1 );

    const BinID geomrc( posid.getRowCol() );
    if ( geomrc.isUdf() )
	return;

    BinIDValueSet::SPos pos = auxdata_[sectionidx]->find( geomrc );
    if ( !pos.isValid() )
    {
	pos = auxdata_[sectionidx]->add( geomrc );
	auxdata_[sectionidx]->getVals( pos )[dataidx] = val;
    }
    else if ( !onlynewpos )
	auxdata_[sectionidx]->getVals( pos )[dataidx] = val;

    changed_ = true;
}


void SurfaceAuxData::setAuxDataVal( int dataidx, const BinID& bid, float val )
{
    if ( !auxdatanames_.validIdx(dataidx) )
	return;

    if ( auxdata_.isEmpty() )
	auxdata_.add( new BinIDValueSet(nrAuxData(),false) );

    BinIDValueSet::SPos pos = auxdata_[0]->find( bid );
    if ( !pos.isValid() )
	pos = auxdata_[0]->add( bid );

    auxdata_[0]->getVals( pos )[dataidx] = val;
    changed_ = true;
}


void SurfaceAuxData::setAuxDataVal( int dataidx, const TrcKey& tk, float val )
{
    setAuxDataVal( dataidx, tk.position(), val );
}


bool SurfaceAuxData::isChanged( int ) const
{ return changed_; }


void SurfaceAuxData::resetChangedFlag()
{
    changed_ = false;
}


static EMSurfaceTranslator* getTranslator( Horizon& horizon )
{
    PtrMan<IOObj> ioobj = IOM().get( horizon.multiID() );
    if ( !ioobj )
    {
	horizon.setErrMsg( uiStrings::sCantFindSurf() );
	return nullptr;
    }

    EMSurfaceTranslator* transl =
			(EMSurfaceTranslator*)ioobj->createTranslator();
    if ( !transl || !transl->startRead(*ioobj) )
    {
	horizon.setErrMsg( transl ? transl->errMsg()
				  : ::toUiString("Cannot find Translator") );
	delete transl;
	return nullptr;
    }

    return transl;
}


Task* SurfaceAuxData::auxDataLoader( int selidx )
{
    PtrMan<EMSurfaceTranslator> transl = getTranslator( horizon_ );
    if ( !transl )
	return nullptr;

    SurfaceIODataSelection& sel = transl->selections();
    const int nrauxdata = sel.sd.valnames.size();
    if ( nrauxdata==0 || selidx >= nrauxdata )
	return nullptr;

    return transl->getAuxdataReader( horizon_, selidx );
}


Task* SurfaceAuxData::auxDataLoader( const char* nm )
{
    PtrMan<EMSurfaceTranslator> transl = getTranslator( horizon_ );
    if ( !transl )
	return nullptr;

    SurfaceIODataSelection& sel = transl->selections();
    const int selidx = sel.sd.valnames.indexOf( nm );
    if ( !sel.sd.valnames.validIdx(selidx) )
	return nullptr;

    return transl->getAuxdataReader( horizon_, selidx );
}


Task* SurfaceAuxData::auxDataLoader( const TypeSet<int>& idxs )
{
    PtrMan<EMSurfaceTranslator> transl = getTranslator( horizon_ );
    if ( !transl )
	return nullptr;

    PtrMan<IOObj> ioobj = IOM().get( horizon_.multiID() );
    if ( !ioobj )
	return nullptr;

    const SurfaceIODataSelection& sel = transl->selections();
    const BufferString expr = ioobj->fullUserExpr( true );
    BufferStringSet fnms;
    for ( int idx=0; idx<idxs.size(); idx++ )
    {
	const int selidx = idxs[idx];
	if ( !sel.sd.valnames.validIdx(selidx) )
	    continue;

	const BufferString fnm = findAuxDataFile( expr.buf(),
					sel.sd.valnames.get(selidx).buf() );
	if ( !fnm.isEmpty() )
	    fnms.add( fnm );
    }

    if ( fnms.isEmpty() )
	return nullptr;

    return createAuxDataTask( horizon_, fnms );
}


BufferString SurfaceAuxData::getFreeFileName( const IOObj& ioobj )
{
    const BufferString basefnm( ioobj.fullUserExpr(true) );
    if ( basefnm.isEmpty() )
	return BufferString::empty();

    const int maxnrfiles = 100000; // just a big number to make this loop end
    for ( int idx=0; idx<maxnrfiles; idx++ )
    {
	BufferString fnm =
	    dgbSurfDataWriter::createHovName( basefnm.buf(), idx );
	if ( !File::exists(fnm.buf()) )
	    return fnm;
    }

    return BufferString::empty();
}


Task* SurfaceAuxData::auxDataSaver( int dataidx, bool overwrite )
{
    PtrMan<IOObj> ioobj = IOM().get( horizon_.multiID() );
    if ( !ioobj )
    {
	horizon_.setErrMsg( uiStrings::sCantFindSurf() );
	return nullptr;
    }

    PtrMan<EMSurfaceTranslator> transl =
		sCast(EMSurfaceTranslator*,ioobj->createTranslator());
    if ( transl && transl->startWrite(horizon_) )
    {
	PtrMan<Task> exec = transl->writer( *ioobj, false );
	if ( exec )
	    return transl->getAuxdataWriter( horizon_, dataidx, overwrite );
    }

    horizon_.setErrMsg(
	transl ? transl->errMsg() : uiStrings::phrCannotFind(tr("Translator")));
    return nullptr;
}


void SurfaceAuxData::removeSection( const SectionID& sectionid )
{
    const int sectionidx = horizon_.sectionIndex( sectionid );
    if ( !auxdata_.validIdx( sectionidx ) )
	return;

    delete auxdata_.removeSingle( sectionidx );
}


bool SurfaceAuxData::hasAttribute( const IOObj& ioobj, const char* attrnm )
{
    return !getFileName(ioobj,attrnm).isEmpty();
}


BufferString SurfaceAuxData::getFileName( const IOObj& ioobj,
					  const char* attrnm )
{ return getFileName( ioobj.fullUserExpr(true), attrnm ); }


BufferString SurfaceAuxData::getFileName( const char* fulluserexp,
					  const char* attrnmptr)
{
    StringView attrnm( attrnmptr );
    const BufferString basefnm( fulluserexp );
    BufferString fnm; int gap = 0;
    for ( int idx=0; ; idx++ )
    {
	if ( gap > 100 ) return "";

	fnm = EM::dgbSurfDataWriter::createHovName(basefnm,idx);
	if ( File::isEmpty(fnm.buf()) )
	    { gap++; continue; }

	EM::dgbSurfDataReader rdr( fnm.buf() );
	if ( attrnm == rdr.dataName() )
	    break;
    }

    return fnm;
}


bool SurfaceAuxData::removeFile( const IOObj& ioobj, const char* attrnm )
{
    const BufferString fnm = getFileName( ioobj, attrnm );
    return !fnm.isEmpty() ? File::remove( fnm ) : false;
}


BufferString SurfaceAuxData::getFileName( const char* attrnm ) const
{
    PtrMan<IOObj> ioobj = IOM().get( horizon_.multiID() );
    return ioobj ? SurfaceAuxData::getFileName( *ioobj, attrnm )
		 : BufferString::empty();
}


bool SurfaceAuxData::removeFile( const char* attrnm ) const
{
    PtrMan<IOObj> ioobj = IOM().get( horizon_.multiID() );
    return ioobj ? SurfaceAuxData::removeFile( *ioobj, attrnm ) : false;
}


Array2D<float>* SurfaceAuxData::createArray2D( int dataidx ) const
{
    if ( horizon_.geometry().geometryElement()->isEmpty() )
	return nullptr;

    const StepInterval<int> rowrg = horizon_.geometry().rowRange();
    const StepInterval<int> colrg = horizon_.geometry().colRange( -1 );

    PosID posid( horizon_.id() );
    Array2DImpl<float>* arr =
	new Array2DImpl<float>( rowrg.nrSteps()+1, colrg.nrSteps()+1 );
    for ( int row=rowrg.start_; row<=rowrg.stop_; row+=rowrg.step_ )
    {
	for ( int col=colrg.start_; col<=colrg.stop_; col+=colrg.step_ )
	{
	    posid.setSubID( RowCol(row,col).toInt64() );
	    const float val = getAuxDataVal( dataidx, posid );
	    arr->set( rowrg.getIndex(row), colrg.getIndex(col), val );
	}
    }

    return arr;
}


void SurfaceAuxData::init( int dataidx, float val )
{ init( dataidx, false, val ); }


void SurfaceAuxData::init( int dataidx, bool onlynewpos, float val )
{
    const Geometry::RowColSurface* rcgeom =
	horizon_.geometry().geometryElement();
    if ( !rcgeom || rcgeom->isEmpty() )
	return;

    const StepInterval<int> rowrg = rcgeom->rowRange();
    const StepInterval<int> colrg = rcgeom->colRange();
    PosID posid( horizon_.id() );
    for ( int row=rowrg.start_; row<=rowrg.stop_; row+=rowrg.step_ )
    {
	for ( int col=colrg.start_; col<=colrg.stop_; col+=colrg.step_ )
	{
	    posid.setSubID( RowCol(row,col).toInt64() );
	    if ( dataidx<0 )
	    {
		for ( int aidx=0; aidx<nrAuxData(); aidx++ )
		    setAuxDataVal( aidx, posid, val, onlynewpos );
	    }
	    else
		setAuxDataVal( dataidx, posid, val, onlynewpos );
	}
    }
}


void SurfaceAuxData::setArray2D( int dataidx,
				 const Array2D<float>& arr2d,
				 const TrcKeySampling* arrtks )
{
    TrcKeySampling tks;
    if ( arrtks )
	tks = *arrtks;
    else
    {
	const Geometry::RowColSurface* rcgeom =
		horizon_.geometry().geometryElement();
	if ( !rcgeom || rcgeom->isEmpty() )
	    return;

	tks.set( rcgeom->rowRange(), rcgeom->colRange() );
    }

    PosID posid( horizon_.id() );
    for ( od_int64 gidx=0; gidx<tks.totalNr(); gidx++ )
    {
	const TrcKey tk = tks.trcKeyAt( gidx );
	float val = mUdf(float);
	if ( arr2d.getData() )
	    val = arr2d.getData()[gidx];
	else
	    val = arr2d.get( tks.inlIdx(tk.inl()), tks.crlIdx(tk.crl()) );

	posid.setSubID( tk.binID().toInt64() );
	setAuxDataVal( dataidx, posid, val );
    }
}


bool SurfaceAuxData::setArray2Ds( const TypeSet<int>& dataidxs,
				  const ObjectSet<Array2D<float>>& arrays )
{
    if ( dataidxs.size() != arrays.size() || arrays.isEmpty() )
	return arrays.isEmpty();

    const Geometry::RowColSurface* rcgeom =
				horizon_.geometry().geometryElement();
    if ( !rcgeom || rcgeom->isEmpty() )
	return false;

    const StepInterval<int> rowrg = rcgeom->rowRange();
    const StepInterval<int> colrg = rcgeom->colRange();
    const int nrows = rowrg.nrSteps() + 1;
    const int nrcols = colrg.nrSteps() + 1;
    if ( rowrg.step_<=0 || colrg.step_<=0 || nrows<=0 || nrcols<=0 )
	return false;

    mAllocLargeVarLenArr( const float*, ptrs, arrays.size() );
    mAllocLargeVarLenArr( int, validxs, arrays.size() );
    if ( !(mIsVarLenArrOK(ptrs)) || !(mIsVarLenArrOK(validxs)) )
	return false;

    const float** ptrsarr = mVarLenArr(ptrs);
    int* validxsarr = mVarLenArr(validxs);
    int nuse = 0;
    for ( int idx=0; idx<arrays.size(); idx++ )
    {
	const Array2D<float>* arr = arrays.get( idx );
	if ( !arr || !arr->getData() || arr->getSize(0)!=nrows ||
	     arr->getSize(1)!=nrcols )
	    return false;

	if ( !dataidxs.validIdx(idx) || dataidxs[idx]<0 ||
	     dataidxs[idx]>=nrAuxData() )
	    continue;

	ptrsarr[nuse] = arr->getData();
	validxsarr[nuse] = dataidxs[idx];
	nuse++;
    }

    if ( nuse == 0 )
	return true;

    const char* skip = nullptr;
    ArrPtrMan<char> mask;
    const Array2D<char>* locked = horizon_.getLockedNodes();
    if ( locked && locked->getData() )
    {
	const char* ldata = locked->getData();
	const od_int64 lsz = locked->info().getTotalSz();
	bool anylocked = false;
	for ( od_int64 idx=0; idx<lsz; idx++ )
	{
	    if ( ldata[idx] == '1' )
	    {
		anylocked = true;
		break;
	    }
	}

	if ( anylocked && locked->getSize(0)==nrows &&
	     locked->getSize(1)==nrcols &&
	     horizon_.getTrackingSampling().inlRange()==rowrg &&
	     horizon_.getTrackingSampling().crlRange()==colrg )
	    skip = ldata;
	else if ( anylocked )
	{
	    const od_int64 total = (od_int64)nrows * nrcols;
	    mTryAllocPtrMan( mask, char[total] );
	    if ( !mask.ptr() )
		return false;

	    OD::memZero( mask.ptr(), total );
	    for ( int irow=0; irow<nrows; irow++ )
	    {
		const int inl = rowrg.atIndex( irow );
		for ( int icol=0; icol<nrcols; icol++ )
		{
		    const BinID bid( inl, colrg.atIndex(icol) );
		    if ( horizon_.isNodeLocked(TrcKey(bid)) )
			mask[(od_int64)irow*nrcols + icol] = '1';
		}
	    }

	    skip = mask.ptr();
	}
    }

    if ( !auxdata_.validIdx(0) )
	auxdata_ += nullptr;

    if ( !auxdata_[0] )
	auxdata_.replace( 0, new BinIDValueSet(nrAuxData(),false) );
    else if ( auxdata_[0]->nrVals() < nrAuxData() )
	auxdata_[0]->setNrVals( nrAuxData(), true );

    if ( !auxdata_[0] ||
	 !auxdata_[0]->setGridValues(rowrg,colrg,validxsarr,ptrsarr,
				     nuse,skip) )
	return false;

    changed_ = true;
    return true;
}


bool SurfaceAuxData::usePar( const IOPar& )
{
    return true;
}


void SurfaceAuxData::fillPar( IOPar& ) const
{}


void SurfaceAuxData::applyPosFilter( const Pos::Filter& pf, int dataidx )
{
    for ( int sidx=0; sidx<auxdata_.size(); sidx++ )
    {
	BinIDValueSet* bvs = auxdata_[sidx];
	if ( !bvs || bvs->nrVals()==0 )
	    continue;

	BinIDValueSet::SPos spos;
	BinID bid; float zval;
	while ( bvs->next(spos) )
	{
	    bid = bvs->getBinID( spos );
	    zval = horizon_.getZ( bid );
	    if ( pf.includes(SI().transform(bid),zval) )
		continue;

	    float* vals = bvs->getVals( spos );
	    for ( int vidx=0; vidx<bvs->nrVals(); vidx++ )
	    {
		if ( dataidx==-1 || vidx==dataidx )
		    vals[vidx] = mUdf(float);
	    }
	}
    }
}


class AuxDataPointSetTask : public ParallelTask
{ mODTextTranslationClass(AuxDataPointSetTask);
public:
AuxDataPointSetTask( const SurfaceAuxData& aux, DataPointSet& dps,
		     TypeSet<float>* shifts, const TrcKeyZSampling* cs,
		     const TypeSet<int>* auxidxs )
    : ParallelTask("Horizon data datapack filler")
    , aux_(aux)
    , dps_(dps)
    , geom_(aux.horizon_.geometry().geometryElement())
{
    dps_.dataSet().add( new DataColDef("Section ID") );
    for ( int idx=0; idx<aux_.nrAuxData(); idx++ )
    {
	if ( auxidxs && !auxidxs->isPresent(idx) )
	    continue;

	const char* nm = aux_.auxDataName( idx );
	if ( !nm )
	    continue;

	if ( shifts )
	    *shifts += aux_.auxDataShift( idx );

	auxidxs_ += idx;
	dps_.dataSet().add( new DataColDef(nm) );
    }

    sectionval_ = (float)SectionID::def().asInt();
    if ( cs )
    {
	hassamp_ = true;
	samp_ = *cs;
    }

    if ( geom_ && !geom_->isEmpty() )
	rowrg_ = geom_->rowRange();

    if ( hassamp_ && geom_ )
	rowrg_.limitTo( samp_.hsamp_.inlRange() );

    if ( !geom_ || geom_->isEmpty() || mIsUdf(rowrg_.start_) ||
	 mIsUdf(rowrg_.stop_) || rowrg_.step_<=0 ||
	 rowrg_.start_>rowrg_.stop_ )
	nrows_ = 0;
    else
	nrows_ = rowrg_.nrSteps() + 1;

    if ( nrows_ == 0 )
	dps_.dataChanged();

    msg_ = tr("Transferring horizon data");
}

~AuxDataPointSetTask()
{
    releaseLines();
}

uiString uiMessage() const override
{
    return msg_;
}

uiString uiNrDoneText() const override
{
    return tr("Gridlines");
}

od_int64 nrIterations() const override
{
    return prepok_ ? nrows_ : 0;
}

int minThreadSize() const override
{
    return 4;
}

bool executeParallel( bool parallel ) override
{
    return prepok_ && ParallelTask::executeParallel(parallel);
}

private:

struct PosLine
{
    TypeSet<int>	crls;
    TypeSet<float>	vals;
};

bool keep( const BinID& bid ) const
{
    if ( !hassamp_ )
	return true;

    if ( !samp_.hsamp_.includes(bid) )
	return false;

    const BinID diff = bid - samp_.hsamp_.start_;
    const int inlstep = samp_.hsamp_.step_.inl();
    const int crlstep = samp_.hsamp_.step_.crl();
    if ( (inlstep && diff.inl()%inlstep) || (crlstep && diff.crl()%crlstep) )
	return false;

    return true;
}

void releaseLines()
{
    delete [] auxlines_;
    auxlines_ = nullptr;
    delete [] rows_;
    rows_ = nullptr;
}

bool doPrepare( int ) override
{
    releaseLines();
    if ( nrows_ <= 0 )
	return true;

    mTryAlloc( auxlines_, PosLine[nrows_] );
    mTryAlloc( rows_, PosLine[nrows_] );
    if ( !auxlines_ || !rows_ )
    {
	releaseLines();
	msg_ = ::toUiString("Not enough memory to extract horizon data");
	return false;
    }

    const BinIDValueSet* src = aux_.auxdata_.validIdx(0)
			     ? aux_.auxdata_[0] : nullptr;
    if ( !src )
	return true;

    const int naux = auxidxs_.size();
    const int nrvals = src->nrVals();
    BinIDValueSet::SPos pos;
    while ( src->next(pos) )
    {
	const BinID bid = src->getBinID( pos );
	if ( !rowrg_.includes(bid.inl(),false) ||
	     (bid.inl()-rowrg_.start_)%rowrg_.step_ )
	    continue;

	const int irow = rowrg_.getIndex( bid.inl() );
	if ( irow<0 || irow>=nrows_ )
	    continue;

	PosLine& line = auxlines_[irow];
	line.crls += bid.crl();
	const float* srcvals = src->getVals( pos );
	for ( int ia=0; ia<naux; ia++ )
	{
	    const int col = auxidxs_[ia];
	    line.vals += col>=0 && col<nrvals ? srcvals[col] : mUdf(float);
	}
    }

    return true;
}

bool doWork( od_int64 start, od_int64 stop, int ) override
{
    const int naux = auxidxs_.size();
    for ( int irow=mCast(int,start); irow<=stop; irow++ )
    {
	if ( !shouldContinue() )
	    return false;

	PosLine& out = rows_[irow];
	out.crls.setEmpty();
	out.vals.setEmpty();
	const int inl = rowrg_.atIndex( irow );
	StepInterval<int> colrg = geom_->colRange( inl );
	if ( hassamp_ )
	    colrg.limitTo( samp_.hsamp_.crlRange() );
	if ( mIsUdf(colrg.start_) || mIsUdf(colrg.stop_) ||
	     colrg.step_<=0 || colrg.start_>colrg.stop_ )
	{
	    addToNrDone( 1 );
	    continue;
	}

	const PosLine& aux = auxlines_[irow];
	int iaux = 0;
	const int nauxpos = aux.crls.size();
	for ( int crl=colrg.start_; crl<=colrg.stop_; crl+=colrg.step_ )
	{
	    const BinID bid( inl, crl );
	    if ( !keep(bid) )
		continue;

	    const float z = geom_->getZ( bid );
	    if ( mIsUdf(z) )
		continue;

	    while ( iaux<nauxpos && aux.crls[iaux]<crl )
		iaux++;

	    const bool hasaux = iaux<nauxpos && aux.crls[iaux]==crl;
	    out.crls += crl;
	    out.vals += z;
	    out.vals += sectionval_;
	    for ( int ia=0; ia<naux; ia++ )
		out.vals += hasaux ? aux.vals[iaux*naux+ia] : mUdf(float);
	    if ( hasaux )
		iaux++;
	}

	addToNrDone( 1 );
    }

    return true;
}

bool doFinish( bool success ) override
{
    bool res = success;
    if ( res && rows_ )
    {
	dps_.clearData();
	dps_.bivSet().allowDuplicateBinIDs( false );
	const int npack = auxidxs_.size() + 2;
	for ( int irow=0; irow<nrows_; irow++ )
	{
	    const PosLine& row = rows_[irow];
	    if ( row.crls.isEmpty() )
		continue;
	    if ( row.vals.size()!=row.crls.size()*npack ||
		 !dps_.bivSet().appendSortedLine(rowrg_.atIndex(irow),
						 row.crls.arr(),
						 row.vals.arr(),
						 row.crls.size()) )
	    {
		res = false;
		break;
	    }
	}

	dps_.dataChanged();
    }

    releaseLines();
    if ( !res && msg_.isEmpty() )
	msg_ = tr("Cannot extract horizon data");

    return res;
}

const SurfaceAuxData&		aux_;
DataPointSet&			dps_;
const Geometry::BinIDSurface*	geom_;
TrcKeyZSampling			samp_;
StepInterval<int>		rowrg_;
TypeSet<int>			auxidxs_;
PosLine*			auxlines_	= nullptr;
PosLine*			rows_		= nullptr;
float				sectionval_	= 0.f;
int				nrows_		= 0;
bool				hassamp_	= false;
bool				prepok_		= true;
uiString			msg_;

};

Task* SurfaceAuxData::createDataPointSetTask( DataPointSet& dps,
		TypeSet<float>* shifts, const TrcKeyZSampling* cs,
		const TypeSet<int>* auxidxs ) const
{
    return new AuxDataPointSetTask( *this, dps, shifts, cs, auxidxs );
}

} // namespace EM

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Apr 2017
________________________________________________________________________

-*/

#include "seisblockswriter.h"
#include "seisblocksdata.h"
#include "posidxpairdataset.h"
#include "paralleltask.h"
#include "uistrings.h"
#include "scaler.h"
#include "file.h"
#include "keystrs.h"
#include "survgeom3d.h"
#include "od_ostream.h"

static const unsigned short cVersion = 1;
static const unsigned short cHdrSz = 128;

Seis::Blocks::DataStorage::DataStorage( const SurvGeom* geom )
    : survgeom_(*(geom ? geom : static_cast<const SurvGeom*>(
				    &SurvGeom::default3D())))
    , dims_(Data::defDims())
    , version_(cVersion)
{
}


BufferString Seis::Blocks::DataStorage::fileNameFor( const GlobIdx& globidx )
{
    BufferString ret;
    ret.add( globidx.inl() ).add( '_' )
	.add( globidx.crl() ).add( '_' )
	.add( globidx.z() ).add( ".bin" );
    return ret;
}


Seis::Blocks::Writer::Writer( const SurvGeom* geom )
    : DataStorage(geom)
    , scaler_(0)
    , component_(0)
    , specfprep_(OD::AutoFPRep)
    , usefprep_(OD::F32)
    , needreset_(true)
    , blocks_(*new Pos::IdxPairDataSet(sizeof(Block*),false,false))
    , nrposperblock_(((int)dims_.inl()) * dims_.crl())
    , writecomplete_(false)
{
}


Seis::Blocks::Writer::~Writer()
{
    Task* task = finisher();
    if ( task )
    {
	task->execute();
	delete task;
    }

    delete scaler_;
    setEmpty();
    delete &blocks_;
}


void Seis::Blocks::Writer::setEmpty()
{
    Pos::IdxPairDataSet::SPos spos;
    while ( blocks_.next(spos) )
	delete (Block*)blocks_.getObj( spos );
    blocks_.setEmpty();
}


void Seis::Blocks::Writer::setBasePath( const File::Path& fp )
{
    if ( fp.fullPath() != basepath_.fullPath() )
    {
	basepath_ = fp;
	needreset_ = true;
    }
}


void Seis::Blocks::Writer::setFileNameBase( const char* nm )
{
    if ( filenamebase_ != nm )
    {
	filenamebase_ = nm;
	needreset_ = true;
    }
}


void Seis::Blocks::Writer::setFPRep( OD::FPDataRepType rep )
{
    if ( specfprep_ != rep )
    {
	specfprep_ = rep;
	needreset_ = true;
    }
}


void Seis::Blocks::Writer::setScaler( const LinScaler* newscaler )
{
    if ( (!scaler_ && !newscaler) )
	return;

    delete scaler_;
    scaler_ = newscaler ? newscaler->clone() : 0;
    needreset_ = true;
}


void Seis::Blocks::Writer::setComponent( int icomp )
{
    if ( component_ != icomp )
    {
	component_ = icomp;
	needreset_ = true;
    }
}


void Seis::Blocks::Writer::resetZ( const Interval<float>& zrg )
{
    globzidxrg_.start = Data::globIdx4Z( survgeom_, zrg.start, dims_.z() );
    globzidxrg_.stop = Data::globIdx4Z( survgeom_, zrg.stop, dims_.z() );
    const float eps = Seis::cDefSampleSnapDist();
    deepErase( zevalinfos_ );

    for ( IdxType globzidx=globzidxrg_.start; globzidx<=globzidxrg_.stop;
		globzidx++ )
    {
	ZEvalInfo* evalinf = new ZEvalInfo( globzidx );
	for ( IdxType sampzidx=0; sampzidx<dims_.z(); sampzidx++ )
	{
	    const float z = Data::z4Idxs( survgeom_, dims_.z(),
					  globzidx, sampzidx );
	    if ( z > zrg.start-eps && z < zrg.stop+eps )
		evalinf->evalpositions_ += ZEvalPos( sampzidx, z );
	}
	if ( evalinf->evalpositions_.isEmpty() )
	    delete evalinf;
	else
	    zevalinfos_ += evalinf;
    }

    if ( zevalinfos_.isEmpty() )
    {
	// only possibility is that zrg is entirely between two output samples.
	// As a service, we'll make sure the nearest sample is set
	const float zeval = zrg.center();
	const IdxType globzidx = Data::globIdx4Z( survgeom_, zeval, dims_.z() );
	const IdxType sampzidx = Data::sampIdx4Z( survgeom_, zeval, dims_.z() );
	globzidxrg_.start = globzidxrg_.stop = globzidx;
	ZEvalInfo* evalinf = new ZEvalInfo( globzidx );
	evalinf->evalpositions_ += ZEvalPos( sampzidx, zeval );
	zevalinfos_ += evalinf;
    }
}


BufferString Seis::Blocks::Writer::dirName() const
{
    File::Path fp( basepath_ );
    fp.add( filenamebase_ );
    return fp.fullPath();
}


BufferString Seis::Blocks::Writer::mainFileName() const
{
    File::Path fp( basepath_ );
    fp.add( filenamebase_ );
    fp.setExtension( "cube", false );
    return fp.fullPath();
}


bool Seis::Blocks::Writer::removeExisting( const char* fnm,
					   uiRetVal& uirv ) const
{
    if ( File::exists(fnm) )
    {
	if ( !File::isDirectory(fnm) )
	{
	    if ( !File::remove(fnm) )
	    {
		uirv.add( tr("Cannot remove file:\n%1").arg(fnm) );
		return false;
	    }
	}
	else if ( !File::removeDir(fnm) )
	{
	    uirv.add( tr("Cannot remove directory:\n%1").arg(fnm) );
	    return false;
	}
    }
    return true;
}


bool Seis::Blocks::Writer::prepareWrite( uiRetVal& uirv )
{
    writecomplete_ = false;
    const BufferString mainfnm = mainFileName();
    const BufferString dirnm = dirName();
    if ( !removeExisting(mainfnm,uirv) || !removeExisting(dirnm,uirv) )
        return false;

    if ( !File::createDir(dirnm) )
    {
	uirv.add( tr("Cannot create directory:\n%1").arg(dirnm) );
	return false;
    }

    return true;
}


uiRetVal Seis::Blocks::Writer::add( const SeisTrc& trc )
{
    uiRetVal uirv;
    if ( needreset_ )
    {
	setEmpty();
	if ( !prepareWrite(uirv) )
	    return uirv;

	resetZ( Interval<float>(trc.startPos(),trc.endPos()) );
	usefprep_ = specfprep_;
	if ( usefprep_ == OD::AutoFPRep )
	    usefprep_ = trc.data().getInterpreter()->dataChar().userType();
    }

    const BinID bid = trc.info().binID();
    GlobIdx globidx( Data::globIdx4Inl(survgeom_,bid.inl(),dims_.inl()),
		     Data::globIdx4Crl(survgeom_,bid.crl(),dims_.crl()), 0 );
    for ( int iblock=0; iblock<zevalinfos_.size(); iblock++ )
    {
	const ZEvalInfo& evalinf = *zevalinfos_[iblock];
	globidx.z() = evalinf.globidx_;
	if ( !add2Block(globidx,trc,evalinf.evalpositions_,uirv) )
	    return uirv;
    }

    return uirv;
}


bool Seis::Blocks::Writer::add2Block( const GlobIdx& globidx,
	const SeisTrc& trc, const ZEvalPosSet& zevals, uiRetVal& uirv )
{
    Block* block = getBlock( globidx );
    if ( !block )
    {
	uirv.set( tr("Memory needed for writing process unavailable.") );
	setEmpty();
	return false;
    }

    if ( block->data_->isRetired() )
	return true; // not writing same block again

    const BinID bid( trc.info().binID() );
    SampIdx sampidx( Data::sampIdx4Inl( survgeom_, bid.inl(), dims_.inl() ),
		     Data::sampIdx4Crl( survgeom_, bid.crl(), dims_.crl() ),
		     0 );

    for ( int idx=0; idx<zevals.size(); idx++ )
    {
	const ZEvalPos& evalpos = zevals[idx];
	sampidx.z() = evalpos.first;
	float val2write = trc.getValue( evalpos.second, component_ );
	if ( scaler_ )
	    val2write = (float)scaler_->scale( val2write );
	block->data_->setValue( sampidx, val2write );
    }

    if ( isCompletionVisit(*block,sampidx) )
	writeBlock( *block, uirv );

    return uirv.isError();
}


Seis::Blocks::Writer::Block*
Seis::Blocks::Writer::getBlock( const GlobIdx& globidx )
{
    const Pos::IdxPair idxpair( globidx.inl(), globidx.crl() );
    Pos::IdxPairDataSet::SPos spos = blocks_.find( idxpair );
    Block* ret = 0;
    if ( spos.isValid() )
	ret = (Block*)blocks_.getObj( spos );
    else
    {
	try {
	    ret = new Block;
	    ret->data_ = new Data( globidx, dims_, usefprep_ );
	    ret->data_->zero();
	    ret->visited_.setSize( nrposperblock_, false );
	} catch ( ... ) { delete ret; ret = 0; }
	if ( ret )
	    blocks_.add( idxpair, ret );
    }

    return ret;
}


bool Seis::Blocks::Writer::isCompletionVisit( Block& block,
					      const SampIdx& sampidx ) const
{
    const int arridx = ((int)sampidx.inl()) * dims_.inl() + sampidx.crl();
    if ( !block.visited_[arridx] )
    {
	block.nruniquevisits_++;
	block.visited_[arridx] = true;
    }

    return block.nruniquevisits_ == nrposperblock_;
}


void Seis::Blocks::Writer::writeBlock( Block& block, uiRetVal& uirv )
{
    uirv.setEmpty();
    Data& data = *block.data_;

    File::Path fp( basepath_ );
    fp.add( filenamebase_ ).add( fileNameFor(data.globIdx()) );

    od_ostream strm( fp.fullPath() );
    if ( strm.isBad() )
    {
	uirv.add( uiStrings::phrCannotOpen( toUiString(strm.fileName()) ) );
	return;
    }

    if ( !writeBlockHeader(strm,data)
      || !writeBlockData(strm,data) )
    {
	uirv.add( uiStrings::phrCannotWrite( toUiString(strm.fileName()) ) );
	return;
    }

    data.retire();
}


bool Seis::Blocks::Writer::writeBlockHeader( od_ostream& strm, Data& data )
{
    strm.addBin( cHdrSz ).addBin( version_ );
    strm.addBin( dims_.inl() ).addBin( dims_.crl() ).addBin( dims_.z() );
    const unsigned short dfmt = (unsigned short)usefprep_;
    strm.addBin( dfmt );
    char buf[cHdrSz]; OD::memZero( buf, cHdrSz );
    if ( scaler_ )
    {
	float* ptr = (float*)buf;
	*ptr++ = (float)scaler_->constant_;
	*ptr = (float)scaler_->factor_;
    }
    strm.addBin( buf, 8 );
    const GlobIdx gli = data.globIdx();
    strm.addBin( gli.inl() ).addBin( gli.crl() ).addBin( gli.z() );
    const unsigned short coordsysid = survgeom_.coordSysID();
    strm.addBin( coordsysid );
    survgeom_.binID2Coord().fillBuf( buf );
    strm.addBin( buf, survgeom_.binID2Coord().sizeInBuf() );

    const int bytes_left_in_hdr = cHdrSz - 76;
    OD::memZero( buf, bytes_left_in_hdr );
    strm.addBin( buf, bytes_left_in_hdr );

    return strm.isOK();
}


bool Seis::Blocks::Writer::writeBlockData( od_ostream& strm, Data& data )
{
    strm.addBin( data.dataBuf().data(), data.dataBuf().totalBytes() );
    return strm.isOK();
}


void Seis::Blocks::Writer::writeMainFile( uiRetVal& uirv )
{
    od_ostream strm( mainFileName() );
    if ( strm.isBad() )
    {
	uirv.add( uiStrings::phrCannotOpen( toUiString(strm.fileName()) ) );
	return;
    }

    //TODO
    IOPar iop;
    iop.set( sKey::Version(), version_ );
    iop.set( sKeyDimensions(), dims_.inl(), dims_.crl(), dims_.z() );
    /*
    const unsigned short dfmt = (unsigned short)usefprep_;
    strm.addBin( dfmt );
    char buf[cHdrSz]; OD::memZero( buf, cHdrSz );
    if ( scaler_ )
    {
	float* ptr = (float*)buf;
	*ptr++ = (float)scaler_->constant_;
	*ptr = (float)scaler_->factor_;
    }
    strm.addBin( buf, 8 );
    const GlobIdx gli = data.globIdx();
    strm.addBin( gli.inl() ).addBin( gli.crl() ).addBin( gli.z() );
    const unsigned short coordsysid = survgeom_.coordSysID();
    strm.addBin( coordsysid );
    survgeom_.binID2Coord().fillBuf( buf );
    strm.addBin( buf, survgeom_.binID2Coord().sizeInBuf() );
    */
}


namespace Seis
{
namespace Blocks
{

class WriterFinisher : public ParallelTask
{ mODTextTranslationClass(Seis::Blocks::WriterFinisher)
public:

WriterFinisher( Writer& wrr )
    : ParallelTask("Block write finisher")
    , wrr_(wrr)
{
    Pos::IdxPairDataSet::SPos spos;
    while ( wrr_.blocks_.next(spos) )
    {
	Writer::Block* block = (Writer::Block*)wrr_.blocks_.getObj( spos );
	if ( !block->data_->isRetired() )
	{
	    if ( block->nruniquevisits_ < 1 )
		block->data_->retire();
	    else
		towrite_ += block;
	}
    }
}

virtual od_int64 nrIterations() const
{
    return towrite_.size();
}

virtual uiString nrDoneText() const
{
   return tr("Blocks written");
}

virtual uiString message() const
{
    if ( uirv_.isOK() )
       return tr("Writing edge blocks");
    return uirv_;
}

virtual bool doWork( od_int64 startidx, od_int64 stopidx, int )
{
    uiRetVal uirv;
    for ( int idx=(int)startidx; idx<=(int)stopidx; idx++ )
    {
	wrr_.writeBlock( *towrite_[idx], uirv );
	if ( uirv.isError() )
	    { uirv_.add( uirv ); return false; }
	addToNrDone( 1 );
    }

    return true;
}

virtual bool doFinish( bool )
{
    uiRetVal uirv;
    wrr_.writeMainFile( uirv );
    uirv_.add( uirv );
    return uirv.isError();
}

    uiRetVal			uirv_;
    Writer&			wrr_;
    ObjectSet<Writer::Block>	towrite_;

};

} // namespace Blocks
} // namespace Seis


Task* Seis::Blocks::Writer::finisher()
{
    return new WriterFinisher( *this );
}

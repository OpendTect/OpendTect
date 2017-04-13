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
#include "scaler.h"
#include "survgeom3d.h"

static const unsigned short cHdrSz = 128;

Seis::Blocks::DataStorage::DataStorage( const SurvGeom* geom )
    : survgeom_(*(geom ? geom : static_cast<const SurvGeom*>(
				    &SurvGeom::default3D())))
    , dims_(Data::defDims())
{
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
    if ( fp != basepath_ )
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


void Seis::Blocks::Writer::setScaler( const Scaler* newscaler )
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


uiRetVal Seis::Blocks::Writer::add( const SeisTrc& trc )
{
    if ( needreset_ )
    {
	setEmpty();
	resetZ( Interval<float>(trc.startPos(),trc.endPos()) );
	usefprep_ = specfprep_;
	if ( usefprep_ == OD::AutoFPRep )
	    usefprep_ = trc.data().getInterpreter()->dataChar().userType();
    }

    uiRetVal uirv;
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
    // TODO implement
    block.data_->retire();
    uirv.setEmpty();
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

    uiRetVal			uirv_;
    Writer&			wrr_;
    ObjectSet<Writer::Block>	towrite_;

};

} // namespace Blocks
} // namespace Seis


Task* Seis::Blocks::Writer::finisher()
{
    WriterFinisher* ret = new WriterFinisher( *this );

    if ( ret->towrite_.isEmpty() )
	{ delete ret; ret = 0; }

    return ret;
}

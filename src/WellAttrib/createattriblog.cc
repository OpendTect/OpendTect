/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "createattriblog.h"

#include "attribdescset.h"
#include "attribengman.h"
#include "attribprocessor.h"
#include "ioobj.h"
#include "survinfo.h"
#include "welldata.h"
#include "welld2tmodel.h"
#include "wellextractdata.h"
#include "welllog.h"
#include "welllogset.h"
#include "wellmarker.h"
#include "welltrack.h"



#define mErrRet(m) errmsg.append(m); return false;

// AttribLogExtractor

AttribLogExtractor::AttribLogExtractor( const Well::Data& wd )
    : wd_(&wd)
    , bidset_(BinIDValueSet(2,true))
{
}


AttribLogExtractor::~AttribLogExtractor()
{
}


bool AttribLogExtractor::extractData( Attrib::EngineMan& aem,
				      TaskRunner* taskr )
{
    uiString errmsg;
    ObjectSet<BinIDValueSet> bivsset;
    bivsset += &bidset_;
    PtrMan<Attrib::Processor> process =
	    aem.createLocationOutput( errmsg, bivsset );
    if ( !process )
	return false;
    return TaskRunner::execute( taskr, *process );
}


bool AttribLogExtractor::fillPositions( const StepInterval<float>& dahintv )
{
    bidset_.setEmpty(); positions_.erase(); depths_.erase();
    const int nrsteps = dahintv.nrSteps();
    for ( int idx=0; idx<nrsteps; idx++ )
    {
	float md = dahintv.atIndex( idx );
	Coord3 pos = wd_->track().getPos( md );
	const BinID bid = SI().transform( pos );
	if ( !bid.inl() && !bid.crl() )
	    continue;

	if ( SI().zIsTime() && wd_->d2TModel() )
            pos.z_ = wd_->d2TModel()->getTime( md, wd_->track() );

        bidset_.add( bid, (float) pos.z_, (float)idx );
	depths_ += md;
	positions_ += BinIDValueSet::SPos(0,0);
    }

    BinIDValueSet::SPos pos;
    while ( bidset_.next(pos) )
    {
	float& vidx = bidset_.getVals(pos)[1];
	int posidx = mNINT32(vidx);
	positions_[posidx] = pos;
	mSetUdf(vidx);
    }
    return ( !positions_.isEmpty() && !depths_.isEmpty() && !bidset_.isEmpty());
}


// AttribLogCreator::Setup

AttribLogCreator::Setup::Setup( const Attrib::DescSet* attr,
				const Well::ExtractParams* wep )
    : nlamodel_(nullptr)
    , attrib_(attr)
    , selspec_(nullptr)
    , taskr_(nullptr)
    , extractparams_(wep)
{
}


AttribLogCreator::Setup::~Setup()
{
}


// AttribLogCreator

AttribLogCreator::AttribLogCreator( const Setup& su, int& selidx )
    : setup_(su)
    , sellogidx_(selidx)
{
}


AttribLogCreator::~AttribLogCreator()
{
}


bool AttribLogCreator::doWork( Well::Data& wdin, uiString& errmsg )
{
    RefMan<Well::Data> wd( &wdin );
    uiString msg = tr("%1 from well %2");
    Attrib::EngineMan aem;
    aem.setAttribSet( setup_.attrib_ );
    aem.setNLAModel( setup_.nlamodel_ );
    aem.setAttribSpec( *setup_.selspec_ );

    static BufferStringSet emptylognms;
    StepInterval<float> dahrg =
		setup_.extractparams_->calcFrom( *wd, emptylognms, errmsg );
    if ( !mIsUdf( setup_.extractparams_->zstep_ ) )
	dahrg.step_ = setup_.extractparams_->zstep_;

    AttribLogExtractor ale( *wd );
    if ( !ale.fillPositions(dahrg) )
    {
	msg.arg(tr("No positions extracted")).arg(wd->name());
	mErrRet(msg)
    }

    if ( !ale.extractData( aem, setup_.taskr_ ) )
    {
	msg.arg(tr("No data extracted")).arg(wd->name());
	mErrRet(msg)
    }

    if ( !createLog(*wd,ale) )
    {
	msg.arg(tr("Unable to create Log")).arg(wd->name());
	mErrRet(msg)
    }

    return true;
}


bool AttribLogCreator::createLog( Well::Data& wd,
				  const AttribLogExtractor& ale )
{
    PtrMan<Well::Log> newlog = new Well::Log( setup_.lognm_ );
    float v[2]; BinID bid;
    for ( int idx=0; idx<ale.depths().size(); idx++ )
    {
	ale.bidset().get( ale.positions()[idx], bid, v );
	if ( !mIsUdf(v[1]) )
	    newlog->addValue( ale.depths()[idx], v[1] );
    }

    if ( newlog->isEmpty() )
	return false;

    Well::LogSet& logs = wd.logs();
    if ( logs.validIdx(sellogidx_) )
    {
	const BufferString lognm( logs.getLogNameByIdx(sellogidx_) );
	Well::Log& log = logs.isLoaded( lognm.buf() )
		       ? *logs.getLog( lognm.buf() )
		       : getNonConst( *logs.getLogInfos( lognm.buf() ) );
	log.setEmpty();
	for ( int idx=0; idx<newlog->size(); idx++ )
	    log.addValue( newlog->dah(idx), newlog->value(idx) );
    }
    else
    {
	logs.add( newlog.release() );
	sellogidx_ = logs.size() - 1;
    }

    return true;
}


BulkAttribLogCreator::BulkAttribLogCreator( const AttribLogCreator::Setup& su,
				    const TypeSet<MultiID>& wellids,
				    const Mnemonic& outmn, bool overwrite )
    : SequentialTask("Creating log attribute")
    , datasetup_(su)
    , wellids_(wellids)
    , outmn_(outmn)
    , lreqs_(*new Well::LoadReqs(Well::Inf,Well::Trck,Well::D2T))
    , overwrite_(overwrite)
{
    lreqs_.include( Well::LogInfos );
    msg_ = tr( "Creating attribute logs" );
}


BulkAttribLogCreator::~BulkAttribLogCreator()
{
    delete &lreqs_;
}


od_int64 BulkAttribLogCreator::nrDone() const
{
    return nrdone_;
}


od_int64 BulkAttribLogCreator::totalNr() const
{
    return wellids_.size();
}


uiString BulkAttribLogCreator::uiNrDoneText() const
{
    return tr("Wells processed");
}


uiRetVal BulkAttribLogCreator::allMessages() const
{
    uiRetVal uirv( msg_ );
    uirv.add( uirv_ );
    return uirv;
}


bool BulkAttribLogCreator::doPrepare( od_ostream* strm )
{
    if ( !datasetup_.selspec_ )
    {
	msg_ = tr("No attribute selspec provided");
	return false;
    }

    if ( !datasetup_.attrib_ || datasetup_.attrib_->isEmpty() )
    {
	msg_ = tr("No attribute desc set provided");
	return false;
    }

    if ( datasetup_.lognm_.isEmpty() )
    {
	msg_ = tr("No output log name provided");
	return false;
    }

    msg_ = tr( "Creating attribute logs" );
    uirv_.setOK();
    nrdone_ = 0;
    return SequentialTask::doPrepare( strm );
}


int BulkAttribLogCreator::nextStep()
{
    const MultiID& wid = wellids_[nrdone_];
    RefMan<Well::Data> wd = Well::MGR().get( wid, lreqs_ );
    if ( !wd )
    {
	uirv_.add( Well::MGR().errMsg() );
	return ++nrdone_ >= totalNr() ? Finished() : MoreToDo();
    }

    Well::LogSet& logs = wd->logs();
    const char* wellnm = wd->name().buf();
    const char* lognm = datasetup_.lognm_.buf();
    if ( logs.isPresent(lognm) && !overwrite_ )
    {
	uirv_.add( tr("Log '%1' is already present in well '%2'")
			.arg(lognm).arg(wellnm) );
	return ++nrdone_ >= totalNr() ? Finished() : MoreToDo();
    }

    int sellogidx = logs.indexOf( lognm ); //not used
    AttribLogCreator attriblog( datasetup_, sellogidx );
    uiString errmsg;
    if ( !attriblog.doWork(*wd.ptr(),errmsg) )
    {
	uirv_.add( errmsg );
	return ++nrdone_ >= totalNr() ? Finished() : MoreToDo();
    }

    sellogidx = logs.indexOf( lognm );
    PtrMan<Well::Log> newlog = wd->logs().remove( sellogidx );
    if ( !newlog )
    {
	pErrMsg("Should not happen");
	return ++nrdone_ >= totalNr() ? Finished() : MoreToDo();
    }

    newlog->setMnemonic( outmn_ );
    const MultiID dbkey = wd->multiID();
    if ( !Well::MGR().writeAndRegister(dbkey,newlog) )
    {
	uirv_.add( Well::MGR().errMsg() );
	++nrdone_ >= totalNr() ? Finished() : MoreToDo();
    }

    return ++nrdone_ >= totalNr() ? Finished() : MoreToDo();
}


bool BulkAttribLogCreator::doFinish( bool success, od_ostream* strm )
{
    if ( uirv_.isError() )
    {
	msg_ = tr("One or several attribute logs could not be computed");
	success = false;
    }

    return SequentialTask::doFinish( success, strm );
}

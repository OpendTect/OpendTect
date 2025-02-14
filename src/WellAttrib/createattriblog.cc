/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "createattriblog.h"

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

    BufferStringSet dummy;
    StepInterval<float> dahrg = setup_.extractparams_->calcFrom( *wd, dummy );
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

    if ( !createLog( *wd, ale ) )
    {
	msg.arg(tr("Unable to create Log")).arg(wd->name());
	mErrRet(msg)
    }

    return true;
}


bool AttribLogCreator::createLog( Well::Data& wdin,
				  const AttribLogExtractor& ale)
{
    RefMan<Well::Data> wd( &wdin );
    Well::Log* newlog = new Well::Log( setup_.lognm_ );
    float v[2]; BinID bid;
    for ( int idx=0; idx<ale.depths().size(); idx++ )
    {
	ale.bidset().get( ale.positions()[idx], bid, v );
	if ( !mIsUdf(v[1]) )
	    newlog->addValue( ale.depths()[idx], v[1] );
    }

    if ( !newlog->size() )
    {
	delete newlog;
	return false;
    }

    if ( sellogidx_ < 0 )
    {
	wd->logs().add( newlog );
	sellogidx_ = wd->logs().size() - 1;
    }
    else
    {
	Well::Log& log = wd->logs().getLog( sellogidx_ );
	log.setEmpty();
	for ( int idx=0; idx<newlog->size(); idx++ )
	    log.addValue( newlog->dah(idx), newlog->value(idx) );
	delete newlog;
    }
    return true;
}


BulkAttribLogCreator::BulkAttribLogCreator( const AttribLogCreator::Setup& su,
				    ObjectSet<Well::Data>& selwells,
				    const Mnemonic& outmn, uiRetVal& uirv,
				    bool overwrite )
    : SequentialTask("Creating log attribute")
    , datasetup_(su)
    , selwells_(selwells)
    , outmn_(outmn)
    , msgs_(uirv)
    , overwrite_(overwrite)
{
    msg_ = tr( "Creating attribute logs" );
}


BulkAttribLogCreator::~BulkAttribLogCreator()
{}


od_int64 BulkAttribLogCreator::nrDone() const
{
    return nrdone_;
}


od_int64 BulkAttribLogCreator::totalNr() const
{
    return selwells_.size();
}


uiString BulkAttribLogCreator::uiNrDoneText() const
{
    return tr( "Wells processed" );
}


uiString BulkAttribLogCreator::uiMessage() const
{
    return msg_;
}


int BulkAttribLogCreator::nextStep()
{
    if ( nrdone_ >= selwells_.size() )
	return Finished();

    const char* lognm = datasetup_.lognm_.str();
    RefMan<Well::Data> wd = selwells_.get( nrdone_ );
    if ( !wd )
    {
	nrdone_++;
	return MoreToDo();
    }

    if ( SI().zIsTime() && !wd->d2TModel() )
    {
	msgs_.add( tr("No depth to time model defined for well '%1'")
		   .arg(wd->name()) );
	nrdone_++;
	return MoreToDo();
    }

    if ( wd->logs().isPresent(lognm) && !overwrite_ )
    {
	msgs_.add( tr("Log: '%1' is already present in '%2'.\n")
				  .arg(lognm).arg(wd->name().buf()) );
	nrdone_++;
	return MoreToDo();
    }

    int sellogidx = wd->logs().indexOf( lognm ); //not used
    AttribLogCreator attriblog( datasetup_, sellogidx );
    uiString errmsg;
    if ( !attriblog.doWork(*wd,errmsg) )
    {
	msgs_.add( errmsg );
	nrdone_++;
	return MoreToDo();
    }

    sellogidx = wd->logs().indexOf( lognm );
    PtrMan<Well::Log> newlog = wd->logs().remove( sellogidx );
    if ( !newlog )
    {
	pErrMsg("Should not happen");
	nrdone_++;
	return MoreToDo();
    }

    newlog->setMnemonic( outmn_ );
    const MultiID dbkey = wd->multiID();
    if ( !Well::MGR().writeAndRegister(dbkey,newlog) )
    {
	msgs_.add( toUiString(Well::MGR().errMsg()) );
	nrdone_++;
	return MoreToDo();
    }

    nrdone_++;
    return MoreToDo();
}

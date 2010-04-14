/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Apr 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: welltiedata.cc,v 1.29 2010-04-14 13:11:02 cvsbruno Exp $";

#include "arrayndimpl.h"
#include "ioman.h"
#include "iostrm.h"
#include "strmprov.h"
#include "survinfo.h"
#include "seistrctr.h"
#include "seistrc.h"
#include "seiswrite.h"
#include "survinfo.h"
#include "wavelet.h"

#include "welldata.h"
#include "welllog.h"
#include "welllogset.h"
#include "wellwriter.h"

#include "welltiecshot.h"
#include "welltiedata.h"
#include "welltieextractdata.h"
#include "welltiesetup.h"
#include "welltied2tmodelmanager.h"
#include "welltiepickset.h"
#include "welltieunitfactors.h"

namespace WellTie
{



DataHolder::DataHolder( WellTie::Params* params, Well::Data* wd, 
			const WellTie::Setup& s )
    	: params_(params)	
	, wd_(wd) 
	, setup_(s)
	, factors_(s.unitfactors_) 	  
	, wvltctio_(*mMkCtxtIOObj(Wavelet))
	, seisctio_(*mMkCtxtIOObj(SeisTrc))
{
    seisctio_.ctxt.forread = false;
    wvltctio_.ctxt.forread = false;

    for ( int idx =0; idx<2; idx++ )
	wvltset_ += Wavelet::get( IOM().get(s.wvltid_) ); 

    uipms_   = &params_->uipms_;
    dpms_    = &params_->dpms_;
    pickmgr_ = new WellTie::PickSetMGR( wd_ );
    geocalc_ = new WellTie::GeoCalculator( *this );
    d2tmgr_  = new WellTie::D2TModelMGR( *this );
    logset_ = new Well::LogSet();
}


DataHolder::~DataHolder()
{
    deepErase( arr_ );
    logset_->empty();
    delete logset_;
    delete pickmgr_;
    delete d2tmgr_;
    delete params_;
    delete seisctio_.ioobj; delete &seisctio_;
    delete wvltctio_.ioobj; delete &wvltctio_;
}


void DataHolder::resetLogData()
{
    logset_->empty();
    for ( int idx=0; idx<dpms_->colnms_.size(); idx++ )
    {
	Well::Log* log = new Well::Log( dpms_->colnms_.get(idx) );
	logset_->add( log );
 	arr_ += new Array1DImpl<float>( log->size() );
    }
}


Array1DImpl<float>* DataHolder::getLogVal( const char* nm, bool isdah )
{
    const int logidx = logset_->indexOf( nm );
    if ( logidx<0 ) return 0;
    Well::Log& log = logset_->getLog( logidx );
    float* val = isdah ? log.dahArr() : log.valArr();
    delete arr_[logidx];
    arr_.replace( logidx, new Array1DImpl<float> (log.size()) );
    memcpy( arr_[logidx]->getData(), val, log.size()*sizeof(float) );
    return arr_[logidx];
}


void DataHolder::setLogVal( const char* nm , 
			    const Array1DImpl<float>* vals,
			    const Array1DImpl<float>* dahs )
{
    if ( !vals || ! dahs ) return;
    Well::Log& log = *logset_->getLog( nm ); 
    log.erase();
    for ( int idx=0; idx<dahs->info().getSize(0); idx++ )
	log.addValue( dahs->get(idx), vals->get(idx) );
}



DataWriter::DataWriter( const WellTie::DataHolder& dh )
    	: holder_(dh)
	, wtr_(0)  
{
    setWellWriter();
}

DataWriter::~DataWriter()
{
    delete wtr_;
}


void DataWriter::setWellWriter()
{
    const MultiID& wid = holder_.setup().wellid_;
    mDynamicCastGet( const IOStream*, iostrm, IOM().get(wid) );
    if ( !iostrm ) return;

    StreamProvider sp( iostrm->fileName() );
    sp.addPathIfNecessary( iostrm->dirName() );
    wtr_ = new Well::Writer(sp.fileName(),*holder_.wd()); 
}


bool DataWriter::writeD2TM() const
{
    return ( wtr_ && wtr_->putD2T() );
}


bool DataWriter::writeLogs( const Well::LogSet& logset ) const
{
    Well::LogSet& wdlogset = const_cast<Well::LogSet&>( holder_.wd()->logs() );
    for ( int idx=0; idx<logset.size(); idx++ )
    {
	Well::Log* log = new Well::Log( logset.getLog(idx) );
	wdlogset.add( log );
    }
    return ( wtr_ && wtr_->putLogs() );
}


bool DataWriter::writeLogs2Cube( LogData& ld ) const
{
    bool allsucceeded = true;
    for ( int idx=0; idx<ld.logset_.size(); idx++ )
    {
	WellTie::TrackExtractor wtextr( holder_.wd() );
	wtextr.timeintv_ = holder_.dpms()->timeintvs_[1];
	if ( !wtextr.execute() )
	    pErrMsg( "unable to extract position" );

	ld.curlog_ = &ld.logset_.getLog( idx );
	ld.curidx_ = idx;
	const int datasz = ld.curlog_->size();

	ld.bids_.erase();
	for ( int idbids=0; idbids<datasz; idbids++ )
	    ld.bids_ += wtextr.getBIDs()[idbids];

	if ( !writeLog2Cube( ld ) )
	    allsucceeded = false;
    }
    return allsucceeded;
}


bool DataWriter::writeLog2Cube( LogData& ld) const
{
    if ( ld.ctioidxset_[ld.curidx_] < 0 ) return false;
    SeisTrcWriter writer( ld.seisctioset_[ld.ctioidxset_[ld.curidx_]]->ioobj );
    bool succeeded = true;
    TypeSet<BinID> bids = ld.bids_;
    ObjectSet<SeisTrc> trcset;
    SeisTrc* curtrc = 0;
    const int datasz = ld.curlog_->size();
    BinID prevbid( bids[0] );
    for ( int idx=0; idx<datasz; idx++ )
    {
	const BinID bid( bids[idx] );
	if ( idx && bid == prevbid )
	    curtrc->set( idx, ld.curlog_->value(idx), 0 );
	else
	{
	    SeisTrc* newtrc = new SeisTrc( datasz );
	    trcset += newtrc;
	    for ( int sidx=0; sidx<datasz; sidx++ )
		newtrc->set( sidx, idx ? mUdf(float):ld.curlog_->value(idx),0 );
	    newtrc->info().sampling.step = SI().zStep();
	    newtrc->info().binid = bid;
	    curtrc = newtrc;
	}
	prevbid = bid;
    }
    for ( int idx=0; idx<trcset.size(); idx++ )
    {
	SeisTrc* trc = trcset[idx];
	BinID curbid = trc->info().binid;
	for ( int crlidx=0; crlidx<ld.nrtraces_; crlidx++ )
	{
	    for ( int inlidx=0; inlidx<ld.nrtraces_; inlidx++ )
	    {
		BinID bid = BinID( curbid.r() - ld.nrtraces_/2 + crlidx , 
				   curbid.c() - ld.nrtraces_/2 + inlidx  );
		trc->info().binid = bid;
		if ( !writer.put(*trc) )
		{ pErrMsg( "cannot write new trace" ); succeeded = false; } 
	    }
	}
    }
    deepErase( trcset );

    return succeeded;
}

}; //namespace WellTie

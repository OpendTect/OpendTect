/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Apr 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: welltiedata.cc,v 1.20 2009-09-29 15:15:34 cvsbruno Exp $";

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

#include "welltiedata.h"
#include "welltieextractdata.h"
#include "welltiesetup.h"
#include "welltied2tmodelmanager.h"
#include "welltiepickset.h"
#include "welltieunitfactors.h"

namespace WellTie
{

Log::Log( const char* nm )
    : Well::Log(nm)
    , arr_(0)
{
}


Log::~Log()
{
    delete arr_;
}


const Array1DImpl<float>* Log::getVal( const Interval<float>* si, bool dah )
{
    delete arr_; arr_ = 0;
    TypeSet<float> vals;
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( si && dah_[idx]<si->start )
	    continue;
	float val = dah ? dah_[idx] : valArr()[idx];
	if ( si && dah_[idx]>=si->stop )
	    val =0;
	vals += val;
    }
    arr_ = new Array1DImpl<float> ( vals.size() );
    memcpy( arr_->getData(), vals.arr(), vals.size()*sizeof(float) );
    return arr_;
}


void Log::setVal( const Array1DImpl<float>* arr, bool isdah )
{
    if ( !arr ) return;

    TypeSet<float>& val = isdah ? dah_ : val_;
    val.erase();
    for ( int idx=0; idx<arr->info().getSize(0); idx++ )
	val.add( arr->get( idx ) );
}


LogSet::~LogSet()
{
    deepErase( logs );
}


const Array1DImpl<float>* LogSet::getVal( const char* nm, bool isdah, 
					  const Interval<float>* st ) const
{
    mDynCast(nm,return 0); return l->getVal(st,isdah);
}


void LogSet::resetData( const WellTie::Params::DataParams& params )
{
    deepErase( logs );
    for ( int idx=0; idx<params.colnms_.size(); idx++ )
	logs += new WellTie::Log( params.colnms_.get(idx) );
}


float LogSet::getExtremVal( const char* colnm, bool ismax ) const
{
    float maxval,             minval;
    maxval = get(colnm, 0);   minval = maxval;

    for ( int idz=0; idz<getLog(colnm)->size(); idz++)
    {
	float val =  get(colnm, idz);
	if ( maxval < val && !mIsUdf( val ) )
	    maxval = val;
	if ( minval > val && !mIsUdf(val) )
	    minval = val;
    }
    return ismax? maxval:minval;
}


DataHolder::DataHolder( WellTie::Params* params, Well::Data* wd, 
			const WellTie::Setup& s )
    	: params_(params)	
	, wd_(wd) 
	, setup_(s)
	, factors_(s.unitfactors_) 	  
	, wvltctio_(*mMkCtxtIOObj(Wavelet))
	, seisctio_(*mMkCtxtIOObj(SeisTrc))

{
    for ( int idx =0; idx<2; idx++ )
	wvltset_ += Wavelet::get( IOM().get(s.wvltid_) ); 
    uipms_   = &params_->uipms_;
    dpms_    = &params_->dpms_;
    pickmgr_ = new WellTie::PickSetMGR( wd_ );
    geocalc_ = new WellTie::GeoCalculator( *this );
    d2tmgr_  = new WellTie::D2TModelMGR( *this );
    logsset_ = new WellTie::LogSet( *this );
}


DataHolder::~DataHolder()
{
    delete logsset_;
    delete pickmgr_;
    delete d2tmgr_;
    delete params_;
    delete seisctio_.ioobj; delete &seisctio_;
    delete wvltctio_.ioobj; delete &wvltctio_;
}


DataWriter::DataWriter( WellTie::DataHolder* dh )
	: holder_(dh)
        , seisctio_(dh->seisCtxt()) 
{
}


const Well::Writer* DataWriter::getWellWriter() const
{
    const MultiID& wid = holder_->setup().wellid_;
    mDynamicCastGet( const IOStream*, iostrm, IOM().get(wid) );
    if ( !iostrm ) return 0;

    StreamProvider sp( iostrm->fileName() );
    sp.addPathIfNecessary( iostrm->dirName() );
    BufferString fname = sp.fileName();
    return new Well::Writer( fname, *holder_->wd() );
}


bool DataWriter::writeD2TM() const
{
    const Well::Writer* wr = getWellWriter();
    if ( wr && wr->putLogs() )
    { delete wr; return true; }

    return false;
}


bool DataWriter::writeLogs( const Well::LogSet& logset ) const
{
    Well::LogSet& wdlogset = const_cast<Well::LogSet&>( holder_->wd()->logs() );
    for ( int idx=0; idx<logset.size(); idx++ )
    {
	Well::Log* log = const_cast<Well::Log*>( &logset.getLog(idx) );
	wdlogset.add( log );
    }

    const Well::Writer* wr = getWellWriter();
    if ( wr && wr->putLogs() )
    {
	delete wr;
	return true;
    }
    return false;
}


bool DataWriter::writeLogs2Cube( const Well::LogSet& logset ) const
{
    for ( int idx=0; idx<logset.size(); idx++ )
    {
	WellTie::TrackExtractor wtextr( 0, holder_->wd() );
	wtextr.timeintv_ = holder_->dpms()->timeintvs_[1];
	if ( !wtextr.execute() )
	    pErrMsg( "unable to  track extract position" );

	const Well::Log& log = logset.getLog(idx);
	const int datasz = log.size();

	TypeSet<BinID> bids;
	for ( int idx=0; idx<datasz; idx++ )
	bids += wtextr.getBIDValues()[idx];

	writeLog2Cube( log, bids );
										    }
    return true;
}


bool DataWriter::writeLog2Cube( const Well::Log& log, const TypeSet<BinID>& bids ) const
{
    SeisTrcWriter writer( seisctio_.ioobj );

    ObjectSet<SeisTrc> trcset;
    SeisTrc* curtrc = 0;
    const int datasz = log.size();
    BinID prevbid( bids[0] );
    for ( int idx=0; idx<datasz; idx++ )
    {
	const BinID bid( bids[idx] );
	if ( idx && bid == prevbid )
	    curtrc->set( idx, log.value(idx), 0 );
	else
	{
	    SeisTrc* newtrc = new SeisTrc( datasz );
	    trcset += newtrc;
	    for ( int sidx=0; sidx<datasz; sidx++ )
		newtrc->set( sidx, idx ? mUdf(float) : log.value(idx), 0 );
	    newtrc->info().sampling.step = SI().zStep();
	    newtrc->info().binid = bid;
	    curtrc = newtrc;
	}
	prevbid = bid;
    }
    for ( int idx=0; idx<trcset.size(); idx++ )
    {
	if ( !writer.put(*trcset[idx]) )
	    pErrMsg( "cannot write new trace" );
    }
    deepErase( trcset );

    return true;
}




}; //namespace WellTie

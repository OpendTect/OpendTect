/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Apr 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: welltiedata.cc,v 1.40 2010-10-07 15:39:30 cvsbruno Exp $";

#include "arrayndimpl.h"
#include "binidvalset.h"
#include "ioman.h"
#include "iostrm.h"
#include "strmprov.h"
#include "survinfo.h"
#include "seistrctr.h"
#include "seistrc.h"
#include "seiswrite.h"
#include "survinfo.h"
#include "wavelet.h"

#include "emhorizon2d.h"
#include "emmanager.h"
#include "emsurfacetr.h"

#include "welldata.h"
#include "welld2tmodel.h"
#include "wellhorpos.h"
#include "welltrack.h"
#include "welllog.h"
#include "welllogset.h"
#include "wellman.h"
#include "wellmarker.h"
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


DataHolder::DataHolder( const WellTie::Setup& s )
	: wellid_(s.wellid_)				  
	, setup_(s)
	, factors_(s.unitfactors_) 	  
	, wvltctio_(*mMkCtxtIOObj(Wavelet))
	, seisctio_(*mMkCtxtIOObj(SeisTrc))
	, wd_(0)			
	, params_(0)		     	
	, redrawViewerNeeded(this)				
	, closeall(this)				   
{
    wd_ = wd();
    params_ = new WellTie::Params( s, wd_ );
    seisctio_.ctxt.forread = false;
    wvltctio_.ctxt.forread = false;

    for ( int idx =0; idx<2; idx++ )
	wvltset_ += Wavelet::get( IOM().get(s.wvltid_) ); 

    uipms_   = &params_->uipms_;
    dpms_    = &params_->dpms_;
    pickmgr_ = new WellTie::PickSetMGR( *this );
    geocalc_ = new WellTie::GeoCalculator( *this );
    d2tmgr_  = new WellTie::D2TModelMGR( *this );
    logset_  = new Well::LogSet();
}


DataHolder::~DataHolder()
{
    deepErase( arr_ );
    logset_->empty();
    delete params_;
    delete logset_;
    delete pickmgr_;
    delete d2tmgr_;
    delete seisctio_.ioobj; delete &seisctio_;
    delete wvltctio_.ioobj; delete &wvltctio_;

    if ( wd_ )
	wd_->tobedeleted.remove( mCB(this,DataHolder,welldataDelNotify) );
    wd_ = 0;
    Well::MGR().release( wellid_ );
}


void DataHolder::welldataDelNotify( CallBacker* )
{ wd_ = 0;  if ( params_ ) params_->resetWD(0); }


Well::Data* DataHolder::wd() const
{
    if ( !wd_ )
    {
	DataHolder* self = const_cast<DataHolder*>( this );
	Well::Data* wd = Well::MGR().get( wellid_, false );
	self->wd_ = wd;
	if ( wd )
	{
	    if ( params_ )
		params_->resetWD( wd );
	    wd->tobedeleted.notify( mCB(self,DataHolder,welldataDelNotify) );
	}
	//else
	  //  self->triggerClose();
    }
    return wd_;
}


const BinID DataHolder::binID() const
{
    Coord3 pos = wd()->track().pos( 0 );
    return SI().transform( pos );
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


Array1DImpl<float>* DataHolder::getLogVal( const char* nm, bool isdah ) const 
{
    DataHolder* self = const_cast<DataHolder*>( this );
    const int logidx = logset_->indexOf( nm );
    if ( logidx<0 ) return 0;
    const Well::Log& log = logset_->getLog( logidx );
    const float* val = isdah ? log.dahArr() : log.valArr();
    delete self->arr_[logidx];
    self->arr_.replace( logidx, new Array1DImpl<float> (log.size()) );
    memcpy( self->arr_[logidx]->getData(), val, log.size()*sizeof(float) );
    return self->arr_[logidx];
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


bool DataHolder::setUpHorizons( const TypeSet<MultiID>& horids, 
				BufferString& errms, TaskRunner& tr)
{
    deepErase( hordatas_ );
    EM::EMManager& em = EM::EMM();
    for ( int idx=0; idx<horids.size(); idx++ )
    {
	PtrMan<IOObj> ioobj = IOM().get( horids[idx] );
	if ( !ioobj )
	{
	    errms += "Cannot get database entry for selected horizon";
	    return false;
	}

	EM::ObjectID emid = EM::EMM().getObjectID( horids[idx] );
	RefMan<EM::EMObject> emobj = EM::EMM().getObject( emid );
	bool success = true;
	if ( !emobj || !emobj->isFullyLoaded() )
	{
	    success = false;
	    PtrMan<Executor> exec = em.objectLoader( horids[idx] );
	    if ( exec )
	    {
		if ( tr.execute( *exec ) )
		    success = true;
	    }
	    if ( success )
	    {
		emid = EM::EMM().getObjectID( horids[idx] );
		emobj = EM::EMM().getObject( emid );
	    }
	    else
	    {
		errms += "Cannot load ";
		errms += ioobj->name();
		errms += ".";
		return false;
	    }
	}
	mDynamicCastGet(EM::Horizon*,hor,emobj.ptr())
	if ( !hor ) continue;
	WellHorPos whpos( wd()->track() );
	whpos.setHorizon( emid );
	BinIDValueSet bivset(2,false);
	whpos.intersectWellHor( bivset );
	if ( bivset.nrVals() )
	{
	    BinIDValueSet::Pos pos = bivset.getPos( 0 );
	    float zval; BinID bid;
	    bivset.get( pos, bid, zval );
	    zval *= 1000;
	    hordatas_ += new HorData( zval, hor->preferredColor() );
	    hordatas_[hordatas_.size()-1]->name_ = hor->name();
	    hordatas_[hordatas_.size()-1]->lvlid_ = hor->stratLevelID();
	}
    }
    return true;
}


bool DataHolder::matchHorWithMarkers( BufferString& errmsg, bool bynames ) 
{
    if ( !hordatas_.size() || !wd()->markers().size() )
    { errmsg = "No horizon or no marker found "; return false; }
    Well::D2TModel* dtm = wd()->d2TModel();
    if ( !dtm || dtm->size()<= 0 )
    { errmsg = "No valid depth/time model found "; return false; }

    bool success = false;
    for ( int idmrk=0; idmrk<wd()->markers().size(); idmrk++ )
    {
	const Well::Marker& mrk = *wd()->markers()[idmrk];
	for ( int idhor=0; idhor<hordatas_.size(); idhor++ )
	{
	    HorData& hd = *hordatas_[idhor];
	    BufferString mrknm( mrk.name() );
	    BufferString hdnm( hd.name_ );
	    if ( ( bynames && !strcmp( mrknm, hdnm ) ) 
		|| ( !bynames && hd.lvlid_ >=0 && hd.lvlid_ == mrk.levelID() ))
	    {
		float zmrkpos = dtm->getTime(mrk.dah())*1000;
		float zhorpos = hd.zval_;
		pickmgr_->addPick( zmrkpos, true );
		pickmgr_->addPick( zhorpos, false );
		success =true;
	    }
	}
    }
    if ( !success )
    { errmsg = "No match found between markers and horizons"; return false; }
    return true;
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
		BinID bid = BinID( curbid.inl - ld.nrtraces_/2 + crlidx , 
				   curbid.crl - ld.nrtraces_/2 + inlidx  );
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

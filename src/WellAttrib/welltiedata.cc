/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Apr 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: welltiedata.cc,v 1.44 2011-01-20 11:14:51 cvsbruno Exp $";

#include "ioman.h"
#include "iostrm.h"
#include "strmprov.h"
#include "seistrc.h"
#include "seiswrite.h"
#include "survinfo.h"
#include "seisioobjinfo.h"

#include "emhorizon2d.h"
#include "emmanager.h"

#include "welldata.h"
#include "welld2tmodel.h"
#include "wellhorpos.h"
#include "welllog.h"
#include "welllogset.h"
#include "wellman.h"
#include "wellmarker.h"
#include "wellwriter.h"
#include "wavelet.h"

#include "welltiecshot.h"
#include "welltiedata.h"
#include "welltieextractdata.h"
#include "welltiesetup.h"
#include "welltietoseismic.h"
#include "welltied2tmodelmanager.h"
#include "welltiepickset.h"

namespace WellTie
{

Data::Data( const Setup& wts )
    : logset_(*new Well::LogSet)  
    , setup_(wts)  
    , initwvlt_(*Wavelet::get(IOM().get( wts.wvltid_)))
    , estimatedwvlt_(*new Wavelet(initwvlt_))
    , timeintv_(SI().zRange(true))
    , synthtrc_(*new SeisTrc)  
    , seistrc_(*new SeisTrc)  
{
    currvellog_ = sonic(); //TODO chge init this 
    estimatedwvlt_.setName( "Estimated wavelet" );
}


Data::~Data()
{
    delete &logset_;
    delete &initwvlt_;
    delete &estimatedwvlt_;
    delete &synthtrc_;
    delete &seistrc_;
}


const char* Data::sonic() const
{ return setup_.vellognm_; }

const char* Data::corrsonic() const
{ return setup_.corrvellognm_; }

const char* Data::currvellog() const
{ return currvellog_; }

const char* Data::density() const
{ return setup_.denlognm_; }

const char* Data::ai() const
{ return "AI"; }

const char* Data::reflectivity() const
{ return "Reflectivity"; }

const char* Data::synthetic() const
{ return "Synthetic"; }

const char* Data::seismic() const
{ return "Seismic"; }

const char* Data::checkshotlog() const
{ return "CheckShot log"; }

bool Data::isSonic() const
{ return setup_.issonic_; }


void HorizonMgr::setUpHorizons( const TypeSet<MultiID>& horids, 
				BufferString& errms, TaskRunner& tr )
{
    horizons_.erase();
    if ( !wd_ ) return;
    EM::EMManager& em = EM::EMM();
    for ( int idx=0; idx<horids.size(); idx++ )
    {
	PtrMan<IOObj> ioobj = IOM().get( horids[idx] );
	if ( !ioobj )
	{
	    errms += "Cannot get database entry for selected horizon"; 
	    return; 
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
		return;
	    }
	}
	mDynamicCastGet(EM::Horizon*,hor,emobj.ptr())
	if ( !hor ) continue;
	WellHorIntersectFinder whfinder( wd_->track(), wd_->d2TModel() );
	whfinder.setHorizon( emid );
	TypeSet<WellHorIntersectFinder::ZPoint> zpts;
	whfinder.findIntersection( zpts );
	if ( !zpts.isEmpty() )
	{
	    const float zval = zpts[0].zval_*= 1000;
	    Marker hd( zval );
	    hd.name_ = hor->name();
	    hd.color_ = hor->preferredColor();
	    horizons_ += hd;
	    hd.id_ = hor->stratLevelID();
	}
    }
}



void HorizonMgr::matchHorWithMarkers( TypeSet<PosCouple>& pcs, 
					bool bynames ) const
{
    const Well::D2TModel* dtm = wd_ ? wd_->d2TModel() : 0;
    if ( !dtm ) return;
    for ( int idmrk=0; idmrk<wd_->markers().size(); idmrk++ )
    {
	const Well::Marker& mrk = *wd_->markers()[idmrk];
	for ( int idhor=0; idhor<horizons_.size(); idhor++ )
	{
	    Marker& hd = horizons_[idhor];
	    BufferString mrknm( mrk.name() );
	    BufferString hdnm( hd.name_ );
	    if ( ( bynames && !strcmp( mrknm, hdnm ) ) 
		|| ( !bynames && hd.id_ >=0 && hd.id_ == mrk.levelID() ))
	    {
		PosCouple pc; pcs += pc;
		pc.z1_ = dtm->getTime(mrk.dah())*1000; 
		pc.z2_ = hd.zpos_;
	    }
	}
    }
}




WellDataMgr::WellDataMgr( const MultiID& wellid )
    : wellid_(wellid)
    , wd_(0)
    , datadeleted_(this)    
{}


WellDataMgr::~WellDataMgr()
{
    if ( wd_ )
	wd_->tobedeleted.remove( mCB(this,WellDataMgr,wellDataDelNotify));
    wd_ = 0;
    Well::MGR().release( wellid_ );
}


void WellDataMgr::wellDataDelNotify( CallBacker* )
{ wd_ = 0; datadeleted_.trigger(); }


Well::Data* WellDataMgr::wellData() const
{
    if ( !wd_ )
    {
	WellDataMgr* self = const_cast<WellDataMgr*>( this );
	Well::Data* wd = Well::MGR().get( wellid_, false );
	self->wd_ = wd;
	if ( wd )
	    wd->tobedeleted.notify( mCB(self,WellDataMgr,wellDataDelNotify) );
    }
    return wd_;
}



DataWriter::DataWriter( Well::Data* wd, const MultiID& wellid )
    : wtr_(0)
    , wd_(wd)
    , wellid_(wellid)  
{
    setWellWriter();
}


DataWriter::~DataWriter()
{
    delete wtr_;
}


void DataWriter::setWellWriter()
{
    IOObj* ioobj = IOM().get( wellid_ );
    if ( ioobj && wd_ )
    {
	wtr_ = new Well::Writer(ioobj->fullUserExpr(true),*wd_ ); 
	delete ioobj;
    }
}


bool DataWriter::writeD2TM() const
{
    return ( wtr_ && wtr_->putD2T() );
}


bool DataWriter::writeLogs( const Well::LogSet& logset ) const
{
    if ( !wd_ ) return false;
    Well::LogSet& wdlogset = const_cast<Well::LogSet&>( wd_->logs() );
    for ( int idx=0; idx<logset.size(); idx++ )
	wdlogset.add( new Well::Log( logset.getLog(idx) ) );
    return ( wtr_ && wtr_->putLogs() );
}


bool DataWriter::writeLogs2Cube( LogData& ld ) const
{
    if ( !wd_ || !wd_->haveD2TModel() ) return false;
    bool allsucceeded = true; 
    for ( int idx=0; idx<ld.logset_.size(); idx++ )
    {
	WellTie::TrackExtractor wtextr( wd_->track(), wd_->d2TModel() );
	if ( !wtextr.execute() )
	    pErrMsg( "unable to extract position" );

	ld.curlog_ = &ld.logset_.getLog( idx );
	ld.curidx_ = idx;
	const int datasz = ld.curlog_->size();
	ld.bids_ = wtextr.getBIDs();

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


Server::Server( const WellTie::Setup& wts )
    : data_(wts)
{
    wdmgr_ = new WellDataMgr( wts.wellid_  );
    wdmgr_->datadeleted_.notify( mCB(this,Server,wellDataDel) );
    Well::Data* w = wdmgr_->wd();
    data_.wd_ = w;
    if ( !w ) return; //close
    if ( w->haveCheckShotModel() && !wts.useexistingd2tm_ )
    {
	Well::Log* wl = w->logs().getLog( data_.sonic() );
	if ( !wl ) return;
	Well::Log* corrlog = new Well::Log( *wl );
	CheckShotCorr cscorr( *corrlog, *w->checkShotModel(), wts.issonic_ );
	w->logs().add( corrlog );
	Well::Log* cslog = new Well::Log( cscorr.csLog() );
	cslog->setName( data_.checkshotlog() );
	w->logs().add( cslog );
    }
    D2TModelMgr::Setup d2ts; 
    d2ts.useexisting_ = wts.useexistingd2tm_; 
    d2ts.currvellog_ = data_.currvellog();
    d2tmgr_ = new D2TModelMgr( *w, d2ts );

    dataplayer_ = new DataPlayer( data_, wts.seisid_, &wts.linekey_ );
    pickmgr_ = new PickSetMgr( data_.pickdata_ );
    hormgr_ = new HorizonMgr( data_.horizons_ );
    hormgr_->resetWD( w );
}


Server::~Server()
{
    delete d2tmgr_;
    delete dataplayer_;
    delete pickmgr_;
    wdmgr_->datadeleted_.remove( mCB(this,Server,wellDataDel) );
    delete wdmgr_;
}


void Server::wellDataDel( CallBacker* )
{
    data_.wd_ = wdmgr_->wd();
    d2tmgr_->resetWD( data_.wd_ );
    hormgr_->resetWD( data_.wd_ );
}


void Server::computeAll()
{
    dataplayer_->computeAll();
}


void Server::computeSynthetics()
{
    dataplayer_->generateSynthetics();
}


void Server::setEstimatedWvlt( float* vals, int sz )
{
    Wavelet& wvlt = data_.estimatedwvlt_;
    sz += sz%2 ? 0 : 1;
    wvlt.reSize( sz );
    memcpy( wvlt.samples(), vals, sz*sizeof(float) );
}

}; //namespace WellTie

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Apr 2009
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "ioman.h"
#include "iostrm.h"
#include "strmprov.h"
#include "survinfo.h"
#include "seisioobjinfo.h"
#include "seistrc.h"

#include "createlogcube.h"
#include "emhorizon2d.h"
#include "emmanager.h"

#include "wavelet.h"
#include "welldata.h"
#include "welld2tmodel.h"
#include "wellextractdata.h"
#include "wellhorpos.h"
#include "welllog.h"
#include "welllogset.h"
#include "wellman.h"
#include "wellmarker.h"
#include "welltrack.h"
#include "wellwriter.h"

#include "welltiecshot.h"
#include "welltiedata.h"
#include "welltieextractdata.h"
#include "welltiesetup.h"
#include "welltietoseismic.h"
#include "welltied2tmodelmanager.h"
#include "welltiepickset.h"

namespace WellTie
{


void DispParams::fillPar( IOPar& iop ) const 
{
    iop.setYN( sKeyIsMarkerDisp(), ismarkerdisp_ );
    iop.setYN( sKeyVwrMarkerDisp(), isvwrmarkerdisp_ );
    iop.setYN( sKeyVwrHorizonDisp(), isvwrhordisp_ );
    iop.setYN( sKeyZInFeet(), iszinft_ );
    iop.setYN( sKeyZInTime(), iszintime_ );	
    iop.setYN( sKeyMarkerFullName(), dispmrkfullnames_ );
    iop.setYN( sKeyHorizonFullName(), disphorfullnames_ );
    mrkdisp_.fillPar( iop );
}


void DispParams::usePar( const IOPar& iop ) 
{
    iop.getYN( sKeyIsMarkerDisp(), ismarkerdisp_ );
    iop.getYN( sKeyVwrMarkerDisp(), isvwrmarkerdisp_ );
    iop.getYN( sKeyVwrHorizonDisp(), isvwrhordisp_ );
    iop.getYN( sKeyZInFeet(), iszinft_ );
    iop.getYN( sKeyZInTime(), iszintime_ );	
    iop.getYN( sKeyMarkerFullName(), dispmrkfullnames_ );
    iop.getYN( sKeyHorizonFullName(), disphorfullnames_ );
    mrkdisp_.usePar( iop );
}


float Data::cDefSeisSr()
{
    return 0.001;
}


Data::Data( const Setup& wts, Well::Data& wdata )
    : logset_(*new Well::LogSet)
    , wd_(&wdata)
    , setup_(wts)
    , initwvlt_(*Wavelet::get(IOM().get( wts.wvltid_)))
    , estimatedwvlt_(*new Wavelet("Estimated wavelet"))
    , seistrc_(*new SeisTrc)
    , synthtrc_(*new SeisTrc)
    , trunner_(0)
{
    const Well::Track& track = wdata.track();
    const Well::D2TModel* d2t = wdata.d2TModel();
    float stoptime = SI().zRange(true).stop;
    const int nrtrackpts = track.nrPoints();
    const float td = track.dah(nrtrackpts-1);
    float tdtime = d2t->getTime( td, track );
    if ( !mIsUdf(tdtime) && tdtime > stoptime )
	stoptime = mCast( float, mNINT32(tdtime/cDefSeisSr())*cDefSeisSr() );

    SeisIOObjInfo oinf( wts.seisid_ );
    if ( oinf.isOK() )
    {
	CubeSampling cs;
	oinf.getRanges( cs );
	if ( cs.zrg.stop > stoptime )
	    stoptime = cs.zrg.stop;
    }

    tracerg_.set( 0.f, stoptime, cDefSeisSr() );
    computeExtractionRange();
    for ( int idx=0; idx<wdata.markers().size(); idx++ )
    {
	dispparams_.allmarkernms_.add( wdata.markers()[idx]->name() );
	dispparams_.mrkdisp_.selmarkernms_.add( wdata.markers()[idx]->name() );
    }

    initwvlt_.reSample( cDefSeisSr() );
}


Data::~Data()
{
    delete trunner_;
    delete &logset_;
    delete &initwvlt_;
    delete &estimatedwvlt_;
    delete &seistrc_;
    delete &synthtrc_;
}


void Data::computeExtractionRange()
{
    if ( !wd_ )
	return;

    const Well::Log* velplog = wd_->logs().getLog( setup_.vellognm_ );
    const Well::Track& track = wd_->track();
    const Well::D2TModel* d2t = wd_->d2TModel();
    if ( !velplog || !d2t || track.isEmpty() )
	return;

    dahrg_ = velplog->dahRange();
    dahrg_.limitTo( track.dahRange() );
    float twtstart = mMAX( 0.f, d2t->getTime( dahrg_.start, track ) );
    float twtstop = d2t->getTime( dahrg_.stop, track );
    twtstart = (float) mNINT32(twtstart/cDefSeisSr()) * cDefSeisSr();
    twtstop = (float) mNINT32(twtstop/cDefSeisSr()) * cDefSeisSr();
    modelrg_ = StepInterval<float>( twtstart, twtstop, cDefSeisSr() );

    dahrg_.start = d2t->getDah( twtstart, track );
    dahrg_.stop = d2t->getDah( twtstop, track );

    twtstart += cDefSeisSr();
    twtstop -= cDefSeisSr();
    reflrg_.set( twtstart, twtstop, cDefSeisSr() );
}




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
		if ( TaskRunner::execute( &tr, *exec ) )
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
	const float zval = 
	    whfinder.findZIntersection()*SI().zDomain().userFactor();
	if ( !mIsUdf( zval ) )
	{
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
		pc.z1_ = dtm->getTime(mrk.dah(), wd_->track())*
		    		      SI().zDomain().userFactor(); 
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
	self->wd_ = Well::MGR().get( wellid_, false );
	if ( wd_ )
	    wd_->tobedeleted.notify( mCB(self,WellDataMgr,wellDataDelNotify) );
    }
    return wd_;
}



DataWriter::DataWriter( Well::Data& wd, const MultiID& wellid )
    : wtr_(0)
    , wd_(&wd)
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
    delete wtr_; wtr_ = 0;
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


bool DataWriter::writeLogs2Cube( LogData& ld, Interval<float> dahrg ) const
{
    if ( ld.logset_.isEmpty() )
	return false;

    Well::Data wd; 
    wd.track() = wd_->track();
    wd.setD2TModel( new Well::D2TModel( *wd_->d2TModel() ) );
    wd.logs().setEmpty(); 
    LogCubeCreator lcr( wd ); 
    ObjectSet<LogCubeCreator::LogCubeData> logdatas;
    for ( int idx=0; idx<ld.logset_.size(); idx++ )
    {
	const Well::Log& log = ld.logset_.getLog( idx );
	wd.logs().add( new Well::Log( log ) );
	BufferString lnm( log.name() );
	int ctxtidx = ld.ctioidxset_.validIdx(idx) ? ld.ctioidxset_[idx] : -1;
	if ( ctxtidx < 0 ) return false;
	CtxtIOObj* ctx = new CtxtIOObj( *ld.seisctioset_[ctxtidx] );
	logdatas += new LogCubeCreator::LogCubeData( lnm, *ctx );
    }
    Well::ExtractParams wep; wep.setFixedRange( dahrg, false );
    lcr.setInput( logdatas, ld.nrtraces_, wep );
    lcr.execute();
    BufferString errmsg = lcr.errMsg();
    return errmsg.isEmpty();
}



Server::Server( const WellTie::Setup& wts )
    : is2d_(wts.is2d_)
    , wellid_(wts.wellid_)  
{
    wdmgr_ = new WellDataMgr( wts.wellid_  );
    wdmgr_->datadeleted_.notify( mCB(this,Server,wellDataDel) );

    Well::Data* wdata = wdmgr_->wd();
    if ( !wdata ) return; //TODO close + errmsg

    // Order below matters
    datawriter_ = new DataWriter( *wdata, wts.wellid_ );
    d2tmgr_ = new D2TModelMgr( *wdata, *datawriter_, wts );
    data_ = new Data( wts, *wdata );
    dataplayer_ = new DataPlayer( *data_, wts.seisid_, &wts.linekey_ );
    pickmgr_ = new PickSetMgr( data_->pickdata_ );
    hormgr_ = new HorizonMgr( data_->horizons_ );

    hormgr_->setWD( wdata );
    d2tmgr_->setWD( wdata );
}


Server::~Server()
{
    delete datawriter_;
    delete d2tmgr_;
    delete dataplayer_;
    delete pickmgr_;
    delete data_;
    wdmgr_->datadeleted_.remove( mCB(this,Server,wellDataDel) );
    delete wdmgr_;
}


void Server::wellDataDel( CallBacker* )
{
    data_->wd_ = wdmgr_->wd();
    d2tmgr_->setWD( data_->wd_ );
    hormgr_->setWD( data_->wd_ );
    datawriter_->setWD( data_->wd_ );
}


bool Server::computeSynthetics( const Wavelet& wvlt )
{
    if ( !dataplayer_ )
	return false;

    if ( !dataplayer_->computeSynthetics(wvlt) )
	{ errmsg_ = dataplayer_->errMSG(); return false; }

    return true;
}


bool Server::extractSeismics()
{
    if ( !dataplayer_ )
	return false;

    if ( !dataplayer_->extractSeismics() )
	{ errmsg_ = dataplayer_->errMSG(); return false; }

    return true;
}


bool Server::updateSynthetics( const Wavelet& wvlt )
{
    if ( !dataplayer_ )
	return false;

    if ( !dataplayer_->doFastSynthetics(wvlt) )
	{ errmsg_ = dataplayer_->errMSG(); return false; }

    return true;
}


bool Server::computeAdditionalInfo( const Interval<float>& zrg )
{
    if ( !dataplayer_ )
	return false;

    if ( !dataplayer_->computeAdditionalInfo( zrg ) )
	{ errmsg_ = dataplayer_->errMSG(); return false; }

    return true;
}


bool Server::computeCrossCorrelation()
{
    if ( !dataplayer_ )
	return false;

    if ( !dataplayer_->computeCrossCorrelation() )
	{ errmsg_ = dataplayer_->errMSG(); return false; }

    return true;
}


bool Server::computeEstimatedWavelet( int newsz )
{
    if ( !dataplayer_ )
	return false;

    if ( !dataplayer_->computeEstimatedWavelet( newsz ) )
	{ errmsg_ = dataplayer_->errMSG(); return false; }

    return true;
}


void Server::setCrossCorrZrg( const Interval<float>& zrg )
{
    if ( !dataplayer_ )
	return;

    dataplayer_->setCrossCorrZrg( zrg );
}


bool Server::hasSynthetic() const
{
    if ( !dataplayer_ )
	return false;

    return dataplayer_->isOKSynthetic() && !wellid_.isEmpty();
}


bool Server::hasSeismic() const
{
    if ( !dataplayer_ )
	return false;

    return dataplayer_->isOKSeismic();
}


bool Server::doSeismic() const
{
    if ( !dataplayer_ )
	return false;

    return dataplayer_->hasSeisId();
}


void Server::updateExtractionRange()
{
    if ( !data_ )
	return;

    data_->computeExtractionRange();
}

}; //namespace WellTie

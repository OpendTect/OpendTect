/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Apr 2009
________________________________________________________________________

-*/

#include "ioman.h"
#include "iostrm.h"
#include "seisbuf.h"
#include "seisioobjinfo.h"
#include "seistrc.h"
#include "strmprov.h"
#include "syntheticdataimpl.h"
#include "survinfo.h"

#include "createlogcube.h"
#include "emhorizon2d.h"
#include "emmanager.h"

#include "wavelet.h"
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

const char* DispParams::sKeyIsMarkerDisp()
{ return "Display Markers on Well Display"; }

const char* DispParams::sKeyVwrMarkerDisp()
{ return "Display Markers on 2D Viewer"; }

const char* DispParams::sKeyVwrHorizonDisp()
{ return "Display Horizon on 2D Viewer"; }

const char* DispParams::sKeyZInFeet()
{ return "Z in Feet"; }

const char* DispParams::sKeyZInTime()
{ return "Z in Time"; }

const char* DispParams::sKeyMarkerFullName()
{ return "Display markers full name"; }

const char* DispParams::sKeyHorizonFullName()
{ return "Display horizon full name"; }


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

static const int welltieseqnr = 0;

Data::Data( const Setup& wts, Well::Data& wdata )
    : logset_(*new Well::LogSet)
    , wd_(&wdata)
    , setup_(wts)
    , initwvlt_(*Wavelet::get(IOM().get( wts.wvltid_)))
    , estimatedwvlt_(*new Wavelet("Deterministic wavelet"))
    , seistrcs_(*new SeisTrcBuf(true))
{
    const Well::Track& track = wdata.track();
    const Well::D2TModel* d2t = wdata.d2TModel();
    float stoptime = SI().zRange(true).stop;
    const float td = track.td();
    float tdtime = d2t->getTime( td, track );
    if ( !mIsUdf(tdtime) && tdtime > stoptime )
	stoptime = mCast( float, mNINT32(tdtime/cDefSeisSr())*cDefSeisSr() );

    SeisIOObjInfo oinf( wts.seisid_ );
    if ( oinf.isOK() )
    {
	TrcKeyZSampling cs;
	oinf.getRanges( cs );
	if ( cs.zsamp_.stop > stoptime )
	    stoptime = cs.zsamp_.stop;
    }

    tracerg_.set( 0.f, stoptime, cDefSeisSr() );
    computeExtractionRange();
    BufferStringSet emptynms;
    for ( int idx=0; idx<wdata.markers().size(); idx++ )
	dispparams_.allmarkernms_.add( wdata.markers()[idx]->name() );

    dispparams_.mrkdisp_.setMarkerNms( dispparams_.allmarkernms_, true );
    dispparams_.mrkdisp_.setMarkerNms( emptynms, false );
    initwvlt_.reSample( cDefSeisSr() );
    BufferString wvltnm( estimatedwvlt_.name(), " from well ", wdata.name() );
    estimatedwvlt_.setName( wvltnm );
}


Data::~Data()
{
    delete trunner_;
    delete &logset_;
    delete &initwvlt_;
    delete &estimatedwvlt_;
    delete &seistrcs_;
}


const SeisTrc* Data::getTrc( bool synth, int ioff ) const
{
    return mSelf().getTrc( synth, ioff );
}


const SeisTrc* Data::getRealTrc( int ioff ) const
{
    return mSelf().getRealTrc( ioff );
}


SeisTrc* Data::getRealTrc( int ioff )
{
    return seistrcs_.validIdx(ioff) ? seistrcs_.get( ioff ) : nullptr;
}


const SeisTrc* Data::getSynthTrc( int ioff ) const
{
    const int seqnr = 0;
    if ( postsd_ )
	return postsd_->getTrace( seqnr );
    if ( presd_ )
	return presd_->getTrace( seqnr, &ioff );

    return nullptr;
}


SeisTrc* Data::getTrc( bool synth, int ioff )
{
    return synth ? getSynthTrc( ioff ) : getRealTrc( ioff );
}


SeisTrc* Data::getSynthTrc( int ioff )
{
    const SeisTrc* trc = const_cast<const Data&>( *this ).getSynthTrc( ioff );
    return const_cast<SeisTrc*>( trc );
}


const SyntheticData* Data::getSynthetics() const
{
    return static_cast<const SyntheticData*>( synthdp_.ptr() );
}


const ReflectivityModelBase* Data::getRefModel() const
{
    const int seqnr = 0;
    const SyntheticData* sd = getSynthetics();
    return sd ? sd->getRefModel( seqnr ) : nullptr;
}


float Data::getZStep() const
{
    const SeisTrc* trc = getRealTrc();
    return trc ? trc->info().sampling.step : mUdf(float);
}


void Data::setRealTrc( const SeisTrc* trc, int ioff )
{
    if ( seistrcs_.validIdx(ioff) )
	delete seistrcs_.replace( ioff, const_cast<SeisTrc*>( trc ) );
    else
	seistrcs_.insert( const_cast<SeisTrc*>( trc ), ioff );
}


void Data::setSynthetics( const SyntheticData* sd )
{
    synthdp_ = sd;
    mDynamicCast(const PostStackSyntheticData*,postsd_,sd);
    mDynamicCast(const PreStackSyntheticData*,presd_,sd);
}


void Data::reverseTrc( bool synth, int ioff )
{
    SeisTrc* trc = getTrc( synth, ioff );
    if ( trc )
	trc->reverse();
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
    twtstart = Math::Ceil( twtstart / cDefSeisSr() ) * cDefSeisSr();
    twtstop = Math::Floor( twtstop / cDefSeisSr() ) * cDefSeisSr();
    modelrg_ = ZSampling( twtstart, twtstop, cDefSeisSr() );

    dahrg_.start = d2t->getDah( twtstart, track );
    dahrg_.stop = d2t->getDah( twtstop, track );

    twtstart += cDefSeisSr();
    twtstop -= cDefSeisSr();
    reflrg_.set( twtstart, twtstop, cDefSeisSr() );
}



HorizonMgr::HorizonMgr( TypeSet<Marker>& hor )
    : horizons_(hor)
{
}


void HorizonMgr::setUpHorizons( const TypeSet<MultiID>& horids,
				uiString& errms, TaskRunner& taskr )
{
    horizons_.erase();
    if ( !wd_ )
	return;

    EM::EMManager& em = EM::EMM();
    for ( int idx=0; idx<horids.size(); idx++ )
    {
	PtrMan<IOObj> ioobj = IOM().get( horids[idx] );
	if ( !ioobj )
	{
	    errms.append(tr("Cannot get database entry for "
			    "selected horizon"));
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
		if ( TaskRunner::execute( &taskr, *exec ) )
		    success = true;
	    }
	    if ( success )
	    {
		emid = EM::EMM().getObjectID( horids[idx] );
		emobj = EM::EMM().getObject( emid );
	    }
	    else
	    {
		errms = tr("Cannot load %1.")
		      .arg(ioobj->name());
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
    const Well::D2TModel* dtm = wd_ ? wd_->d2TModel() : nullptr;
    if ( !dtm ) return;
    for ( int idmrk=0; idmrk<wd_->markers().size(); idmrk++ )
    {
	const Well::Marker& mrk = *wd_->markers()[idmrk];
	for ( int idhor=0; idhor<horizons_.size(); idhor++ )
	{
	    Marker& hd = horizons_[idhor];
	    const BufferString mrknm( mrk.name() );
	    if ( ( bynames && mrknm == hd.name_ )
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
    , datadeleted_(this)
{}


WellDataMgr::~WellDataMgr()
{
}


void WellDataMgr::wellDataDelNotify( CallBacker* )
{ wd_ = nullptr; datadeleted_.trigger(); }


const Well::Data* WellDataMgr::wellData() const
{
    return mSelf().wd();
}


Well::Data* WellDataMgr::wd()
{
    if ( !wd_ )
	wd_ = Well::MGR().get( wellid_ );

    return wd_;
}



DataWriter::DataWriter( Well::Data& wd, const MultiID& wellid )
    : wd_(&wd)
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
    deleteAndZeroPtr( wtr_ );
    IOObj* ioobj = IOM().get( wellid_ );
    if ( ioobj && wd_ )
    {
	wtr_ = new Well::Writer( *ioobj, *wd_ );
	delete ioobj;
    }
}


bool DataWriter::writeD2TM() const
{
    return wtr_ && wtr_->putD2T();
}


bool DataWriter::writeLogs( const Well::LogSet& logset, bool todisk ) const
{
    if ( !wd_ || !wtr_ )
	return false;

    Well::LogSet& wdlogset = const_cast<Well::LogSet&>( wd_->logs() );
    for ( int idx=0; idx<logset.size(); idx++ )
    {
	Well::Log* log = new Well::Log( logset.getLog(idx) );
	wdlogset.add( log );
    }

    if ( todisk )
	if ( !wtr_->putLogs() )
	    return false;

    return true;
}


bool DataWriter::removeLogs( const Well::LogSet& logset ) const
{
    if ( !wd_ )
	return false;

    Well::LogSet& wdlogset = const_cast<Well::LogSet&>( wd_->logs() );
    int nrlogs = wdlogset.size();
    for ( int idx=0; idx<logset.size(); idx++ )
    {
	nrlogs--;
	wdlogset.remove( nrlogs );
    }

    return true;
}



Server::Server( const WellTie::Setup& wts )
    : wellid_(wts.wellid_)
{
    wdmgr_ = new WellDataMgr( wts.wellid_  );
    mAttachCB( wdmgr_->datadeleted_, Server::wellDataDel );

    Well::Data* wdata = wdmgr_->wd();
    if ( !wdata ) return; //TODO close + errmsg

    // Order below matters
    datawriter_ = new DataWriter( *wdata, wts.wellid_ );
    d2tmgr_ = new D2TModelMgr( *wdata, *datawriter_, wts );
    data_ = new Data( wts, *wdata );
    dataplayer_ = new DataPlayer( *data_, wts.seisid_, wts.linenm_ );
    pickmgr_ = new PickSetMgr( data_->pickdata_ );
    hormgr_ = new HorizonMgr( data_->horizons_ );

    hormgr_->setWD( wdata );
    d2tmgr_->setWD( wdata );
}


Server::~Server()
{
    detachAllNotifiers();
    delete datawriter_;
    delete d2tmgr_;
    delete dataplayer_;
    delete pickmgr_;
    delete data_;
    delete wdmgr_;
}


void Server::wellDataDel( CallBacker* )
{
    data_->wd_ = wdmgr_->wd();
    d2tmgr_->setWD( data_->wd_ );
    hormgr_->setWD( data_->wd_ );
    datawriter_->setWD( data_->wd_ );
}


bool Server::setNewWavelet( const MultiID& mid )
{
    if ( !data_ )
	return false;

    PtrMan<IOObj> ioobj = IOM().get( mid );
    if ( !ioobj ) return false;

    PtrMan<Wavelet> wvlt = Wavelet::get( ioobj );
    if ( !wvlt ) return false;

    wvlt->reSample( Data::cDefSeisSr() );
    data_->initwvlt_ = *wvlt;
    data_->initwvlt_.setName( wvlt->name() );
    const_cast<WellTie::Setup&>(data_->setup()).wvltid_ = mid;
    return updateSynthetics( data_->initwvlt_ );
}


bool Server::computeSynthetics( const Wavelet& wvlt )
{
    if ( !dataplayer_ )
	return false;

    if ( !dataplayer_->computeSynthetics(wvlt) )
	{ errmsg_ = dataplayer_->errMsg(); return false; }

    handleDataPlayerWarning();

    return true;
}


bool Server::extractSeismics()
{
    if ( !dataplayer_ )
	return false;

    if ( !dataplayer_->extractSeismics() )
	{ errmsg_ = dataplayer_->errMsg(); return false; }

    handleDataPlayerWarning();

    return true;
}


bool Server::updateSynthetics( const Wavelet& wvlt )
{
    if ( !dataplayer_ )
	return false;

    if ( !dataplayer_->doFastSynthetics(wvlt) )
	{ errmsg_ = dataplayer_->errMsg(); return false; }

    handleDataPlayerWarning();

    return true;
}


bool Server::computeAdditionalInfo( const Interval<float>& zrg )
{
    if ( !dataplayer_ )
	return false;

    if ( !dataplayer_->computeAdditionalInfo( zrg ) )
	{ errmsg_ = dataplayer_->errMsg(); return false; }

    handleDataPlayerWarning();

    return true;
}


bool Server::computeCrossCorrelation()
{
    if ( !dataplayer_ )
	return false;

    if ( !dataplayer_->computeCrossCorrelation() )
	{ errmsg_ = dataplayer_->errMsg(); return false; }

    handleDataPlayerWarning();

    return true;
}


bool Server::computeEstimatedWavelet( int newsz )
{
    if ( !dataplayer_ )
	return false;

    if ( !dataplayer_->computeEstimatedWavelet( newsz ) )
	{ errmsg_ = dataplayer_->errMsg(); return false; }

    handleDataPlayerWarning();

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


void Server::handleDataPlayerWarning() const
{
    if ( !dataplayer_ || dataplayer_->warnMsg().isEmpty() )
	return;

    warnmsg_ = dataplayer_->warnMsg();
}

} // namespace WellTie

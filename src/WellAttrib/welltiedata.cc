/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Apr 2009
________________________________________________________________________

-*/

#include "iostrm.h"
#include "survinfo.h"
#include "seisioobjinfo.h"
#include "seistrc.h"
#include "synthseisdataset.h"

#include "createlogcube.h"
#include "emhorizon2d.h"
#include "emmanager.h"

#include "waveletmanager.h"
#include "welldata.h"
#include "welld2tmodel.h"
#include "wellextractdata.h"
#include "wellhorpos.h"
#include "welllog.h"
#include "welllogset.h"
#include "wellmanager.h"
#include "wellmarker.h"
#include "welltrack.h"

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


Data::Data( const Setup& wts, Well::Data& wdata )
    : logset_(*new Well::LogSet)
    , wd_(&wdata)
    , setup_(wts)
    , initwvlt_(new Wavelet)
    , estimatedwvlt_(new Wavelet("Deterministic wavelet"))
    , seistrc_(*new SeisTrc)
    , synthtrc_(*new SeisTrc)
    , trunner_(0)
    , sd_(0 )
{
    ConstRefMan<Wavelet> wvlt = WaveletMGR().fetch( wts.wvltid_ );
    if ( wvlt )
	*initwvlt_ = *wvlt;
    else
	{ pErrMsg( "Wvlt ID invalid" ); initwvlt_ = new Wavelet( true, 25.f ); }

    const Well::Track& track = wdata.track();
    const Well::D2TModel& d2t = wdata.d2TModel();
    float stoptime = SI().zRange().stop;
    const float td = track.td();
    float tdtime = d2t.getTime( td, track );
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
    const Well::MarkerSet& markers = wdata.markers();
    Well::MarkerSetIter miter( markers );
    while( miter.next() )
    {
	const BufferString markername = miter.markerName();
	dispparams_.allmarkernms_.add( markername );
	dispparams_.mrkdisp_.addSelMarkerName( markername );
    }

    initwvlt_->reSample( cDefSeisSr() );
    BufferString wvltnm( estimatedwvlt_->name(), " from well ", wdata.name() );
    estimatedwvlt_->setName( wvltnm );
}


Data::~Data()
{
    delete trunner_;
    delete &logset_;
    delete &seistrc_;
    delete &synthtrc_;
}


void Data::computeExtractionRange()
{
    if ( !wd_ )
	return;

    const Well::Log* velplog = wd_->logs().getLogByName( setup_.vellognm_ );
    const Well::Track& track = wd_->track();
    const Well::D2TModel& d2t = wd_->d2TModel();
    if ( !velplog || d2t.isEmpty() || track.isEmpty() )
	return;

    dahrg_ = velplog->dahRange();
    dahrg_.limitTo( track.dahRange() );
    float twtstart = mMAX( 0.f, d2t.getTime( dahrg_.start, track ) );
    float twtstop = d2t.getTime( dahrg_.stop, track );
    twtstart = Math::Ceil( twtstart / cDefSeisSr() ) * cDefSeisSr();
    twtstop = Math::Floor( twtstop / cDefSeisSr() ) * cDefSeisSr();
    modelrg_ = StepInterval<float>( twtstart, twtstop, cDefSeisSr() );

    dahrg_.start = d2t.getDah( twtstart, track );
    dahrg_.stop = d2t.getDah( twtstop, track );

    twtstart += cDefSeisSr();
    twtstop -= cDefSeisSr();
    reflrg_.set( twtstart, twtstop, cDefSeisSr() );
}




void HorizonMgr::setUpHorizons( const DBKeySet& horids,
				uiString& errms, TaskRunner& taskr )
{
    horizons_.erase();
    if ( !wd_ ) return;
    EM::ObjectManager& mgr = EM::MGR();
    ExistingTaskRunnerProvider trprov( &taskr );
    for ( int idx=0; idx<horids.size(); idx++ )
    {
	PtrMan<IOObj> ioobj = horids[idx].getIOObj();
	if ( !ioobj )
	{
	    errms.appendPhrase(tr("Cannot get database entry for "
			    "selected horizon"));
	    return;
	}

	ConstRefMan<EM::Object> emobj = mgr.fetch( horids[idx], trprov );
	if ( !emobj )
	    { errms = ioobj->phrCannotLoadObj(); return; }

	mDynamicCastGet(const EM::Horizon*,hor,emobj.ptr())
	if ( !hor )
	    continue;
	WellHorIntersectFinder whfinder( wd_->track(), &wd_->d2TModel() );
	whfinder.setHorizon( horids[idx] );
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
    const Well::D2TModel* dtm = wd_ ? &wd_->d2TModel() : 0;
    if ( dtm && dtm->isEmpty() )
	dtm = 0;

    const Well::MarkerSet& markers = wd_->markers();
    Well::MarkerSetIter miter( markers );
    while( miter.next() )
    {
	const Well::Marker& mrk = miter.get();
	for ( int idhor=0; idhor<horizons_.size(); idhor++ )
	{
	    Marker& hd = horizons_[idhor];
	    const BufferString mrknm( mrk.name() );
	    if ( ( bynames && mrknm == hd.name_ )
		|| ( !bynames && hd.id_.isValid() && hd.id_ == mrk.levelID() ))
	    {
		PosCouple pc; pcs += pc;
		if ( dtm )
		    pc.z1_ = dtm->getTime(mrk.dah(), wd_->track())*
					  SI().zDomain().userFactor();
		else
		    pc.z1_ = wd_->track().valueAt(mrk.dah());
		pc.z2_ = hd.zpos_;
	    }
	}
    }
}


DataWriter::DataWriter( Well::Data& wd, const DBKey& wellid )
    : wd_(&wd)
    , wellid_(wellid)
{}


bool DataWriter::writeD2TM() const
{
    if ( !wd_ ) return false;
    SilentTaskRunnerProvider trprov;
    const uiRetVal uirv = Well::MGR().store( *wd_, wellid_, trprov );
    if ( !uirv.isOK() )
	{ errmsg_ = uirv; return false; }

    return true;
}


bool DataWriter::writeLogs( const Well::LogSet& logset, bool todisk ) const
{
    if ( !wd_ ) return false;
    Well::LogSet& wdlogset = const_cast<Well::LogSet&>( wd_->logs() );
    Well::LogSetIter iter( logset );
    while ( iter.next() )
	wdlogset.add( new Well::Log( iter.log() ) );
    iter.retire();

    SilentTaskRunnerProvider trprov;
    if ( todisk )
    {
	const uiRetVal uirv = Well::MGR().store( *wd_, wellid_, trprov );
	if ( !uirv.isOK() )
	    { errmsg_ = uirv; return false; }
    }

    return true;
}


bool DataWriter::removeLogs( const Well::LogSet& logset ) const
{
    if ( !wd_ ) return false;
    Well::LogSet& wdlogset = const_cast<Well::LogSet&>( wd_->logs() );
    Well::LogSetIter iter( logset );
    while ( iter.next() )
	wdlogset.removeByName( iter.log().name() );

    return true;
}



Server::Server( const WellTie::Setup& wts )
    : wellid_(wts.wellid_)
{
    wd_ = Well::MGR().fetchForEdit( wts.wellid_ );
    if ( !wd_ )
	return; //TODO close + errmsg

    // Order below matters
    datawriter_ = new DataWriter( *wd_, wts.wellid_ );
    d2tmgr_ = new D2TModelMgr( *wd_, *datawriter_, wts );
    data_ = new Data( wts, *wd_ );
    dataplayer_ = new DataPlayer( *data_, wts.seisid_, wts.linenm_ );
    pickmgr_ = new PickSetMgr( data_->pickdata_ );
    hormgr_ = new HorizonMgr( data_->horizons_ );

    hormgr_->setWD( wd_ );
    d2tmgr_->setWD( wd_ );
}


Server::~Server()
{
    detachAllNotifiers();
    delete datawriter_;
    delete d2tmgr_;
    delete dataplayer_;
    delete pickmgr_;
    delete data_;
}


bool Server::setNewWavelet( const DBKey& key )
{
    if ( !data_ ) return false;

    ConstRefMan<Wavelet> wvlt = WaveletMGR().fetch( key );
    if ( !wvlt ) return false;

    *data_->initwvlt_ = *wvlt;
    data_->initwvlt_->reSample( Data::cDefSeisSr() );
    const_cast<WellTie::Setup&>(data_->setup()).wvltid_ = key;
    return updateSynthetics( *data_->initwvlt_ );
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

    return dataplayer_->isOKSynthetic() && wellid_.isValid();
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

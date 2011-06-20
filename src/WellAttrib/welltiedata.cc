/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Apr 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: welltiedata.cc,v 1.52 2011-06-20 11:55:52 cvsbruno Exp $";

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

static const char* sKeyIsCheckShotDisp = "Display Check-Shot";
static const char* sKeyIsMarkerDisp = "Display Markers on Well Display";
static const char* sKeyVwrMarkerDisp = "Display Markers on 2D Viewer";
static const char* sKeyVwrHorizonDisp = "Display Horizon on 2D Viewer";
static const char* sKeyZInFeet	= "Z in Feet";
static const char* sKeyZInTime	= "Z in Time";
static const char* sKeyMarkerFullName	= "Dilplay markers full name";
static const char* sKeyHorizonFullName	= "Dilplay horizon full name";


namespace WellTie
{


void DispParams::fillPar( IOPar& iop ) const 
{
    iop.setYN( sKeyIsCheckShotDisp, iscsdisp_ );
    iop.setYN( sKeyIsMarkerDisp, ismarkerdisp_ );
    iop.setYN( sKeyVwrMarkerDisp, isvwrmarkerdisp_ );
    iop.setYN( sKeyVwrHorizonDisp, isvwrhordisp_ );
    iop.setYN( sKeyZInFeet, iszinft_ );
    iop.setYN( sKeyZInTime, iszintime_ );	
    iop.setYN( sKeyMarkerFullName, dispmrkfullnames_ );
    iop.setYN( sKeyHorizonFullName, disphorfullnames_ );
    mrkdisp_.fillPar( iop );
}


void DispParams::usePar( const IOPar& iop ) 
{
    iop.getYN( sKeyIsCheckShotDisp, iscsdisp_ );
    iop.getYN( sKeyIsMarkerDisp, ismarkerdisp_ );
    iop.getYN( sKeyVwrMarkerDisp, isvwrmarkerdisp_ );
    iop.getYN( sKeyVwrHorizonDisp, isvwrhordisp_ );
    iop.getYN( sKeyZInFeet, iszinft_ );
    iop.getYN( sKeyZInTime, iszintime_ );	
    iop.getYN( sKeyMarkerFullName, dispmrkfullnames_ );
    iop.getYN( sKeyHorizonFullName, disphorfullnames_ );
    mrkdisp_.usePar( iop );
}    



Data::Data( const Setup& wts, Well::Data& w)
    : logset_(*new Well::LogSet)  
    , wd_(&w)  
    , setup_(wts)  
    , iscscorr_(false)
    , initwvlt_(*Wavelet::get(IOM().get( wts.wvltid_)))
    , estimatedwvlt_(*new Wavelet(initwvlt_))
    , isinitwvltactive_(true)					     
    , timeintv_(SI().zRange(true))
    , seistrc_(*new SeisTrc)  
    , synthtrc_(*new SeisTrc)  
    , trunner_(0)
{
    estimatedwvlt_.setName( "Estimated wavelet" );
    for ( int idx=0; idx<w.markers().size(); idx++ )
    {
	dispparams_.allmarkernms_.add( w.markers()[idx]->name() );
	dispparams_.mrkdisp_.selmarkernms_.add( w.markers()[idx]->name() );
    }
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

const char* Data::sonic() const
{ return iscscorr_ ? setup_.corrvellognm_ : setup_.vellognm_; }

const char* Data::corrsonic() const
{ return setup_.corrvellognm_; }

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

void Data::setIsCSCorr( bool iscs )
{ iscscorr_ = iscs; }


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

    HorSampling hrg( false );
    for ( int idbid=0; idbid<ld.bids_.size(); idbid++ )
	hrg.include( ld.bids_[idbid] );

    BinID bidvar( ld.nrtraces_-1, ld.nrtraces_-1 );
    hrg.stop += bidvar;
    hrg.start -= bidvar;
    hrg.snapToSurvey();

    SeisTrc trc( SI().zRange(true).nrSteps() );
    trc.info().sampling.step = SI().zStep();
    const Well::D2TModel* d2t = wd_ ? wd_->d2TModel() : 0;
    if ( !d2t )
	return false;
    for ( int idx=0; idx<trc.size(); idx++ )
    {
	const float dah = d2t->getDah( trc.info().sampling.atIndex( idx )  );
	trc.set( idx, ld.curlog_->getValue( dah, true ),0 );
    }
    HorSamplingIterator hsit( hrg );
    bool succeeded = true;
    BinID bid;
    while ( hsit.next( bid ) )
    {
	trc.info().binid = bid;
	if ( !writer.put(trc) )
	    { pErrMsg( "cannot write new trace" ); succeeded = false; }
    }

    return succeeded;
}


Server::Server( const WellTie::Setup& wts )
    : is2d_(wts.is2d_)
{
    wdmgr_ = new WellDataMgr( wts.wellid_  );
    wdmgr_->datadeleted_.notify( mCB(this,Server,wellDataDel) );

    Well::Data* w = wdmgr_->wd();
    if ( !w ) return; //TODO close + errmsg

    data_ = new Data( wts, *w );
    datawriter_ = new DataWriter( *w, wts.wellid_ );

    if ( w->haveCheckShotModel() && !wts.useexistingd2tm_ )
    {
	Well::Log* wl = w->logs().getLog( data_->sonic() );
	if ( !wl ) return;
	Well::Log* corrlog = new Well::Log( *wl );
	CheckShotCorr cscorr( *corrlog, *w->checkShotModel(), wts.issonic_ );
	corrlog->setName( data_->corrsonic() );
	w->logs().add( corrlog );
	data_->setIsCSCorr( true );

	Well::Log* cslog = new Well::Log( cscorr.csLog() );
	cslog->setName( data_->checkshotlog() );
	w->logs().add( cslog );
    }

    D2TModelMgr::Setup s(wts.useexistingd2tm_,wts.issonic_,data_->sonic()); 
    d2tmgr_ = new D2TModelMgr( *w, *datawriter_, s );
    dataplayer_ = new DataPlayer( *data_, wts.seisid_, &wts.linekey_ );
    pickmgr_ = new PickSetMgr( data_->pickdata_ );
    hormgr_ = new HorizonMgr( data_->horizons_ );

    hormgr_->setWD( w );
    d2tmgr_->setWD( w );
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


bool Server::computeAll()
{
    if ( !dataplayer_->computeAll() )
	{ errmsg_ = dataplayer_->errMSG(); return false; }
    return true;
}


bool Server::computeSynthetics()
{
    if ( !dataplayer_->generateSynthetics() )
	{ errmsg_ = dataplayer_->errMSG(); return false; }
    return true;
}


void Server::setEstimatedWvlt( float* vals, int sz )
{
    Wavelet& wvlt = data_->estimatedwvlt_;
    memcpy( wvlt.samples(), vals, sz*sizeof(float) );
}

}; //namespace WellTie

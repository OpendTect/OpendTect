/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Feb 2010
-*/

static const char* rcsID mUnusedVar = "$Id: uisynthtorealscale.cc,v 1.19 2012-08-10 03:50:07 cvsaneesh Exp $";

#include "uisynthtorealscale.h"

#include "emhorizon3d.h"
#include "emmanager.h"
#include "emsurfacetr.h"
#include "survinfo.h"
#include "polygon.h"
#include "position.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "seisbuf.h"
#include "seisread.h"
#include "seisselectionimpl.h"
#include "stratlevel.h"
#include "statparallelcalc.h"
#include "picksettr.h"
#include "wavelet.h"
#include "ioman.h"

#include "uislider.h"
#include "uistratseisevent.h"
#include "uiseissel.h"
#include "uiseparator.h"
#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
#include "uihistogramdisplay.h"
#include "uiaxishandler.h"
#include "uigeninput.h"
#include "uibutton.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uitaskrunner.h"

static const char* sKeyAmplVals = "       [Amplitude values]       ";
static const char* sKeyRMSVals = "[Amplitude RMS values]";


#define mErrRetBool(s)\
{ uiMSG().error(s); return false; }\

#define mErrRet(s)\
{ uiMSG().error(s); return; }\

class uiSynthToRealScaleStatsDisp : public uiGroup
{
public:

uiSynthToRealScaleStatsDisp( uiParent* p, const char* nm, bool left )
    : uiGroup(p,nm)
    , usrval_(mUdf(float))
    , usrValChanged(this)
    , markerlineitem_(0)
{
    uiFunctionDisplay::Setup su;
    su.annoty( false ).noyaxis( true ).noy2axis( true ).drawgridlines( false );
    dispfld_ = new uiHistogramDisplay( this, su );
    dispfld_->xAxis()->setName( "" );
    dispfld_->setPrefWidth( 260 );
    dispfld_->setPrefHeight( GetGoldenMinor(260) );

    valueslider_ = new uiSliderExtra( this,
	    	uiSliderExtra::Setup("Value"), "Value" );
    valueslider_->sldr()->valueChanged.notify(
			    mCB(this,uiSynthToRealScaleStatsDisp,sliderChgCB) );
    avgfld_ = new uiGenInput( this, "", FloatInpSpec() );
    valueslider_->attach( leftAlignedBelow, dispfld_ );
    avgfld_->attach( rightOf, valueslider_ );
    avgfld_->valuechanging.notify(mCB(this,uiSynthToRealScaleStatsDisp,avgChg));

    uiLabel* lbl = new uiLabel( this, nm );
    dispfld_->attach( centeredBelow, lbl );
    setHAlignObj( dispfld_ );
}

void updateSlider( float val )
{
    const uiAxisHandler* xaxis = dispfld_->xAxis();
    const StepInterval<float> xrg = xaxis->range();
    valueslider_->sldr()->setScale( xrg.step/1000, 0 );
    valueslider_->sldr()->setInterval( xrg );
    valueslider_->sldr()->setValue( val );
    drawMarkerLine( val );
}

void avgChg( CallBacker* )
{
    usrval_ = avgfld_->getfValue();
    dispfld_->setMarkValue( usrval_, true );
    usrValChanged.trigger();
}

void sliderChgCB( CallBacker* )
{
    const float val = valueslider_->sldr()->getValue();
    drawMarkerLine( val );
    avgfld_->setValue( val );
}

void drawMarkerLine( float val )
{
    const uiAxisHandler* xaxis = dispfld_->xAxis();
    const int valx = xaxis->getPix( val );
    if ( valx < xaxis->getPix( xaxis->range().start) ||
	 valx > xaxis->getPix( xaxis->range().stop) )
	return;
    
    const uiAxisHandler* yaxis = dispfld_->yAxis(false);
    const int valytop = yaxis->getPix( yaxis->range().start );
    const int valybottom = yaxis->getPix( yaxis->range().stop );

    if ( !markerlineitem_ )
    {
	LineStyle ls( LineStyle::Solid, 2, Color(0,255,0) );
	markerlineitem_ = dispfld_->scene().addItem( new uiLineItem() );
	markerlineitem_->setPenStyle( ls );
	markerlineitem_->setZValue( 3 );
    }

    markerlineitem_->setLine( valx, valytop, valx, valybottom );
}

    float		usrval_;

    uiHistogramDisplay*	dispfld_;
    uiGenInput*		avgfld_;
    uiSliderExtra*	valueslider_;
    uiLineItem*		markerlineitem_;
    Notifier<uiSynthToRealScaleStatsDisp>	usrValChanged;

};


uiSynthToRealScale::uiSynthToRealScale( uiParent* p, bool is2d, SeisTrcBuf& tb,
					const MultiID& wid, const char* lvlnm )
    : uiDialog(p,Setup("Scale synthetics","Determine scaling for synthetics",
			"110.2.1"))
    , seisev_(*new Strat::SeisEvent)
    , is2d_(is2d)
    , synth_(tb)
    , inpwvltid_(wid)
    , seisfld_(0)
    , horizon_(0)
    , horiter_(0)
    , polygon_(0)
{
#define mNoDealRet(cond,msg) \
    if ( cond ) \
	{ new uiLabel( this, msg ); return; }
    mNoDealRet( Strat::LVLS().isEmpty(), "No Stratigraphic Levels defined" )
    mNoDealRet( tb.isEmpty(), "Please generate models first" )
    mNoDealRet( inpwvltid_.isEmpty(), "Please create a Wavelet first" )
    mNoDealRet( !lvlnm || !*lvlnm || (*lvlnm == '-' && *(lvlnm+1) == '-'),
	    "Please select Stratigraphic Level\nbefore starting this tool" )

    BufferString wintitle( "Determine scaling for synthetics using '" );
    wintitle.add( IOM().nameOf( inpwvltid_ ) ).add( "'" );
    setTitleText( wintitle );

    uiSeisSel::Setup sssu( is2d_, false );
    seisfld_ = new uiSeisSel( this, uiSeisSel::ioContext(sssu.geom_,true),
	    		      sssu );

    const IOObjContext horctxt( mIOObjContext(EMHorizon3D) );
    uiIOObjSel::Setup horsu( BufferString("Horizon for '",lvlnm,"'") );
    horfld_ = new uiIOObjSel( this, horctxt, horsu );
    horfld_->attach( alignedBelow, seisfld_ );

    IOObjContext polyctxt( mIOObjContext(PickSet) );
    polyctxt.toselect.require_.set( sKey::Type, sKey::Polygon );
    uiIOObjSel::Setup polysu( "Within Polygon" ); polysu.optional( true );
    polyfld_ = new uiIOObjSel( this, polyctxt, polysu );
    polyfld_->attach( alignedBelow, horfld_ );

    uiStratSeisEvent::Setup ssesu( true );
    ssesu.fixedlevel( Strat::LVLS().get(lvlnm) );
    evfld_ = new uiStratSeisEvent( this, ssesu );
    evfld_->attach( alignedBelow, polyfld_ );

    uiPushButton* gobut = new uiPushButton( this, "&Extract amplitudes",
	    			mCB(this,uiSynthToRealScale,goPush), true );
    gobut->attach( alignedBelow, evfld_ );

    uiSeparator* sep = new uiSeparator( this, "separator" );
    sep->attach( stretchedBelow, gobut );

    valislbl_ = new uiLabel( this, sKeyAmplVals );
    valislbl_->setAlignment( Alignment::HCenter );
    valislbl_->attach( centeredBelow, sep );

    uiGroup* statsgrp = new uiGroup( this, "Stats displays" );

    synthstatsfld_ = new uiSynthToRealScaleStatsDisp( statsgrp, "Synthetics",
	    					      true );
    realstatsfld_ = new uiSynthToRealScaleStatsDisp( statsgrp, "Real Seismics",
	    					     false );
    realstatsfld_->attach( rightOf, synthstatsfld_ );
    const CallBack setsclcb( mCB(this,uiSynthToRealScale,setScaleFld) );
    synthstatsfld_->usrValChanged.notify( setsclcb );
    statsgrp->attach( centeredBelow, valislbl_ );
    realstatsfld_->usrValChanged.notify( setsclcb );
    statsgrp->setHAlignObj( realstatsfld_ );

    finalscalefld_ = new uiGenInput( this, "", FloatInpSpec() );
    finalscalefld_->attach( centeredBelow, statsgrp );
    new uiLabel( this, "Scaling factor", finalscalefld_ );

    IOObjContext wvltctxt( mIOObjContext(Wavelet) );
    wvltctxt.forread = false;
    wvltfld_ = new uiIOObjSel( this, wvltctxt, "Save scaled Wavelet as" );
    wvltfld_->attach( alignedBelow, finalscalefld_ );

    postFinalise().notify( mCB(this,uiSynthToRealScale,initWin) );
}


uiSynthToRealScale::~uiSynthToRealScale()
{
    delete horiter_;
    if ( horizon_ )
	horizon_->unRef();
    delete polygon_;
    delete &seisev_;
}


void uiSynthToRealScale::initWin( CallBacker* )
{
    updSynthStats();
}

#define mUpdateSlider( type, val ) \
     if ( !mIsUdf(val) ) \
	type->updateSlider( val ); \

void uiSynthToRealScale::setScaleFld( CallBacker* )
{
    const float synthval = synthstatsfld_->avgfld_->getfValue();
    const float realval = realstatsfld_->avgfld_->getfValue();
    if ( mIsUdf(synthval) || mIsUdf(realval) || synthval == 0 )
	finalscalefld_->setValue( mUdf(float) );
    else
	finalscalefld_->setValue( realval / synthval );

    mUpdateSlider( synthstatsfld_, synthval );
    mUpdateSlider( realstatsfld_, realval );
}


bool uiSynthToRealScale::getEvent()
{
    if ( !evfld_->getFromScreen() )
	return false;
    seisev_ = evfld_->event();
    const bool isrms = seisev_.extrwin_.nrSteps() > 0;
    valislbl_->setText( isrms ? sKeyRMSVals : sKeyAmplVals );
    return true;
}


bool uiSynthToRealScale::getHorData( TaskRunner& tr )
{
    delete polygon_; polygon_ = 0;
    if ( horizon_ )
	{ horizon_->unRef(); horizon_ = 0; }

    if ( polyfld_->isChecked() )
    {
	const IOObj* ioobj = polyfld_->ioobj();
	if ( !ioobj ) return false;
	BufferString errmsg;
	polygon_ = PickSetTranslator::getPolygon( *ioobj, errmsg );
	if ( !polygon_ )
	    mErrRetBool( errmsg );
    }

    const IOObj* ioobj = horfld_->ioobj();
    if ( !ioobj ) return false;
    EM::EMObject* emobj = EM::EMM().loadIfNotFullyLoaded( ioobj->key(), &tr );
    mDynamicCastGet(EM::Horizon3D*,hor,emobj);
    if ( !hor ) return false;
    horizon_ = hor;
    horizon_->ref();
    horiter_ = horizon_->createIterator( horizon_->sectionID(0) );
    return true;
}


float uiSynthToRealScale::getTrcValue( const SeisTrc& trc, float reftm ) const
{
    const int lastsamp = trc.size() - 1;
    const Interval<float> trg( trc.startPos(), trc.samplePos(lastsamp) );
    const int nrtms = seisev_.extrwin_.nrSteps() + 1;
    const bool calcrms = nrtms > 1;
    float sumsq = 0;
    for ( int itm=0; itm<nrtms; itm++ )
    {
	float extrtm = reftm + seisev_.extrwin_.atIndex(itm);
	float val;
	if ( extrtm <= trg.start )
	    val = trc.get( 0, 0 );
	if ( extrtm >= trg.stop )
	    val = trc.get( lastsamp, 0 );
	else
	    val = trc.getValue( extrtm, 0 );
	if ( calcrms ) val *= val;
	sumsq += val;
    }
    return calcrms ? sqrt( sumsq / nrtms ) : sumsq;
}


void uiSynthToRealScale::updSynthStats()
{
    if ( !getEvent() )
	return;

    TypeSet<float> vals;
    for ( int idx=0; idx<synth_.size(); idx++ )
    {
	const SeisTrc& trc = *synth_.get( idx );
	const float reftm = seisev_.snappedTime( trc );
	if ( !mIsUdf(reftm) )
	    vals += getTrcValue( *synth_.get(idx), reftm );
    }

    uiHistogramDisplay& histfld = *synthstatsfld_->dispfld_;
    histfld.setData( vals.arr(), vals.size() );
    histfld.putN();
    synthstatsfld_->avgfld_->setValue( histfld.getStatCalc().average() );
}


class uiSynthToRealScaleRealStatCollector : public Executor
{
public:
uiSynthToRealScaleRealStatCollector( uiSynthToRealScale& d, SeisTrcReader& r )
    : Executor( "Collect Amplitudes" )
    , dlg_(d)
    , rdr_(r)
    , msg_("Collecting")
    , nrdone_(0)
    , totalnr_(-1)
    , seldata_(0)
{
    if ( dlg_.polygon_ )
    	seldata_ = new Seis::PolySelData( *dlg_.polygon_ );
    if ( seldata_ )
	totalnr_ = seldata_->expectedNrTraces( dlg_.is2d_ );
    else
	totalnr_ = dlg_.horiter_->approximateSize();
}

~uiSynthToRealScaleRealStatCollector()
{
    delete seldata_;
}

const char* message() const	{ return msg_; }
const char* nrDoneText() const	{ return "Traces handled"; }
od_int64 nrDone() const		{ return nrdone_; }
od_int64 totalNr() const	{ return totalnr_; }

bool getNextPos()
{
    while ( true )
    {
	const EM::PosID posid = dlg_.horiter_->next();
	if ( posid.isUdf() )
	    return false;

	const Coord3 crd = dlg_.horizon_->getPos( posid );
	bid_ = SI().transform( crd );
	if ( seldata_ && !seldata_->isOK(bid_) )
	    continue;

	z_ = (float)crd.z;
	break;
    }

    nrdone_++;
    return true;
}

bool getTrc()
{
    if ( dlg_.is2d_ )
	return false; //TODO
    else
    {
	if ( !rdr_.seisTranslator()->goTo(bid_) || !rdr_.get(trc_) )
	    return false;
    }
    return true;
}

int nextStep()
{
    if ( dlg_.is2d_ )
	{ msg_ = "TODO: extract from 2D data"; return ErrorOccurred(); }

    while ( true )
    {
	if ( !getNextPos() )
	    return Finished();
	if ( !getTrc() )
	    continue;
	break;
    }

    const float val = dlg_.getTrcValue( trc_, z_ );
    if ( !mIsUdf(val) )
	vals_ += val;
    return MoreToDo();
}

    uiSynthToRealScale&	dlg_;
    SeisTrcReader&	rdr_;
    Seis::SelData*	seldata_;
    SeisTrc		trc_;
    BufferString	msg_;
    od_int64		nrdone_;
    od_int64		totalnr_;
    BinID		bid_;
    float		z_;
    TypeSet<float>	vals_;

};


void uiSynthToRealScale::updRealStats()
{
    if ( !getEvent() )
	return;

    uiTaskRunner tr( this );
    if ( !getHorData(tr) )
	return;

    SeisTrcReader rdr( seisfld_->ioobj() );
    if ( !rdr.prepareWork() )
	mErrRet( "Error opening input seismic data" );

    uiSynthToRealScaleRealStatCollector coll( *this, rdr );
    if ( !tr.execute(coll) )
	return;

    uiHistogramDisplay& histfld = *realstatsfld_->dispfld_;
    histfld.setData( coll.vals_.arr(), coll.vals_.size() );
    histfld.putN();
    realstatsfld_->avgfld_->setValue( histfld.getStatCalc().average() );
    setScaleFld( 0 );
}


bool uiSynthToRealScale::acceptOK( CallBacker* )
{
    if ( !evfld_->getFromScreen() )
	return false;

    const float scalefac = finalscalefld_->getfValue();
    if ( mIsUdf(scalefac) )
	{ uiMSG().error( "Please enter the scale factor" ); return false; }

    const IOObj* ioobj = wvltfld_->ioobj();
    if ( !ioobj )
	return false;

    IOObj* inpioobj = IOM().get( inpwvltid_ );
    Wavelet* wvlt = Wavelet::get( inpioobj );
    delete inpioobj;
    if ( !wvlt )
    {
	uiMSG().error( "Cannot save scaled wavelet because:\nThe "
		"original wavelet cannot be read." );
	delete ioobj; return false;
    }

    wvlt->transform( 0, scalefac );
    if ( !wvlt->put(ioobj) )
    {
	uiMSG().error( "Cannot write scaled Wavelet.\n"
			"Please check file permissions" );
	delete ioobj; return false;
    }
    delete wvlt;

    outwvltid_ = ioobj->key();
    Wavelet::markScaled( outwvltid_, inpwvltid_, horfld_->key(),
	    		seisfld_->ioobj()->key(), evfld_->levelName() );
    return true;
}

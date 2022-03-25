/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          April 2002
________________________________________________________________________

-*/

#include "uislicesel.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uispinbox.h"
#include "uitoolbutton.h"

#include "posinfo2dsurv.h"
#include "survinfo.h"
#include "survgeom2d.h"
#include "thread.h"
#include "timer.h"
#include "od_helpids.h"


uiSliceSel::uiSliceSel( uiParent* p, Type type, const ZDomain::Info& zi,
			bool dogeomcheck )
    : uiGroup(p,"Slice Selection")
    , inl0fld_(0)
    , inl1fld_(0)
    , applycb_(0)
    , scrolldlg_(0)
    , scrollbut_(0)
    , applybut_(0)
    , zdominfo_(zi)
    , dogeomcheck_(dogeomcheck)
{
    isinl_ = type == Inl;
    iscrl_ = type == Crl;
    istsl_ = type == Tsl;
    isvol_ = type == Vol;
    is2d_ = type == TwoD;

    if ( !is2d_ )
	createInlFld();

    createCrlFld();
    createZFld();

    if ( inl0fld_ )
	mainObject()->setTabOrder( (uiObject*)inl0fld_, (uiObject*)crl0fld_ );
    mainObject()->setTabOrder( (uiObject*)crl0fld_, (uiObject*)z0fld_ );

    if ( !isvol_ && !is2d_ )
    {
	applybut_ = uiButton::getStd( this, OD::Apply,
				    mCB(this,uiSliceSel,applyPush), true );
	mainObject()->setTabOrder( (uiObject*)z0fld_, (uiObject*)applybut_ );
	applybut_->attach( alignedBelow, z0fld_ );
	applybut_->display( false );

	scrollbut_ = new uiPushButton( this, tr("Scroll"), false );
	scrollbut_->activated.notify( mCB(this,uiSliceSel,scrollPush) );
	scrollbut_->attach( rightOf, isinl_ ? inl0fld_
					    : (iscrl_?crl0fld_:z0fld_));
    }

    if ( isvol_ )
    {
	uiToolButton* fullbut = new uiToolButton( this, "exttofullsurv",
					tr("Set ranges to full survey"),
					mCB(this,uiSliceSel,fullPush) );
	fullbut->attach( rightTo, inl1fld_ );
    }

    setHAlignObj( crl0fld_ );
    updateUI();
}


uiString uiSliceSel::sButTxtAdvance()
{
    return tr("Advance >>");
}


uiString uiSliceSel::sButTxtPause()
{
    return tr("Pause");
}


void uiSliceSel::setApplyCB( const CallBack& acb )
{
    delete applycb_;
    applycb_ = new CallBack( acb );
    if ( applybut_ ) applybut_->display( true );
}


void uiSliceSel::createInlFld()
{
    uiString label = isinl_ ? uiStrings::sInline() : uiStrings::sInlineRange();
    inl0fld_ = new uiLabeledSpinBox( this, label, 0,
			BufferString(isinl_ ? "Inl nr" : "Inl Start") );
    inl1fld_ = new uiSpinBox( this, 0, "Inl Stop" );
    inl1fld_->attach( rightTo, inl0fld_ );
    inl1fld_->display( !isinl_ );
}


void uiSliceSel::createCrlFld()
{
    uiString label = is2d_ ? uiStrings::sTraceRange()
			   : (iscrl_ ? uiStrings::sCrossline()
				     : uiStrings::sCrosslineRange());
    crl0fld_ = new uiLabeledSpinBox( this, label, 0,
			 BufferString( iscrl_ ? "Crl nr" : "Crl Start ") );
    crl1fld_ = new uiSpinBox( this, 0, "Crl Stop" );
    crl1fld_->attach( rightTo, crl0fld_ );
    crl1fld_->display( !iscrl_ );
    if ( inl0fld_ ) crl0fld_->attach( alignedBelow, inl0fld_ );
}


void uiSliceSel::createZFld()
{
    uiString label = tr("%1 %2")
		.arg(istsl_ ? uiStrings::sZ() : uiStrings::sZRange())
		.arg(zdominfo_.uiUnitStr(true));
    z0fld_ = new uiLabeledSpinBox( this, label, 0, istsl_ ? "Z" : "Z Start" );
    z1fld_ = new uiSpinBox( this, 0, "Z Stop" );
    z1fld_->attach( rightTo, z0fld_ );
    z1fld_->display( !istsl_ );
    z0fld_->attach( alignedBelow, crl0fld_ );
}


void uiSliceSel::setBoxValues( uiSpinBox* box, const StepInterval<int>& intv,
			       int curval )
{
    box->setInterval( intv.start, intv.stop );
    box->setStep( intv.step, true );
    box->setValue( curval );
}


class uiSliceScroll : public uiDialog
{ mODTextTranslationClass(uiSliceScroll);
public:

uiSliceScroll( uiSliceSel* ss )
	: uiDialog(ss,uiDialog::Setup(tr("Scrolling"),
				      mToUiStringTodo(getTitle(ss)),
				      mODHelpKey(mSliceScrollHelpID) )
				      .modal(false))
	, slcsel_(ss)
	, inauto_(false)
	, paused_(false)
	, zfact_(ss->zdominfo_.userFactor())
{
    setCtrlStyle( CloseOnly );
    timer = new Timer( "uiSliceScroll timer" );
    timer->tick.notify( mCB(this,uiSliceScroll,timerTick) );

    const TrcKeyZSampling& cs = SI().sampling( false );
    const TrcKeySampling& hs = cs.hsamp_;
    int step = hs.step_.inl();
    int maxstep = hs.start_.inl() - hs.stop_.inl();
    if  ( ss->iscrl_ )
    {
	step = hs.step_.crl();
	maxstep = hs.start_.crl() - hs.stop_.crl();
    }
    else if ( ss->istsl_ )
    {
	step = mNINT32(cs.zsamp_.step*zfact_);
	float zrg = (cs.zsamp_.stop - cs.zsamp_.start) * zfact_;
	maxstep = mNINT32(zrg);
    }
    if ( maxstep < 0 ) maxstep = -maxstep;
    stepfld_ = new uiLabeledSpinBox( this, tr("Scroll step") );
    stepfld_->box()->setMinValue( !ss->dogeomcheck_ ? -1 : -maxstep );
    stepfld_->box()->setMaxValue( !ss->dogeomcheck_ ? 1 : maxstep );
    stepfld_->box()->setStep( !ss->dogeomcheck_ ? 1 : step );
    stepfld_->box()->setValue( !ss->dogeomcheck_ ? 1 : step );

    typfld_ = new uiLabeledComboBox( this, tr("Control") );
    typfld_->box()->addItem( uiStrings::sManual() );
    typfld_->box()->addItem( tr("Auto") );
    typfld_->box()->selectionChanged.notify( mCB(this,uiSliceScroll,typSel) );
    typfld_->attach( alignedBelow, stepfld_ );
    ctrlbut = new uiPushButton( this, uiSliceSel::sButTxtAdvance(), true );
    ctrlbut->activated.notify( mCB(this,uiSliceScroll,butPush) );
    ctrlbut->attach( alignedBelow, typfld_ );
    backbut = new uiPushButton( this, tr("<< Step Back"), true );
    backbut->activated.notify( mCB(this,uiSliceScroll,butPush) );
    backbut->attach( leftOf, ctrlbut );

    dtfld_ = new uiGenInput( this, tr("Time between updates (s)"),
			     FloatInpSpec(2));
    dtfld_->attach( alignedBelow, ctrlbut );

    postFinalize().notify( mCB(this,uiSliceScroll,typSel) );
}


~uiSliceScroll()
{
    delete timer; timer = 0;
}


void typSel( CallBacker* )
{
    const bool autoreq = typfld_->box()->currentItem() == 1;
    dtfld_->display( autoreq );
    if ( inauto_ != autoreq )
    {
	if ( autoreq )
	    startAuto();
	else
	    stopAuto( false );
    }
    ctrlbut->setText( autoreq ? uiSliceSel::sButTxtPause() :
				uiSliceSel::sButTxtAdvance() );
    backbut->display( !autoreq );
    inauto_ = autoreq;
}


void butPush( CallBacker* cb )
{
    if ( !inauto_ )
	doAdvance( cb != ctrlbut );
    else
    {
	paused_ = ctrlbut->text().getOriginalString()[0] == 'P';
	ctrlbut->setText( paused_ ? uiStrings::sGo() :
						  uiSliceSel::sButTxtPause() );
    }
}


void startAuto()
{
    paused_ = false;
    doAdvance( false );
    ctrlbut->setText( uiSliceSel::sButTxtPause() );
    setTimer();
}

void stopAuto( bool setmanual )
{
    timer->stop();
    inauto_ = false;
    if ( setmanual )
    {
	typfld_->box()->setCurrentItem( 0 );
	ctrlbut->setText( uiSliceSel::sButTxtAdvance() );
	backbut->display( true );
    }
}


void doAdvance( bool reversed )
{
    if ( !timer ) return;

    const int step = (reversed ? -1 : 1) * stepfld_->box()->getIntValue();
    slcsel_->readInput();
    if ( slcsel_->isinl_ )
    {
	int newval = slcsel_->tkzs_.hsamp_.start_.inl() + step;
	if (slcsel_->dogeomcheck_ && !SI().sampling(true).hsamp_.inlOK(newval))
	    stopAuto( true );
	else
	    slcsel_->inl0fld_->box()->setValue( newval );
    }
    else if ( slcsel_->iscrl_ )
    {
	int newval = slcsel_->tkzs_.hsamp_.start_.crl() + step;
	if (slcsel_->dogeomcheck_ && !SI().sampling(true).hsamp_.crlOK(newval))
	    stopAuto( true );
	else
	    slcsel_->crl0fld_->box()->setValue( newval );
    }
    else
    {
	float newval = slcsel_->tkzs_.zsamp_.start + step / zfact_;
	if ( slcsel_->dogeomcheck_ &&
	     !SI().sampling(true).zsamp_.includes(newval,false) )
	    stopAuto( true );
	else
	{
	    if ( zfact_ < 10 )
		slcsel_->z0fld_->box()->setValue( newval );
	    else
	    {
		newval *= zfact_;
		slcsel_->z0fld_->box()->setValue( mNINT32(newval) );
	    }
	}
    }

    slcsel_->applyPush(0);
}


void timerTick( CallBacker* )
{
    if ( !inauto_ )
	return;
    if ( !paused_ )
	doAdvance( false );
    setTimer();
}


void setTimer()
{
    if ( !timer ) return;

    float val = dtfld_->getFValue();
    if ( mIsUdf(val) || val < 0.2 )
	val = 200;
    else
	val *= 1000;
    timer->start( mNINT32(val), true );
}


bool rejectOK( CallBacker* )
{
    paused_ = true;
    inauto_ = false;
    return true;
}


uiString getTitle( uiSliceSel* ss ) const
{
    uiString title = tr("Control scrolling through %1");
    if ( !ss->istsl_ )
    {
	title.arg( ss->isinl_ ? uiStrings::sInline(mPlural)
			      : uiStrings::sCrossline(mPlural) );
    }
    else
    {
	uiString slicetxt = tr( "%1 slices" )
	    .arg( SI().zIsTime() ? uiStrings::sTime() : uiStrings::sDepth() );
	title.arg( slicetxt );
    }
    return title;
}

    uiSliceSel*		slcsel_;
    uiLabeledSpinBox*	stepfld_;
    uiLabeledComboBox*	typfld_;
    uiPushButton*	ctrlbut;
    uiPushButton*	backbut;
    uiGenInput*		dtfld_;
    const float		zfact_;

    bool		paused_;
    bool		inauto_;
    Timer*		timer;

};


uiSliceSel::~uiSliceSel()
{
    delete applycb_;
    delete scrolldlg_;
}


void uiSliceSel::scrollPush( CallBacker* )
{
    if ( !scrolldlg_ )
	scrolldlg_ = new uiSliceScroll( this );
    scrolldlg_->show();
}


void uiSliceSel::applyPush( CallBacker* )
{
    Threads::Locker lckr( updatelock_, Threads::Locker::DontWaitForLock );
    if ( !lckr.isLocked() )
	return;

    readInput();
    if ( applycb_ )
	applycb_->doCall(this);
}


void uiSliceSel::fullPush( CallBacker* )
{
    setTrcKeyZSampling( maxcs_ );
}


void uiSliceSel::readInput()
{
    const TrcKeySampling& hs = maxcs_.hsamp_;
    Interval<int> inlrg, crlrg;
    hs.get( inlrg, crlrg );
    if ( inl0fld_ )
    {
	inlrg.start = inl0fld_->box()->getIntValue();
	inlrg.stop = isinl_ ? inlrg.start : inl1fld_->getIntValue();
	if ( !isinl_ && inlrg.start == inlrg.stop )
	    inlrg.stop += hs.step_.inl();
    }

    crlrg.start = crl0fld_->box()->getIntValue();
    crlrg.stop = iscrl_ ? crlrg.start : crl1fld_->getIntValue();
    if ( !iscrl_ && crlrg.start == crlrg.stop )
	crlrg.stop += hs.step_.crl();

    const float zfac( zdominfo_.userFactor() );
    Interval<float> zrg;
    zrg.start = z0fld_->box()->getFValue() / zfac;
    zrg.start = maxcs_.zsamp_.snap( zrg.start );
    if ( istsl_ )
	zrg.stop = zrg.start;
    else
    {
	zrg.stop = z1fld_->getFValue() / zfac;
	zrg.sort();
	zrg.stop = maxcs_.zsamp_.snap( zrg.stop );
	if ( mIsEqual(zrg.start,zrg.stop,mDefEps) )
	    zrg.stop += maxcs_.zsamp_.step;
    }

    tkzs_.hsamp_.set( inlrg, crlrg );
    tkzs_.zsamp_.setFrom( zrg );

    if ( dogeomcheck_ )
    {
	SI().snap( tkzs_.hsamp_.start_, BinID(0,0) );
	SI().snap( tkzs_.hsamp_.stop_, BinID(0,0) );
    }
}


void uiSliceSel::updateUI()
{
    if ( inl0fld_ )
    {
	Interval<int> inlrg( tkzs_.hsamp_.start_.inl(),
			     tkzs_.hsamp_.stop_.inl() );
	StepInterval<int> maxinlrg( maxcs_.hsamp_.start_.inl(),
				    maxcs_.hsamp_.stop_.inl(),
				    maxcs_.hsamp_.step_.inl() );
	setBoxValues( inl0fld_->box(), maxinlrg, inlrg.start );
	setBoxValues( inl1fld_, maxinlrg, inlrg.stop );
    }

    Interval<int> crlrg( tkzs_.hsamp_.start_.crl(), tkzs_.hsamp_.stop_.crl() );
    StepInterval<int> maxcrlrg( maxcs_.hsamp_.start_.crl(),
				maxcs_.hsamp_.stop_.crl(),
				maxcs_.hsamp_.step_.crl() );
    setBoxValues( crl0fld_->box(), maxcrlrg, crlrg.start );
    setBoxValues( crl1fld_, maxcrlrg, crlrg.stop );

    const float zfac( zdominfo_.userFactor() );
    const int nrdec = Math::NrSignificantDecimals( tkzs_.zsamp_.step*zfac );

    if ( nrdec==0 )
    {
	Interval<int> zrg( mNINT32(tkzs_.zsamp_.start*zfac),
			   mNINT32(tkzs_.zsamp_.stop*zfac) );
	StepInterval<int> maxzrg =
	    StepInterval<int>( mNINT32(maxcs_.zsamp_.start*zfac),
			       mNINT32(maxcs_.zsamp_.stop*zfac),
			       mNINT32(maxcs_.zsamp_.step*zfac) );
	setBoxValues( z0fld_->box(), maxzrg, zrg.start );
	setBoxValues( z1fld_, maxzrg, zrg.stop );
    }
    else
    {
	StepInterval<float> zrg = tkzs_.zsamp_;
	zrg.scale( zfac );
	StepInterval<float> maxzrg = maxcs_.zsamp_;
	maxzrg.scale( zfac );

	z0fld_->box()->setInterval( maxzrg );
	z0fld_->box()->setValue( tkzs_.zsamp_.start*zfac );

	z1fld_->setInterval( maxzrg );
	z1fld_->setValue( tkzs_.zsamp_.stop*zfac );
    }

    z0fld_->box()->setNrDecimals( nrdec );
    z1fld_->setNrDecimals( nrdec );
}


void uiSliceSel::setTrcKeyZSampling( const TrcKeyZSampling& cs )
{
    tkzs_ = cs;
    updateUI();
}


void uiSliceSel::setMaxTrcKeyZSampling( const TrcKeyZSampling& maxcs )
{
    maxcs_ = maxcs;
    updateUI();
}


bool uiSliceSel::acceptOK()
{
#ifdef __mac__
    crl0fld_->setFocus(); crl1fld_->setFocus(); // Hack
#endif
    readInput();
    return true;
}


void uiSliceSel::enableApplyButton( bool yn )
{
    if ( !applybut_ ) return;
    applybut_->display( yn );
}


void uiSliceSel::enableScrollButton( bool yn )
{
    if ( !scrollbut_ ) return;
    scrollbut_->display( yn );
}


void uiSliceSel::fillPar( IOPar& iop )
{
    TrcKeyZSampling cs;
    cs.hsamp_.start_.inl() = is2d_ ? 0 : inl0fld_->box()->getIntValue();

    if ( isinl_ )
	cs.hsamp_.stop_.inl() =  is2d_ ? 0 : inl0fld_->box()->getIntValue();
    else
	cs.hsamp_.stop_.inl() = is2d_ ? 0 : inl1fld_->getIntValue();

    cs.hsamp_.start_.crl() = crl0fld_->box()->getIntValue();
    cs.hsamp_.stop_.crl() = iscrl_ ? crl0fld_->box()->getIntValue()
				   : crl1fld_->getIntValue();

    cs.zsamp_.start = float( z0fld_->box()->getIntValue() );
    cs.zsamp_.stop = float( istsl_ ? z0fld_->box()->getIntValue()
				   : z1fld_->getIntValue() );
    cs.fillPar( iop );
}


void uiSliceSel::usePar( const IOPar& par )
{
    if ( !is2d_ )
    {
	int inlnr; par.get( sKey::FirstInl(), inlnr );
	inl0fld_->box()->setValue( inlnr );

	int inl1; par.get( sKey::LastInl(), inl1 );
	if ( inl1fld_->isDisplayed() )
	    inl1fld_->setValue( inl1 );
    }

    int crl0; par.get( sKey::FirstCrl(), crl0 );
    crl0fld_->box()->setValue( crl0 );
    int crl1; par.get( sKey::LastCrl(), crl1 );
    if ( crl1fld_->isDisplayed() )
	crl1fld_->setValue( crl1 );

    StepInterval<float> zrg;
    par.get( sKey::ZRange(), zrg );
    z0fld_->box()->setValue( zrg.start );
    if ( z1fld_->isDisplayed() )
	z1fld_->setValue( zrg.stop );
}


//uiSliceSelDlg
uiSliceSelDlg::uiSliceSelDlg( uiParent* p, const TrcKeyZSampling& curcs,
			const TrcKeyZSampling& maxcs,
			const CallBack& acb, uiSliceSel::Type type,
			const ZDomain::Info& zdominfo )
    : uiDialog(p,uiDialog::Setup(tr("Positioning"),
				 tr("Specify the element's position"),
				 mODHelpKey(mSliceSelHelpID) )
		 .modal(type==uiSliceSel::Vol||type==uiSliceSel::TwoD))
{
    slicesel_ = new uiSliceSel( this, type, zdominfo );
    slicesel_->setMaxTrcKeyZSampling( maxcs );
    slicesel_->setTrcKeyZSampling( curcs );
    slicesel_->setApplyCB( acb );
    slicesel_->enableScrollButton( true );
}


bool uiSliceSelDlg::acceptOK( CallBacker* )
{
    return slicesel_->acceptOK();
}


// uiLinePosSelDlg
uiLinePosSelDlg::uiLinePosSelDlg( uiParent* p )
    : uiDialog( p, uiDialog::Setup(tr("Select line position"),
				   mNoDlgTitle,mNoHelpKey) )
    , prefcs_(0)
    , is2d_(true)
    , inlcrlfld_(0)
    , posdlg_(0)
{
    BufferStringSet linenames;
    TypeSet<Pos::GeomID> geomids;
    Survey::GM().getList( linenames, geomids, true );
    linesfld_ = new uiGenInput( this, tr("Compute on line:"),
				StringListInpSpec(linenames) );
    setOkText( uiStrings::sNext() );
}


uiLinePosSelDlg::uiLinePosSelDlg( uiParent* p, const TrcKeyZSampling& cs )
    : uiDialog( p, uiDialog::Setup(tr("Select line position"),
				   mNoDlgTitle,mNoHelpKey) )
    , tkzs_( cs )
    , prefcs_(0)
    , is2d_(false)
    , linesfld_(0)
    , posdlg_(0)
{
    inlcrlfld_ = new uiGenInput( this, tr("Compute on:"),
			BoolInpSpec(true,uiStrings::sInline(),
			uiStrings::sCrossline()));
    setOkText( uiStrings::sNext() );
}


uiLinePosSelDlg::~uiLinePosSelDlg()
{
    delete posdlg_;
}


bool uiLinePosSelDlg::acceptOK( CallBacker* )
{
    return linesfld_ ? selectPos2D()
		     : selectPos3D();
}


bool uiLinePosSelDlg::selectPos2D()
{
    mDynamicCastGet( const Survey::Geometry2D*, geom2d,
		     Survey::GM().getGeometry(linesfld_->text()) );
    if ( !geom2d ) return false;

    TrcKeyZSampling inputcs = tkzs_;
    if ( prefcs_ )
	inputcs = *prefcs_;
    else
    {
	inputcs.hsamp_.setCrlRange( geom2d->data().trcNrRange() );
	inputcs.zsamp_ = geom2d->data().zRange();
    }

    const ZDomain::Info info( ZDomain::SI() );
    const uiSliceSel::Type tp = uiSliceSel::TwoD;
    posdlg_ = new uiSliceSelDlg( this, inputcs, tkzs_, CallBack(), tp, info );
    posdlg_->grp()->enableApplyButton( false );
    posdlg_->grp()->enableScrollButton( false );
    posdlg_->setModal( true );
    if ( !prevpar_.isEmpty() )
	posdlg_->grp()->usePar( prevpar_ );

    return posdlg_->go();
}


bool uiLinePosSelDlg::selectPos3D()
{
    CallBack dummycb;
    const bool isinl = inlcrlfld_->getBoolValue();

    TrcKeyZSampling inputcs = tkzs_;
    if ( prefcs_ )
	inputcs = *prefcs_;
    else
    {
	if ( isinl )
	    inputcs.hsamp_.stop_.inl() = inputcs.hsamp_.start_.inl()
				   = inputcs.hsamp_.inlRange().snappedCenter();
	else
	    inputcs.hsamp_.stop_.crl() = inputcs.hsamp_.start_.crl()
				   = inputcs.hsamp_.crlRange().snappedCenter();

	inputcs.zsamp_.start = 0;
    }

    const ZDomain::Info info( ZDomain::SI() );
    const uiSliceSel::Type tp = isinl ? uiSliceSel::Inl : uiSliceSel::Crl;
    posdlg_ = new uiSliceSelDlg( this, inputcs, tkzs_, dummycb, tp, info );
    posdlg_->grp()->enableApplyButton( false );
    posdlg_->grp()->enableScrollButton( false );
    posdlg_->setModal( true );
    if ( !prevpar_.isEmpty() )
	posdlg_->grp()->usePar( prevpar_ );

    return posdlg_->go();
}


const TrcKeyZSampling& uiLinePosSelDlg::getTrcKeyZSampling() const
{ return posdlg_ ? posdlg_->getTrcKeyZSampling() : tkzs_; }


const char* uiLinePosSelDlg::getLineName() const
{ return linesfld_ ? linesfld_->text() : ""; }

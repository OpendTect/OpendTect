/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/

#include "uistratsimplelaymoddisp.h"
#include "uistratlaymodtools.h"
#include "uistrateditlayer.h"
#include "uigraphicsitemimpl.h"
#include "uigraphicsscene.h"
#include "uigraphicsview.h"
#include "uigeninput.h"
#include "uifilesel.h"
#include "uifiledlg.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiflatviewer.h"
#include "uimultiflatviewcontrol.h"
#include "uiusershowwait.h"
#include "envvars.h"
#include "flatposdata.h"
#include "stratlevel.h"
#include "arrayndimpl.h"
#include "stratlayermodel.h"
#include "stratlayersequence.h"
#include "stratreftree.h"
#include "od_iostream.h"
#include "survinfo.h"
#include "unitofmeasure.h"
#include "property.h"
#include "keystrs.h"
#include "oddirs.h"
#include "file.h"
#include "od_helpids.h"

#define mDispEach() tools_.dispEach()
#define mUseLithCols() tools_.dispLith()
#define mShowZoomed() tools_.dispZoomed()
#define mSelLevelIdx() tools_.selLevelIdx()

#define mGetConvZ(var,conv) \
    if ( zinfeet_ ) var *= conv
#define mEnsureZInMeter(var) mGetConvZ(var,mFromFeetFactorF)
#define mEnsureZInUserUnit(var) mGetConvZ(var,mToFeetFactorF)
#define mGetZrgInUserUnit( src, target ) \
    Interval<float> target( src ); \
    if ( zinfeet_ ) \
	target.scale( mToFeetFactorF )


uiStratLayerModelDisp::uiStratLayerModelDisp( uiStratLayModEditTools& t,
					  const LayerModelSuite& lms )
    : uiGroup(t.parent(),"LayerModel display")
    , tools_(t)
    , lms_(lms )
    , selseqidx_(-1)
    , zinfeet_(SI().depthsInFeet())
    , vwr_(*new uiFlatViewer(this))
    , modtypetxtitm_(0)
    , sequenceSelected(this)
    , genNewModelNeeded(this)
    , infoChanged(this)
    , modelChanged(this)
    , sequencesAdded(this)
{
    vwr_.setInitialSize( initialSize() );
    vwr_.setStretch( 2, 2 );
    vwr_.disableStatusBarUpdate();
    vwr_.setZDomain( ZDomain::Depth() );
    FlatView::Appearance& app = vwr_.appearance();
    app.setGeoDefaults( true );
    app.setDarkBG( false );
    app.annot_.title_.setEmpty();
    app.annot_.x1_.showAll( true );
    app.annot_.x2_.showAll( true );
    app.annot_.x1_.name_ = tr("Model Nr");
    app.annot_.x2_.name_ = uiStrings::sDepth();
    app.ddpars_.wva_.allowuserchange_ = false;
    app.ddpars_.vd_.allowuserchange_ = false;
    app.ddpars_.wva_.allowuserchangedata_ = false;
    app.ddpars_.vd_.allowuserchangedata_ = false;
    app.annot_.x1_.showannot_ = true;
    app.annot_.x1_.showgridlines_ = false;
    app.annot_.x1_.annotinint_ = true;
    app.annot_.x2_.showannot_ = true;
    app.annot_.x2_.showgridlines_ = true;
    app.annot_.allowuserchangereversedaxis_ = false;

    mAttachCB( postFinalise(), uiStratLayerModelDisp::initGrp );
}


uiStratLayerModelDisp::~uiStratLayerModelDisp()
{
    detachAllNotifiers();
}


void uiStratLayerModelDisp::initGrp( CallBacker* )
{
    mAttachCB( vwr_.rgbCanvas().reSize, uiStratLayerModelDisp::vwResizeCB );
    mAttachCB( lms_.curChanged, uiStratLayerModelDisp::curModEdChgCB );

#   define mSetMouseCB(notifnm,cbnm) \
	mAttachCB( vwr_.rgbCanvas().getMouseEventHandler().notifnm, \
		   uiStratLayerModelDisp::cbnm )
    mSetMouseCB( buttonReleased, usrClickedCB );
    mSetMouseCB( doubleClick, doubleClickedCB );
    mSetMouseCB( movement, mouseMovedCB );

#   define mSetCB(notifnm) tools_.notifnm.notify( \
	mCB(this,uiStratLayerModelDisp,notifnm##CB)  )
    mSetCB( selPropChg ); mSetCB( dispLithChg ); mSetCB( selContentChg );
    mSetCB( selLevelChg ); mSetCB( dispEachChg ); mSetCB( dispZoomedChg );
    mSetCB( showFlatChg );
}


uiGraphicsScene& uiStratLayerModelDisp::scene() const
{
    return const_cast<uiStratLayerModelDisp*>(this)->vwr_.rgbCanvas().scene();
}


const Strat::LayerModel& uiStratLayerModelDisp::layerModel() const
{
    return lms_.getCurrent();
}


uiWorldRect uiStratLayerModelDisp::zoomBox() const
{
    return vwr_.curView();
}


void uiStratLayerModelDisp::setZoomBox( const uiWorldRect& wr )
{
    vwr_.setView( wr );
}


void uiStratLayerModelDisp::clearZoom()
{
    vwr_.setViewToBoundingBox();
}


void uiStratLayerModelDisp::dispZoomedChg()
{
    mDynamicCastGet(uiMultiFlatViewControl*,stdctrl,vwr_.control())
    if ( stdctrl )
    {
	const bool showzoomed = mShowZoomed();
	stdctrl->setZoomCoupled( showzoomed );
	stdctrl->setDrawZoomBoxes( !showzoomed );
    }
}


void uiStratLayerModelDisp::showFlatChg()
{
    handleModelChange();
    clearZoom();
}


static float seqLvlDepth( const Strat::LayerSequence& seq,
			  const Strat::Level& lvl )
{
    const int posidx = seq.positionOf( lvl );
    float zlvl = mUdf(float);
    if ( posidx >= seq.size() )
	zlvl = seq.layers().last()->zBot();
    else if ( posidx >= 0 )
	zlvl = seq.layers().get(posidx)->zTop();
    return zlvl;
}


void uiStratLayerModelDisp::fillLevelDepths()
{
    lvldpths_.erase();
    const BufferStringSet& lvlnms = tools_.levelNames();
    const int nrlvls = lvlnms.size();
    lvldpths_.setSize( nrlvls );

    for( int ilvl=0; ilvl<nrlvls; ilvl++ )
    {
	const Level lvl = Strat::LVLS().getByIdx( ilvl );
	auto& dpths = lvldpths_[ilvl];
	for ( int iseq=0; iseq<layerModel().size(); iseq++ )
	{
	    const LayerSequence& seq = layerModel().sequence( iseq );
	    if ( lvl.id().isInvalid() || seq.isEmpty() )
		dpths += mUdf(float);
	    else
		dpths += seqLvlDepth( seq, lvl );
	}
    }
}


uiFlatViewer* uiStratLayerModelDisp::getViewerClone( uiParent* p )	const
{
    uiFlatViewer* vwr = new uiFlatViewer( p );
    vwr->rgbCanvas().disableImageSave();
    vwr->setInitialSize( initialSize() );
    vwr->setStretch( 2, 2 );
    vwr->appearance() = vwr_.appearance();
    vwr->setPack( true, vwr_.packID(true), false );
    vwr->setPack( false, vwr_.packID(false), false );
    return vwr;
}


void uiStratLayerModelDisp::selectSequence( int selidx )
{
    selseqidx_ = selidx;
    drawSelectedSequence();
}


int uiStratLayerModelDisp::selLevelIdx() const
{
    return mSelLevelIdx();
}


float uiStratLayerModelDisp::getLayerPropValue( const Layer& lay,
				const UnitOfMeasure* uom, int propidx ) const
{
    const float sival = propidx < lay.nrValues() ? lay.value( propidx )
						 : mUdf(float);
    return uom ? uom->getUserValueFromSI( sival ) : sival;
}


float uiStratLayerModelDisp::getLayerPropValue( const Layer& lay,
				const PropertyRef& pr, int propidx ) const
{
    const UnitOfMeasure* uom = UoMR().getDefault( pr.name(), pr.stdType() );
    return getLayerPropValue( lay, uom, propidx );
}


void uiStratLayerModelDisp::curModEdChgCB( CallBacker* )
{
    handleModelChange();

    const bool ised = lms_.curIsEdited();
    if ( !modtypetxtitm_ )
    {
	const uiPoint pos( mNINT32( scene().nrPixX()/2 ),
			   mNINT32( scene().nrPixY()-10 ) );
	modtypetxtitm_ = scene().addItem( new uiTextItem(pos,uiString::empty(),
						mAlignment(HCenter,VCenter)) );
	modtypetxtitm_->setPenColor( Color::Black() );
	modtypetxtitm_->setZValue( 999999 );
	modtypetxtitm_->setMovable( true );
    }

    modtypetxtitm_->setVisible( ised );
    if ( ised )
	modtypetxtitm_->setText( lms_.uiDesc(lms_.curIdx()) );
}


void uiStratLayerModelDisp::vwResizeCB( CallBacker* )
{
    if ( modtypetxtitm_ )
    {
	const uiPoint pos( mNINT32( scene().nrPixX()/2 ),
			   mNINT32( scene().nrPixY()-10 ) );
	modtypetxtitm_->setPos( pos );
    }
}


#define mErrRet(s) { uiMSG().error(s); return false; }


class uiStratLayerModelDispIO : public uiDialog
{ mODTextTranslationClass(uiStratLayerModelDispIO);
public:

static const char* sKeyUseEach()	{ return "Use Each"; }
static const char* sKeyDoClear()	{ return "Clear First"; }
static const char* sKeyNrDisplay()	{ return "Display Nr Models"; }
static const char* sKeyPreserveMath()	{ return "Preserve Math formulas"; }


uiStratLayerModelDispIO( uiParent* p, const Strat::LayerModel& lm, IOPar& pars,
			 bool doread )
    : uiDialog( p, Setup(doread ? tr("Read dumped models") : tr("Dump models"),
	    mNoDlgTitle, mODHelpKey(mStratLayerModelDispIOHelpID)))
    , doreplacefld_(0)
    , eachfld_(0)
    , presmathfld_(0)
    , lm_(lm)
    , pars_(pars)
    , forread_(doread)
{
    const BufferString fixeddumpfnm = GetEnvVar( "OD_FIXED_LAYMOD_DUMPFILE" );
    fnm_ = fixeddumpfnm;

    if ( !forread_ && !fixeddumpfnm.isEmpty() )
	setTitleText( tr("Dumping to %1").arg( fnm_ ) );
    else
    {
	uiFileSel::Setup su( fnm_ );
	su.setForWrite( !forread_ );
	filefld_ = new uiFileSel( this, uiStrings::sFileName(), su );
    }

    if ( forread_ )
    {
	const Interval<int> valrg( 1, 1000 );
	IntInpSpec val( 1, valrg );
	eachfld_ = new uiGenInput( this, tr("Use Each"), val );
	if ( filefld_ )
	    eachfld_->attach( alignedBelow, filefld_ );

	doreplacefld_ = new uiGenInput( this,
					tr("Clear existing model before add"),
					BoolInpSpec(false) );
	doreplacefld_->attach( alignedBelow, eachfld_ );
    }
    else
    {
	presmathfld_ = new uiGenInput( this, tr("Preserve Math Formulas"),
				       BoolInpSpec(true) );
	if ( filefld_ )
	    presmathfld_->attach( alignedBelow, filefld_ );
    }

    usePar();
}


bool usePar()
{
    if ( filefld_ )
    {
	BufferString fnm;
	if ( pars_.get(sKey::FileName(),fnm) )
	    filefld_->setFileName( fnm );
    }

    if ( forread_ )
    {
	int each;
	if ( pars_.get(sKeyUseEach(),each) )
	    eachfld_->setValue( each );

	bool doreplace = true;
	if ( pars_.getYN(sKeyDoClear(),doreplace) )
	    doreplacefld_->setValue( doreplace );
    }
    else
    {
	bool preservemath = true;
	if ( pars_.getYN(sKeyPreserveMath(),preservemath) )
	    presmathfld_->setValue( preservemath );
    }

    return true;
}


void fillPar()
{
    if ( filefld_ )
	pars_.set( sKey::FileName(), filefld_->fileName() );
    if ( !forread_ )
	pars_.setYN( sKeyPreserveMath(), presmathfld_->getBoolValue() );
    else
    {
	pars_.set( sKeyUseEach(), eachfld_->getIntValue() );
	pars_.setYN( sKeyDoClear(), doreplacefld_->getBoolValue() );
    }
}


int getNrDisplayModels()
{
    int nrmoddisp;
    if ( !pars_.get(sKeyNrDisplay(),nrmoddisp) )
	return mUdf(int);

    return nrmoddisp;
}


bool acceptOK()
{
    if ( filefld_ )
	fnm_ = filefld_->fileName();
    if ( fnm_.isEmpty() )
	mErrRet(tr("Please provide a file name"))

    if ( forread_ )
    {
	uiUserShowWait usw( this, uiStrings::sReadingData() );
	od_istream strm( fnm_ );
	if ( !strm.isOK() )
	    mErrRet(tr("Cannot open:\n%1\nfor read").arg(fnm_))

	Strat::LayerModel& lm = const_cast<Strat::LayerModel&>( lm_ );
	const bool replace = doreplacefld_->getBoolValue();
	if ( replace )
	    { lm.setEmpty(); changedmodel_ = true; }

	const auto szbefore = lm.size();
	const int each = eachfld_->getIntValue();
	if ( !lm.read(strm,true,each) )
	    mErrRet(tr("Cannot read layer model from file.\nDetails may be "
		       "in the log file ('Utilities-Show log file')"))

	changedmodel_ |= szbefore != lm.size();
	addedmodels_ = !replace && szbefore<lm.size();
    }
    else
    {
	od_ostream strm( fnm_ );
	if ( !strm.isOK() )
	    mErrRet(tr("Cannot open:\n%1\nfor write").arg(fnm_))

	uiUserShowWait usw( this, uiStrings::sWriting() );
	if ( !lm_.write(strm,0,presmathfld_->getBoolValue()) )
	    mErrRet(uiStrings::phrErrDuringWrite())
    }

    fillPar();

    return true;
}

    uiFileSel*			filefld_		= 0;
    uiGenInput*			doreplacefld_;
    uiGenInput*			eachfld_;
    uiGenInput*			presmathfld_;

    const Strat::LayerModel&	lm_;
    BufferString		fnm_;
    BufferString		fixeddumpfnm_;
    IOPar&			pars_;
    const bool			forread_;
    bool			changedmodel_		= false;
    bool			addedmodels_		= false;

};


bool uiStratLayerModelDisp::doLayerModelIO( bool foradd )
{
    const LayerModel& lm = layerModel();
    if ( !foradd && lm.isEmpty() )
	mErrRet( tr("Please generate at least one layer sequence") )

    uiStratLayerModelDispIO dlg( this, lm, dumppars_, foradd );
    const bool ret = dlg.go();
    if ( ret && dlg.addedmodels_ )
	sequencesAdded.trigger();
    if ( dlg.changedmodel_ )
	modelChanged.trigger();

    return ret;
}


bool uiStratLayerModelDisp::showFlattened() const
{
    bool flattened = tools_.showFlattened();
    if ( flattened && mSelLevelIdx()<0 )
	flattened = false;
    return flattened;
}


int uiStratLayerModelDisp::curPropIdx() const
{
    const int pidx = tools_.selPropIdx();
    return pidx < 0 ? -1 : pidx+1;
}


int uiStratLayerModelDisp::usrPointedModelNr() const
{
    MouseEventHandler& mevh = vwr_.rgbCanvas().getMouseEventHandler();
    if ( layerModel().isEmpty() || !mevh.hasEvent() )
	return -1;

    const MouseEvent& mev = mevh.event();
    const float fseqidx = vwr_.getWorld2Ui().toWorldX( mev.pos().x_ ) - 1.0f;
    int seqidx = (int)fseqidx; // not mNINT32
    if ( fseqidx < 0 || seqidx > layerModel().size() )
	seqidx = -1; // use fseqidx: (int) will round upward for neg nmbrs
    return seqidx;
}


Strat::Layer* uiStratLayerModelDisp::usrPointedLayer( int& layidx ) const
{
    const int seqidx = usrPointedModelNr();
    if ( seqidx < 0 || seqidx >= layerModel().size() )
	return 0;
    auto& seq = const_cast<LayerSequence&>( layerModel().sequence(seqidx) );
    auto& lays = seq.layers();
    if ( lays.isEmpty() )
	return 0;

    MouseEventHandler& mevh = vwr_.rgbCanvas().getMouseEventHandler();
    float zsel = vwr_.getWorld2Ui().toWorldY( mevh.event().pos().y_ );
    mEnsureZInMeter( zsel );

    if ( showFlattened() )
    {
	const auto lvlidx = mSelLevelIdx();
	const float lvlz = lvldpths_.get( lvlidx ).get( seqidx );
	if ( mIsUdf(lvlz) )
	    return 0;
	zsel += lvlz;
    }

    layidx = seq.layerIdxAtZ( zsel );
    return layidx < 0 ? 0 : seq.layers().get( layidx );
}


void uiStratLayerModelDisp::mouseMovedCB( CallBacker* )
{
    const int seqidx = usrPointedModelNr();
    if ( seqidx<0 || seqidx>=layerModel().size() )
	return;
    int layidx;
    const auto* lay = usrPointedLayer( layidx );
    if ( !lay )
	return;

    uiString infomsg = uiStrings::sModelNumber().addMoreInfo( seqidx+1 );
    infomsg.postFixWord( uiStrings::sLayer().addMoreInfo( lay->name() ) );
    float dpth = lay->depth(); mEnsureZInUserUnit( dpth );
    infomsg.postFixWord( uiStrings::sDepth().addMoreInfo( dpth ) );
    float th = lay->thickness(); mEnsureZInUserUnit( th );
    infomsg.postFixWord( uiStrings::sThickness().addMoreInfo( th ) );

    const int pridx = curPropIdx();
    if ( pridx >= 0 )
    {
	const auto& props = layerModel().sequence(seqidx).propertyRefs();
	const auto& pr = *props.get( pridx );
	const auto val = getLayerPropValue( *lay, pr, pridx );
	infomsg.postFixWord( toUiString(pr.name()).addMoreInfo(val) );
    }

    infoChanged.trigger( &infomsg, this );
}


void uiStratLayerModelDisp::usrClickedCB( CallBacker* )
{
    handleClick( false );
}


void uiStratLayerModelDisp::doubleClickedCB( CallBacker* )
{
    handleClick( true );
}


//=========================================================================>>


class uiSSLMFlatViewDataPack : public FlatDataPack
{
public:

uiSSLMFlatViewDataPack()
    : FlatDataPack( "Empty uiSSLM", new Array2DImpl<float>(0,0) )
{}
const char* dimName( bool dim0 ) const
{ return dim0 ? "Model Nr" : "Depth"; }

};


uiStratSimpleLayerModelDisp::uiStratSimpleLayerModelDisp(
		uiStratLayModEditTools& t, const LayerModelSuite& lms )
    : uiStratLayerModelDisp(t,lms)
    , emptyitm_(0)
    , zrg_(0.f,1.f)
    , curproprg_(mUdf(float),mUdf(float))
    , selseqad_(0)
{
    vwr_.appearance().ddpars_.show( false, false );
    fvdp_ = new uiSSLMFlatViewDataPack;
    DPM( DataPackMgr::FlatID() ).add( fvdp_ );
    vwr_.setPack( true, fvdp_->id() );
    vwr_.setPack( false, fvdp_->id() );
}


uiStratSimpleLayerModelDisp::~uiStratSimpleLayerModelDisp()
{
    detachAllNotifiers();
    vwr_.removeAuxDatas( layerads_ );
    deepErase( layerads_ );
    vwr_.removeAuxDatas( levelads_ );
    deepErase( levelads_ );
    delete vwr_.removeAuxData( selseqad_ );
}


void uiStratSimpleLayerModelDisp::selPropChg()
{
    reDrawSeqs();
}


void uiStratSimpleLayerModelDisp::dispLithChg()
{
    reDrawSeqs();
}


void uiStratSimpleLayerModelDisp::selContentChg()
{
    reDrawSeqs();
}


void uiStratSimpleLayerModelDisp::selLevelChg()
{
    if ( showFlattened() )
	showFlatChg();
    else
	reDrawLevels();
}


void uiStratSimpleLayerModelDisp::dispEachChg()
{
    reDrawAll();
}


void uiStratSimpleLayerModelDisp::handleClick( bool dbl )
{
    const int seqidx = usrPointedModelNr();
    if ( seqidx < 0 )
	return;

    MouseEventHandler& mevh = vwr_.rgbCanvas().getMouseEventHandler();
    const bool isright = OD::rightMouseButton( mevh.event().buttonState() );
    if ( dbl )
    {
	int layidx = 0;
	Layer* lay = usrPointedLayer( layidx );
	if ( !lay )
	    return;

	mevh.setHandled( true );
	const auto& seq = layerModel().sequence( seqidx );
	uiStratEditLayer dlg( this, *lay, seq, true );
	if ( dlg.go() && dlg.isChanged() )
	    forceRedispAll( true );
    }
    else if ( isright )
	handleRightClick( seqidx );
    else
    {
	selectSequence( seqidx );
	sequenceSelected.trigger();
	mevh.setHandled( true );
    }
}


void uiStratSimpleLayerModelDisp::handleRightClick( int seqidx )
{
    if ( seqidx < 0 || seqidx >= layerModel().size() )
	return;
    auto& seq = const_cast<LayerSequence&>( layerModel().sequence( seqidx ) );
    int layidx = 0;
    auto* lay = usrPointedLayer( layidx );

    uiMenu mnu( parent(), uiStrings::sAction() );
    if ( lay )
    {
	mnu.insertAction( new uiAction(m3Dots(uiStrings::sProperties())), 1 );
	mnu.insertAction( new uiAction(m3Dots(uiStrings::phrRemove(
				       uiStrings::sLayer().toLower()))), 2 );
    }
    mnu.insertAction( new uiAction(m3Dots(uiStrings::phrRemove(
                                                     tr("this Well")))), 3 );
    mnu.insertAction( new uiAction(m3Dots(tr("Dump all wells to file"))), 4 );
    mnu.insertAction( new uiAction(m3Dots(uiStrings::phrAdd(tr(
                                          "dumped wells from file")))), 5 );
    const int mnuid = mnu.exec();
    if ( mnuid < 0 )
	return;

    if ( mnuid == 1 )
    {
	uiStratEditLayer dlg( this, *lay, seq, true );
	if ( dlg.go() && dlg.isChanged() )
	    forceRedispAll( true );
    }
    else if ( mnuid == 4 || mnuid == 5 )
	doLayerModelIO( mnuid == 5 );
    else if ( mnuid == 3 )
    {
	const_cast<LayerModel&>(layerModel()).removeSequence( seqidx );
	forceRedispAll( true );
    }
    else
    {
	uiDialog dlg( this, uiDialog::Setup( uiStrings::phrRemove(
				  uiStrings::sLayer().toLower()),
		                  uiStrings::phrRemove(toUiString("'%1'")
				  .arg(lay->name())),
                                  mODHelpKey(mStratSimpleLayerModDispHelpID)));
	uiGenInput* gi = new uiGenInput( &dlg, uiStrings::sRemove(),
                                         BoolInpSpec(true,
                                         tr("Only this layer"),
                                         tr("All layers with this ID")) );
	if ( dlg.go() )
	    removeLayers( seq, layidx, !gi->getBoolValue() );
    }
}


void uiStratSimpleLayerModelDisp::removeLayers( LayerSequence& seq,
					int layidx, bool doall )
{
    if ( !doall )
    {
	delete seq.layers().removeSingle( layidx );
	seq.prepareUse();
    }
    else
    {
	const Strat::LeafUnitRef& lur = seq.layers()[layidx]->unitRef();
	for ( int ils=0; ils<layerModel().size(); ils++ )
	{
	    auto& ls = const_cast<LayerSequence&>( layerModel().sequence(ils) );
	    bool needprep = false;
	    for ( int ilay=0; ilay<ls.layers().size(); ilay++ )
	    {
		const Layer& lay = *ls.layers()[ilay];
		if ( &lay.unitRef() == &lur )
		{
		    delete ls.layers().removeSingle( ilay );
		    ilay--; needprep = true;
		}
	    }
	    if ( needprep )
		ls.prepareUse();
	}
    }

    forceRedispAll( true );
}


void uiStratSimpleLayerModelDisp::reDrawLevels()
{
    updateLevelAuxData();
    vwr_.handleChange( FlatView::Viewer::Auxdata );
}


void uiStratSimpleLayerModelDisp::reDrawSeqs()
{
    updateLayerAuxData();
    vwr_.handleChange( FlatView::Viewer::Auxdata );
}


void uiStratSimpleLayerModelDisp::drawSelectedSequence()
{
    updateSelSeqAuxData();
    vwr_.handleChange( FlatView::Viewer::Auxdata );
}


void uiStratSimpleLayerModelDisp::reDrawAll()
{
    if ( layerModel().isEmpty() )
    {
	if ( !emptyitm_ )
	    emptyitm_ = vwr_.rgbCanvas().scene().addItem(
				new uiTextItem( uiString::empty(),
				mAlignment(HCenter,VCenter) ) );

	emptyitm_->setPenColor( Color::Black() );
	emptyitm_->setPos( uiPoint( vwr_.rgbCanvas().viewWidth()/2,
				    vwr_.rgbCanvas().viewHeight() / 2 ) );
	return;
    }
    else if ( emptyitm_ )
    {
	delete vwr_.rgbCanvas().scene().removeItem( emptyitm_ );
	emptyitm_ = 0;
    }

    uiUserShowWait usw( this, uiStrings::sUpdatingDisplay() );
    updateLayerAuxData();
    updateLevelAuxData();
    updateSelSeqAuxData();
    vwr_.handleChange( FlatView::Viewer::Auxdata );
}


void uiStratLayerModelDisp::getFlattenedZRange( Interval<float>& zrg ) const
{
    const auto& laymod = layerModel();
    const auto sellvl = tools_.selLevel();
    const auto nrseqs = laymod.size();
    for ( int iseq=0; iseq<nrseqs; iseq++ )
    {
	const auto& seq = laymod.sequence( iseq );
	auto seqzrg = seq.zRange();
	const float lvldpth = seqLvlDepth( seq, sellvl );
	if ( mIsUdf(lvldpth) )
	    continue;

	seqzrg.shift( -lvldpth );
	if ( mIsUdf(zrg.start) )
	    zrg = seqzrg;
	else
	    zrg.include( seqzrg, false );
    }
}


Interval<float> uiStratLayerModelDisp::getModelRange( int propidx ) const
{
    const auto nrseqs = layerModel().size();
    Interval<float> rg( mUdf(float), mUdf(float) );
    const bool wantzrg = propidx < 0;

    if ( wantzrg && showFlattened() )
	getFlattenedZRange( rg );
    else
    {
	const auto& laymod = layerModel();
	for ( int iseq=0; iseq<nrseqs; iseq++ )
	{
	    const auto vrg = laymod.sequence(iseq).propRange( propidx );
	    if ( mIsUdf(rg.start) )
		rg = vrg;
	    else
		rg.include( vrg, false );
	}
    }

    if ( wantzrg && mIsUdf(rg.start) )
	rg.start = rg.stop = 0.f;

    return rg;
}


void uiStratSimpleLayerModelDisp::updateSelSeqAuxData()
{
    if ( selseqidx_ < 0 )
	{ delete selseqad_; selseqad_ = 0; return; }

    if ( !selseqad_ )
    {
	selseqad_ = vwr_.createAuxData( 0 );
	selseqad_->linestyle_ =
		OD::LineStyle( OD::LineStyle::Dot, 2, Color::Black() );
	selseqad_->zvalue_ = uiFlatViewer::auxDataZVal() + 2;
	vwr_.addAuxData( selseqad_ );
    }

    StepInterval<double> yrg = fvdp_->posData().range( false );
    selseqad_->poly_.erase();
    selseqad_->poly_ += FlatView::Point( selseqidx_+1, yrg.start);
    selseqad_->poly_ += FlatView::Point( selseqidx_+1, yrg.stop );
}


void uiStratSimpleLayerModelDisp::clearObsoleteAuxDatas( AuxDataSet& ads,
							 int fromidx )
{
    while ( ads.size() > fromidx )
    {
	auto* ad = ads.removeSingle( fromidx );
	delete vwr_.removeAuxData( ad );
    }
}


void uiStratSimpleLayerModelDisp::updateLevelAuxData()
{
    if ( layerModel().isEmpty() )
	return;

    const auto nrlevels = tools_.levelNames().size();
    const auto dispeach = mDispEach();
    const auto sellvlidx = mSelLevelIdx();
    const auto flattened = showFlattened();

    int auxdataidx = 0;
    const auto& sellvlzvals = lvldpths_.get( sellvlidx );
    for( int ilvl=0; ilvl<nrlevels; ilvl++ )
    {
	const Level lvl = Strat::LVLS().getByIdx( ilvl );
	const Color lvlcol = lvl.color();
	const auto& zvals = lvldpths_.get( ilvl );

	for ( int iseq=0; iseq<zvals.size(); iseq+=dispeach )
	{
	    float zlvl = zvals[iseq];
	    if ( mIsUdf(zlvl) )
		continue;
	    if ( flattened )
	    {
		const auto lvldpth = sellvlzvals.get( iseq );
		if ( mIsUdf(lvldpth) )
		    continue;
		zlvl -= lvldpth;
	    }

	    mEnsureZInUserUnit( zlvl );
	    const double ypos = zlvl;
	    const double xpos1 = iseq + 1;
	    const double xpos2 = iseq + 1 + dispeach;
	    FlatView::AuxData* levelad = 0;
	    if ( levelads_.validIdx(auxdataidx) )
		levelad = levelads_[auxdataidx];
	    else
	    {
		levelad = vwr_.createAuxData( 0 );
		levelad->zvalue_ = uiFlatViewer::auxDataZVal() + 1;
		vwr_.addAuxData( levelad );
		levelads_ += levelad;
	    }
	    levelad->poly_.erase();
	    levelad->linestyle_ = OD::LineStyle(OD::LineStyle::Dot,2,lvlcol);
	    levelad->poly_ += FlatView::Point( xpos1, ypos );
	    levelad->poly_ += FlatView::Point( xpos2, ypos );

	    if ( ilvl != sellvlidx )
		levelad->zvalue_ = 3;
	    else
	    {
		levelad->zvalue_ = 5;
		levelad->linestyle_.type_ = OD::LineStyle::Solid;
		levelad->linestyle_.width_ *= 2;
	    }

	    auxdataidx++;
	}
    }

    clearObsoleteAuxDatas( levelads_, auxdataidx );
}


void uiStratSimpleLayerModelDisp::updateLayerAuxData()
{
    const auto laymod = layerModel();
    const auto nrseq = laymod.size();
    const auto dispeach = mDispEach();
    const auto dispprop = curPropIdx();
    const auto uselithcols = mUseLithCols();
    const BufferString cntnm( tools_.selContent() );
    const auto* selcontent = layerModel().refTree().contents().getByName(cntnm);
    const auto isallcontents = cntnm == sKey::All();
    Interval<float> vrg( curproprg_ );
    if ( vrg.width() == 0 )
	{ vrg.start -= 1; vrg.stop += 1; }
    const float vwdth = vrg.width();
    const auto sellvlidx = mSelLevelIdx();
    const auto flattened = showFlattened();

    int auxdataidx = 0;
    for ( auto iseq=0; iseq<nrseq; iseq+=dispeach )
    {
	const auto& seq = laymod.sequence( iseq );
	const auto& layers = seq.layers();
	const auto nrlayers = layers.size();
	float zshift = 0.f;
	if ( flattened )
	{
	    zshift = lvldpths_.get( sellvlidx ).get( iseq );
	    if ( mIsUdf(zshift) )
		continue;
	}

	const auto& propref = *seq.propertyRefs().get( dispprop );
	const UnitOfMeasure* uom = UoMR().getDefault( propref.name(),
				    propref.stdType() );
	for ( auto ilay=0; ilay<nrlayers; ilay++ )
	{
	    const auto& lay = *layers.get( ilay );
	    const float val = getLayerPropValue( lay, uom, dispprop );

	    bool mustannotcont = false;
	    if ( !lay.content().isUnspecified() )
		mustannotcont = isallcontents
		    || (selcontent && lay.content() == *selcontent);
	    const Color laycol = lay.dispColor( uselithcols );
	    const Color pencol = mustannotcont ? lay.content().color_ : laycol;
	    bool canjoinlayers = ilay > 0;
	    if ( canjoinlayers )
	    {
		const Layer& prevlay = *seq.layers()[ilay-1];
		const Color prevlaycol = prevlay.dispColor( uselithcols );
		canjoinlayers =
		    prevlay.content()==lay.content() && prevlaycol==laycol;
	    }

	    if ( canjoinlayers )
		auxdataidx--;
	    FlatView::AuxData* layad = 0;
	    if ( layerads_.validIdx(auxdataidx) )
		layad = layerads_[auxdataidx];
	    else
	    {
		layad = vwr_.createAuxData( lay.name().buf() );
		layad->zvalue_ = uiFlatViewer::auxDataZVal()-1;
		layad->close_ = true;
		vwr_.addAuxData( layad );
		layerads_ += layad;
	    }

	    if ( !canjoinlayers )
		layad->poly_.erase();
	    else
		layad->poly_.pop();

	    layad->fillcolor_ = laycol;
	    layad->linestyle_ = OD::LineStyle( OD::LineStyle::Solid, 2, pencol);
	    if ( mustannotcont )
		layad->fillpattern_ = lay.content().pattern_;
	    else
	    {
		OD::FillPattern fp; fp.setFullFill();
		layad->fillpattern_ = fp;
	    }
	    const double x0 = iseq + 1;
	    double relx = (double)(val-vrg.start)/vwdth;
	    relx *= dispeach;
	    const double x1 = iseq + 1 + relx;
	    float z0 = lay.zTop() - zshift;
            float z1 = lay.zBot() - zshift;
	    mEnsureZInUserUnit( z0 );
	    mEnsureZInUserUnit( z1 );
	    if ( !canjoinlayers )
		layad->poly_ += FlatView::Point( x0, (double)z0 );
	    layad->poly_ += FlatView::Point( x1, (double)z0 );
	    layad->poly_ += FlatView::Point( x1, (double)z1 );
	    layad->poly_ += FlatView::Point( x0, (double)z1 );
	    auxdataidx++;
	}
    }

    clearObsoleteAuxDatas( layerads_, auxdataidx );
}


void uiStratSimpleLayerModelDisp::handleModelChange()
{
    forceRedispAll( false );
}


void uiStratSimpleLayerModelDisp::forceRedispAll( bool triggermodchg )
{
    zrg_ = getModelRange( -1 );
    curproprg_ = getModelRange( curPropIdx() );

    updateDataPack();
    fillLevelDepths();

    reDrawAll();

    if ( triggermodchg )
	modelChanged.trigger();
}


void uiStratSimpleLayerModelDisp::updateDataPack()
{
    const LayerModel& lm = layerModel();
    const int nrseqs = lm.size();
    mGetZrgInUserUnit( zrg_, dispzrg );
    StepInterval<double> zrg( dispzrg.start, dispzrg.stop,
			      dispzrg.width() * 0.2 );

    fvdp_->posData().setRange( true,
			StepInterval<double>( 1, nrseqs<2 ? 1 : nrseqs, 1 ) );
    fvdp_->posData().setRange( false, zrg );
    fvdp_->setName( "Simple Layer Model Display BackDrop" );
}

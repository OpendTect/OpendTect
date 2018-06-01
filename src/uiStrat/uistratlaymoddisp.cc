/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uistratsimplelaymoddisp.h"
#include "uistratlaymodtools.h"
#include "uistrateditlayer.h"
#include "uigraphicsitemimpl.h"
#include "uigraphicsscene.h"
#include "uigraphicsview.h"
#include "uifileinput.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiflatviewer.h"
#include "uimultiflatviewcontrol.h"
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
#include "od_helpids.h"
#include <stdio.h>

#define mGetConvZ(var,conv) \
    if ( SI().depthsInFeet() ) var *= conv
#define mGetRealZ(var) mGetConvZ(var,mFromFeetFactorF)
#define mGetDispZ(var) mGetConvZ(var,mToFeetFactorF)
#define mGetDispZrg(src,target) \
    Interval<float> target( src ); \
    if ( SI().depthsInFeet() ) \
	target.scale( mToFeetFactorF )

static const int cMaxNrLayers4RectDisp = 50000; // Simple displayer


uiStratLayerModelDisp::uiStratLayerModelDisp( uiStratLayModEditTools& t,
					  const Strat::LayerModelProvider& lmp)
    : uiGroup(t.parent(),"LayerModel display")
    , tools_(t)
    , lmp_(lmp)
    , zrg_(0,1)
    , selseqidx_(-1)
    , vwr_(* new uiFlatViewer(this))
    , flattened_(false)
    , frtxtitm_(0)
    , sequenceSelected(this)
    , genNewModelNeeded(this)
    , rangeChanged(this)
    , modelEdited(this)
    , infoChanged(this)
    , dispPropChanged(this)
{
    vwr_.setInitialSize( uiSize(600,250) );
    vwr_.setStretch( 2, 2 );
    vwr_.disableStatusBarUpdate();
    vwr_.setZDomain( ZDomain::Depth() );
    FlatView::Appearance& app = vwr_.appearance();
    app.setGeoDefaults( true );
    app.setDarkBG( false );
    app.annot_.title_.setEmpty();
    app.annot_.x1_.showAll( true );
    app.annot_.x2_.showAll( true );
    app.annot_.x1_.name_ = "Model Nr";
    app.annot_.x2_.name_ = "Depth";
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

    vwr_.rgbCanvas().getMouseEventHandler().buttonReleased.notify(
			mCB(this,uiStratLayerModelDisp,usrClicked) );
    vwr_.rgbCanvas().getMouseEventHandler().doubleClick.notify(
			mCB(this,uiStratLayerModelDisp,doubleClicked) );
    vwr_.rgbCanvas().getMouseEventHandler().movement.notify(
			mCB(this,uiStratLayerModelDisp,mouseMoved) );
    mAttachCB( vwr_.rgbCanvas().reSize, uiStratLayerModelDisp::updateTextPosCB);
#   define mSetCB(notifnm) tools_.notifnm.notify( \
	mCB(this,uiStratLayerModelDisp,notifnm##CB)  )

    mSetCB( selPropChg ); mSetCB( dispLithChg ); mSetCB( selContentChg );
    mSetCB( selLevelChg ); mSetCB( dispEachChg ); mSetCB( dispZoomedChg );
}


uiStratLayerModelDisp::~uiStratLayerModelDisp()
{
    detachAllNotifiers();
}


uiGraphicsScene& uiStratLayerModelDisp::scene() const
{
    return const_cast<uiStratLayerModelDisp*>(this)->vwr_.rgbCanvas().scene();
}


const Strat::LayerModel& uiStratLayerModelDisp::layerModel() const
{
    return lmp_.getCurrent();
}


uiGroup* uiStratLayerModelDisp::getDisplayClone( uiParent* p )	const
{
    uiFlatViewer* vwr = new uiFlatViewer( p );
    vwr->rgbCanvas().disableImageSave();
    vwr->setInitialSize( uiSize(800,300) );
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


void uiStratLayerModelDisp::setFlattened( bool yn, bool trigger )
{
    flattened_ = yn;
    if ( !trigger ) return;

    modelUpdate();
}


bool uiStratLayerModelDisp::haveAnyZoom() const
{
    const int nrseqs = layerModel().size();
    mGetDispZrg(zrg_,dispzrg);
    uiWorldRect wr( 1, dispzrg.start, nrseqs, dispzrg.stop );
    return zoomBox().isInside( wr, 1e-5 );
}


float uiStratLayerModelDisp::getLayerPropValue( const Strat::Layer& lay,
						const PropertyRef* pr,
						int propidx ) const
{
    const UnitOfMeasure* uom = UoMR().getDefault( pr->name(), pr->stdType());
    const float sival = propidx < lay.nrValues() ? lay.value( propidx )
						 : mUdf(float);
    return uom ? uom->getUserValueFromSI( sival ) : sival;
}


void uiStratLayerModelDisp::displayFRText()
{ displayFRText( fluidreplon_, isbrinefilled_ ); }


void uiStratLayerModelDisp::displayFRText( bool yn, bool isbrine )
{
    if ( !frtxtitm_ )
    {
	const uiPoint pos( mNINT32( scene().width()/2 ),
			   mNINT32( scene().height()-10 ) );
	frtxtitm_ = scene().addItem( new uiTextItem(pos,uiString::emptyString(),
						mAlignment(HCenter,VCenter)) );
	frtxtitm_->setPenColor( Color::Black() );
	frtxtitm_->setZValue( 999999 );
	frtxtitm_->setMovable( true );
    }

    frtxtitm_->setVisible( yn );
    if ( yn )
    {
	frtxtitm_->setText( isbrine ? tr("Brine filled")
				    : tr("Hydrocarbon filled") );
    }
}


void uiStratLayerModelDisp::updateTextPosCB( CallBacker* )
{
    if ( !frtxtitm_ )
	return;

    const uiPoint pos( mNINT32( scene().width()/2 ),
		       mNINT32( scene().height()-10 ) );
    frtxtitm_->setPos( pos );
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
			 mNoDlgTitle,
			 mODHelpKey(mStratLayerModelDispIOHelpID)) )
    , doreplacefld_(0)
    , eachfld_(0)
    , nrdisplayfld_(0)
    , presmathfld_(0)
    , lm_(lm)
    , pars_(pars)
    , doread_(doread)
{
    mDefineStaticLocalObject( BufferString, fixeddumpfnm,
			      = GetEnvVar( "OD_FIXED_LAYMOD_DUMPFILE" ) );
    if ( !fixeddumpfnm.isEmpty() )
	fnm_ = BufferString( fixeddumpfnm );

    uiFileInput::Setup su( uiFileDialog::Gen, fnm_ );
    su.forread_ = doread;
    filefld_ = new uiFileInput( this, uiStrings::sFileName(), su );

    if ( doread )
    {
	const Interval<int> valrg( 1, 1000 );
	IntInpSpec val( 1, valrg );
	eachfld_ = new uiGenInput( this, tr("Use Each"), val );
	eachfld_->attach( alignedBelow, filefld_ );

	doreplacefld_ = new uiGenInput( this,
					tr("Clear existing model before add"),
					BoolInpSpec(true) );
	doreplacefld_->attach( alignedBelow, eachfld_ );

	val = 10;
	val.setLimits( valrg );
	nrdisplayfld_ = new uiGenInput( this, tr("Display Nr Models"), val );
	nrdisplayfld_->setWithCheck();
	nrdisplayfld_->setChecked( true );
	nrdisplayfld_->attach( alignedBelow, doreplacefld_ );
    }
    else
    {
	presmathfld_ = new uiGenInput( this, tr("Preserve Math Formulas"),
				       BoolInpSpec(true) );
	presmathfld_->attach( alignedBelow, filefld_ );
    }

    usePar();
}


bool usePar()
{
    BufferString fnm;
    if ( pars_.get(sKey::FileName(),fnm) )
	filefld_->setFileName( fnm );

    if ( doread_ )
    {
	int each;
	if ( pars_.get(sKeyUseEach(),each) )
	    eachfld_->setValue( each );

	bool doreplace = true;
	if ( pars_.getYN(sKeyDoClear(),doreplace) )
	    doreplacefld_->setValue( doreplace );

	int nrmodels;
	if ( pars_.get(sKeyNrDisplay(),nrmodels) )
	{
	    nrdisplayfld_->setValue( nrmodels );
	    nrdisplayfld_->setChecked( true );
	}
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
    pars_.set( sKey::FileName(), filefld_->fileName() );
    if ( doread_ )
    {
	pars_.set( sKeyUseEach(), eachfld_->getIntValue() );
	pars_.setYN( sKeyDoClear(), doreplacefld_->getBoolValue() );
	if ( nrdisplayfld_->isChecked() )
	    pars_.set( sKeyNrDisplay(), nrdisplayfld_->getIntValue() );
    }
    else
    {
	pars_.setYN( sKeyPreserveMath(), presmathfld_->getBoolValue() );
    }
}


int getNrDisplayModels()
{
    int nrmoddisp;
    if ( !pars_.get(sKeyNrDisplay(),nrmoddisp) )
	return mUdf(int);

    return nrmoddisp;
}


bool acceptOK( CallBacker* )
{
    if ( fnm_.isEmpty() )
    {
	if ( !filefld_->fileName() )
	    mErrRet(tr("Please provide a file name"))

	fnm_ = filefld_->fileName();
    }

    if ( doread_ && !File::exists(fnm_) )
	mErrRet(tr("Input file does not exist"))

    if ( doread_ )
    {
	od_istream strm( fnm_ );
	if ( !strm.isOK() )
	    mErrRet(tr("Cannot open:\n%1\nfor read").arg(fnm_))

	Strat::LayerModel newlm;
	if ( !newlm.read(strm) )
	    mErrRet(tr("Cannot read layer model from file.\nDetails may be "
		       "in the log file ('Utilities-Show log file')"))

	const int each = eachfld_->getIntValue();
	Strat::LayerModel& lm = const_cast<Strat::LayerModel&>( lm_ );
	if ( doreplacefld_->getBoolValue() )
	    lm.setEmpty();

	for ( int iseq=0; iseq<newlm.size(); iseq+=each )
	    lm.addSequence( newlm.sequence(iseq) );
    }
    else
    {
	od_ostream strm( fnm_ );
	if ( !strm.isOK() )
	    mErrRet(tr("Cannot open:\n%1\nfor write").arg(fnm_))

	if ( !lm_.write(strm,0,presmathfld_->getBoolValue()) )
	    mErrRet(m3Dots(tr("Unknown error during write")))
    }

    fillPar();

    return true;
}

    uiFileInput*		filefld_;
    uiGenInput*			doreplacefld_;
    uiGenInput*			eachfld_;
    uiGenInput*			nrdisplayfld_;
    uiGenInput*			presmathfld_;

    const Strat::LayerModel&	lm_;
    BufferString		fnm_;
    IOPar&			pars_;
    bool			doread_;

};


#undef mErrRet
#define mErrRet(s) \
{ \
    uiMainWin* mw = uiMSG().setMainWin( parent()->mainwin() ); \
    uiMSG().error(s); \
    uiMSG().setMainWin(mw); \
    return false; \
}

bool uiStratLayerModelDisp::doLayerModelIO( bool foradd )
{
    const Strat::LayerModel& lm = layerModel();
    if ( !foradd && lm.isEmpty() )
	mErrRet( tr("Please generate at least one layer sequence") )

    uiStratLayerModelDispIO dlg( this, lm, dumppars_, foradd );
    if ( !dlg.go() )
	return false;

    const int nrdisplaymodels = dlg.getNrDisplayModels();
    if ( !mIsUdf(nrdisplaymodels) && nrdisplaymodels > 0 )
    {
	const int nrmodels = lm.size();
	if ( nrdisplaymodels <= nrmodels )
	    tools_.setDispEach( nrmodels/nrdisplaymodels );
    }

    return true;
}


bool uiStratLayerModelDisp::getCurPropDispPars(
	LMPropSpecificDispPars& pars ) const
{
    LMPropSpecificDispPars disppars;
    disppars.propnm_ = tools_.selProp();
    const int curpropidx = lmdisppars_.indexOf( disppars );
    if ( curpropidx<0 )
	return false;
    pars = lmdisppars_[curpropidx];
    return true;
}


bool uiStratLayerModelDisp::setPropDispPars(const LMPropSpecificDispPars& pars)
{
    BufferStringSet propnms;
    for ( int idx=0; idx<layerModel().propertyRefs().size(); idx++ )
	propnms.add( layerModel().propertyRefs()[idx]->name() );

    if ( !propnms.isPresent(pars.propnm_) )
	return false;
    const int propidx = lmdisppars_.indexOf( pars );
    if ( propidx<0 )
	lmdisppars_ += pars;
    else
	lmdisppars_[propidx] = pars;
    return true;
}


int uiStratLayerModelDisp::getClickedModelNr() const
{
    MouseEventHandler& mevh = vwr_.rgbCanvas().getMouseEventHandler();
    if ( layerModel().isEmpty() || !mevh.hasEvent() || mevh.isHandled() )
	return -1;
    const MouseEvent& mev = mevh.event();
    const float xsel = vwr_.getWorld2Ui().toWorldX( mev.pos().x );
    int selidx = (int)(ceil( xsel )) - 1;
    if ( selidx < 0 || selidx > layerModel().size() )
	selidx = -1;
    return selidx;
}


void uiStratLayerModelDisp::mouseMoved( CallBacker* )
{
    IOPar statusbarmsg;
    const int selseq = getClickedModelNr();
    BufferString modelnrstr;
    sprintf( modelnrstr.getCStr(), "%5d", selseq );
    statusbarmsg.set( "Model Number", modelnrstr );
    const MouseEvent& mev = vwr_.rgbCanvas().getMouseEventHandler().event();
    float depth = vwr_.getWorld2Ui().toWorldY( mev.pos().y );
    if ( !Math::IsNormalNumber(depth) )
    {
	mDefineStaticLocalObject( bool, havewarned, = false );
	if ( !havewarned )
	    { havewarned = true; pErrMsg("Invalid number from axis handler"); }
	depth = 0;
    }

    BufferString depthstr;
    sprintf( depthstr.getCStr(), "%6.0f", depth );
    depthstr += SI().depthsInFeet() ? "(ft)" : "(m)";
    statusbarmsg.set( "Depth", depthstr );
    if ( SI().depthsInFeet() )
	depth *= mFromFeetFactorF;

    if ( selseq >0 && selseq<=layerModel().size() )
    {
	const Strat::LayerSequence& seq = layerModel().sequence( selseq-1 );
	for ( int ilay=0; ilay<seq.size(); ilay++ )
	{
	    const Strat::Layer& lay = *seq.layers()[ilay];
	    float z0 = lay.zTop();
	    float z1 = lay.zBot();
	    if ( flattened_ && !lvldpths_.validIdx(selseq-1) )
	    {
		const float lvldpth = lvldpths_[selseq-1];
		z0 -= lvldpth;
		z1 -= lvldpth;
	    }

	    const int disppropidx = tools_.selPropIdx();
	    if ( depth >= z0 && depth<= z1 &&
		 seq.propertyRefs().validIdx(disppropidx) )
	    {
		const PropertyRef* pr = seq.propertyRefs()[disppropidx];
		const float val = getLayerPropValue(lay,pr,disppropidx);
		statusbarmsg.set( pr->name(), val );
		statusbarmsg.set( "Layer", lay.name() );
		statusbarmsg.set( "Lithology", lay.lithology().name() );
		if ( !lay.content().isUnspecified() )
		    statusbarmsg.set( "Content", lay.content().name() );
		break;
	    }
	}
    }
    infoChanged.trigger( statusbarmsg, this );
}


void uiStratLayerModelDisp::usrClicked( CallBacker* )
{
    handleClick( false );
}


void uiStratLayerModelDisp::doubleClicked( CallBacker* )
{
    handleClick( true );
}



//=========================================================================>>

class LayerModelDataPack : public FlatDataPack
{
public:

LayerModelDataPack()
    : FlatDataPack( "Layer Model", new Array2DImpl<float>(0,0) )
{}


const char* dimName( bool dim0 ) const
{ return dim0 ? "Model Nr" : "Depth"; }

};

uiStratSimpleLayerModelDisp::uiStratSimpleLayerModelDisp(
		uiStratLayModEditTools& t, const Strat::LayerModelProvider& l )
    : uiStratLayerModelDisp(t,l)
    , emptyitm_(0)
    , zoomboxitm_(0)
    , dispprop_(1)
    , dispeach_(1)
    , zoomwr_(mUdf(double),0,0,0)
    , fillmdls_(false)
    , uselithcols_(true)
    , showzoomed_(true)
    , vrg_(0,1)
    , logblcklineitms_(*new uiGraphicsItemSet)
    , logblckrectitms_(*new uiGraphicsItemSet)
    , lvlitms_(*new uiGraphicsItemSet)
    , contitms_(*new uiGraphicsItemSet)
    , selseqitm_(0)
    , selseqad_(0)
    , selectedlevel_(-1)
    , selectedcontent_(0)
    , allcontents_(false)
{
    vwr_.appearance().ddpars_.show( false, false );
    emptydp_ = new LayerModelDataPack();
    DPM( DataPackMgr::FlatID() ).addAndObtain( emptydp_ );
    vwr_.setPack( true, emptydp_->id() );
    vwr_.setPack( false, emptydp_->id() );
}


uiStratSimpleLayerModelDisp::~uiStratSimpleLayerModelDisp()
{
    DPM( DataPackMgr::FlatID() ).release( emptydp_ );
    eraseAll();
    delete &lvlitms_;
    delete &logblcklineitms_;
    delete &logblckrectitms_;
}


void uiStratSimpleLayerModelDisp::eraseAll()
{
    MouseCursorChanger mc( MouseCursor::Wait );
    logblcklineitms_.erase();
    logblckrectitms_.erase();
    lvlitms_.erase();
    delete vwr_.rgbCanvas().scene().removeItem( emptyitm_ ); emptyitm_ = 0;
    lvldpths_.erase();
    vwr_.removeAuxDatas( layerads_ );
    deepErase( layerads_ );
    vwr_.removeAuxDatas( levelads_ );
    deepErase( levelads_ );
    delete vwr_.removeAuxData( selseqad_ );
}



void uiStratSimpleLayerModelDisp::selPropChgCB( CallBacker* )
{ reDrawSeq(); }


void uiStratSimpleLayerModelDisp::dispLithChgCB( CallBacker* )
{ reDrawSeq(); }


void uiStratSimpleLayerModelDisp::selContentChgCB( CallBacker* )
{ reDrawSeq(); }


void uiStratSimpleLayerModelDisp::selLevelChgCB( CallBacker* )
{ reDrawLevels(); }


void uiStratSimpleLayerModelDisp::dispEachChgCB( CallBacker* )
{ reDrawAll(); }


void uiStratSimpleLayerModelDisp::handleClick( bool dbl )
{
    const int selidx = getClickedModelNr()-1;
    if ( selidx < 0 ) return;

    MouseEventHandler& mevh = vwr_.rgbCanvas().getMouseEventHandler();
    const bool isright = OD::rightMouseButton( mevh.event().buttonState() );
    if ( dbl || isright )
	handleRightClick(selidx);
    else
    {
	selectSequence( selidx );
	sequenceSelected.trigger();
	mevh.setHandled( true );
    }
}


void uiStratSimpleLayerModelDisp::handleRightClick( int selidx )
{
    if ( selidx < 0 || selidx >= layerModel().size() )
	return;

    Strat::LayerSequence& ls = const_cast<Strat::LayerSequence&>(
					layerModel().sequence( selidx ) );
    ObjectSet<Strat::Layer>& lays = ls.layers();
    MouseEventHandler& mevh = vwr_.rgbCanvas().getMouseEventHandler();
    float zsel = vwr_.getWorld2Ui().toWorldY( mevh.event().pos().y );
    mGetRealZ( zsel );
    mevh.setHandled( true );
    if ( flattened_ )
    {
	const float lvlz = lvldpths_[selidx];
	if ( mIsUdf(lvlz) )
	    return;
	zsel += lvlz;
    }

    const int layidx = ls.layerIdxAtZ( zsel );
    if ( lays.isEmpty() || layidx < 0 )
	return;

    uiMenu mnu( parent(), uiStrings::sAction() );
    mnu.insertAction( new uiAction(m3Dots(uiStrings::sProperties())), 1 );
    mnu.insertAction( new uiAction(m3Dots(uiStrings::phrRemove(
                                       uiStrings::sLayer().toLower()))), 2 );
    mnu.insertAction( new uiAction(m3Dots(uiStrings::phrRemove(
                                                     tr("this Well")))), 3 );
    mnu.insertAction( new uiAction(m3Dots(tr("Dump all wells to file"))), 4 );
    mnu.insertAction( new uiAction(m3Dots(uiStrings::phrAdd(tr(
                                          "dumped wells from file")))), 5 );
    const int mnuid = mnu.exec();
    if ( mnuid < 0 ) return;

    Strat::Layer& lay = *ls.layers()[layidx];
    if ( mnuid == 1 )
    {
	uiStratEditLayer dlg( this, lay, ls, true );
	if ( dlg.go() && dlg.isChanged() )
	    forceRedispAll( true );
    }
    else if ( mnuid == 4 || mnuid == 5 )
	doLayModIO( mnuid == 5 );
    else if ( mnuid == 3 )
    {
	const_cast<Strat::LayerModel&>(layerModel()).removeSequence( selidx );
	forceRedispAll( true );
    }
    else
    {

	uiDialog dlg( this, uiDialog::Setup( uiStrings::phrRemove(
				  uiStrings::sLayer().toLower()),
		                  uiStrings::phrRemove(toUiString("'%1'")
				  .arg(lay.name())),
                                  mODHelpKey(mStratSimpleLayerModDispHelpID)));
	uiGenInput* gi = new uiGenInput( &dlg, uiStrings::sRemove(),
                                         BoolInpSpec(true,
                                         tr("Only this layer"),
                                         tr("All layers with this ID")) );
	if ( dlg.go() )
	    removeLayers( ls, layidx, !gi->getBoolValue() );
    }
}


void uiStratSimpleLayerModelDisp::doLayModIO( bool foradd )
{
    if ( doLayerModelIO(foradd) && foradd )
	forceRedispAll( true );
}


void uiStratSimpleLayerModelDisp::removeLayers( Strat::LayerSequence& seq,
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
	    Strat::LayerSequence& ls = const_cast<Strat::LayerSequence&>(
						layerModel().sequence( ils ) );
	    bool needprep = false;
	    for ( int ilay=0; ilay<ls.layers().size(); ilay++ )
	    {
		const Strat::Layer& lay = *ls.layers()[ilay];
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


void uiStratSimpleLayerModelDisp::forceRedispAll( bool modeledited )
{
    reDrawAll();
    if ( modeledited )
	modelEdited.trigger();
}


void uiStratSimpleLayerModelDisp::dispZoomedChgCB( CallBacker* )
{
    mDynamicCastGet(uiMultiFlatViewControl*,stdctrl,vwr_.control())
    if ( stdctrl )
    {
	stdctrl->setZoomCoupled( tools_.dispZoomed() );
	stdctrl->setDrawZoomBoxes( !tools_.dispZoomed() );
    }
}


void uiStratSimpleLayerModelDisp::reDrawLevels()
{
    if ( flattened_ )
    {
	updateDataPack();
	updateLayerAuxData();
    }
    else
	getBounds();
    updateLevelAuxData();
    vwr_.handleChange( mCast(unsigned int,FlatView::Viewer::Auxdata) );
}


void uiStratSimpleLayerModelDisp::reDrawSeq()
{
    getBounds();
    updateLayerAuxData();
    vwr_.handleChange( mCast(unsigned int,FlatView::Viewer::Auxdata) );
}


void uiStratSimpleLayerModelDisp::reDrawAll()
{
    layerModel().prepareUse();
    if ( layerModel().isEmpty() )
    {
	if ( !emptyitm_ )
	    emptyitm_ = vwr_.rgbCanvas().scene().addItem(
				new uiTextItem( tr("<---empty--->"),
				mAlignment(HCenter,VCenter) ) );

	emptyitm_->setPenColor( Color::Black() );
	emptyitm_->setPos( uiPoint( vwr_.rgbCanvas().width()/2,
				    vwr_.rgbCanvas().height() / 2 ) );
	return;
    }
    else if ( emptyitm_ )
    {
	delete vwr_.rgbCanvas().scene().removeItem( emptyitm_ );
	emptyitm_ = 0;
    }

    doDraw();
}


void uiStratSimpleLayerModelDisp::setZoomBox( const uiWorldRect& wr )
{
}


float uiStratSimpleLayerModelDisp::getDisplayZSkip() const
{
    if ( layerModel().isEmpty() ) return 0;
    return layerModel().sequence(0).startDepth();
}


void uiStratSimpleLayerModelDisp::updZoomBox()
{
}


#define mStartLayLoop(chckdisp,perseqstmt) \
    const int nrseqs = layerModel().size(); \
    for ( int iseq=0; iseq<nrseqs; iseq++ ) \
    { \
	if ( chckdisp && !isDisplayedModel(iseq) ) continue; \
	const float lvldpth = lvldpths_[iseq]; \
	if ( flattened_ && mIsUdf(lvldpth) ) continue; \
	int layzlvl = 0; \
	const Strat::LayerSequence& seq = layerModel().sequence( iseq ); \
	const int nrlays = seq.size(); \
	perseqstmt; \
	for ( int ilay=0; ilay<nrlays; ilay++ ) \
	{ \
	    layzlvl++; \
	    const Strat::Layer& lay = *seq.layers()[ilay]; \
	    float z0 = lay.zTop(); if ( flattened_ ) z0 -= lvldpth; \
	    float z1 = lay.zBot(); if ( flattened_ ) z1 -= lvldpth; \
	    const float val = \
	      getLayerPropValue(lay,seq.propertyRefs()[dispprop_],dispprop_); \

#define mEndLayLoop() \
	} \
    }



void uiStratSimpleLayerModelDisp::updateSelSeqAuxData()
{
    if ( !selseqad_ )
    {
	selseqad_ = vwr_.createAuxData( 0 );
	selseqad_->enabled_ = true;
	selseqad_->linestyle_ =
		OD::LineStyle( OD::LineStyle::Dot, 2, Color::Black() );
	selseqad_->zvalue_ = uiFlatViewer::auxDataZVal() + 2;
	vwr_.addAuxData( selseqad_ );
    }

    StepInterval<double> yrg = emptydp_->posData().range( false );
    selseqad_->poly_.erase();
    selseqad_->poly_ += FlatView::Point( mCast(double,selseqidx_+1), yrg.start);
    selseqad_->poly_ += FlatView::Point( mCast(double,selseqidx_+1), yrg.stop );
}


void uiStratSimpleLayerModelDisp::updateLevelAuxData()
{
    if ( layerModel().isEmpty() )
	return;

    lvlcol_ = tools_.selLevelColor();
    int auxdataidx = 0;
    for ( int iseq=0; iseq<lvldpths_.size(); iseq++ )
    {
	if ( !isDisplayedModel(iseq) )
	    continue;
	float zlvl = lvldpths_[iseq];
	if ( mIsUdf(zlvl) )
	    continue;

	mGetDispZ(zlvl);
	const double ypos = mCast( double, flattened_ ? 0. : zlvl );
	const double xpos1 = mCast( double, iseq+1 );
	const double xpos2 = mCast( double, iseq +1+dispeach_ );
	FlatView::AuxData* levelad = 0;
	if ( !levelads_.validIdx(auxdataidx) )
	{
	    levelad = vwr_.createAuxData( 0 );
	    levelad->zvalue_ = uiFlatViewer::auxDataZVal() + 1;
	    vwr_.addAuxData( levelad );
	    levelads_ += levelad;
	}
	else
	    levelad = levelads_[auxdataidx];

	levelad->poly_.erase();
	levelad->close_ = false;
	levelad->enabled_ = true;
	levelad->linestyle_ = OD::LineStyle(OD::LineStyle::Solid,2,lvlcol_);
	levelad->poly_ += FlatView::Point( xpos1, ypos );
	levelad->poly_ += FlatView::Point( xpos2, ypos );
	auxdataidx++;
    }
    while ( auxdataidx < levelads_.size() )
	levelads_[auxdataidx++]->enabled_ = false;
}


void uiStratSimpleLayerModelDisp::updateLayerAuxData()
{
    dispprop_ = tools_.selPropIdx();
    selectedlevel_ = tools_.selLevelIdx();
    dispeach_ = tools_.dispEach();
    showzoomed_ = tools_.dispZoomed();
    uselithcols_ = tools_.dispLith();
    selectedcontent_ = layerModel().refTree().contents()
				.getByName(tools_.selContent());
    allcontents_ = FixedString(tools_.selContent()) == sKey::All();
    if ( vrg_.width() == 0 )
	{ vrg_.start -= 1; vrg_.stop += 1; }
    const float vwdth = vrg_.width();
    int auxdataidx = 0;
    mStartLayLoop( false, )

	if ( !isDisplayedModel(iseq) )
	    continue;
	const Color laycol = lay.dispColor( uselithcols_ );
	bool mustannotcont = false;
	if ( !lay.content().isUnspecified() )
	    mustannotcont = allcontents_
		|| (selectedcontent_ && lay.content() == *selectedcontent_);
	const Color pencol = mustannotcont ? lay.content().color_ : laycol;
	bool canjoinlayers = ilay > 0;
	if ( canjoinlayers )
	{
	    const Strat::Layer& prevlay = *seq.layers()[ilay-1];
	    const Color prevlaycol = prevlay.dispColor( uselithcols_ );
	    canjoinlayers =
		prevlay.content()==lay.content() && prevlaycol==laycol;
	}

	if ( canjoinlayers )
	    auxdataidx--;
	FlatView::AuxData* layad = 0;
	if ( !layerads_.validIdx(auxdataidx) )
	{
	    layad = vwr_.createAuxData( lay.name().buf() );
	    layad->zvalue_ = uiFlatViewer::auxDataZVal()-1;
	    layad->close_ = true;
	    vwr_.addAuxData( layad );
	    layerads_ += layad;
	}
	else
	    layad = layerads_[auxdataidx];

	if ( !canjoinlayers )
	    layad->poly_.erase();
	else
	    layad->poly_.pop();

	layad->fillcolor_ = laycol;
	layad->enabled_ = true;
	layad->linestyle_ = OD::LineStyle( OD::LineStyle::Solid, 2, pencol );
	if ( mustannotcont )
	    layad->fillpattern_ = lay.content().pattern_;
	else
	{
	    FillPattern fp; fp.setFullFill();
	    layad->fillpattern_ = fp;
	}
	const double x0 = mCast( double, iseq + 1 );
	double relx = mCast( double, (val-vrg_.start)/vwdth );
	relx *= dispeach_;
	const double x1 = mCast( double, iseq+1+relx );
	mGetDispZ( z0 ); // TODO check if needed
	mGetDispZ( z1 );
	if ( !canjoinlayers )
	    layad->poly_ += FlatView::Point( x0, (double)z0 );
	layad->poly_ += FlatView::Point( x1, (double)z0 );
	layad->poly_ += FlatView::Point( x1, (double)z1 );
	layad->poly_ += FlatView::Point( x0, (double)z1 );
	auxdataidx++;

    mEndLayLoop()

    while ( auxdataidx < layerads_.size() )
	layerads_[auxdataidx++]->enabled_ = false;

}


void uiStratSimpleLayerModelDisp::updateDataPack()
{
    getBounds();
    const Strat::LayerModel& lm = layerModel();
    const int nrseqs = lm.size();
    const bool haveprop = lm.propertyRefs().validIdx(dispprop_);
    mGetDispZrg(zrg_,dispzrg);
    StepInterval<double> zrg( dispzrg.start, dispzrg.stop,
			      dispzrg.width()/5.0f );
    emptydp_->posData().setRange(
	    true, StepInterval<double>(1, nrseqs<2 ? 1 : nrseqs,1) );
    emptydp_->posData().setRange( false, zrg );
    emptydp_->setName( !haveprop ? "No property selected"
				 : lm.propertyRefs()[dispprop_]->name().buf() );
    vwr_.setViewToBoundingBox();
}


void uiStratSimpleLayerModelDisp::modelChanged()
{
    zoomwr_ = uiWorldRect(mUdf(double),0,0,0);
    forceRedispAll();
}


void uiStratSimpleLayerModelDisp::reSetView()
{
    updateDataPack();
}


void uiStratSimpleLayerModelDisp::getBounds()
{
    lvldpths_.erase();
    dispprop_ = tools_.selPropIdx();
    const Strat::Level* lvl = tools_.selStratLevel();
    for ( int iseq=0; iseq<layerModel().size(); iseq++ )
    {
	const Strat::LayerSequence& seq = layerModel().sequence( iseq );
	if ( !lvl || seq.isEmpty() )
	    { lvldpths_ += mUdf(float); continue; }

	const int posidx = seq.positionOf( *lvl );
	float zlvl = mUdf(float);
	if ( posidx >= seq.size() )
	    zlvl = seq.layers()[seq.size()-1]->zBot();
	else if ( posidx >= 0 )
	    zlvl = seq.layers()[posidx]->zTop();
	lvldpths_ += zlvl;
    }

    Interval<float> zrg(mUdf(float),mUdf(float)), vrg(mUdf(float),mUdf(float));
    mStartLayLoop( false,  )
#	define mChckBnds(var,op,bnd) \
	if ( (mIsUdf(var) || var op bnd) && !mIsUdf(bnd) ) \
	    var = bnd
	mChckBnds(zrg.start,>,z0);
	mChckBnds(zrg.stop,<,z1);
	mChckBnds(vrg.start,>,val);
	mChckBnds(vrg.stop,<,val);
    mEndLayLoop()
    if ( mIsUdf(zrg.start) )
	zrg_ = Interval<float>( 0, 1 );
    else
	zrg_ = zrg;
    vrg_ = mIsUdf(vrg.start) ? Interval<float>(0,1) : vrg;

    if ( mIsUdf(zoomwr_.left()) )
    {
	zoomwr_.setLeft( 1 );
	zoomwr_.setRight( nrseqs+1 );
	mGetDispZrg(zrg_,dispzrg);
	zoomwr_.setTop( dispzrg.stop );
	zoomwr_.setBottom( dispzrg.start );
    }
}


int uiStratSimpleLayerModelDisp::getXPix( int iseq, float relx ) const
{
    const float margin = 0.05f;
    relx = (1-margin) * relx + margin * .5f;// get relx between 0.025 and 0.975
    relx *= dispeach_;
    return vwr_.getWorld2Ui().toUiX( iseq + 1 + relx );
}


bool uiStratSimpleLayerModelDisp::isDisplayedModel( int iseq ) const
{
    if ( iseq % dispeach_ )
	return false;

    if ( showzoomed_ )
    {
	const uiWorld2Ui& w2ui = vwr_.getWorld2Ui();
	const int xpix0 = getXPix( iseq, 0 );
	const int xpix1 = getXPix( iseq, 1 );
	if ( w2ui.toWorldX(xpix1) > zoomwr_.right()
	  || w2ui.toWorldX(xpix0) < zoomwr_.left() )
	    return false;
    }
    return true;
}


int uiStratSimpleLayerModelDisp::totalNrLayersToDisplay() const
{
    const int nrseqs = layerModel().size();
    int ret = 0;
    for ( int iseq=0; iseq<nrseqs; iseq++ )
    {
	if ( isDisplayedModel(iseq) )
	    ret += layerModel().sequence(iseq).size();
    }
    return ret;
}


void uiStratSimpleLayerModelDisp::doDraw()
{
    MouseCursorChanger mc( MouseCursor::Wait );
    getBounds();
    updateLayerAuxData();
    updateLevelAuxData();
    updateSelSeqAuxData();
    vwr_.handleChange( mCast(unsigned int,FlatView::Viewer::Auxdata) );
}


void uiStratSimpleLayerModelDisp::drawLevels()
{
}


void uiStratSimpleLayerModelDisp::doLevelChg()
{ reDrawAll(); }


void uiStratSimpleLayerModelDisp::drawSelectedSequence()
{
    updateSelSeqAuxData();
    vwr_.handleChange( mCast(unsigned int, FlatView::Viewer::Auxdata) );
}

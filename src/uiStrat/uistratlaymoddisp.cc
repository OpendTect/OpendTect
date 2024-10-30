/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uistratsimplelaymoddisp.h"

#include "uifileinput.h"
#include "uiflatviewer.h"
#include "uigeninput.h"
#include "uigraphicsitemimpl.h"
#include "uigraphicsscene.h"
#include "uigraphicsview.h"
#include "uiioobjsel.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uimultiflatviewcontrol.h"
#include "uistrateditlayer.h"
#include "uistratlaymodtools.h"
#include "uitaskrunner.h"
#include "uitextedit.h"
#include "uitoolbutton.h"

#include "arrayndimpl.h"
#include "ascstream.h"
#include "flatposdata.h"
#include "flatviewaxesdrawer.h"
#include "od_helpids.h"
#include "od_iostream.h"
#include "stratlayer.h"
#include "stratlayermodel.h"
#include "stratlayersequence.h"
#include "stratlevel.h"
#include "stratlith.h"
#include "stratreftree.h"
#include "strattransl.h"
#include "zdomain.h"


uiStratLayerModelDisp::uiStratLayerModelDisp( uiStratLayModEditTools& t,
					  const Strat::LayerModelSuite& lms)
    : uiGroup(t.parent(),"LayerModel display")
    , sequenceSelected(this)
    , genNewModelNeeded(this)
    , sequencesRead(this)
    , sequencesAdded(this)
    , infoChanged(this)
    , lms_(lms)
    , tools_(t)
    , vwr_(*new uiFlatViewer(this))
    , depthlbl_( " (", PropertyRef::thickness().disp_.getUnitLbl(), ")" )
{
    vwr_.setInitialSize( initialSize() );
    vwr_.setStretch( 2, 2 );
    vwr_.disableStatusBarUpdate();
    vwr_.setZDomain( ZDomain::Depth() );

    const int fontheight = vwr_.getAxesDrawer().getNeededHeight();
    vwr_.setExtraBorders( uiSize(0,-fontheight), uiSize(0,-fontheight) );

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

    mAttachCB( postFinalize(), uiStratLayerModelDisp::initGrp );
}


uiStratLayerModelDisp::~uiStratLayerModelDisp()
{
    detachAllNotifiers();
}


void uiStratLayerModelDisp::initGrp( CallBacker* )
{
    mAttachCB( lms_.curChanged, uiStratLayerModelDisp::curModEdChgCB );
    mAttachCB( lms_.modelChanged, uiStratLayerModelDisp::modelChangedCB );
    mAttachCB( vwr_.rgbCanvas().reSize, uiStratLayerModelDisp::vwResizeCB );
    uiFlatViewControl* control = vwr_.control();
    if ( control )
	mAttachCB( control->zoomChanged, uiStratLayerModelDisp::zoomChgCB );

    MouseEventHandler& mehdlr = vwr_.rgbCanvas().getMouseEventHandler();
    mAttachCB( mehdlr.buttonReleased, uiStratLayerModelDisp::usrClickedCB );
    mAttachCB( mehdlr.doubleClick, uiStratLayerModelDisp::doubleClickedCB );
    mAttachCB( mehdlr.movement, uiStratLayerModelDisp::mouseMovedCB );

    mAttachCB( tools_.selPropChg, uiStratLayerModelDisp::selPropChgCB );
    mAttachCB( tools_.selLevelChg, uiStratLayerModelDisp::selLevelChgCB );
    mAttachCB( tools_.selContentChg, uiStratLayerModelDisp::selContentChgCB );
    mAttachCB( tools_.dispEachChg, uiStratLayerModelDisp::dispEachChgCB );
    mAttachCB( tools_.dispLithChg, uiStratLayerModelDisp::dispLithChgCB );
    mAttachCB( tools_.dispZoomedChg, uiStratLayerModelDisp::dispZoomedChgCB );
    mAttachCB( tools_.showFlatChg, uiStratLayerModelDisp::showFlatChgCB );
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
	const bool showzoomed = tools_.dispZoomed();
	stdctrl->setZoomCoupled( showzoomed );
	stdctrl->setDrawZoomBoxes( !showzoomed );
    }
}


void uiStratLayerModelDisp::showFlatChg()
{
    modelChangedCB( nullptr );
}


void uiStratLayerModelDisp::zoomChgCB( CallBacker* cb )
{
    if ( !showFlattened() )
	return;

    mDynamicCastGet(uiMultiFlatViewControl*,stdctrl,vwr_.control())
    if ( !stdctrl || !stdctrl->activeVwr() )
	return;

    uiWorldRect activewr = stdctrl->activeVwr()->curView();
    activewr.sortCorners();
    const double relpos = ( 0. - activewr.top() ) / activewr.height();

    uiWorldRect bb = vwr_.boundingBox(); bb.sortCorners();

    uiWorldRect newview = vwr_.curView();
    double depthdiff = newview.height();
    if ( -bb.top() < depthdiff/2. )
	depthdiff = -bb.top() * 2.;
    if ( bb.bottom() < depthdiff/2. )
	depthdiff = bb.bottom() * 2.;

    newview.setTop( -relpos * depthdiff );
    newview.setBottom( (1.-relpos) * depthdiff );
    stdctrl->setZoomCoupled( false );
    NotifyStopper ns( stdctrl->zoomChanged, this );
    stdctrl->setNewView( newview.centre(), newview.size(), &vwr_ );
    stdctrl->setZoomCoupled( true );
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
    lvldpths_.setEmpty();
    const Strat::LevelSet& lvls = Strat::LVLS();
    const int nrlvls = lvls.size();
    lvldpths_.setSize( nrlvls );

    const Strat::LayerModel& laymodel = layerModel();
    //TODO: parallel
    for( int ilvl=0; ilvl<nrlvls; ilvl++ )
    {
	const Strat::Level lvl = lvls.getByIdx( ilvl );
	TypeSet<float>& dpths = lvldpths_[ilvl];
	for ( int iseq=0; iseq<laymodel.size(); iseq++ )
	{
	    const Strat::LayerSequence& seq = laymodel.sequence( iseq );
	    if ( !lvl.id().isValid() || seq.isEmpty() )
		dpths += mUdf(float);
	    else
		dpths += seqLvlDepth( seq, lvl );
	}
    }
}


uiFlatViewer* uiStratLayerModelDisp::getViewerClone( uiParent* p ) const
{
    auto* vwr = new uiFlatViewer( p );
    vwr->rgbCanvas().disableImageSave();
    vwr->setInitialSize( initialSize() );
    vwr->setStretch( 2, 2 );
    vwr_.setZDomain( ZDomain::Depth() );
    vwr->appearance() = vwr_.appearance();
    auto wvadp = vwr_.getPack( true ).get();
    auto vddp = vwr_.getPack( false ).get();
    if ( wvadp.ptr() == vddp.ptr() )
	vwr->setPack( FlatView::Viewer::Both, wvadp.ptr(), false );
    else
    {
	const bool canupdate = vwr->enableChange( false );
	vwr->setPack( FlatView::Viewer::WVA, wvadp.ptr(), false );
	vwr->enableChange( canupdate );
	vwr->setPack( FlatView::Viewer::VD, vddp.ptr(), false );
    }

    return vwr;
}


void uiStratLayerModelDisp::selectSequence( int selidx )
{
    selseqidx_ = selidx;
    drawSelectedSequence();
}


int uiStratLayerModelDisp::selLevelIdx() const
{
    return tools_.selLevelIdx();
}


float uiStratLayerModelDisp::getLayerPropValue( const Strat::Layer& lay,
						const PropertyRef& pr,
						int propidx ) const
{
    return propidx < lay.nrValues() ? lay.value( propidx ) : mUdf(float);
}


void uiStratLayerModelDisp::modelChangedCB( CallBacker* )
{
    handleModelChange();
}


void uiStratLayerModelDisp::curModEdChgCB( CallBacker* cb )
{
    if ( cb != this )
	modelChangedCB( cb );

    if ( !modtypetxtitm_ )
    {
	modtypetxtitm_ = scene().addItem( new uiTextItem() );
	modtypetxtitm_->setAlignment( mAlignment(Left,VCenter) );
	modtypetxtitm_->setPenColor( OD::Color::Black() );
	modtypetxtitm_->setZValue( 999999 );
	modtypetxtitm_->setMovable( true );
    }

    const uiString moddesc = lms_.uiDesc( lms_.curIdx() );
    const bool dodisp = !moddesc.isEmpty();
    modtypetxtitm_->setVisible( dodisp );
    if ( dodisp )
    {
	modtypetxtitm_->setText( moddesc );
	if ( cb != this )
	    vwResizeCB( nullptr );
    }
}


void uiStratLayerModelDisp::vwResizeCB( CallBacker* )
{
    if ( !modtypetxtitm_ )
	return;

    modtypetxtitm_->setPos( 20, 10 );
}


#define mErrRetVoid(s) { uiMSG().error(s); return; }
#define mErrRet(s) { uiMSG().error(s); return false; }

static int sUseEach = 1;
static int sStartAt = 1;
static bool sDoReplace = true;
static bool sPresMath = false;

class uiStratLayerModelDispIO : public uiDialog
{ mODTextTranslationClass(uiStratLayerModelDispIO)
public:

uiStratLayerModelDispIO( uiParent* p, const Strat::LayerModel& lm,
			 bool doread )
    : uiDialog( p, Setup(doread ? tr("Read pseudo-wells")
				: tr("Save pseudo-wells"),
			 mNoDlgTitle,
			 mODHelpKey(mStratLayerModelDispIOHelpID)) )
    , lm_(lm)
    , doread_(doread)
{
    setOkText( doread ? uiStrings::sOpen() : uiStrings::sSave() );
    IOObjContext ctxt = StratLayerModelsTranslatorGroup::ioContext();
    ctxt.forread_ = doread;

    if ( doread )
    {
	inputfld_ = new uiGenInput( this, tr("Saved in v6.4 or earlier"),
				BoolInpSpec(false,uiStrings::sEmptyString()) );
	mAttachCB( inputfld_->valueChanged, uiStratLayerModelDispIO::inputCB );

	filefld_ = new uiFileInput( this, uiStrings::sFileName() );
	filefld_->attach( alignedBelow, inputfld_ );
	mAttachCB( filefld_->valueChanged, uiStratLayerModelDispIO::selCB );

	laymodfld_ = new uiIOObjSel( this, ctxt );
	laymodfld_->attach( alignedBelow, inputfld_ );
	mAttachCB( laymodfld_->selectionDone, uiStratLayerModelDispIO::selCB );

	infofld_ = new uiTextBrowser( this, "Info", 20, false );
	infofld_->setPrefHeightInChar( 10 );
	infofld_->attach( alignedBelow, laymodfld_ );

	eachfld_ = new uiGenInput( this, tr("Read each"),
				   IntInpSpec(sUseEach,1,1000) );
	mAttachCB( eachfld_->valueChanging, uiStratLayerModelDispIO::nrCB );
	eachfld_->attach( alignedBelow, infofld_ );

	startatfld_ = new uiGenInput( this, tr("Start at"),
				      IntInpSpec(sStartAt,1,100000) );
	mAttachCB( startatfld_->valueChanging, uiStratLayerModelDispIO::nrCB );
	startatfld_->attach( rightTo, eachfld_ );

	nrreadfld_ = new uiGenInput( this, tr("Models to read" ) );
	nrreadfld_->setReadOnly( true );
	nrreadfld_->attach( rightTo, startatfld_ );

	doreplacefld_ = new uiGenInput( this, tr("Replace existing model"),
					BoolInpSpec(sDoReplace) );
	doreplacefld_->attach( alignedBelow, eachfld_ );
    }
    else
    {
	laymodfld_ = new uiIOObjSel( this, ctxt );
#ifdef __debug__
	presmathfld_ = new uiGenInput( this, tr("Preserve Math Formulas"),
				       BoolInpSpec(sPresMath) );
	presmathfld_->attach( alignedBelow, laymodfld_ );
#endif
    }

    mAttachCB( postFinalize(), uiStratLayerModelDispIO::finalizeCB );
}


~uiStratLayerModelDispIO()
{
    detachAllNotifiers();
}


void finalizeCB( CallBacker* )
{
    if ( !doread_ )
	return;

    inputCB( nullptr );
    selCB( nullptr );
    nrCB( nullptr );
}


void inputCB( CallBacker* )
{
    const bool usefile = inputfld_->getBoolValue();
    filefld_->display( usefile );
    laymodfld_->display( !usefile );
}


od_istream* getStream() const
{
    const bool usefile = inputfld_ ? inputfld_->getBoolValue() : false;
    BufferString fnm;
    if ( usefile )
    {
	fnm = filefld_->fileName();
	if ( fnm.isEmpty() )
	    uiMSG().error( tr("Please select a file") );
    }
    else
    {
	const IOObj* ioobj = laymodfld_->ioobj();
	if ( ioobj )
	    fnm = ioobj->fullUserExpr();
    }

    if ( fnm.isEmpty() )
	return nullptr;

    od_istream* strm = new od_istream( fnm );
    if ( !usefile )
	ascistream astrm( *strm );

    if ( !strm->isOK() )
    {
	uiMSG().error( tr("Cannot open:\n%1\nfor read").arg(fnm) );
	return nullptr;
    }

    return strm;
}


void selCB( CallBacker* )
{
    nrwells_ = 0;
    infofld_->setEmpty();
    nrreadfld_->setValue( 0 );

    PtrMan<od_istream> strm = getStream();
    if ( !strm )
	return;

    Strat::LayerModel newlm;
    PropertyRefSelection propref;
    int nrseqs = 0;
    bool mathpreserved = false;
    if ( !newlm.readHeader(*strm,propref,nrseqs,mathpreserved) )
	mErrRetVoid( tr("Cannot read header.") );

    nrwells_ = nrseqs;

    BufferString txt( "<ul><li>Nr pseudo-wells: " );
    txt.add( nrseqs ).add( "</li>" );
    BufferStringSet props, extraprops, missingprops;
    for ( int idx=0; idx<propref.size(); idx++ )
    {
	props.add( propref[idx]->name() );
	const int oldidx = lm_.propertyRefs().indexOf( propref[idx] );
	if ( oldidx < 0 )
	    extraprops.add( propref[idx]->name() );
    }

    const bool allmatch = propref.size() == lm_.propertyRefs().size() &&
			  extraprops.isEmpty();
    if ( allmatch )
	txt.add( "<li>The Pseudo-wells and the current Layer Model Description "
		"have the same set of properties: " )
	    .add( props.getDispString() ).add( "</li>" );
    else
    {
	const char* liwithyellowbg = "<li style=\"background-color:#ffffaa;\">";
	txt.add( liwithyellowbg )
	   .add( "Warning: The Pseudo-wells and the current Layer Model "
		 "Description have different sets of properties. <ul>" );
	for ( int idx=0; idx<lm_.propertyRefs().size(); idx++ )
	{
	    const int newidx = propref.indexOf( lm_.propertyRefs()[idx] );
	    if ( newidx < 0 )
		missingprops.add( lm_.propertyRefs()[idx]->name() );
	}

	if ( !missingprops.isEmpty() )
	    txt.add( liwithyellowbg )
		.add( "The following Layer Model Description properties "
		    "are missing from the Pseudo-wells and hence "
		    "will get undefined values: " )
		.add( missingprops.getDispString() ).add( "</li>" );

	if ( !extraprops.isEmpty() )
	    txt.add( liwithyellowbg )
		.add( "The following Pseudo-wells properties are not "
		    "used in the Layer Model Description and hence "
		    "will be ignored: " )
		.add( extraprops.getDispString() ).add( "</li>" );

	txt.add( "</ul></li>" );
    }

    txt.add( "</ul>" );
    infofld_->setHtmlText( txt );
    nrCB( nullptr );
}


void nrCB( CallBacker* )
{
    int well0 = startatfld_->getIntValue();
    if ( well0 > nrwells_ )
	well0 = nrwells_;
    StepInterval<int> wellrg( well0, nrwells_, eachfld_->getIntValue() );
    const int wells2read = wellrg.nrSteps()+1;
    nrreadfld_->setValue( wells2read );
}


bool acceptOK( CallBacker* ) override
{
    if ( doread_ )
    {
	PtrMan<od_istream> strm = getStream();
	if ( !strm )
	    return false;

	const int each = eachfld_->getIntValue();
	sUseEach = each;
	const int firstmdl = startatfld_->getIntValue();
	sStartAt = firstmdl;
	Strat::LayerModel newlm;
	uiTaskRunner dlg( this );
	uiString msg;
	if ( !newlm.read(*strm,firstmdl-1,each,msg,&dlg,lm_.startDepth(),
			 lm_.overburdenVelocity()) )
	    mErrRet(tr("Cannot read layer model from file.\nDetails may be "
		       "in the log file ('Utilities-Show log file')"))

	if ( !msg.isEmpty() )
	{
	    msg.appendPhrase( tr("Do you want to continue") );
	    if ( !uiMSG().askGoOn(msg) )
		return false;
	}

	sDoReplace = doreplacefld_->getBoolValue();
	auto& lm = const_cast<Strat::LayerModel&>( lm_ );
	if ( sDoReplace )
	{
	    lm.setEmpty();
	    changedmodel_ = true;
	}

	const int szbefore = lm.size();
	lm.append( newlm );
	changedmodel_ |= szbefore != lm.size();
	addedmodels_ = !sDoReplace && szbefore<lm.size();
    }
    else
    {
	const IOObj* ioobj = laymodfld_->ioobj();
	if ( !ioobj )
	    return false;

	const BufferString fnm = ioobj->fullUserExpr();
	od_ostream strm( fnm );
	if ( !strm.isOK() )
	    mErrRet( tr("Cannot open:\n%1\nfor write").arg(fnm) )

	ascostream astrm( strm );
	if ( !astrm.putHeader("PseudoWells") )
	    mErrRet( tr("Cannot write to output file:\n%1").arg(fnm) )

	sPresMath = presmathfld_ ? presmathfld_->getBoolValue() : false;
	if ( !lm_.write(strm,0,sPresMath) )
	    mErrRet( tr("Unknown error during write") )
    }

    return true;
}

    uiGenInput*			inputfld_		= nullptr;
    uiFileInput*		filefld_		= nullptr;
    uiIOObjSel*			laymodfld_;
    uiTextBrowser*		infofld_		= nullptr;
    uiGenInput*			doreplacefld_		= nullptr;
    uiGenInput*			eachfld_		= nullptr;
    uiGenInput*			startatfld_		= nullptr;
    uiGenInput*			nrreadfld_		= nullptr;
    uiGenInput*			presmathfld_		= nullptr;

    const Strat::LayerModel&	lm_;
    bool			doread_;
    int				nrwells_		= 0;
    bool			changedmodel_		= false;
    bool			addedmodels_		= false;
};


void uiStratLayerModelDisp::notifyModelChanged( int ctyp )
{
    const_cast<Strat::LayerModelSuite&>( lms_ ).touch( ctyp );
}


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
    const int curidx = lms_.curIdx();
    const Strat::LayerModel& lm = lms_.baseModel();
    if ( !foradd && lm.isEmpty() )
	mErrRet( tr("Please generate at least one layer sequence") )

    uiStratLayerModelDispIO dlg( this, lm, foradd );
    if ( dlg.go() != uiDialog::Accepted )
	return false;

    if ( curidx > 0 )
    {
	auto& lms = const_cast<Strat::LayerModelSuite&>( lms_ );
	lms.setCurIdx( 0 );
	lms.clearEditedData();
	lms.setCurIdx( curidx );
    }

    if ( dlg.addedmodels_ )
	sequencesAdded.trigger();

    if ( dlg.changedmodel_ )
    {
	sequencesRead.trigger();
	notifyModelChanged(-1);
    }

    return true;
}


bool uiStratLayerModelDisp::showFlattened() const
{
    bool flattened = tools_.showFlattened();
    if ( flattened && selLevelIdx()<0 )
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
    const float fseqidx = vwr_.getWorld2Ui().toWorldX( mev.pos().x_ ) - 1.f;
    int seqidx = (int)fseqidx; // not mNINT32
    if ( seqidx < 0 || seqidx > layerModel().size() )
	seqidx = -1; // use fseqidx: (int) will round upward for neg nmbrs

    return seqidx;
}


Strat::Layer* uiStratLayerModelDisp::usrPointedLayer( int& layidx ) const
{
    const Strat::LayerModel& laymodel = layerModel();
    const int seqidx = usrPointedModelNr();
    if ( seqidx < 0 || seqidx >= laymodel.size() )
	return 0;

    const Strat::LayerSequence& seq = laymodel.sequence( seqidx );
    if ( seq.isEmpty() )
	return nullptr;

    MouseEventHandler& mevh = vwr_.rgbCanvas().getMouseEventHandler();
    float zsel = vwr_.getWorld2Ui().toWorldY( mevh.event().pos().y_ );
    if ( showFlattened() )
    {
	const int lvlidx = selLevelIdx();
	const float lvlz = lvldpths_.get( lvlidx ).get( seqidx );
	if ( mIsUdf(lvlz) )
	    return nullptr;
	zsel += lvlz;
    }

    layidx = seq.layerIdxAtZ( zsel );
    const ObjectSet<Strat::Layer>& lays = seq.layers();
    return lays.validIdx(layidx) ? const_cast<Strat::Layer*>( lays.get(layidx) )
				 : nullptr;
}


static uiString getKeyValStr( const char* key, const char* val )
{
    return toUiString( "%1: %2" ).arg( key ).arg( val );
}


void uiStratLayerModelDisp::mouseMovedCB( CallBacker* )
{
    const int selseq = usrPointedModelNr();
    if ( selseq<0 || selseq>=layerModel().size() )
	return;

    uiString statusbarmsg;
    BufferString modelnrstr( 16, true );
    od_sprintf( modelnrstr.getCStr(), modelnrstr.bufSize(), "%5d", selseq+1 );
    statusbarmsg.append( getKeyValStr("Model Number",modelnrstr) );

    int layidx;
    const Strat::Layer* lay = usrPointedLayer( layidx );
    float depth;
    if ( lay )
	depth = lay->depth();
    else
    {
	const MouseEvent& mev = vwr_.rgbCanvas().getMouseEventHandler().event();
        depth = vwr_.getWorld2Ui().toWorldY( mev.pos().y_ );
	if ( !Math::IsNormalNumber(depth) )
	{
	    pErrMsgOnce("Invalid number from axis handler");
	    depth = 0.f;
	}
    }

    BufferString depthstr( 16, true );
    od_sprintf( depthstr.getCStr(), depthstr.bufSize(), "%6.0f", depth );
    depthstr.add( depthlbl_ );
    statusbarmsg.append( getKeyValStr("TVDSS Depth",depthstr) );

    const Strat::LayerSequence& seq = layerModel().sequence( selseq );
    const int disppropidx = tools_.selPropIdx()+1;
    if ( lay && seq.propertyRefs().validIdx(disppropidx) )
    {
	const PropertyRef& pr = *seq.propertyRefs()[disppropidx];
	BufferString valstr( getLayerPropValue(*lay,pr,disppropidx) );
	valstr.addSpace().add( pr.disp_.getUnitLbl() );
	statusbarmsg.append( getKeyValStr(pr.name(),valstr) );
	statusbarmsg.append( getKeyValStr("Layer",lay->name()) );
	if ( !lay->lithology().isUdf() )
	    statusbarmsg.append(
		    getKeyValStr("Lithology",lay->lithology().name()) );
	if ( !lay->content().isUnspecified() )
	    statusbarmsg.append(
		    getKeyValStr("Content",lay->content().name()) );
    }

    infoChanged.trigger( &statusbarmsg, this );
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


const char* dimName( bool dim0 ) const override
{ return dim0 ? "Model Nr" : "Depth"; }

protected:

~uiSSLMFlatViewDataPack()
{
}

};

uiStratSimpleLayerModelDisp::uiStratSimpleLayerModelDisp(
	    uiStratLayModEditTools& tools, const Strat::LayerModelSuite& lms )
    : uiStratLayerModelDisp(tools,lms)
    , zrg_(0.f,1.f)
    , curproprg_(mUdf(float),mUdf(float))
{
    const FlatView::Viewer::VwrDest dest = FlatView::Viewer::Both;
    vwr_.setVisible( dest, false );
    fvdp_ = new uiSSLMFlatViewDataPack();
    fvdp_->setName( "Simple Layer Model Display BackDrop" );
    vwr_.setPack( dest, fvdp_.ptr() );

    const int sz = 25; // looks best
    auto* savebut = new uiToolButton( this, "save",
				tr("Save all pseudo-wells"),
				mCB(this,uiStratSimpleLayerModelDisp,saveCB) );
    savebut->setMaximumHeight( sz );
    savebut->setMaximumWidth( sz );
    savebut->attach( rightBorder, 6 ); // for a single pixel spacing
    savebut->attach( topBorder, 2 ); // for a single pixel spacing
    auto* openbut = new uiToolButton( this, "open",
				tr("Open saved pseudo-wells"),
				mCB(this,uiStratSimpleLayerModelDisp,openCB) );
    openbut->setMaximumHeight( sz );
    openbut->setMaximumWidth( sz );
    openbut->attach( leftOf, savebut, 2 ); // for a single pixel spacing
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
    curproprg_ = getModelRange( curPropIdx() );
    reDrawSeqs();
}


void uiStratSimpleLayerModelDisp::selLevelChg()
{
    if ( showFlattened() )
	showFlatChg();
    else
	reDrawLevels();
}


void uiStratSimpleLayerModelDisp::selContentChg()
{
    reDrawSeqs();
}


void uiStratSimpleLayerModelDisp::dispEachChg()
{
    reDrawAll();
}


void uiStratSimpleLayerModelDisp::dispLithChg()
{
    reDrawSeqs();
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
	Strat::Layer* lay = usrPointedLayer( layidx );
	if ( !lay )
	    return;

	mevh.setHandled( true );
	const Strat::LayerSequence& seq = layerModel().sequence( seqidx );
	uiStratEditLayer dlg( this, *lay, seq, true );
	if ( dlg.go() && dlg.isChanged() )
	    notifyModelChanged(-1);
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

    auto& seq = const_cast<Strat::LayerSequence&>(
					layerModel().sequence( seqidx ) );
    int layidx = 0;
    Strat::Layer* lay = usrPointedLayer( layidx );

    uiMenu mnu( parent(), uiStrings::sAction() );
    if ( lay )
    {
	mnu.insertAction( new uiAction(m3Dots(uiStrings::sProperties())), 1 );
	mnu.insertAction( new uiAction(m3Dots(uiStrings::phrRemove(
				       uiStrings::sLayer().toLower()))), 2 );
    }

    mnu.insertAction( new uiAction(m3Dots(uiStrings::phrRemove(
						     tr("this Well")))), 3 );
    mnu.insertAction( new uiAction(m3Dots(tr("Save all pseudo-wells"))), 4 );
    mnu.insertAction( new uiAction(m3Dots(tr("Open saved pseudo-wells"))), 5 );

    const int mnuid = mnu.exec();
    if ( mnuid < 0 )
	return;

    if ( mnuid == 1 )
    {
	uiStratEditLayer dlg( this, *lay, seq, true );
	if ( dlg.go() && dlg.isChanged() )
	    notifyModelChanged(-1);
    }
    else if ( mnuid == 4 || mnuid == 5 )
	doLayerModelIO( mnuid == 5 );
    else if ( mnuid == 3 )
    {
	const_cast<Strat::LayerModel&>(layerModel()).removeSequence( seqidx );
	notifyModelChanged(-1);
    }
    else
    {
	uiDialog dlg( this, uiDialog::Setup( uiStrings::phrRemove(
				  uiStrings::sLayer().toLower()),
				  uiStrings::phrRemove(toUiString("'%1'")
				  .arg(lay->name())),
				  mODHelpKey(mStratSimpleLayerModDispHelpID)));
	auto* gi = new uiGenInput( &dlg, uiStrings::sRemove(),
				     BoolInpSpec(true,
				     tr("Only this layer"),
				     tr("All layers with this ID")) );
	if ( dlg.go() )
	    removeLayers( seq, layidx, !gi->getBoolValue() );
    }
}


void uiStratSimpleLayerModelDisp::removeLayers( Strat::LayerSequence& seq,
					int layidx, bool doall )
{
    if ( doall )
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
    else
    {
	delete seq.layers().removeSingle( layidx );
	seq.prepareUse();
    }

    notifyModelChanged(-1);
}


void uiStratSimpleLayerModelDisp::reDrawLevels()
{
    updateLevelAuxData();
    vwr_.handleChange( sCast(od_uint32,FlatView::Viewer::Auxdata) );
}


void uiStratSimpleLayerModelDisp::reDrawSeqs()
{
    updateLayerAuxData();
    vwr_.handleChange( sCast(od_uint32,FlatView::Viewer::Auxdata) );
}


void uiStratSimpleLayerModelDisp::drawSelectedSequence()
{
    updateSelSeqAuxData();
    vwr_.handleChange( sCast(od_uint32,FlatView::Viewer::Auxdata) );
}


void uiStratSimpleLayerModelDisp::reDrawAll()
{
    if ( layerModel().isEmpty() )
    {
	if ( !emptyitm_ )
	    emptyitm_ = vwr_.rgbCanvas().scene().addItem(
				new uiTextItem( tr("<---empty--->"),
				mAlignment(HCenter,VCenter) ) );

	emptyitm_->setPenColor( OD::Color::Black() );
	emptyitm_->setPos( uiPoint( vwr_.rgbCanvas().viewWidth()/2,
				    vwr_.rgbCanvas().viewHeight() / 2 ) );
	return;
    }
    else if ( emptyitm_ )
    {
	delete vwr_.rgbCanvas().scene().removeItem( emptyitm_ );
	emptyitm_ = nullptr;
    }

    uiUserShowWait usw( this, uiStrings::sUpdatingDisplay() );
    updateLayerAuxData();
    updateLevelAuxData();
    updateSelSeqAuxData();
    curModEdChgCB( this );
    vwr_.handleChange( sCast(od_uint32,FlatView::Viewer::Auxdata) );
}


void uiStratLayerModelDisp::getFlattenedZRange( Interval<float>& zrg ) const
{
    const Strat::LayerModel& laymod = layerModel();
    const Strat::Level sellvl = tools_.selLevel();
    const int nrseqs = laymod.size();
    for ( int iseq=0; iseq<nrseqs; iseq++ )
    {
	const Strat::LayerSequence& seq = laymod.sequence( iseq );
	Interval<float> seqzrg = seq.zRange();
	const float lvldpth = seqLvlDepth( seq, sellvl );
	if ( mIsUdf(lvldpth) )
	    continue;

	seqzrg.shift( -lvldpth );
        if ( mIsUdf(zrg.start_) )
	    zrg = seqzrg;
	else
	    zrg.include( seqzrg, false );
    }
}


Interval<float> uiStratLayerModelDisp::getModelRange( int propidx ) const
{
    const int nrseqs = layerModel().size();
    Interval<float> rg( mUdf(float), mUdf(float) );
    const bool wantzrg = propidx < 0;

    if ( wantzrg && showFlattened() )
	getFlattenedZRange( rg );
    else
    {
	const Strat::LayerModel& laymod = layerModel();
	for ( int iseq=0; iseq<nrseqs; iseq++ )
	{
	    const Strat::LayerSequence& seq = laymod.sequence( iseq );
	    const Interval<float> vrg = propidx < 0
				      ? seq.zRange()
				      : seq.propRange( propidx );
            if ( mIsUdf(rg.start_) )
		rg = vrg;
	    else
		rg.include( vrg, false );
	}
    }

    if ( wantzrg && mIsUdf(rg.start_) )
        rg.start_ = rg.stop_ = 0.f;

    return rg;
}


void uiStratSimpleLayerModelDisp::updateSelSeqAuxData()
{
    if ( selseqidx_ < 0 )
    {
	deleteAndNullPtr( selseqad_ );
	return;
    }

    if ( !selseqad_ )
    {
	selseqad_ = vwr_.createAuxData( 0 );
	selseqad_->enabled_ = true;
	selseqad_->linestyle_ =
		OD::LineStyle( OD::LineStyle::Dot, 2, OD::Color::Black() );
	selseqad_->zvalue_ = uiFlatViewer::auxDataZVal() + 2;
	vwr_.addAuxData( selseqad_ );
    }

    StepInterval<double> yrg = fvdp_->posData().range( false );
    selseqad_->poly_.erase();
    selseqad_->poly_ += FlatView::Point( mCast(double,selseqidx_+1), yrg.start_);
    selseqad_->poly_ += FlatView::Point( mCast(double,selseqidx_+1), yrg.stop_ );
}


void uiStratSimpleLayerModelDisp::clearObsoleteAuxDatas(
				ObjectSet<FlatView::AuxData>& ads, int fromidx )
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

    const Strat::LevelSet& lvls = Strat::LVLS();
    const int nrlevels = lvls.size();
    const int dispeach = tools_.dispEach();
    const int sellvlidx = selLevelIdx();
    const bool flattened = showFlattened();

    int auxdataidx = 0;
    if ( !lvldpths_.validIdx(sellvlidx) )
	return;

    const TypeSet<float>& sellvlzvals = lvldpths_.get( sellvlidx );
    //TODO: really display them all, or only the current ?
    for( int ilvl=0; ilvl<nrlevels; ilvl++ )
    {
	const Strat::Level lvl = lvls.getByIdx( ilvl );
	const OD::Color lvlcol = lvl.color();
	const TypeSet<float>& zvals = lvldpths_.get( ilvl );

	for ( int iseq=0; iseq<zvals.size(); iseq+=dispeach )
	{
	    float zlvl = zvals[iseq];
	    if ( mIsUdf(zlvl) )
		continue;

	    if ( flattened )
	    {
		const float lvldpth = sellvlzvals.get( iseq );
		if ( mIsUdf(lvldpth) )
		    continue;
		zlvl -= lvldpth;
	    }

	    const double ypos = zlvl;
	    const double xpos1 = iseq + 1;
	    const double xpos2 = iseq + 1 + dispeach;
	    FlatView::AuxData* levelad = nullptr;
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
	    levelad->close_ = false;
	    levelad->enabled_ = true;
	    levelad->linestyle_ = OD::LineStyle(OD::LineStyle::Solid,2,lvlcol);
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
    const Strat::LayerModel& laymod = layerModel();
    const int nrseq = laymod.size();
    const int dispeach = tools_.dispEach();
    const int dispprop = curPropIdx();
    const bool uselithcols = tools_.dispLith();
    const BufferString cntnm( tools_.selContent() );
    const Strat::Content* selcontent =
			  laymod.refTree().contents().getByName( cntnm );
    const bool isallcontents = cntnm == sKey::All();
    Interval<float> vrg( curproprg_ );
    if ( mIsZero(vrg.width(),
                 Math::Abs(vrg.start_)>1e-6f ? Math::Abs(vrg.start_)*1e-6f : 1e-9f) )
    { vrg.start_ -= 1.f; vrg.stop_ += 1.f; }

    const float vwdth = vrg.width();
    const int sellvlidx = selLevelIdx();
    const bool flattened = showFlattened();

    int auxdataidx = 0;
    for ( auto iseq=0; iseq<nrseq; iseq+=dispeach )
    {
	const Strat::LayerSequence& seq = laymod.sequence( iseq );
	const ObjectSet<Strat::Layer>& layers = seq.layers();
	const int nrlayers = layers.size();
	float zshift = 0.f;
	if ( flattened )
	{
	    zshift = lvldpths_.get( sellvlidx ).get( iseq );
	    if ( mIsUdf(zshift) )
		continue;
	}

	if ( !seq.propertyRefs().validIdx(dispprop) )
	    continue;

	const PropertyRef& pr = *seq.propertyRefs().get( dispprop );
	for ( int ilay=0; ilay<nrlayers; ilay++ )
	{
	    const Strat::Layer& lay = *layers.get( ilay );
	    const float val = getLayerPropValue( lay, pr, dispprop );

	    bool mustannotcont = false;
	    if ( !lay.content().isUnspecified() )
		mustannotcont = isallcontents
		    || (selcontent && lay.content() == *selcontent);
	    const OD::Color laycol = lay.dispColor( uselithcols );
	    const OD::Color pencol = mustannotcont ? lay.content().color_
						   : laycol;
	    bool canjoinlayers = ilay > 0;
	    if ( canjoinlayers )
	    {
		const Strat::Layer& prevlay = *seq.layers()[ilay-1];
		const OD::Color prevlaycol = prevlay.dispColor( uselithcols );
		canjoinlayers =
		    prevlay.content()==lay.content() && prevlaycol==laycol;
	    }

	    if ( canjoinlayers )
		auxdataidx--;

	    FlatView::AuxData* layad = nullptr;
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

	    if ( canjoinlayers )
		layad->poly_.pop();
	    else
		layad->poly_.erase();

	    layad->fillcolor_ = laycol;
	    layad->linestyle_ = OD::LineStyle( OD::LineStyle::Solid, 2, pencol);
	    if ( mustannotcont )
		layad->fillpattern_ = lay.content().pattern_;
	    else
	    {
		FillPattern fp;
		fp.setFullFill();
		layad->fillpattern_ = fp;
	    }

	    const double x0 = iseq + 1;
            double relx = (double)(val-vrg.start_)/vwdth;
	    relx *= dispeach;
	    const double x1 = iseq + 1 + relx;
	    float z0 = lay.zTop() - zshift;
	    float z1 = lay.zBot() - zshift;
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
    forceRedispAll();
}


void uiStratSimpleLayerModelDisp::forceRedispAll()
{
    zrg_ = getModelRange( -1 );
    curproprg_ = getModelRange( curPropIdx() );

    updateDataPack();
    fillLevelDepths();

    reDrawAll();
}


void uiStratSimpleLayerModelDisp::updateDataPack()
{
    const Strat::LayerModel& lm = layerModel();
    const int nrseqs = lm.size();

    const StepInterval<double> zrg( zrg_.start_, zrg_.stop_, zrg_.width() * 0.2 );
    fvdp_->posData().setRange( true,
			StepInterval<double>( 1, nrseqs<2 ? 1 : nrseqs, 1 ) );
    fvdp_->posData().setRange( false, zrg );
}


void uiStratSimpleLayerModelDisp::openCB( CallBacker* )
{
    if ( layerads_.isEmpty() )
    {
	uiMSG().error( tr("Please open a Layer Model Description first") );
	return;
    }

    doLayerModelIO( true );
}


void uiStratSimpleLayerModelDisp::saveCB( CallBacker* )
{
    doLayerModelIO( false );
}

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2002
________________________________________________________________________

-*/

#include "uicolortable.h"

#include "coltabsequence.h"
#include "mouseevent.h"
#include "settings.h"

#include "uibutton.h"
#include "uicoltabman.h"
#include "uicoltabtools.h"
#include "uigeninput.h"
#include "uilineedit.h"
#include "uimenu.h"
#include "uipixmap.h"
#include "od_helpids.h"


class uiAutoRangeClipDlg : public uiDialog
{ mODTextTranslationClass(uiAutoRangeClipDlg);
public:

uiAutoRangeClipDlg( uiParent* p, ColTab::MapperSetup& ms,
		    Notifier<uiColorTable>& nf )
    : uiDialog(p,uiDialog::Setup(tr("Ranges/Clipping"),mNoDlgTitle,
				 mODHelpKey(mAutoRangeClipDlgHelpID) )
				.modal(false))
    , ms_(ms)
    , scaleChanged(nf)
{
    showAlwaysOnTop();
    setOkCancelText( uiStrings::sApply(), uiStrings::sClose() );

    doclipfld = new uiGenInput( this, tr("Auto-set scale ranges"),
				BoolInpSpec(true) );
    doclipfld->valuechanged.notify( mCB(this,uiAutoRangeClipDlg,clipPush) );

    clipfld = new uiGenInput( this, tr("Percentage clipped"),
			      FloatInpIntervalSpec() );
    clipfld->setElemSzPol( uiObject::Small );
    clipfld->attach( alignedBelow, doclipfld );

    autosymfld = new uiGenInput( this, tr("Auto detect symmetry"),
				 BoolInpSpec(true) );
    autosymfld->attach( alignedBelow, clipfld );
    autosymfld->valuechanged.notify( mCB(this,uiAutoRangeClipDlg,autoSymPush));

    symfld = new uiGenInput( this, tr("Set symmetrical"), BoolInpSpec(true) );
    symfld->attach( alignedBelow, autosymfld );
    symfld->valuechanged.notify( mCB(this,uiAutoRangeClipDlg,symPush) );

    midvalfld = new uiGenInput( this, tr("Symmetrical Mid Value"),
				FloatInpSpec() );
    midvalfld->setElemSzPol( uiObject::Small );
    midvalfld->attach( alignedBelow, symfld );

    storfld = new uiCheckBox( this, uiStrings::sSaveAsDefault() );
    storfld->attach( alignedBelow, midvalfld );

    updateFields();
}


void updateFields()
{
    doclipfld->setValue( ms_.type_!=ColTab::MapperSetup::Fixed );

    Interval<float> cliprate( ms_.cliprate_.start*100,
			      ms_.cliprate_.stop*100 );
    clipfld->setValue( cliprate );
    autosymfld->setValue( ms_.autosym0_ );
    symfld->setValue( !mIsUdf(ms_.symmidval_) );
    midvalfld->setValue( mIsUdf(ms_.symmidval_) ? 0 : ms_.symmidval_ );

    clipPush(nullptr);
    autoSymPush(nullptr);
    symPush(nullptr);
}


void clipPush( CallBacker* )
{
    const bool doclip = doclipfld->getBoolValue();
    clipfld->display( doclip );
    autosymfld->display( doclip );
    autoSymPush( nullptr );
}

void symPush( CallBacker* )
{
    midvalfld->display( doclipfld->getBoolValue() &&
			!autosymfld->getBoolValue() &&
			symfld->getBoolValue() );
}

void autoSymPush( CallBacker* )
{
    symfld->display( doclipfld->getBoolValue() &&
		     !autosymfld->getBoolValue() );
    symPush( 0 );
}

void enabDefSetts( bool yn ) { storfld->display( yn ); }

bool saveDef()
{ return doclipfld->getBoolValue() && storfld->isChecked(); }

bool acceptOK( CallBacker* )
{
    doApply();
    scaleChanged.trigger();

    if ( saveDef() )
	ColTab::setMapperDefaults( ms_.cliprate_, ms_.symmidval_,
				   ms_.autosym0_ );
    return false;
}


void doApply()
{
    ms_.type_ = doclipfld->getBoolValue()
	? ColTab::MapperSetup::Auto : ColTab::MapperSetup::Fixed;

    Interval<float> cliprate = clipfld->getFInterval();
    cliprate.start = fabs( cliprate.start * 0.01f );
    cliprate.stop = fabs( cliprate.stop * 0.01f );
    ms_.cliprate_ = cliprate;
    const bool autosym = autosymfld->getBoolValue();
    const bool symmetry = !autosym && symfld->getBoolValue();
    ms_.autosym0_ = autosym;
    ms_.symmidval_ = symmetry && !autosym ? midvalfld->getFValue()
						: mUdf(float);
}

protected:
    uiGenInput*		doclipfld;
    uiGenInput*		clipfld;
    uiGenInput*		autosymfld;
    uiGenInput*		symfld;
    uiGenInput*		midvalfld;
    uiCheckBox*		storfld;

    Notifier<uiColorTable>&	scaleChanged;
    ColTab::MapperSetup&	ms_;
};


// uiColorTableSel
uiColorTableSel::uiColorTableSel( uiParent* p, const char* nm )
    : uiComboBox(p,nm)
{
    update();
    ColTab::SM().seqAdded.notify( mCB(this,uiColorTableSel,seqChgCB) );
    ColTab::SM().seqRemoved.notify( mCB(this,uiColorTableSel,seqChgCB) );
}


uiColorTableSel::~uiColorTableSel()
{
    ColTab::SM().seqAdded.remove( mCB(this,uiColorTableSel,seqChgCB) );
    ColTab::SM().seqRemoved.remove( mCB(this,uiColorTableSel,seqChgCB) );
}


void uiColorTableSel::seqChgCB( CallBacker* )
{ update(); }

void uiColorTableSel::update()
{
    const BufferString curitm = getCurrent();
    setEmpty();
    BufferStringSet seqnames;
    ColTab::SM().getSequenceNames( seqnames );
    seqnames.sort();
    for ( int idx=0; idx<seqnames.size(); idx++ )
    {
	const int seqidx = ColTab::SM().indexOf( seqnames.get(idx) );
	if ( seqidx<0 ) continue;

	const ColTab::Sequence& seq = *ColTab::SM().get( seqidx );
	addItem( mToUiStringTodo(seq.name()) );
	uiPixmap pm( 16, 10 ); pm.fill( seq, true );
	setPixmap( idx, pm );
    }

    if ( !curitm.isEmpty() )
    {
	setCurrent( curitm );
	selectionChanged.trigger();
    }
}

void uiColorTableSel::setCurrent( const ColTab::Sequence& seq )
{ setCurrentItem( seq.name() ); }

void uiColorTableSel::setCurrent( const char* nm )
{ setCurrentItem( nm ); }

const char* uiColorTableSel::getCurrent() const
{ return textOfItem( currentItem() ); }



// uiColorTable



uiColorTable::uiColorTable( const ColTab::Sequence& colseq )
    : seqChanged(this)
    , scaleChanged(this)
    , enabmanage_(true)
    , enabclipdlg_(true)
    , mapsetup_( *new ColTab::MapperSetup(ColTab::MapperSetup()
		.type(ColTab::MapperSetup::Auto)
		.symmidval(mUdf(float))
		.cliprate(ColTab::defClipRate())) )
    , coltabseq_(*new ColTab::Sequence(colseq))
    , parent_(nullptr)
    , minfld_(nullptr)
    , maxfld_(nullptr)
    , scalingdlg_(nullptr)
    , enabletrans_(true)
{
}


void uiColorTable::createFields( uiParent* parnt, OD::Orientation orient,
				 bool withminmax )
{
    parent_ = parnt;

    if ( withminmax )
    {
	minfld_ = new uiLineEdit( parnt, "Min" );
	minfld_->returnPressed.notify( mCB(this,uiColorTable,rangeEntered) );
	minfld_->setMinimumWidthInChar( 12 );
    }

    canvas_ = new uiColorTableCanvas( parnt, coltabseq_, true, orient );
    canvas_->getMouseEventHandler().buttonPressed.notify(
			mCB(this,uiColorTable,canvasClick) );
    canvas_->getMouseEventHandler().doubleClick.notify(
			mCB(this,uiColorTable,canvasDoubleClick) );
    canvas_->setStretch( 0, 0 );
    canvas_->reSize.notify( mCB(this,uiColorTable,canvasreDraw) );
    canvas_->setDrawArr( true );

    if ( withminmax )
    {
	maxfld_ = new uiLineEdit( parnt, "Max" );
	maxfld_->setMinimumWidthInChar( 12 );
	maxfld_->returnPressed.notify( mCB(this,uiColorTable,rangeEntered) );
    }

    selfld_ = new uiColorTableSel( parnt, "Table selection" );
    selfld_->selectionChanged.notify( mCB(this,uiColorTable,tabSel) );
    selfld_->setStretch( 0, 0 );
    selfld_->setCurrent( coltabseq_ );
}


uiColorTable::~uiColorTable()
{
    delete &coltabseq_;
    delete &mapsetup_;
    delete scalingdlg_;
}


void uiColorTable::setDispPars( const FlatView::DataDispPars::VD& disppar )
{
    mapsetup_ = disppar.mappersetup_;
    if ( scalingdlg_ ) scalingdlg_->updateFields();
}


void uiColorTable::getDispPars( FlatView::DataDispPars::VD& disppar ) const
{
    disppar.mappersetup_ = mapsetup_;
}


void uiColorTable::setInterval( const Interval<float>& range )
{
    mapsetup_.range_ =  range;
    updateRgFld();
    if ( scalingdlg_ ) scalingdlg_->updateFields();
}


static void getValueFormat( char& format, int& precision )
{
    static bool readfromsettings = true;
    static BufferString fmt = "g";
    static int prec = 5;
    if ( readfromsettings )
    {
	BufferString str1, str2;
	Settings::common().get( "dTect.Color table.Number format", str1, str2 );
	fmt = str1;
	prec = toInt( str2.buf() );
	readfromsettings = false;
    }

    format = fmt.getCStr()[0];
    precision = prec;
}


static void setValue( uiLineEdit* fld, float val )
{
    if ( mIsUdf(val) )
	fld->setText( "" );
    else
    {
	char format;
	int precision;
	getValueFormat( format, precision );
	fld->setValue( toString(val,format,precision) );
    }
}


void uiColorTable::updateRgFld()
{
    if ( !minfld_ ) return;

    setValue( minfld_, mapsetup_.range_.start );
    setValue( maxfld_, mapsetup_.range_.stop );
}


void uiColorTable::setSequence( const char* tblnm, bool emitnotif )
{
    ColTab::Sequence colseq( tblnm );
    setSequence( &colseq, true, emitnotif );
}


void uiColorTable::setSequence( const ColTab::Sequence* ctseq, bool edit,
				bool emitnotif )
{
    if ( ctseq )
    {
	coltabseq_ = *ctseq;
	selfld_->setCurrent( coltabseq_ );
	canvas_->setFlipped( mapsetup_.flipseq_ );
	canvas_->setRGB();
	if ( !emitnotif )
	    seqChanged.trigger();
    }

    selfld_->setSensitive( ctseq && edit );
}


void uiColorTable::canvasreDraw( CallBacker* )
{
    canvas_->setFlipped( mapsetup_.flipseq_ );
    canvas_->setRGB();
}


void uiColorTable::setMapperSetup( const ColTab::MapperSetup* ms,
				   bool emitnotif )
{
    if ( ms )
    {
	mapsetup_ = *ms;
	updateRgFld();
	if ( scalingdlg_ ) scalingdlg_->updateFields();

	if ( !emitnotif )
	    scaleChanged.trigger();
    }

    if ( minfld_ )
    {
	minfld_->setSensitive( ms );
	maxfld_->setSensitive( ms );
    }
}



void uiColorTable::setHistogram( const TypeSet<float>* hist )
{
    histogram_.erase();
    if ( hist )
	histogram_.copy( *hist );
}


void uiColorTable::tabSel( CallBacker* )
{
    const char* seqnm = selfld_->getCurrent();
    setSequence( seqnm );
    seqChanged.trigger();
}


void uiColorTable::canvasClick( CallBacker* )
{
    if ( canvas_->getMouseEventHandler().isHandled() )
	return;

    const MouseEvent& ev = canvas_->getMouseEventHandler().event();
    const bool showmenu = ev.rightButton() ||
			 (ev.ctrlStatus() && ev.leftButton());
    if ( !showmenu )
	return;

    const bool hasseq = selfld_->sensitive();
    const bool hasmapper = minfld_ && minfld_->sensitive();
    if ( !hasseq && !hasmapper )
	return;

    PtrMan<uiMenu> mnu = new uiMenu( parent_, uiStrings::sAction() );

    uiAction* itm = new uiAction(tr("Flipped"), mCB(this,uiColorTable,doFlip) );
    mnu->insertAction( itm, 0 );
    itm->setCheckable( true );
    itm->setChecked( mapsetup_.flipseq_ );

    if ( enabclipdlg_ && hasmapper )
	mnu->insertAction( new uiAction(m3Dots(tr("Ranges/Clipping")),
	    mCB(this,uiColorTable,editScaling)), 1 );
    if ( enabmanage_ && hasseq )
    {
	mnu->insertAction( new uiAction(m3Dots(tr("Manage")),
	    mCB(this,uiColorTable,doManage)), 2 );
	mnu->insertAction( new uiAction(tr("Set as default"),
	    mCB(this,uiColorTable,setAsDefault)), 3 );
    }

    mnu->exec();

    canvas_->getMouseEventHandler().setHandled( true );
}


void uiColorTable::canvasDoubleClick( CallBacker* )
{
    if ( enabmanage_ && selfld_->sensitive() )
	doManage(0);
}


void uiColorTable::commitInput()
{
    mapsetup_.range_.start = minfld_ ? minfld_->getFValue() : 0.f;
    mapsetup_.range_.stop = maxfld_ ? maxfld_->getFValue() : 1.f;
    mapsetup_.type_ = ColTab::MapperSetup::Fixed;
    scaleChanged.trigger();
    if ( scalingdlg_ ) scalingdlg_->updateFields();
}


void uiColorTable::rangeEntered( CallBacker* )
{
    commitInput();
}


void uiColorTable::editScaling( CallBacker* )
{
    if ( !scalingdlg_ )
	scalingdlg_ = new uiAutoRangeClipDlg( parent_, mapsetup_,
					      scaleChanged );

    scalingdlg_->enabDefSetts( enabmanage_ );
    scalingdlg_->show();
    scalingdlg_->raise();
}


void uiColorTable::doFlip( CallBacker* )
{
    mapsetup_.flipseq_ = !mapsetup_.flipseq_;
    canvas_->setFlipped( mapsetup_.flipseq_ );
    canvas_->setRGB();
    scaleChanged.trigger();
}


void uiColorTable::makeSymmetrical( CallBacker* )
{
    Interval<float> rg = mapsetup_.range_;
    const float maxval = fabs(rg.start) > fabs(rg.stop)
		     ? fabs(rg.start) : fabs(rg.stop);
    bool flipped = rg.stop < rg.start;
    rg.start = flipped ? maxval : -maxval;
    rg.stop = flipped ? -maxval : maxval;

    mapsetup_.range_ =  rg;
    mapsetup_.type_ = ColTab::MapperSetup::Fixed;
    updateRgFld();
    if ( scalingdlg_ ) scalingdlg_->updateFields();

    scaleChanged.trigger();
}


void uiColorTable::enableTransparencyEdit( bool yn )
{ enabletrans_ = yn; }


void uiColorTable::doManage( CallBacker* )
{
    mDynamicCastGet( uiToolBar*, toolbar, parent_ );
    uiParent* dlgparent = toolbar ? toolbar->parent() : parent_;
    uiColorTableMan coltabman( dlgparent, coltabseq_, enabletrans_ );
    coltabman.tableChanged.notify( mCB(this,uiColorTable,colTabManChgd) );
    coltabman.tableAddRem.notify( mCB(this,uiColorTable,tableAdded) );
    coltabman.setHistogram( histogram_ );
    coltabman.go();
}


void uiColorTable::colTabManChgd( CallBacker* )
{
    selfld_->setCurrent( coltabseq_ );
    canvas_->setFlipped( mapsetup_.flipseq_ );
    canvas_->setRGB();
    seqChanged.trigger();
}


void uiColorTable::setAsDefault( CallBacker* )
{
    mSettUse(set,"dTect.Color table.Name","",coltabseq_.name());
    Settings::common().write();
}


void uiColorTable::tableAdded( CallBacker* )
{
    selfld_->update();
}


void uiColorTable::orientationChgd( CallBacker* )
{
    OD::Orientation neworient = getOrientation();
    canvas_->setOrientation( neworient );
}


// uiColorTableGroup
uiColorTableGroup::uiColorTableGroup( uiParent* p, OD::Orientation orient,
				      bool nominmax )
    : uiGroup(p,"Color table display/edit")
    , uiColorTable(ColTab::Sequence(""))
{
    init( orient, nominmax );
}


uiColorTableGroup::uiColorTableGroup( uiParent* p, const ColTab::Sequence& seq,
				      OD::Orientation orient, bool nominmax )
    : uiGroup(p,"Color table display/edit")
    , uiColorTable(seq)
{
    init( orient, nominmax );
}


void uiColorTableGroup::init( OD::Orientation orient, bool nominmax )
{
    orientation_ = orient;
    createFields( this, orient, !nominmax );

    const bool vertical = orientation_==OD::Vertical;
    canvas_->setPrefHeight( vertical ? 160 : 25 );
    canvas_->setPrefWidth( vertical ? 30 : 80 );

    if ( vertical )
    {
	if ( !minfld_ )
	    selfld_->attach( centeredBelow, canvas_, 2 );
	else
	{
	    maxfld_->attach( topBorder, 2 );
	    canvas_->attach( centeredBelow, maxfld_, 2 );
	    minfld_->attach( centeredBelow, canvas_, 2 );
	    selfld_->attach( centeredBelow, minfld_, 2 );
	}
	setHCenterObj(canvas_);
    }
    else
    {
	if ( !minfld_ )
	    selfld_->attach( rightOf, canvas_, 2 );
	else
	{
	    canvas_->attach( rightOf, minfld_ );
	    maxfld_->attach( rightOf, canvas_ );
	    selfld_->attach( rightOf, maxfld_ );
	}
    }

    if ( minfld_ )
	setHAlignObj( minfld_ );
    else
	setHAlignObj( canvas_ );
}


uiColorTableGroup::~uiColorTableGroup()
{
    delete canvas_;
}


OD::Orientation uiColorTableGroup::getOrientation() const
{ return orientation_; }



// uiColorTableToolBar
uiColorTableToolBar::uiColorTableToolBar( uiParent* p, bool newline )
    : uiToolBar(p,uiStrings::phrJoinStrings(uiStrings::sColorTable(),
		uiStrings::sToolbar()),uiToolBar::Top,newline)
		, uiColorTable(ColTab::Sequence(""))
{
    init();
}


uiColorTableToolBar::uiColorTableToolBar( uiParent* p,
					  const ColTab::Sequence& seq,
					  bool newline )
    : uiToolBar(p,uiStrings::phrJoinStrings(uiStrings::sColorTable(),
		uiStrings::sToolbar()),uiToolBar::Top,newline)
    , uiColorTable(seq)
{
    init();
}


void uiColorTableToolBar::init()
{
    mDynamicCastGet(uiToolBar*,tb,this)
    createFields( tb, OD::Horizontal, true );

#define mAddTBObj( fld, sz ) \
    fld->setMaximumWidth( sz*uiObject::iconSize() ); \
    tb->addObject( fld );

    mAddTBObj( minfld_, 2 );
    mAddTBObj( canvas_, 2 );
    mAddTBObj( maxfld_, 2 );
    tb->addObject( selfld_ );

    orientationChanged.notify( mCB(this,uiColorTable,orientationChgd) );
}


uiColorTableToolBar::~uiColorTableToolBar()
{}


OD::Orientation uiColorTableToolBar::getOrientation() const
{ return uiToolBar::getOrientation(); }

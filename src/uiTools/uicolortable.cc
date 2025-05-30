/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uicolortable.h"

#include "coltab.h"
#include "coltabsequence.h"
#include "mouseevent.h"
#include "settings.h"

#include "uibutton.h"
#include "uicoltabman.h"
#include "uicoltabtools.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uilineedit.h"
#include "uimenu.h"
#include "uipixmap.h"
#include "uiseparator.h"

#include "od_helpids.h"


static const char* sdTectNumberFormat()
	{ return "dTect.Color table.Number format"; }


class uiAutoRangeClipDlg : public uiDialog
{ mODTextTranslationClass(uiAutoRangeClipDlg);
public:

uiAutoRangeClipDlg( uiParent* p, ColTab::MapperSetup& ms,
		    Notifier<uiColorTable>& nf, bool enabsave )
    : uiDialog(p,Setup(tr("Ranges/Clipping"),
		       mODHelpKey(mAutoRangeClipDlgHelpID))
		    .modal(false).savebutton(enabsave))
    , formatChanged(this)
    , scaleChanged(nf)
    , ms_(ms)
{
    showAlwaysOnTop();
    setOkCancelText( uiStrings::sApply(), uiStrings::sClose() );

    doclipfld_ = new uiGenInput( this, tr("Auto-set scale ranges"),
				BoolInpSpec(true) );
    mAttachCB( doclipfld_->valueChanged, uiAutoRangeClipDlg::clipPush );

    cliptypefld_ = new uiGenInput( this, tr("Clip type"),
			BoolInpSpec(true,tr("Symmetrical"),tr("Asymmetrical")));
    cliptypefld_->attach( alignedBelow, doclipfld_ );
    mAttachCB( cliptypefld_->valueChanged, uiAutoRangeClipDlg::cliptypePush );

    clipfld_ = new uiGenInput( this, tr("Percentage clipped (low/high)"),
			      FloatInpIntervalSpec() );
    clipfld_->setElemSzPol( uiObject::Small );
    clipfld_->attach( alignedBelow, cliptypefld_ );

    autosymfld_ = new uiGenInput( this, tr("Auto detect symmetry"),
				 BoolInpSpec(true) );
    autosymfld_->attach( alignedBelow, clipfld_ );
    mAttachCB( autosymfld_->valueChanged, uiAutoRangeClipDlg::autoSymPush );

    symfld_ = new uiGenInput( this, tr("Set symmetrical"), BoolInpSpec(true) );
    symfld_->attach( alignedBelow, autosymfld_ );
    mAttachCB( symfld_->valueChanged, uiAutoRangeClipDlg::symPush );

    midvalfld_ = new uiGenInput( this, tr("Symmetrical Mid Value"),
				FloatInpSpec() );
    midvalfld_->setElemSzPol( uiObject::Small );
    midvalfld_->attach( alignedBelow, symfld_ );

    auto* sep = new uiSeparator( this );
    sep->attach( stretchedBelow, midvalfld_ );

    BufferStringSet formats;
    formats.add( "e - format as [-]9.9e[+|-]999" );
    formats.add( "f - format as [-]9.9");
    formats.add( "g - use e or f format, whichever is the most concise");
    formatfld_ = new uiGenInput( this, tr("Number format"),
				StringListInpSpec(formats) );
    formatfld_->attach( alignedBelow, midvalfld_ );
    formatfld_->attach( ensureBelow, sep );
    mAttachCB( formatfld_->valueChanged, uiAutoRangeClipDlg::formatChangedCB );

    precisionfld_ = new uiGenInput( this, tr("Precision"),
				    IntInpSpec(5,0,8) );
    precisionfld_->attach( alignedBelow, formatfld_ );
    mAttachCB(precisionfld_->valueChanging,uiAutoRangeClipDlg::formatChangedCB);

    precisioninfolbl_ = new uiLabel( this, getPrecicionInfo(2) );
    precisioninfolbl_->attach( rightTo, precisionfld_ );

    updateFields();
}


~uiAutoRangeClipDlg()
{
    detachAllNotifiers();
}


uiString getPrecicionInfo( int sel )
{
    return sel==2 ? tr("(number of significant digits)")
		  : tr("(number of decimals)");
}


void updateFields()
{
    doclipfld_->setValue( ms_.type_!=ColTab::MapperSetup::Fixed );

    Interval<float> cliprate( ms_.cliprate_.start_*100,
			      ms_.cliprate_.stop_*100 );
    clipfld_->setValue( cliprate );
    autosymfld_->setValue( ms_.autosym0_ );
    symfld_->setValue( !mIsUdf(ms_.symmidval_) );
    midvalfld_->setValue( mIsUdf(ms_.symmidval_) ? 0 : ms_.symmidval_ );

    const bool issym = mIsEqual(cliprate.start_,cliprate.stop_,1e-4);
    cliptypefld_->setValue( issym );

    clipPush( nullptr );
    cliptypePush( nullptr );
    autoSymPush( nullptr );
    symPush( nullptr );
}


void clipPush( CallBacker* )
{
    const bool doclip = doclipfld_->getBoolValue();
    cliptypefld_->display( doclip );
    cliptypePush( nullptr );
}


void cliptypePush( CallBacker* )
{
    const bool doclip = doclipfld_->getBoolValue();
    const bool symclip = cliptypefld_->getBoolValue();
    clipfld_->display( doclip );
    if ( doclip )
	clipfld_->displayField( !symclip, 1, 0 );

    autosymfld_->display( doclip && symclip );
    autoSymPush( nullptr );
}


void autoSymPush( CallBacker* )
{
    const bool doclip = doclipfld_->getBoolValue();
    const bool symclip = cliptypefld_->getBoolValue();
    const bool autosym = autosymfld_->getBoolValue();
    symfld_->display( doclip && symclip && !autosym );
    symPush( nullptr );
}


void symPush( CallBacker* )
{
    const bool doclip = doclipfld_->getBoolValue();
    const bool symclip = cliptypefld_->getBoolValue();
    const bool autosym = autosymfld_->getBoolValue();
    const bool setsymval = symfld_->getBoolValue();
    midvalfld_->display( doclip && symclip && !autosym && setsymval );
}


bool saveDef()
{
    return doclipfld_->getBoolValue() && saveButtonChecked();
}


void setNumberFormat( char fmt, int p )
{
    const int sel = fmt=='e' ? 0 : (fmt=='f' ? 1 : 2);
    formatfld_->setValue( sel );
    precisionfld_->setValue( p );
    precisioninfolbl_->setText( getPrecicionInfo(sel) );
}


void getNumberFormat( char& fmt, int& p )
{
    const int sel = formatfld_->getIntValue();
    fmt = sel==0 ? 'e' : (sel==1 ? 'f' : 'g');
    p = precisionfld_->getIntValue();
}


void formatChangedCB( CallBacker* )
{
    const int sel = formatfld_->getIntValue();
    precisioninfolbl_->setText( getPrecicionInfo(sel) );
    formatChanged.trigger();
}


bool acceptOK( CallBacker* ) override
{
    doApply();
    scaleChanged.trigger();

    if ( saveDef() )
	ColTab::setMapperDefaults( ms_.cliprate_, ms_.symmidval_,
				   ms_.autosym0_ );

    if ( saveButtonChecked() )
    {
	char fmt; int prec;
	getNumberFormat( fmt, prec );
	BufferString fmtstr; fmtstr.add( fmt );
	FileMultiString fullstr;
	fullstr.add( fmt ).add( prec );
	Settings::common().set( sdTectNumberFormat(), fullstr );
	Settings::common().write();
    }

    return false;
}


void doApply()
{
    const bool doclip = doclipfld_->getBoolValue();
    const bool symclip = doclip && cliptypefld_->getBoolValue();
    const bool autosym = symclip && autosymfld_->getBoolValue();
    const bool setsymval = symclip && !autosym && symfld_->getBoolValue();

    ms_.type_ = doclip ? ColTab::MapperSetup::Auto : ColTab::MapperSetup::Fixed;

    Interval<float> cliprate = clipfld_->getFInterval();
    if ( symclip )
	cliprate.stop_ = cliprate.start_;

    cliprate.start_ = Math::Abs( cliprate.start_ * 0.01f );
    cliprate.stop_ = Math::Abs( cliprate.stop_ * 0.01f );
    ms_.cliprate_ = cliprate;
    ms_.autosym0_ = autosym;
    ms_.symmidval_ = setsymval ? midvalfld_->getFValue() : mUdf(float);
}

    Notifier<uiAutoRangeClipDlg>	formatChanged;

protected:
    uiGenInput*		doclipfld_;
    uiGenInput*		cliptypefld_;
    uiGenInput*		clipfld_;
    uiGenInput*		autosymfld_;
    uiGenInput*		symfld_;
    uiGenInput*		midvalfld_;

    uiGenInput*		formatfld_;
    uiGenInput*		precisionfld_;
    uiLabel*		precisioninfolbl_;

    Notifier<uiColorTable>&	scaleChanged;
    ColTab::MapperSetup&	ms_;
};


// uiColorTableSel
uiColorTableSel::uiColorTableSel( uiParent* p, const char* nm )
    : uiComboBox(p,nm)
{
    update();
    mAttachCB( ColTab::SM().seqAdded, uiColorTableSel::seqChgCB );
    mAttachCB( ColTab::SM().seqRemoved, uiColorTableSel::seqChgCB );
}


uiColorTableSel::~uiColorTableSel()
{
    detachAllNotifiers();
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
	if ( seqidx<0 )
	    continue;

	const ColTab::Sequence& seq = *ColTab::SM().get( seqidx );
	addItem( toUiString(seq.name()) );
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
{ setCurrentItem( seq.name().buf() ); }

void uiColorTableSel::setCurrent( const char* nm )
{ setCurrentItem( nm ); }

const char* uiColorTableSel::getCurrent() const
{ return textOfItem( currentItem() ); }



// uiColorTable

static char sNumberFormat = 'g';
static int sNumberPrecision = 5;
static int sExtraFieldWidthef = 7;
static int sExtraFieldWidthg = 6;

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

    canvas_ = new uiColorTableCanvas( parnt, coltabseq_, true, orient );
    canvas_->getMouseEventHandler().buttonPressed.notify(
			mCB(this,uiColorTable,canvasClick) );
    canvas_->getMouseEventHandler().doubleClick.notify(
			mCB(this,uiColorTable,canvasDoubleClick) );
    canvas_->setStretch( 0, 0 );
    canvas_->reSize.notify( mCB(this,uiColorTable,canvasreDraw) );
    canvas_->setDrawArr( true );

    selfld_ = new uiColorTableSel( parnt, "Table selection" );
    selfld_->selectionChanged.notify( mCB(this,uiColorTable,tabSel) );
    selfld_->setStretch( 0, 0 );
    selfld_->setCurrent( coltabseq_ );

    if ( withminmax )
    {
	minfld_ = new uiLineEdit( parnt, "Min" );
	mAttachCB( minfld_->returnPressed, uiColorTable::rangeEntered );

	maxfld_ = new uiLineEdit( parnt, "Max" );
	mAttachCB( maxfld_->returnPressed, uiColorTable::rangeEntered );

	BufferString str1, str2;
	const bool res = Settings::common().get( sdTectNumberFormat(),
						 str1, str2 );
	if ( res )
	{
	    const char fmt = str1.isEmpty() ? 'g' : str1.getCStr()[0];
	    if ( fmt=='e' || fmt=='f' || fmt=='g' )
		sNumberFormat = fmt;

	    const int prec = toInt( str2.buf() );
	    if ( prec>=0 && prec<8 )
		sNumberPrecision = prec;
	}

	const int extrawidth =
		sNumberFormat=='g' ? sExtraFieldWidthg : sExtraFieldWidthef;
	const int fldwidth = sNumberPrecision + extrawidth;
	minfld_->setMinimumWidthInChar( fldwidth );
	maxfld_->setMinimumWidthInChar( fldwidth );
    }
}


uiColorTable::~uiColorTable()
{
    detachAllNotifiers();
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


void uiColorTable::setNumberFormat( char format, int precision )
{
    sNumberFormat = format;
    sNumberPrecision = precision;
    updateRgFld();

    if ( minfld_ && maxfld_ )
    {
	const int extrawidth =
		sNumberFormat=='g' ? sExtraFieldWidthg : sExtraFieldWidthef;
	const int fldwidth = sNumberPrecision + extrawidth;
	minfld_->setMinimumWidthInChar( fldwidth );
	maxfld_->setMinimumWidthInChar( fldwidth );
    }
}


void uiColorTable::getNumberFormat( char &format, int& precision ) const
{
    format = sNumberFormat;
    precision = sNumberPrecision;
}


static void setValue( uiLineEdit* fld, float val )
{
    if ( mIsUdf(val) )
	fld->setText( "" );
    else
	fld->setValue( toStringSpec(val,sNumberFormat,sNumberPrecision) );
}


void uiColorTable::updateRgFld()
{
    if ( !minfld_ ) return;

    setValue( minfld_, mapsetup_.range_.start_ );
    setValue( maxfld_, mapsetup_.range_.stop_ );
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
    const float minval = minfld_ ? minfld_->getFValue() : 0.f;
    const float maxval = maxfld_ ? maxfld_->getFValue() : 1.f;
    if ( mIsEqual(minval,maxval,mDefEpsF) )
    {
	minfld_->setValue( mapsetup_.range_.start_ );
	maxfld_->setValue( mapsetup_.range_.stop_ );
	return;
    }

    mapsetup_.range_.start_ = minval;
    mapsetup_.range_.stop_ = maxval;
    mapsetup_.type_ = ColTab::MapperSetup::Fixed;
    scaleChanged.trigger();
    if ( scalingdlg_ )
	scalingdlg_->updateFields();
}


void uiColorTable::rangeEntered( CallBacker* )
{
    commitInput();
}


void uiColorTable::editScaling( CallBacker* )
{
    if ( !scalingdlg_ )
    {
	scalingdlg_ = new uiAutoRangeClipDlg( parent_, mapsetup_,
					      scaleChanged, enabmanage_ );
	mAttachCB( scalingdlg_->formatChanged,
		   uiColorTable::numberFormatChgdCB );
    }

    scalingdlg_->setNumberFormat( sNumberFormat, sNumberPrecision );
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
    const float maxval = fabs(rg.start_) > fabs(rg.stop_)
			 ? fabs(rg.start_) : fabs(rg.stop_);
    bool flipped = rg.stop_ < rg.start_;
    rg.start_ = flipped ? maxval : -maxval;
    rg.stop_ = flipped ? -maxval : maxval;

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


void uiColorTable::numberFormatChgdCB( CallBacker* )
{
    if ( !scalingdlg_ )
	return;

    char fmt; int prec;
    scalingdlg_->getNumberFormat( fmt, prec );
    setNumberFormat( fmt, prec );
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
	    canvas_->attach( rightOf, minfld_, 2 );
	    maxfld_->attach( rightOf, canvas_, 2 );
	    selfld_->attach( rightOf, maxfld_, 2 );
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
{
    return orientation_;
}


void uiColorTableGroup::setCanvasAsAlignObj()
{
    setHAlignObj( canvas_ );
}


void uiColorTableGroup::setHSzPol( uiObject::SzPolicy pol )
{
    if ( !minfld_ )
	return;

    minfld_->setMinimumWidth( 10 );
    maxfld_->setMinimumWidth( 10 );
    minfld_->setHSzPol( pol );
    maxfld_->setHSzPol( pol );
}


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
    tb->setTabOrder( minfld_, maxfld_ );
    tb->setTabOrder( maxfld_, selfld_ );

    orientationChanged.notify( mCB(this,uiColorTable,orientationChgd) );
}


uiColorTableToolBar::~uiColorTableToolBar()
{}


OD::Orientation uiColorTableToolBar::getOrientation() const
{ return uiToolBar::getOrientation(); }

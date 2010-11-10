/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uicolortable.cc,v 1.39 2010-11-10 15:26:43 cvsbert Exp $";

#include "uicolortable.h"

#include "bufstringset.h"
#include "coltab.h"
#include "color.h"
#include "coltabsequence.h"
#include "coltabmapper.h"
#include "datainpspec.h"
#include "flatview.h"
#include "iopar.h"
#include "pixmap.h"
#include "mouseevent.h"
#include "settings.h"

#include "uibutton.h"
#include "uicoltabman.h"
#include "uicoltabtools.h"
#include "uicombobox.h"
#include "uidialog.h"
#include "uigeninput.h"
#include "uigeom.h"
#include "uilineedit.h"
#include "uimenu.h"
#include "uirgbarray.h"
#include "uirgbarraycanvas.h"

#include <math.h>


uiColorTableSel::uiColorTableSel( uiParent* p, const char* nm )
    : uiComboBox(p,nm)
{
    update();
}


void uiColorTableSel::update()
{
    setEmpty();
    BufferStringSet seqnames;
    ColTab::SM().getSequenceNames( seqnames );
    seqnames.sort();
    for ( int idx=0; idx<seqnames.size(); idx++ )
    {
	const int seqidx = ColTab::SM().indexOf( seqnames.get(idx) );
	if ( seqidx<0 ) continue;

	const ColTab::Sequence& seq = *ColTab::SM().get( seqidx );
	addItem( seq.name() );
	setPixmap( ioPixmap(seq,16,10,true), idx );
    }
}

void uiColorTableSel::setCurrent( const ColTab::Sequence& seq )
{ setCurrentItem( seq.name() ); }

void uiColorTableSel::setCurrent( const char* nm )
{ setCurrentItem( nm ); }

const char* uiColorTableSel::getCurrent() const
{ return textOfItem( currentItem() ); }


#define mStdInitList \
	  seqChanged(this) \
	, scaleChanged(this) \
	, minfld_(0) \
	, maxfld_(0) \
	, enabmanage_(true)


uiColorTable::uiColorTable( uiParent* p, const ColTab::Sequence& colseq,
			    bool vert )
    : uiGroup(p,"Color table display/edit")
    , mStdInitList
    , coltabseq_( *new ColTab::Sequence(colseq) )
    , mapsetup_( *new ColTab::MapperSetup(ColTab::MapperSetup()
	    .type(ColTab::MapperSetup::Auto)
	    .symmidval(mUdf(float))
	    .cliprate(ColTab::defClipRate())) )
    , enabletrans_( true )
{
    minfld_ = new uiLineEdit( this, "Min" );
    minfld_->returnPressed.notify( mCB(this,uiColorTable,rangeEntered) );
    minfld_->setHSzPol( uiObject::Small );
    minfld_->setStretch( 0, 0 );

    canvas_ = new uiColorTableCanvas( this, coltabseq_, true, vert );
    canvas_->getMouseEventHandler().buttonPressed.notify(
			mCB(this,uiColorTable,canvasClick) );
    canvas_->setPrefHeight( vert ? 160 : 25 );
    canvas_->setPrefWidth( vert ? 30 : 80 );
    canvas_->setStretch( 0, 0 );
    canvas_->reSize.notify( mCB(this,uiColorTable,canvasreDraw) );
    canvas_->setDrawArr( true );

    maxfld_ = new uiLineEdit( this, "Max" );
    maxfld_->setHSzPol( uiObject::Small );
    maxfld_->returnPressed.notify( mCB(this,uiColorTable,rangeEntered) );
    maxfld_->setStretch( 0, 0 );

    selfld_ = new uiColorTableSel( this, "Table selection" );
    selfld_->selectionChanged.notify( mCB(this,uiColorTable,tabSel) );
    selfld_->setStretch( 0, 0 );
    selfld_->setCurrent( colseq );

    if ( vert )
    {
	maxfld_->attach( topBorder, 2 );
	canvas_->attach( centeredBelow, maxfld_, 2 );
	minfld_->attach( centeredBelow, canvas_, 2 );
	selfld_->attach( centeredBelow, minfld_, 2 );
    }
    else
    {
	canvas_->attach( rightOf, minfld_ );
	maxfld_->attach( rightOf, canvas_ );
	selfld_->attach( rightOf, maxfld_ );
	setHAlignObj(selfld_); setHCentreObj(selfld_);
    }
}


uiColorTable::uiColorTable( uiParent* p, const char* ctnm, bool vert )
    : uiGroup(p,"Color table display")
    , mStdInitList
    , coltabseq_( *new ColTab::Sequence )
    , mapsetup_( *new ColTab::MapperSetup(ColTab::MapperSetup()
	    .type(ColTab::MapperSetup::Auto)
	    .symmidval(mUdf(float))
	    .cliprate(ColTab::defClipRate())) )
{
    ColTab::SM().get( ctnm, coltabseq_ );
    canvas_ = new uiColorTableCanvas( this, coltabseq_, true, vert );
    canvas_->setPrefHeight( vert ? 160 : 25 );
    canvas_->setPrefWidth( vert ? 30 : 80 );
    canvas_->setStretch( 0, 0 );

    selfld_ = new uiColorTableSel( this, "Table selection" );
    selfld_->attach( vert ? alignedBelow : rightOf, canvas_ );
    selfld_->selectionChanged.notify( mCB(this,uiColorTable,tabSel) );
    selfld_->setStretch( 0, vert ? 1 : 0 );
    selfld_->setCurrent( coltabseq_ );
    setHAlignObj(canvas_); setHCentreObj(canvas_);
}


uiColorTable::~uiColorTable()
{
    delete &coltabseq_;
    delete &mapsetup_;
}


void uiColorTable::setDispPars( const FlatView::DataDispPars::VD& disppar )
{
    mapsetup_.type_ = disppar.autoscale_ ? ColTab::MapperSetup::Auto
					 : ColTab::MapperSetup::Fixed;
    mapsetup_.cliprate_ = disppar.clipperc_.start * 0.01;
    mapsetup_.symmidval_ = disppar.symmidvalue_;
    mapsetup_.start_ =  disppar.rg_.start;
    mapsetup_.width_ =  disppar.rg_.width( false );
}


void uiColorTable::getDispPars( FlatView::DataDispPars::VD& disppar ) const
{
    disppar.autoscale_ = mapsetup_.type_!=ColTab::MapperSetup::Fixed;
    disppar.clipperc_ = Interval<float>(mapsetup_.cliprate_* 100, mUdf(float) );
    disppar.symmidvalue_ = mapsetup_.symmidval_;
    if ( !disppar.autoscale_ )
    {
	disppar.rg_ =
	    Interval<float>(mapsetup_.start_,mapsetup_.start_+mapsetup_.width_);
    }
}


void uiColorTable::setInterval( const Interval<float>& range )
{
    mapsetup_.start_ =  range.start;
    mapsetup_.width_ =  range.width( false );

    updateRgFld();
}


void uiColorTable::updateRgFld()
{
    if ( !minfld_ ) return;

    minfld_->setValue( mapsetup_.start_ );
    maxfld_->setValue( mapsetup_.start_ + mapsetup_.width_ );
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
	canvas_->setRGB();
	if ( !emitnotif )
	    seqChanged.trigger();
    }

    selfld_->setSensitive( ctseq && edit );
}


void uiColorTable::canvasreDraw( CallBacker* )
{
    canvas_->setRGB();
}


void uiColorTable::setMapperSetup( const ColTab::MapperSetup* ms,
				   bool emitnotif )
{
    if ( ms )
    {
	mapsetup_ = *ms;
	updateRgFld();

	if ( !emitnotif )
	    scaleChanged.trigger();
    }

    minfld_->setSensitive( ms );
    maxfld_->setSensitive( ms );
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
    if ( OD::RightButton != ev.buttonState() )
	return;

    const bool hasseq = selfld_->sensitive();
    const bool hasmapper = minfld_->sensitive();
    if ( !hasseq && !hasmapper )
	return;

    PtrMan<uiPopupMenu> mnu = new uiPopupMenu( this, "Action" );
    if ( hasmapper ) 
	mnu->insertItem( new uiMenuItem("Flip",
	    mCB(this,uiColorTable,doFlip)), 0 );
    if ( hasmapper )
	mnu->insertItem( new uiMenuItem("Ranges/Clipping ...",
	    mCB(this,uiColorTable,editScaling)), 1 );
    if ( enabmanage_ && hasseq )
    {
	mnu->insertItem( new uiMenuItem("Manage ...",
	    mCB(this,uiColorTable,doManage)), 2 );
	mnu->insertItem( new uiMenuItem("Set as default",
	    mCB(this,uiColorTable,setAsDefault)), 3 );
    }

    mnu->exec();

    canvas_->getMouseEventHandler().setHandled( true );
}


void uiColorTable::commitInput()
{
    mapsetup_.start_ = minfld_->getfValue();
    mapsetup_.width_ = maxfld_->getfValue() - mapsetup_.start_;
    mapsetup_.type_ = ColTab::MapperSetup::Fixed;
    scaleChanged.trigger();
}


void uiColorTable::rangeEntered( CallBacker* )
{
    commitInput();
}


class uiAutoRangeClipDlg : public uiDialog
{
public:

uiAutoRangeClipDlg( uiParent* p, const ColTab::MapperSetup& ms )
    : uiDialog(p,uiDialog::Setup("Ranges/Clipping","Auto-range and clipping",
				 "50.1.3"))
{
    doclipfld = new uiGenInput( this, "Auto-set scale ranges",
				BoolInpSpec(true) );
    doclipfld->setValue( ms.type_!=ColTab::MapperSetup::Fixed );
    doclipfld->valuechanged.notify( mCB(this,uiAutoRangeClipDlg,clipPush) );

    clipfld = new uiGenInput( this, "Percentage clipped",
			    FloatInpSpec(ms.cliprate_*100) );
    clipfld->setElemSzPol( uiObject::Small );
    clipfld->attach( alignedBelow, doclipfld );

    autosymfld = new uiGenInput( this, "Auto detect symmetry",
				 BoolInpSpec(ms.autosym0_) );
    autosymfld->attach( alignedBelow, clipfld );
    autosymfld->valuechanged.notify( mCB(this,uiAutoRangeClipDlg,autoSymPush));

    symfld = new uiGenInput( this, "Set symmetrical",
	    		     BoolInpSpec(!mIsUdf(ms.symmidval_)) );
    symfld->attach( alignedBelow, autosymfld );
    symfld->valuechanged.notify( mCB(this,uiAutoRangeClipDlg,symPush) );

    midvalfld = new uiGenInput( this, "Symmetrical Mid Value",
		    FloatInpSpec(mIsUdf(ms.symmidval_) ? 0 : ms.symmidval_) );
    midvalfld->setElemSzPol( uiObject::Small );
    midvalfld->attach( alignedBelow, symfld );

    storfld = new uiCheckBox( this, "Save as default" );
    storfld->attach( alignedBelow, midvalfld );

    clipPush(0);
    autoSymPush(0);
    symPush(0);
}

void clipPush( CallBacker* )
{
    const bool doclip = doclipfld->getBoolValue();
    clipfld->display( doclip );
    autosymfld->display( doclip );
    autoSymPush( 0 );
}

void symPush( CallBacker* )
{
    midvalfld->display( doclipfld->getBoolValue() && !autosymfld->getBoolValue()
	   		&& symfld->getBoolValue() );
}

void autoSymPush( CallBacker* )
{
    symfld->display( doclipfld->getBoolValue() && !autosymfld->getBoolValue() );
    symPush( 0 );
}

bool saveDef()
{
    return doclipfld->getBoolValue() && storfld->isChecked();
}


    uiGenInput*		doclipfld;
    uiGenInput*		clipfld;
    uiGenInput*		autosymfld;
    uiGenInput*		symfld;
    uiGenInput*		midvalfld;
    uiCheckBox*		storfld;
};



void uiColorTable::editScaling( CallBacker* )
{
    uiAutoRangeClipDlg dlg( this, mapsetup_ );
    if ( !dlg.go() ) return;

    mapsetup_.type_ = dlg.doclipfld->getBoolValue()
	? ColTab::MapperSetup::Auto : ColTab::MapperSetup::Fixed;

    mapsetup_.cliprate_ = dlg.clipfld->getfValue() * 0.01;
    const bool autosym = dlg.autosymfld->getBoolValue();
    const bool symmetry = !autosym && dlg.symfld->getBoolValue();
    mapsetup_.autosym0_ = autosym;
    mapsetup_.symmidval_ = symmetry && !autosym ? dlg.midvalfld->getfValue()
						: mUdf(float);
    if ( dlg.saveDef() )
	ColTab::setMapperDefaults( mapsetup_.cliprate_, mapsetup_.symmidval_,
	       			   mapsetup_.autosym0_ );

    scaleChanged.trigger();
}


void uiColorTable::doFlip( CallBacker* )
{ 
    mapsetup_.start_ += mapsetup_.width_;
    mapsetup_.width_ *= -1;
    mapsetup_.type_ = ColTab::MapperSetup::Fixed;

    updateRgFld();
    scaleChanged.trigger();
}


void uiColorTable::makeSymmetrical( CallBacker* )
{
    Interval<float> rg( mapsetup_.start_, mapsetup_.start_ + mapsetup_.width_ );
    const float maxval = fabs(rg.start) > fabs(rg.stop)
		     ? fabs(rg.start) : fabs(rg.stop);
    bool flipped = rg.stop < rg.start;
    rg.start = flipped ? maxval : -maxval;
    rg.stop = flipped ? -maxval : maxval;

    mapsetup_.start_ =  rg.start;
    mapsetup_.width_ =  rg.width( false );
    mapsetup_.type_ = ColTab::MapperSetup::Fixed;

    updateRgFld();

    scaleChanged.trigger();
}


void uiColorTable::enableTransparencyEdit( bool yn )
{ enabletrans_ = yn; }


void uiColorTable::doManage( CallBacker* )
{
    uiColorTableMan coltabman( this, coltabseq_, enabletrans_ );
    coltabman.tableChanged.notify( mCB(this,uiColorTable,colTabManChgd) );
    coltabman.tableAddRem.notify( mCB(this,uiColorTable,tableAdded) );
    coltabman.setHistogram( histogram_ );
    coltabman.go();
}


void uiColorTable::colTabManChgd( CallBacker* )
{
    selfld_->setCurrent( coltabseq_ );
    canvas_->setRGB();
    seqChanged.trigger();
}


void uiColorTable::setAsDefault( CallBacker* )
{
    mSettUse(set,"dTect.Color table.Name","",coltabseq_.name());
    Settings::common().write();
}


void uiColorTable::tableAdded( CallBacker* cb )
{
    selfld_->update();
}

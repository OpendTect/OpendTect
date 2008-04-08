/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          June 2002
 RCS:           $Id: uicolortable.cc,v 1.5 2008-04-08 05:05:08 cvssatyaki Exp $
________________________________________________________________________

-*/
#include "uicolortable.h"

#include "bufstringset.h"
#include "coltab.h"
#include "color.h"
#include "coltabsequence.h"
#include "datainpspec.h"
#include "pixmap.h"
#include "iopar.h"

#include "uibutton.h"
#include "uicoltabtools.h"
#include "uicoltabtransp.h"
#include "uidialog.h"
#include "uilineedit.h"
#include "uicombobox.h"
#include "uicoltabman.h"
#include "uimenu.h"
#include "uigeom.h"
#include "uigeninput.h"
#include "uirgbarray.h"
#include "uirgbarraycanvas.h"
#include <math.h>

#define mStdInitList \
	  vertical_(false) \
	, seqChanged(this) \
	, scaleChanged(this) 


uiColorTable::uiColorTable( uiParent* p, ColTab::Sequence& colseq, bool vert )
	: uiGroup(p,"Color table display/edit")
	, mStdInitList
	, coltabseq_(*new ColTab::Sequence(colseq))
	, minfld_(0)
	, canvas_(0)
	, maxfld_(0)
	, selfld_(0)
	, symmidval_(0)
{
    FloatInpSpec minfis;
    minfld_ = new uiLineEdit( this, minfis, "Min" );
    minfld_->returnPressed.notify( mCB(this,uiColorTable,rangeEntered) );
    minfld_->setHSzPol( uiObject::Small );
    minfld_->setStretch( 0, 0 );

    canvas_ = new uiColorTableCanvas( this, coltabseq_, vertical_ );
    canvas_->getMouseEventHandler().buttonPressed.notify(
			mCB(this,uiColorTable,canvasClick) );
    canvas_->setDrawArr( true );

    FloatInpSpec maxfis;
    maxfld_ = new uiLineEdit( this, maxfis, "Max" );
    maxfld_->setHSzPol( uiObject::Small );
    maxfld_->returnPressed.notify( mCB(this,uiColorTable,rangeEntered) );
    maxfld_->setStretch( 0, 0 );

    selfld_ = new uiComboBox( this, "Table selection" );
    selfld_->selectionChanged.notify( mCB(this,uiColorTable,tabSel) );
    selfld_->setStretch( 0, vertical_ ? 1 : 0 );

    if ( vertical_ )
    {
	canvas_->attach( rightAlignedBelow, maxfld_ );
	minfld_->attach( rightAlignedBelow, canvas_ );
	selfld_->attach( centeredBelow, minfld_ );
    }
    else
    {
	canvas_->attach( rightOf, minfld_ );
	maxfld_->attach( rightOf, canvas_ );
	selfld_->attach( rightOf, maxfld_ );
    }

    setHAlignObj(selfld_); setHCentreObj(selfld_);
    fillTabList();
}


uiColorTable::uiColorTable( uiParent* p, const char* ctnm, bool vert )
	: uiGroup(p,"Color table display")
	, mStdInitList
	, coltabseq_( *new ColTab::Sequence() )
	, canvas_(0)
	, selfld_(0)
{
    ColTab::SM().get( ctnm, coltabseq_ );
    canvas_ = new uiColorTableCanvas( this, coltabseq_, vertical_ );

    selfld_ = new uiComboBox( this, "Table selection" );
    selfld_->attach( vertical_ ? alignedBelow : rightOf, canvas_ );
    selfld_->selectionChanged.notify( mCB(this,uiColorTable,tabSel) );
    selfld_->setStretch( 0, vertical_ ? 1 : 0 );
    setHAlignObj(canvas_); setHCentreObj(canvas_);
    
    fillTabList();
}


uiColorTable::~uiColorTable()
{
    delete &coltabseq_;
}


void uiColorTable::setEdits( const Interval<float>& range )
{
    if ( !minfld_ ) return;
    coltabrg_ = range;

    minfld_->setValue( coltabrg_.start );
    maxfld_->setValue( coltabrg_.stop );
}


void uiColorTable::setTable( const char* tblnm, bool emitnotif )
{
    ColTab::Sequence colseq( tblnm );
    setTable( colseq );
}


void uiColorTable::setTable( ColTab::Sequence& ctseq, bool emitnotif )
{
    coltabseq_ = ctseq;
    selfld_->setCurrentItem( coltabseq_.name() );
    canvas_->forceNewFill();
}


void uiColorTable::setHistogram( const TypeSet<float>* hist )
{
    histogram_.erase();
    if ( hist )
	histogram_.copy( *hist );
}


void uiColorTable::fillTabList()
{
    selfld_->empty();
    for ( int idx=0; idx<ColTab::SM().size(); idx++ )
    {
	const ColTab::Sequence& seq = *ColTab::SM().get( idx );
	selfld_->addItem( seq.name() );
	selfld_->setPixmap( ioPixmap(seq,16,10), idx );
    }

    selfld_->setCurrentItem( coltabseq_.name() );
}


void uiColorTable::tabSel( CallBacker* )
{
    const int cbidx = selfld_->currentItem();
    const char* seqnm = selfld_->textOfItem( cbidx );
    setTable( seqnm );
    canvas_->forceNewFill();
    seqChanged.trigger();
}


void uiColorTable::canvasClick( CallBacker* )
{
    if ( canvas_->getMouseEventHandler().isHandled() ) return;
    const MouseEvent& ev = canvas_->getMouseEventHandler().event();

    if ( OD::RightButton != ev.buttonState() )  return;
    PtrMan<uiPopupMenu> mnu = new uiPopupMenu( this, "Action" );
    mnu->insertItem( new uiMenuItem("Flip",
	mCB(this,uiColorTable,doFlip)), 0 );
    mnu->insertItem( new uiMenuItem("Ranges/Clipping ...",
	mCB(this,uiColorTable,editClipRate)), 1 );
    mnu->insertItem( new uiMenuItem("Manage ...",
	mCB(this,uiColorTable,doEdit)), 2 );
    mnu->exec();

    canvas_->getMouseEventHandler().setHandled( true );
}


void uiColorTable::rangeEntered( CallBacker* )
{
    coltabrg_.start = minfld_->getfValue();
    coltabrg_.stop = maxfld_->getfValue();
    scaleChanged.trigger();
}


class uiAutoRangeClipDlg : public uiDialog
{
public:

uiAutoRangeClipDlg( uiParent* p, bool useclip, float cliprate, float symmmidval)
    : uiDialog(p,uiDialog::Setup("Ranges/Clipping","Auto-range and clipping",
				 "50.1.3"))
{
    doclipfld = new uiGenInput( this, "Auto-set scale ranges",
				BoolInpSpec(true) );
    doclipfld->setValue( useclip );
    doclipfld->valuechanged.notify( mCB(this,uiAutoRangeClipDlg,clipPush) );

    clipfld = new uiGenInput( this, "Percentage clipped",
			    FloatInpSpec(cliprate*100) );
    clipfld->setElemSzPol( uiObject::Small );
    clipfld->attach( alignedBelow, doclipfld );

    const bool setsymm = !mIsUdf(symmmidval);
    symmfld = new uiGenInput( this, "Set symmetrical", BoolInpSpec(setsymm) );
    symmfld->attach( alignedBelow, clipfld );
    symmfld->valuechanged.notify( mCB(this,uiAutoRangeClipDlg,symPush) );
    symmidval = new uiGenInput( this, "Symmetrical Mid Value",
	    			FloatInpSpec(0.0) );
    symmidval->setElemSzPol( uiObject::Small );
    symmidval->attach( alignedBelow, symmfld );

    storfld = new uiCheckBox( this, "Save as default" );
    storfld->attach( alignedBelow, symmidval );

    clipPush(0);
    symPush(0);
}

void clipPush( CallBacker* )
{
    const bool doclip = doclipfld->getBoolValue();
    clipfld->display( doclip );
    symmfld->display( doclip );
    storfld->display( doclip );
}

void symPush( CallBacker* )
{
    const bool symmetry = symmfld->getBoolValue();
    symmidval->display( symmetry );
}

bool saveDef()
{
    return doclipfld->getBoolValue() && storfld->isChecked();
}


    uiGenInput*         doclipfld;
    uiGenInput*         clipfld;
    uiGenInput*         symmfld;
    uiGenInput*         symmidval;
    uiCheckBox*		storfld;
};



void uiColorTable::editClipRate( CallBacker* )
{
    uiAutoRangeClipDlg dlg( this, autoscale_, cliprate_, symmidval_ );
    if ( dlg.go() )
    {
	autoscale_ = dlg.doclipfld->getBoolValue();
	cliprate_ = dlg.clipfld->getfValue() * 0.01;
	symmetry_ = dlg.symmfld->getBoolValue();
	if ( symmetry_ )
	    symmidval_ = dlg.symmidval->getfValue();
	if ( dlg.saveDef() )
	    ColTab::setMapperDefaults( cliprate_, symmidval_ );
	scaleChanged.trigger();
    }
}


void uiColorTable::doFlip( CallBacker* )
{
    Swap( coltabrg_.start , coltabrg_.stop );
    autoscale_ = false;
    scaleChanged.trigger();
    setEdits( coltabrg_ );
}


void uiColorTable::makeSymmetrical( CallBacker* )
{
    float maxval = fabs(coltabrg_.start) > fabs(coltabrg_.stop)
		 ? fabs(coltabrg_.start) : fabs(coltabrg_.stop);
    bool flipped = coltabrg_.stop < coltabrg_.start;
    coltabrg_.start = flipped ? maxval : -maxval;
    coltabrg_.stop = flipped ? -maxval : maxval;
}


void uiColorTable::doEdit( CallBacker* )
{
    uiColorTableMan coltabman( this, coltabseq_ );
    coltabman.applycb.notify( mCB(this,uiColorTable,colTabManChgd) );
    coltabman.tableadded.notify( mCB(this,uiColorTable,tableAdded) );
    coltabman.setHistogram( histogram_ );
    coltabman.go();
}


void uiColorTable::colTabManChgd( CallBacker* cb )
{
    mDynamicCastGet(uiColorTableMan*,ctman,cb);
    coltabseq_ = ctman->currentColTab();
    seqChanged.trigger();
}


void uiColorTable::tableAdded( CallBacker* cb )
{
    fillTabList();
}

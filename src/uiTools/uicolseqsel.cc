/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2017
________________________________________________________________________

-*/

#include "uicolseqsel.h"

#include "settings.h"

#include "uimsg.h"
#include "uimenu.h"
#include "uilabel.h"
#include "uicolseqdisp.h"
#include "uicolseqman.h"
#include "uibutton.h"
#include "uipixmap.h"
#include "uimenu.h"



uiColSeqSelTool::uiColSeqSelTool()
    : mandlg_(0)
    , seqChanged(this)
    , newManDlg(this)
    , seqModified(this)
    , refreshReq(this)
    , seqMenuReq(this)
{
}


void uiColSeqSelTool::initialise( OD::Orientation orient )
{
    disp_ = new uiColSeqDisp( asParent(), orient );
    mAttachCB( disp_->selReq, uiColSeqSelTool::selectCB );
    mAttachCB( disp_->menuReq, uiColSeqSelTool::menuCB );
    mAttachCB( disp_->upReq, uiColSeqSelTool::upCB );
    mAttachCB( disp_->downReq, uiColSeqSelTool::downCB );
    mAttachCB( disp_->sequence()->objectChanged(), uiColSeqSelTool::seqModifCB);
}


void uiColSeqSelTool::setToolTip()
{
    disp_->setToolTip( tr("'%1' - Click to change, Right-click for Menu")
			.arg( seqName() ) );
}


#define mMaxLongSz 3*uiObject::iconSize()

void uiColSeqSelTool::addObjectsToToolBar( uiToolBar& tbar )
{
    if ( disp_ )
    {
	disp_->setMaximumWidth( mMaxLongSz );
	disp_->setMaximumHeight( mMaxLongSz );
	tbar.addObject( disp_ );
    }
}


void uiColSeqSelTool::orientationChanged()
{
    disp_->setOrientation( orientation() );
}


uiColSeqSelTool::~uiColSeqSelTool()
{
}


ConstRefMan<ColTab::Sequence> uiColSeqSelTool::sequence() const
{
    return disp_->sequence();
}


void uiColSeqSelTool::setSequence( const Sequence& seq )
{
    mDetachCB( disp_->sequence()->objectChanged(), uiColSeqSelTool::seqModifCB);
    disp_->setSequence( seq );
    mAttachCB( disp_->sequence()->objectChanged(), uiColSeqSelTool::seqModifCB);

    setToolTip();
    seqChanged.trigger();
}


const char* uiColSeqSelTool::seqName() const
{
    return disp_->seqName();
}


void uiColSeqSelTool::setSeqName( const char* nm )
{
    setSequence( *ColTab::SeqMGR().getAny(nm) );
}


ColTab::SeqUseMode uiColSeqSelTool::seqUseMode() const
{
    return disp_->seqUseMode();
}


void uiColSeqSelTool::setSeqUseMode( ColTab::SeqUseMode mode )
{
    disp_->setSeqUseMode( mode );
}


OD::Orientation uiColSeqSelTool::orientation() const
{
    return disp_->orientation();
}


void uiColSeqSelTool::setIsVertical( bool yn )
{
    disp_->setOrientation( yn ? OD::Vertical : OD::Horizontal );
}


void uiColSeqSelTool::setNonSeisDefault()
{
    setSequence( *ColTab::SeqMGR().getDefault(false) );
}


void uiColSeqSelTool::selectCB( CallBacker* )
{
    MonitorLock ml( ColTab::SeqMGR() );
    const int sz = ColTab::SeqMGR().size();
    if ( sz < 1 )
	return;

    uiMenu* mnu = new uiMenu( asParent(), uiStrings::sAction() );
    BufferStringSet nms;
    ColTab::SeqMGR().getSequenceNames( nms );
    nms.sort();

    int curidx = nms.indexOf( disp_->seqName() );
    BufferStringSet menunms;
    if  ( curidx < 0 )
	menunms = nms;
    else
    {
	for ( int ipass=0; ipass<2; ipass++ )
	{
	    for ( int inm=0; inm<nms.size(); inm++ )
	    {
		if (    (ipass == 0 && inm == curidx)
		    ||  (ipass == 1 && inm != curidx) )
		    menunms.add( nms.get(inm) );
	    }
	}
	curidx = 0;
    }

    for ( int inm=0; inm<menunms.size(); inm++ )
    {
	ConstRefMan<Sequence> seq = ColTab::SeqMGR().getByName(
						menunms.get(inm) );
	uiAction* act = new uiAction( toUiString(seq->name()) );
	uiPixmap uipm( 32, 16 );
	uipm.fill( *seq, true );
	act->setPixmap( uipm );
	mnu->insertItem( act, inm );
    }

    const int newnmidx = mnu->exec();
    if ( newnmidx < 0 || newnmidx == curidx )
	return;

    ConstRefMan<Sequence> newseq = ColTab::SeqMGR().getByName(
							menunms.get(newnmidx) );
    if ( !newseq )
	return; // someone has removed it while the menu was up

    ml.unlockNow();
    setSequence( *newseq );
}


void uiColSeqSelTool::seqModifCB( CallBacker* )
{
    seqModified.trigger();
}


void uiColSeqSelTool::menuCB( CallBacker* )
{
    PtrMan<uiMenu> mnu = new uiMenu( asParent(), uiStrings::sAction() );

    mnu->insertItem( new uiAction(tr("Set as default"),
	mCB(this,uiColSeqSelTool,setAsDefaultCB)), 0 );
    mnu->insertItem( new uiAction(m3Dots(tr("Manage")),
	mCB(this,uiColSeqSelTool,manageCB)), 1 );

    seqMenuReq.trigger( mnu );

    mnu->exec();
}


void uiColSeqSelTool::setAsDefaultCB( CallBacker* )
{
    setCurrentAsDefault();
}


void uiColSeqSelTool::manDlgSeqSelCB( CallBacker* )
{
    if ( mandlg_ )
	setSequence( mandlg_->current() );
}


void uiColSeqSelTool::setCurrentAsDefault()
{
    mSettUse( set, "dTect.Color table.Name", "", seqName() );
    Settings::common().write();
}


void uiColSeqSelTool::showManageDlg()
{
    if ( !mandlg_ )
    {
	mandlg_ = new uiColSeqMan( asParent(), seqName() );
	mAttachCB( mandlg_->windowClosed, uiColSeqSelTool::manDlgCloseCB );
	mAttachCB( mandlg_->selectionChanged, uiColSeqSelTool::manDlgSeqSelCB );
	mandlg_->setDeleteOnClose( true );
	mandlg_->show();
	newManDlg.trigger();
    }

    mandlg_->raise();
}


void uiColSeqSelTool::nextColSeq( bool prev )
{
    const int curidx = ColTab::SeqMGR().indexOf( seqName() );
    if ( (prev && curidx < 1) || (!prev && curidx>=ColTab::SeqMGR().size()-1) )
	return;

    const int newidx = prev ? curidx-1 : curidx+1;
    setSeqName( ColTab::SeqMGR().getByIdx(newidx)->name() );
}


uiColSeqSel::uiColSeqSel( uiParent* p, OD::Orientation orient, uiString lbltxt )
    : uiGroup(p,"Color Sequence Selector")
    , lbl_(0)
{
    initialise( orient );

    if ( !lbltxt.isEmpty() )
    {
	lbl_ = new uiLabel( this, lbltxt );
	lbl_->attach( leftOf, disp_ );
    }

    setHAlignObj( disp_ );
}


void uiColSeqSel::setLabelText( const uiString& txt )
{
    lbl_->setText( txt );
}


mImpluiColSeqSelToolBarTool( uiColSeqToolBar, uiColSeqSelTool )

uiColSeqToolBar::uiColSeqToolBar( uiParent* p )
    : uiToolBar(p,tr("Color Selection"))
    , seltool_(*new uiColSeqToolBarTool(this))
{
}


uiColSeqUseMode::uiColSeqUseMode( uiParent* p, uiString lbltxt )
    : uiGroup(p,"ColTab SeqUseMode Group")
    , modeChange(this)
{
    flippedbox_ = new uiCheckBox( this, tr("Flipped") );
    cyclicbox_ = new uiCheckBox( this, tr("Cyclic") );
    cyclicbox_->attach( rightOf, flippedbox_ );
    if ( !lbltxt.isEmpty() )
	new uiLabel( this, lbltxt, flippedbox_ );

    setHAlignObj( flippedbox_ );
    mAttachCB( flippedbox_->activated, uiColSeqUseMode::boxChgCB );
    mAttachCB( cyclicbox_->activated, uiColSeqUseMode::boxChgCB );
}


ColTab::SeqUseMode uiColSeqUseMode::mode() const
{
    return ColTab::getSeqUseMode( flippedbox_->isChecked(),
				  cyclicbox_->isChecked() );
}


void uiColSeqUseMode::setMode( ColTab::SeqUseMode usemode )
{
    flippedbox_->setChecked( ColTab::isFlipped(usemode) );
    cyclicbox_->setChecked( ColTab::isCyclic(usemode) );
}

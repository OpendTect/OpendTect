/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uicoltabman.h"

#include "bufstring.h"
#include "coltabsequence.h"
#include "draw.h"
#include "mouseevent.h"
#include "settings.h"
#include "uistring.h"
#include "uistrings.h"

#include "uibutton.h"
#include "uibuttongroup.h"
#include "uicolor.h"
#include "uicoltabexport.h"
#include "uicoltabimport.h"
#include "uicoltabmarker.h"
#include "uicoltabtools.h"
#include "uifunctiondisplay.h"
#include "uigeninput.h"
#include "uigeninputdlg.h"
#include "uimsg.h"
#include "uispinbox.h"
#include "uisplitter.h"
#include "uistrings.h"
#include "uitoolbutton.h"
#include "uitreeview.h"
#include "uiworld2ui.h"
#include "uistrings.h"
#include "od_helpids.h"

#define mTransHeight	150
#define mTransWidth	200


uiColorTableMan::uiColorTableMan( uiParent* p, ColTab::Sequence& ctab,
       				  bool enabletrans )
    : uiDialog(p,
	uiDialog::Setup(uiStrings::phrManage(uiStrings::sColorTable(mPlural)),
				 tr("Add, remove, change color tables"),
				 mODHelpKey(mColorTableManHelpID) ))
    , tableAddRem(this)
    , tableChanged(this)
    , ctab_(ctab)
    , enabletrans_(enabletrans)
{
    setShrinkAllowed( false );

    auto* leftgrp = new uiGroup( this, "Left" );

    coltablistfld_ = new uiTreeView( leftgrp, "Color Table List" );
    BufferStringSet labels;
    labels.add( "Color table" ).add( "Source" );
    coltablistfld_->addColumns( labels );
    coltablistfld_->setRootDecorated( false );
    coltablistfld_->setStretch( 2, 2 );
    coltablistfld_->setSelectionBehavior( uiTreeView::SelectRows );
    mAttachCB( coltablistfld_->selectionChanged, uiColorTableMan::selChg );

    auto* rightgrp = new uiGroup( this, "Right" );

    OD::LineStyle ls( OD::LineStyle::Solid, 1, OD::Color::LightGrey() );
    uiFunctionDisplay::Setup su;
    su.border(uiBorder(2,5,3,5)).xrg(Interval<float>(0,1)).editable(true)
      .yrg(Interval<float>(0,255)).canvaswidth(mTransWidth).closepolygon(true)
      .canvasheight(mTransHeight).drawscattery1(true) .ycol(OD::Color(255,0,0))
      .y2col(OD::Color(190,190,190)).drawliney2(false).fillbelowy2(true)
      .pointsz(3).ptsnaptol(0.08).noxaxis(true).noxgridline(true).noyaxis(true)
      .noygridline(true).noy2axis(true).noy2gridline(true).drawborder(true)
      .borderstyle(ls);
    cttranscanvas_ = new uiFunctionDisplay( rightgrp, su );
    cttranscanvas_->setStretch( 2, 0 );
    if ( enabletrans_ )
    {
	mAttachCB( cttranscanvas_->pointChanged, uiColorTableMan::transptChg );
	mAttachCB( cttranscanvas_->pointSelected, uiColorTableMan::transptSel );
    }

    markercanvas_ = new uiColTabMarkerCanvas( rightgrp, ctab_ );
    markercanvas_->setPrefWidth( mTransWidth );
    markercanvas_->setPrefHeight( mTransWidth/15 );
    markercanvas_->setStretch( 2, 0 );
    markercanvas_->attach( alignedBelow, cttranscanvas_ );

    ctabcanvas_ =
	new uiColorTableCanvas( rightgrp, ctab_, false, OD::Horizontal );
    mAttachCB( ctabcanvas_->getMouseEventHandler().buttonPressed,
		uiColorTableMan::rightClick );
    mAttachCB( ctabcanvas_->reSize, uiColorTableMan::reDrawCB );

    w2uictabcanvas_ = new uiWorld2Ui( uiWorldRect(0,255,1,0),
				      uiSize(mTransWidth,mTransWidth/10) );
    ctabcanvas_->attach( alignedBelow, markercanvas_, 0 );
    ctabcanvas_->setPrefWidth( mTransWidth );
    ctabcanvas_->setPrefHeight( mTransWidth/10 );
    ctabcanvas_->setStretch( 2, 0 );

    const char* segtypes[] = { "None", "Fixed", "Variable", 0 };
    segmentfld_ = new uiGenInput( rightgrp, tr("Segmentation"),
				  StringListInpSpec(segtypes) );
    mAttachCB( segmentfld_->valueChanged, uiColorTableMan::segmentSel );
    segmentfld_->attach( leftAlignedBelow, ctabcanvas_ );

    nrsegbox_ = new uiSpinBox( rightgrp, 0, 0 );
    nrsegbox_->setInterval( 2, 64 ); nrsegbox_->setValue( 8 );
    nrsegbox_->display( false );
    mAttachCB( nrsegbox_->valueChanging, uiColorTableMan::nrSegmentsCB );
    nrsegbox_->attach( rightTo, segmentfld_ );

    uiColorInput::Setup cisetup( ctab_.undefColor(), enabletrans_ ?
		 uiColorInput::Setup::Separate : uiColorInput::Setup::None );
    cisetup.lbltxt( tr("Undefined color") );
    undefcolfld_ = new uiColorInput( rightgrp, cisetup );
    mAttachCB( undefcolfld_->colorChanged, uiColorTableMan::undefColSel );
    undefcolfld_->attach( alignedBelow, segmentfld_ );

    uiColorInput::Setup ctsu( ctab_.markColor(), uiColorInput::Setup::None );
    ctsu.withdesc( false );
    markercolfld_ = new uiColorInput( rightgrp,
				      ctsu.lbltxt(tr("Marker color")) );
    mAttachCB( markercolfld_->colorChanged, uiColorTableMan::markerColChgd );
    markercolfld_->attach( alignedBelow, undefcolfld_ );

    auto* splitter = new uiSplitter( this, "Splitter", true );
    splitter->addGroup( leftgrp );
    splitter->addGroup( rightgrp );

    auto* butgrp = new uiButtonGroup( this, "actions", OD::Horizontal );
    butgrp->attach( alignedBelow, splitter );

    auto* renamebut = new uiToolButton( butgrp, "renameobj",
					uiStrings::sRename() );
    mAttachCB( renamebut->activated, uiColorTableMan::renameColTab );

    importbut_ = new uiToolButton( butgrp, "import", uiStrings::sImport() );
    mAttachCB( importbut_->activated, uiColorTableMan::importColTab );

    exportbut_ = new uiToolButton( butgrp, "export", uiStrings::sExport() );
    mAttachCB( exportbut_->activated, uiColorTableMan::exportColTab );

    removebut_ = new uiToolButton( butgrp, "delete", uiStrings::sDelete() );
    mAttachCB( removebut_->activated, uiColorTableMan::removeCB );

    auto* saveasbut = new uiToolButton( this, "saveas", uiStrings::sSaveAs() );
    mAttachCB( saveasbut->activated, uiColorTableMan::saveAsCB );
    saveasbut->attach( rightBorder, 0 );

    auto* savebut = new uiToolButton( this, "save", uiStrings::sSave() );
    mAttachCB( savebut->activated, uiColorTableMan::saveCB );
    savebut->attach( leftOf, saveasbut );

    auto* flipbut = new uiToolButton( this, "revpol", tr("Flip") );
    mAttachCB( flipbut->activated, uiColorTableMan::flipCB );
    flipbut->attach( leftOf, savebut );
    flipbut->attach( rightTo, butgrp );

    mAttachCB( markercanvas_->markerChanged, uiColorTableMan::markerChange );
    mAttachCB( ctab_.colorChanged, uiColorTableMan::sequenceChange );
    mAttachCB( ctab_.transparencyChanged, uiColorTableMan::sequenceChange );
    mAttachCB( postFinalize(), uiColorTableMan::doFinalize );
}


uiColorTableMan::~uiColorTableMan()
{
    detachAllNotifiers();
    ctab_.colorChanged.remove( mCB(this,uiColorTableMan,sequenceChange) );
    ctab_.transparencyChanged.remove( mCB(this,uiColorTableMan,sequenceChange));

    delete orgctab_;
    delete w2uictabcanvas_;
}


uiString uiColorTableMan::sKeyDefault()
{ return tr("System"); }
uiString uiColorTableMan::sKeyEdited()
{ return tr("Edited"); }
uiString uiColorTableMan::sKeyOwn()
{ return tr("Own"); }


void uiColorTableMan::doFinalize( CallBacker* )
{
    refreshColTabList( ctab_.name() );
    sequenceChange( 0 );
    toStatusBar( uiString::emptyString(), 1 );
}


void uiColorTableMan::refreshColTabList( const char* selctnm )
{
    BufferStringSet allctnms;
    ColTab::SM().getSequenceNames( allctnms );
    allctnms.sort();
    coltablistfld_->clear();

    BufferString defaultct;
    Settings::common().get( "dTect.Color table.Name", defaultct );
    for ( int idx=0; idx<allctnms.size(); idx++ )
    {
	const int seqidx = ColTab::SM().indexOf( allctnms.get(idx) );
	if ( seqidx<0 ) continue;
	const ColTab::Sequence* seq = ColTab::SM().get( seqidx );

	uiString status;
	if ( seq->type() == ColTab::Sequence::System )
	    status = sKeyDefault();
	else if ( seq->type() == ColTab::Sequence::Edited )
	    status = sKeyEdited();
	else
	    status = sKeyOwn();

	auto* itm = new uiTreeViewItem( coltablistfld_,
		uiTreeViewItem::Setup().label(toUiString(seq->name()))
		.label(status) );
	if ( defaultct == seq->name() )
	    itm->setBold( -1, true );
    }

    uiTreeViewItem* itm = coltablistfld_->findItem( selctnm, 0, true );
    if ( !itm ) return;

    coltablistfld_->setCurrentItem( itm );
    coltablistfld_->setSelected( itm, true );
    coltablistfld_->ensureItemVisible( itm );
}


void uiColorTableMan::updateTransparencyGraph()
{
    TypeSet<float> xvals;
    TypeSet<float> yvals;
    for ( int idx=0; idx<ctab_.transparencySize(); idx++ )
    {
	const Geom::Point2D<float> transp = ctab_.transparency( idx );
	if ( !transp.isDefined() )
	    continue;

        xvals += transp.x_;
        yvals += transp.y_;
    }

    cttranscanvas_->setVals( xvals.arr(), yvals.arr(), xvals.size()  );
}


void uiColorTableMan::selChg( CallBacker* )
{
    const uiTreeViewItem* itm = coltablistfld_->selectedItem();
    if ( !itm || !ColTab::SM().get(itm->text(0),ctab_) )
	return;

    selstatus_ = itm->text( 1 );
    removebut_->setSensitive( selstatus_ != sKeyDefault().getFullString() );
    removebut_->setText( selstatus_==sKeyEdited().getFullString()
				? tr("Revert") : uiStrings::sRemove() );

    markercanvas_->reDrawNeeded.trigger();
    undefcolfld_->setColor( ctab_.undefColor() );
    markercolfld_->setColor( ctab_.markColor() );

    if ( enabletrans_ )
	updateTransparencyGraph();

    delete orgctab_;
    orgctab_ = new ColTab::Sequence;
    *orgctab_ = ctab_;
    issaved_ = true;
    updateSegmentFields();
    tableChanged.trigger();
}


void uiColorTableMan::removeCB( CallBacker* )
{
    if ( selstatus_ == sKeyDefault().getFullString() )
    {
	uiMSG().error(
		tr("This is a default colortable and connot be deleted") );
	return;
    }

    const char* ctnm = ctab_.name();
    uiString msg(tr("%1 '%2' will be deleted.\n%3\nDo you wish to continue?")
	     .arg(selstatus_ == sKeyEdited().getFullString() ?
	     uiStrings::phrJoinStrings(sKeyEdited(),uiStrings::sColorTable()):
	     uiStrings::phrJoinStrings(tr("Own made"),uiStrings::sColorTable()))
	     .arg(ctnm).arg(selstatus_ == sKeyEdited().getFullString()
					 ? tr("and replaced by the default.\n")
					 : uiString::emptyString()));
    if ( !uiMSG().askDelete(msg) )
	return;

    BufferStringSet allctnms;
    ColTab::SM().getSequenceNames( allctnms );
    allctnms.sort();
    int newctabidx = allctnms.indexOf( ctnm );
    const BufferString selctnm = allctnms.get( newctabidx );
    const int toberemovedid = ColTab::SM().indexOf( ctnm );
    ColTab::SM().remove( toberemovedid );
    ColTab::SM().write();
    ColTab::SM().refresh();

    refreshColTabList( selctnm.buf() );
    selChg(0);
    tableAddRem.trigger();
}


void uiColorTableMan::flipCB( CallBacker* )
{
    ctab_.flipColor();
    ctab_.flipTransparency();
    tableChanged.trigger();
    markercanvas_->reDrawNeeded.trigger();
    ctabcanvas_->setRGB();
    updateTransparencyGraph();
}


void uiColorTableMan::saveCB( CallBacker* )
{
    if ( saveColTab(false) )
	tableAddRem.trigger();
}


void uiColorTableMan::saveAsCB( CallBacker* )
{
    if ( saveColTab( true ) )
	tableAddRem.trigger();
}


bool uiColorTableMan::saveColTab( bool saveas )
{
    BufferString newname = ctab_.name();
    if ( saveas )
    {
	uiGenInputDlg dlg( this, tr("Colortable name"), uiStrings::sName(),
			   new StringInpSpec(newname) );
	if ( !dlg.go() )
	    return false;

	newname = dlg.text();
    }

    ColTab::Sequence newctab;
    newctab = ctab_;
    newctab.setName( newname );

    const int newidx = ColTab::SM().indexOf( newname );

    uiString msg;
    if ( newidx<0 )
	newctab.setType( ColTab::Sequence::User );
    else
    {
	ColTab::Sequence::Type tp = ColTab::SM().get(newidx)->type();
	if ( tp == ColTab::Sequence::System )
	{
	    msg = tr("The default colortable will be replaced.\n"
		     "Do you wish to continue?\n"
		     "(Default colortable can be recovered by "
		     "removing the edited one)");
	    newctab.setType( ColTab::Sequence::Edited );
	}
	else if ( tp == ColTab::Sequence::User )
	{
	    msg = tr("Your own made colortable will be replaced\n"
		     "Do you wish to continue?");
	}
	else if ( tp == ColTab::Sequence::Edited )
	{
	    msg = tr("The Edited colortable will be replaced\n"
		     "Do you wish to continue?");
	}
    }

    if ( !msg.isEmpty() && !uiMSG().askContinue( msg ) )
	return false;

    newctab.setUndefColor( undefcolfld_->color() );
    newctab.setMarkColor( markercolfld_->color() );

    ColTab::SM().set( newctab );
    ColTab::SM().write();

    refreshColTabList( newctab.name() );
    selChg(0);
    return true;
}


bool uiColorTableMan::acceptOK( CallBacker* )
{
    ctab_.setUndefColor( undefcolfld_->color() );
    ctab_.setMarkColor( markercolfld_->color() );

    if ( !orgctab_ )
    {
	issaved_ = false;
	saveColTab( true );
    }
    else
    {
	if ( !(ctab_==*orgctab_) )
	    issaved_ = false;

	if ( !issaved_ )
	    saveColTab( false );
    }

    return issaved_;
}


bool uiColorTableMan::rejectOK( CallBacker* )
{
    if ( orgctab_ )
	ctab_ = *orgctab_;

    tableChanged.trigger();
    return true;
}


void uiColorTableMan::undefColSel( CallBacker* )
{
    ctab_.setUndefColor( undefcolfld_->color() );
    ctabcanvas_->setRGB();
    tableChanged.trigger();
}


void uiColorTableMan::markerColChgd( CallBacker* )
{
    ctab_.setMarkColor( markercolfld_->color() );
    tableChanged.trigger();
}


void uiColorTableMan::setHistogram( const TypeSet<float>& hist )
{
    TypeSet<float>& myhist = const_cast<TypeSet<float>&>(hist);
    if ( !myhist.isEmpty() )
    { myhist.removeSingle( 0 ); myhist.removeSingle( hist.size()-1 ); }

    TypeSet<float> x2vals;
    const float step = (float)1/(float)myhist.size();
    for ( int idx=0; idx<myhist.size(); idx++ )
	x2vals += idx*step;
    cttranscanvas_->setY2Vals( x2vals.arr(), myhist.arr(), myhist.size() );
}


void uiColorTableMan::updateSegmentFields()
{
    NotifyStopper ns( nrsegbox_->valueChanging );
    int val = 0;
    if ( ctab_.isSegmentized() )
	val = ctab_.nrSegments()>0 ? 1 : 2;

    segmentfld_->setValue( val );
    nrsegbox_->display( val==1 );
    nrsegbox_->setValue( val==1 ? ctab_.nrSegments() : 8 );
}


void uiColorTableMan::segmentSel( CallBacker* )
{
    nrsegbox_->display( segmentfld_->getIntValue()==1 );
    doSegmentize();
}


void uiColorTableMan::nrSegmentsCB( CallBacker* )
{
    NotifyStopper( ctab_.colorChanged );
    doSegmentize();
}

#define mAddColor(orgidx,newpos) { \
    const Color col = indextbl.colorForIndex( orgidx ); \
    ctab_.setColor( newpos, col.r(), col.g(), col.b() ); }

#define mEps 0.00001

void uiColorTableMan::doSegmentize()
{
    NotifyStopper ns( ctab_.colorChanged );
    if ( segmentfld_->getIntValue()==0 )
	ctab_.setNrSegments( 0 );
    else if ( segmentfld_->getIntValue()==1 )
    {
	const int nrseg = nrsegbox_->getIntValue();
	if ( mIsUdf(nrseg) || nrseg < 2 )
	    return;

	ctab_.setNrSegments( nrseg );
    }
    else
	ctab_.setNrSegments( -1 );

    markercanvas_->reDrawNeeded.trigger();
    ctabcanvas_->setRGB();
    tableChanged.trigger();
}


void uiColorTableMan::rightClick( CallBacker* )
{
    if ( !segmentfld_->isChecked() )
	return;
    if ( ctabcanvas_->getMouseEventHandler().isHandled() )
	return;

    const MouseEvent& ev =
	ctabcanvas_->getMouseEventHandler().event();
    uiWorldPoint wpt = w2uictabcanvas_->transform( ev.pos() );

    selidx_ = -1;
    for ( int idx=0; idx<ctab_.size(); idx++ )
    {
        if ( ctab_.position(idx) > wpt.x_ )
	{
	    selidx_ = idx;
	    break;
	}
    }

    if ( selidx_<0 ) return;
    OD::Color col = ctab_.color( (float) wpt.x_ );
    if ( selectColor(col,this,tr("Color selection"),false) )
    {
	ctab_.changeColor( selidx_-1, col.r(), col.g(), col.b() );
	ctab_.changeColor( selidx_, col.r(), col.g(), col.b() );
	tableChanged.trigger();
    }

    ctabcanvas_->getMouseEventHandler().setHandled( true );
}


void uiColorTableMan::markerChange( CallBacker* )
{
    updateSegmentFields();
    ctabcanvas_->setRGB();
    sequenceChange( 0 );
    tableChanged.trigger();
}


void uiColorTableMan::reDrawCB( CallBacker* )
{
    ctabcanvas_->setRGB();
}


void uiColorTableMan::sequenceChange( CallBacker* )
{
    ctabcanvas_->setRGB();
}


void uiColorTableMan::transptChg( CallBacker* )
{
    const int ptidx = cttranscanvas_->selPt();
    const int nrpts = cttranscanvas_->xVals().size();
    if ( ptidx < 0 )
    {
	ctab_.removeTransparencies();
	for ( int idx=0; idx<nrpts; idx++ )
	{
	    Geom::Point2D<float> pt( cttranscanvas_->xVals()[idx],
				     cttranscanvas_->yVals()[idx] );
	    if ( !pt.isDefined() )
		continue;

            if ( idx==0 && pt.x_>0 )
                pt.x_ = 0;
            else if ( idx==nrpts-1 && pt.x_<1 )
                pt.x_ = 1;

	    ctab_.setTransparency( pt );
	}
    }
    else
    {
	Geom::Point2D<float> pt( cttranscanvas_->xVals()[ptidx],
				 cttranscanvas_->yVals()[ptidx] );
	if ( !pt.isDefined() )
	    return;

	bool reset = false;
        if ( ptidx==0 && !mIsZero(pt.x_,mEps) )
	{
            pt.x_ = 0;
	    reset = true;
	}
        else if ( ptidx==nrpts-1 && !mIsZero(pt.x_-1,mEps) )
	{
            pt.x_ = 1;
	    reset = true;
	}

	if ( reset )
	    updateTransparencyGraph();

	ctab_.changeTransparency( ptidx, pt );
    }
    tableChanged.trigger();
}


void uiColorTableMan::transptSel( CallBacker* )
{
    const int ptidx = cttranscanvas_->selPt();
    const int nrpts = cttranscanvas_->xVals().size();
    if ( ptidx<0 || nrpts == ctab_.transparencySize() )
	return;

    Geom::Point2D<float> pt( cttranscanvas_->xVals()[ptidx],
			     cttranscanvas_->yVals()[ptidx] );
    if ( !pt.isDefined() )
	return;

    ctab_.setTransparency( pt );
    tableChanged.trigger();
}


void uiColorTableMan::importColTab( CallBacker* )
{
    NotifyStopper notifstop( importbut_->activated );
    uiColTabImport dlg( this );
    if ( dlg.go() )
    {
	refreshColTabList( dlg.getCurrentSelColTab() );
	tableAddRem.trigger();
	selChg(0);
    }
}


void uiColorTableMan::exportColTab( CallBacker* )
{
    uiColTabExport dlg( this );
    dlg.go();
}


class uiColTabRenameDlg : public uiGenInputDlg
{ mODTextTranslationClass(uiColTabRenameDlg)
public:
uiColTabRenameDlg( uiParent* p, const char* oldnm )
    : uiGenInputDlg(p,tr("Rename Color Table"),tr("New name"),
		    new StringInpSpec(oldnm))
    , oldnm_(oldnm)
{
    setTitleText( toUiString("%1 '%2'").arg(uiStrings::sRename()).arg(oldnm));
}


bool acceptOK( CallBacker* ) override
{
    const BufferString newnm = text();
    if ( newnm==oldnm_ )
	return true;

    if ( ColTab::SM().indexOf(newnm.buf()) < 0 )
    {
	newnm_ = newnm.buf();
    }
    else
    {
	BufferString altnm;
	for ( int idx=2; idx<=10; idx++ )
	{
	    altnm.set( newnm ).add( " (").add(idx).add(")");
	    if ( ColTab::SM().indexOf(altnm.buf()) >= 0 )
		continue;

	    break;
	}

	const bool res = uiMSG().askGoOn( tr("Rename color table to '%1'?")
					  .arg(altnm) );
	if ( !res )
	    return false;

	newnm_ = altnm.buf();
    }

    return true;
}


BufferString oldnm_;
BufferString newnm_;


};

void uiColorTableMan::renameColTab( CallBacker* )
{
    auto* item = coltablistfld_->currentItem();
    if ( !item )
	return;

    if ( ctab_.type() != ColTab::Sequence::User )
    {
	uiMSG().error( tr("You can only rename user defined color tables") );
	return;
    }

    const BufferString oldnm = ctab_.name();
    uiColTabRenameDlg dlg( this, oldnm );
    if ( !dlg.go() )
	return;

    ColTab::SM().rename( oldnm, dlg.newnm_ );
    ColTab::SM().write();
    refreshColTabList( dlg.newnm_ );
}

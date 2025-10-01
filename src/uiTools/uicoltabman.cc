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
#include "hiddenparam.h"
#include "mouseevent.h"
#include "od_helpids.h"
#include "settings.h"

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
#include "uimenu.h"
#include "uimsg.h"
#include "uispinbox.h"
#include "uisplitter.h"
#include "uistring.h"
#include "uistrings.h"
#include "uistrings.h"
#include "uistrings.h"
#include "uitable.h"
#include "uitoolbutton.h"
#include "uitreeview.h"
#include "uiworld2ui.h"

#define mTransHeight	150
#define mTransWidth	200


static const int sPosCol = 0;
static const int sDataCol = 1;
static const int sTranspCol = 2;
static HiddenParam<uiColorTableMan,Interval<float>*> hp_ctabrange( nullptr );

uiTranspValuesDlg::uiTranspValuesDlg( uiParent* p, ColTab::Sequence& ctab,
				      const Interval<float>& ctabrange )
    : uiDialog(p,Setup(tr("View Point Values"),mNoHelpKey))
    , valuesChanged(this)
    , ctab_(ctab)
    , ctabrange_(ctabrange)
{
    table_ = new uiTable( this, uiTable::Setup(ctab_.transparencySize(),3)
				   .rowgrow(true)
				   .rowdesc(uiStrings::sPoint())
				   .defrowlbl(true)
				   .manualresize(true)
				   .removeselallowed(false)
				   .fillrow(true),
			  "Position Table");

    const uiString color = tr("Color Positions\n(0 - 1)");
    const uiString data = uiStrings::phrJoinStrings( uiStrings::sData(),
						   uiStrings::sValue(mPlural) );
    const uiString transparency = uiStrings::phrJoinStrings(
					uiStrings::sTransparency(),tr("%") );
    const uiStringSet columnlabels = { color, data, transparency };
    table_->setColumnLabels( columnlabels );
    table_->setSelectionMode( uiTable::SingleRow );
    fillTable();

    mAttachCB( table_->valueChanged, uiTranspValuesDlg::colorOrDataChgdCB );
    mAttachCB( table_->valueChanged, uiTranspValuesDlg::transpValChgdCB );
    mAttachCB( table_->rowInserted, uiTranspValuesDlg::pointInsertedCB );
    mAttachCB( table_->rowDeleted, uiTranspValuesDlg::pointDeletedCB );
}


uiTranspValuesDlg::~uiTranspValuesDlg()
{
    detachAllNotifiers();
}


void uiTranspValuesDlg::fillTable()
{
    const int transpsize = ctab_.transparencySize();
    const bool udfrange = ctabrange_.isUdf();
    for ( int cidx=0; cidx<transpsize; cidx++ )
    {
	const Geom::Point2D<float> position = ctab_.transparency( cidx );
	float dataval = mUdf(float);
	if ( !udfrange )
	{
	    const float min = ctabrange_.start_;
	    const float max = ctabrange_.stop_;
	    dataval = position.x_*(max-min)+min;
	}

	const float transperc = position.y_/255*100;
	const float colpos = position.x_;

	table_->setText( RowCol(cidx,sPosCol), toUiStringDec(colpos,2) );
	table_->setText( RowCol(cidx,sDataCol), toUiString(dataval,0,'g',5));
	table_->setText( RowCol(cidx,sTranspCol), toUiStringDec(transperc,0) );
    }

    table_->setCellReadOnly( RowCol(0,0), true );
    table_->setCellReadOnly( RowCol(0,1), true);
    table_->setCellReadOnly( RowCol(table_->nrRows()-1,0), true );
    table_->setCellReadOnly( RowCol(table_->nrRows()-1,1), true );

    if ( udfrange )
	table_->hideColumn( sDataCol, true);

    table_->setMinimumWidthInChar( 52 );
    table_->setPrefHeightInRows( 7 );
}


void uiTranspValuesDlg::colorOrDataChgdCB( CallBacker* )
{
    const RowCol rc = table_->currentCell();
    const int col = rc.col();

    if ( col == 0 )
	handleColorPos();
    else if ( col == 1 )
	handleDataVal();
    else
	return;
}


void uiTranspValuesDlg::handleColorPos()
{
    const RowCol rc = table_->currentCell();
    const int row = rc.row();
    const int col = rc.col();

    const float newcolpos = table_->getFValue( rc );

    const RowCol prevrc = RowCol( row-1, col );
    const float prevval = table_->getFValue( prevrc );

    const RowCol nextrc = RowCol( row+1, col );
    const float nextval = table_->getFValue( nextrc );

    if ( newcolpos>1 || newcolpos<0 )
    {
	const uiString msg = tr("Entered Color Position '%1' is out of range.\n"
				"Please enter a position between 0 and 1.")
				 .arg( newcolpos );
	uiMSG().error( msg );
	table_->setText( rc, toUiStringDec(ctab_.transparency(row).x_,2) );
	return;
    }
    else if ( newcolpos<prevval || newcolpos>nextval )
    {
	const uiString msg = tr("Entered Color Position '%1' is Invalid for "
				"Point %2.\n"
				"Please enter a position between %3 and %4.")
				 .arg( newcolpos )
				 .arg( row+1 )
				 .arg( prevval )
				 .arg( nextval );
	uiMSG().error( msg );
	table_->setText( rc, toUiStringDec(ctab_.transparency(row).x_,2) );
	return;
    }

    if ( !ctabrange_.isUdf() )
    {
	const RowCol rchist = RowCol( row, sDataCol );
	const float min = ctabrange_.start_;
	const float max = ctabrange_.stop_;
	const float dataval = newcolpos*(max-min)+min;
	table_->setText( rchist, toUiString(dataval,0,'g',5) );
    }

    table_->setText( rc, toUiStringDec(newcolpos,2) );
    const float transpval = ctab_.transparency( row ).y_;
    const Geom::Point2D<float> newpoint = { newcolpos, transpval };
    ctab_.changeTransparency( row, newpoint );
    valuesChanged.trigger();
}


void uiTranspValuesDlg::handleDataVal()
{
    if ( ctabrange_.isUdf() )
    {
	pErrMsg("Attempted to change the Data Value column when range is Udf");
	return;
    }

    const RowCol rc = table_->currentCell();
    const int row = rc.row();
    const int col = rc.col();

    const float newdataval = table_->getFValue( rc );

    const RowCol prevrc = RowCol( row-1, col );
    const float prevval = table_->getFValue( prevrc );

    const RowCol nextrc = RowCol( row+1, col );
    const float nextval = table_->getFValue( nextrc );

    const float min = ctabrange_.start_;
    const float max = ctabrange_.stop_;

    if ( newdataval>max || newdataval<min )
    {
	const uiString msg = tr("Entered Data Value '%1' is out of range.\n"
				"Please enter a value between %2 and %3.")
				 .arg( newdataval )
				 .arg( min )
				 .arg( max );
	uiMSG().error( msg );
	const float colorpos = ctab_.transparency(row).x_;
	const float olddataval = colorpos*(max-min)+min;
	table_->setText( rc, toUiString(olddataval,0,'g',5) );
	return;
    }
    else if ( newdataval<prevval || newdataval>nextval )
    {
	const uiString msg = tr("Entered Data Value '%1' is Invalid for "
				"Point %2: "
				"Please enter a value between %3 and %4.")
				 .arg( newdataval )
				 .arg( row+1 )
				 .arg( prevval )
				 .arg( nextval );
	uiMSG().error( msg );
	const float colorpos = ctab_.transparency(row).x_;
	const float olddataval = colorpos*(max-min)+min;
	table_->setText( rc, toUiString(olddataval,0,'g',5) );
	return;
    }

    table_->setText( rc, toUiString(newdataval,0,'g',5) );

    const RowCol colrc = RowCol( row, sPosCol );
    const float newcolorpos = (newdataval-min)/(max-min);
    table_->setText( colrc, toUiStringDec(newcolorpos,2) );

    const float transpval = ctab_.transparency( row ).y_;
    const Geom::Point2D<float> newpoint = { newcolorpos, transpval };
    ctab_.changeTransparency( row, newpoint );
    valuesChanged.trigger();
}


void uiTranspValuesDlg::transpValChgdCB( CallBacker* )
{
    const RowCol rc = table_->currentCell();
    const int row = rc.row();
    const int col = rc.col();
    if ( col < 2 )
	return;

    const float newtranspval = table_->getFValue( rc );

    if ( newtranspval>100 || newtranspval<0 )
    {
	uiMSG().error( tr("Entered Transparency Value '%1' is out of range.\n"
			  "Please enter a percentage between 0 and 100%.")
			    .arg( newtranspval ) );
	const float transperc = ctab_.transparency(row).y_/255*100;
	table_->setText( rc, toUiStringDec(transperc,0) );
	return;
    }

    table_->setText( rc, toUiStringDec(newtranspval,0) );
    const float newcolpos = ctab_.transparency( row ).x_;
    const float convtranspval = newtranspval/100*255;
    const Geom::Point2D<float> newval = { newcolpos, convtranspval };
    ctab_.changeTransparency( row, newval );
    valuesChanged.trigger();
}


void uiTranspValuesDlg::pointInsertedCB( CallBacker* )
{
    NotifyStopper notifstop( table_->valueChanged );
    RowCol rcpos = table_->newCell();
    const int row = rcpos.row();
    if ( row-1<0 || row>=ctab_.transparencySize() )
    {
	table_->removeRow( rcpos );
	uiMSG().error( tr("Cannot insert new points before or after "
			  "extreme positions.") );
	return;
    }

    const float x1 = ctab_.transparency(row-1).x_;
    const float x2 = ctab_.transparency(row).x_;
    const float y1 = ctab_.transparency(row-1).y_;
    const float y2 = ctab_.transparency(row).y_;

    const float epsilonx = 1e-2;
    if ( x2-x1<epsilonx )
    {
	table_->removeRow( rcpos );
	uiMSG().error( tr("Cannot insert a new point here since the point will "
			  "exceed the proximity threshold between "
			  "the previous and next points.") );
	return;
    }

    const float newposval = (x1+x2)/2;
    const float newtransval = (y1+y2)/2;
    const Geom::Point2D<float> newval = { newposval, newtransval };
    table_->setCurrentCell( RowCol(row,sPosCol) );
    ctab_.setTransparency( newval );
    fillTable();
    valuesChanged.trigger();
}


void uiTranspValuesDlg::pointDeletedCB( CallBacker* )
{
    NotifyStopper notifstop( table_->valueChanged );
    const RowCol rc = table_->notifiedCell();
    const int row = rc.row();
    if ( row==0 || row==ctab_.transparencySize()-1 )
    {
	table_->insertRows( rc, 1 );
	fillTable();
	uiMSG().error( tr("Cannot remove remove points "
			  "from extreme positions.") );
	return;
    }

    ctab_.removeTransparencyAt( row );
    fillTable();
    valuesChanged.trigger();
}



uiColorTableMan::uiColorTableMan( uiParent* p, ColTab::Sequence& ctab,
       				  bool enabletrans )
    : uiDialog(p,Setup(uiStrings::phrManage(uiStrings::sColorTable(mPlural)),
		       tr("Add, Remove, and Edit Color Tables"),
		       mODHelpKey(mColorTableManHelpID)).nrstatusflds(2))
    , tableAddRem(this)
    , tableChanged(this)
    , ctab_(ctab)
    , enabletrans_(enabletrans)
{
    hp_ctabrange.setParam( this,
			   new Interval<float>(Interval<float>::udf()) );
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
      .canvasheight(mTransHeight).drawscattery1(true)
      .ylinestyle(OD::LineStyle(OD::LineStyle::Solid,2,OD::Color(255,0,0)))
      .y2linestyle(OD::LineStyle(OD::LineStyle::Solid,2,OD::Color(190,190,190)))
      .drawliney2(false).fillbelowy2(true)
      .pointsz(3).ptsnaptol(0.08).noxaxis(true).noxgridline(true).noyaxis(true)
      .noygridline(true).noy2axis(true).noy2gridline(true).drawborder(true)
      .borderstyle(ls);
    cttranscanvas_ = new uiFunctionDisplay( rightgrp, su );
    cttranscanvas_->setStretch( 2, 0 );
    if ( enabletrans_ )
    {
	mAttachCB( cttranscanvas_->pointChanged, uiColorTableMan::transptChg );
	mAttachCB( cttranscanvas_->pointSelected, uiColorTableMan::transptSel );
	mAttachCB( cttranscanvas_->mouseMove,
		   uiColorTableMan::mouseMoveCB );
	mAttachCB( cttranscanvas_->getMouseEventHandler().buttonReleased,
		   uiColorTableMan::rightClickTranspCB );
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
    mAttachCB( ctab_.transparencyChanged, uiColorTableMan::transpTableChgd );
    mAttachCB( postFinalize(), uiColorTableMan::doFinalize );
}


uiColorTableMan::~uiColorTableMan()
{
    detachAllNotifiers();
    ctab_.colorChanged.remove( mCB(this,uiColorTableMan,sequenceChange) );
    ctab_.transparencyChanged.remove( mCB(this,uiColorTableMan,sequenceChange));
    hp_ctabrange.removeAndDeleteParam( this );

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

    coltablistfld_->setColumnWidthMode( uiTreeView::ResizeToContents );

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
    setHistogram( hist, Interval<float>::udf() );
}


void uiColorTableMan::setHistogram( const TypeSet<float>& hist,
				    const Interval<float>& minmax )
{
    *hp_ctabrange.getParam(this) = minmax;
    TypeSet<float>& myhist = const_cast<TypeSet<float>&>(hist);
    if ( !myhist.isEmpty() )
    {
	myhist.removeSingle( 0 );
	myhist.removeSingle( hist.size()-1 );
    }

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


void uiColorTableMan::rightClickTranspCB(CallBacker*)
{
    const MouseEvent& ev = cttranscanvas_->getMouseEventHandler().event();
    if ( OD::RightButton != ev.buttonState() )
	return;

    uiMenu mnu( parent_, uiStrings::sAction() );
    mnu.insertAction( new uiAction(uiStrings::phrJoinStrings(uiStrings::sView(),
					      uiStrings::sValue(mPlural))), 0 );

    const int res = mnu.exec();

    if ( res == 0 )
    {
	ColTab::Sequence coltab = ctab_;
	uiTranspValuesDlg dlg( parent_, ctab_, *hp_ctabrange.getParam( this ) );
	dlg.valuesChanged.notify( mCB(this,uiColorTableMan,transpTableChgd) );
	if ( !dlg.go() && !(ctab_==coltab) )
	{
	    const uiString msg = tr("Save or Discard changes made to the "
				    "Color Positions?");
	    const bool save = uiMSG().askGoOn( msg, uiStrings::sSaveChanges(),
						    uiStrings::sDiscard());
	    if ( !save )
	    {
		ctab_ = coltab;
		return;
	    }
	}
    }
}


void uiColorTableMan::mouseMoveCB( CallBacker* cb )
{
    mCBCapsuleUnpack(const Geom::PointF&,mousepos,cb);
    const Geom::PointF pos = cttranscanvas_->mapToValue( mousepos, false );
    const Geom::PointF pos2 = cttranscanvas_->mapToValue( mousepos, true );
    float xpos = pos.x_;
    float ypos = pos.y_;

    const bool inyrange = cttranscanvas_->yAxis(false)->range()
							.includes(ypos,true);

    float transperc = 0;
    const float ymax = cttranscanvas_->yAxis(false)->range().stop_;
    if ( inyrange )
    {
	transperc = float(ypos/ymax);
    }
    else
    {
	const float ymin = cttranscanvas_->yAxis(false)->range().start_;
	transperc = std::clamp( ypos, ymin, ymax );
    }
    transperc = std::clamp( transperc, 0.0f, 1.0f )*100;

    const float xval = std::clamp( xpos, 0.0f, 1.0f );

    float dataval = mUdf(float);
    uiString datavalstr = uiStrings::sEmptyString();
    const auto& ctabrange = *hp_ctabrange.getParam( this );
    const bool udfrange = ctabrange.isUdf();
    if ( !udfrange )
    {
	const float min = ctabrange.start_;
	const float max = ctabrange.stop_;
	dataval = xval*(max-min)+min;
	datavalstr = toUiString( dataval, 0, 'g', 5 );
    }

    const uiString colorposstr = toUiString( xval, 0, 'f', 2 );
    const uiString transpvalstr = toUiString( transperc, 0, 'f', 0 );

    const uiString posstr = toUiString("Color Pos: %1%2%3  /  Transp: %4%")
				.arg( colorposstr )
				.arg( udfrange ? "" : "  /  Data Val: " )
				.arg( datavalstr )
				.arg( transpvalstr );
    toStatusBar( posstr );
}


void uiColorTableMan::markerChange( CallBacker* )
{
    updateSegmentFields();
    ctabcanvas_->setRGB();
    sequenceChange( 0 );
    tableChanged.trigger();
}


void uiColorTableMan::transpTableChgd( CallBacker* cb )
{
    sequenceChange( nullptr );
    updateTransparencyGraph();
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

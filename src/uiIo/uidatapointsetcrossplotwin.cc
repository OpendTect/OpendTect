/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki Maitra
 Date:          August 2009
________________________________________________________________________

-*/

#include "uidatapointsetcrossplotwin.h"

#include "uicolor.h"
#include "uicolortable.h"
#include "uicombobox.h"
#include "uicreatedpspdf.h"
#include "uidatapointsetcrossplot.h"
#include "uidpscrossplotpropdlg.h"
#include "uidpsoverlayattrdlg.h"
#include "uidpsrefineseldlg.h"
#include "uidpsselgrpdlg.h"
#include "uigraphicsscene.h"
#include "uilabel.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uipixmap.h"
#include "uispinbox.h"
#include "uistrings.h"
#include "uitable.h"
#include "uitoolbar.h"
#include "uitoolbutton.h"

#include "arrayndimpl.h"
#include "dpsdispmgr.h"
#include "randcolor.h"
#include "settings.h"

static const int cMinPtsForDensity = 20000;

uiDataPointSetCrossPlotter::Setup uiDataPointSetCrossPlotWin::defsetup_;

uiDataPointSetCrossPlotWin::uiDataPointSetCrossPlotWin( uiDataPointSet& uidps )
    : uiMainWin(&uidps,toUiString("%1 - %2").arg(uiStrings::sCrossPlot())
					.arg(uidps.pointSet().name()),2,false)
    , uidps_(uidps)
    , plotter_(*new uiDataPointSetCrossPlotter(this,uidps,defsetup_))
    , disptb_(*new uiToolBar(this,tr("%1 %2 %3")
				     .arg(uiStrings::sCrossPlot())
				     .arg(uiStrings::sDisplay())
				     .arg(uiStrings::sToolbar())))
    , seltb_(*new uiToolBar(this,uiStrings::phrCrossPlot(mJoinUiStrs(
				 sSelection(),sToolbar()))))
    , maniptb_(*new uiToolBar(this,uiStrings::phrCrossPlot(
	  uiStrings::phrJoinStrings(tr("Manipulation"), uiStrings::sToolbar())),
	  uiToolBar::Left))
    , colortb_(*new uiColorTableToolBar(this,ColTab::Sequence("Rainbow"),true))
    , propdlg_(nullptr)
    , selgrpdlg_(nullptr)
    , refineseldlg_(nullptr)
    , grpfld_(nullptr)
    , wantnormalplot_(false)
    , showselptswstbid_(-1)
    , multicolcodtbid_(-1)
{
    mAttachCB( windowClosed, uiDataPointSetCrossPlotWin::closeNotif );

    Settings& setts = Settings::common();
    if ( !setts.get(sKeyMinDPPts(),minptsfordensity_) )
	minptsfordensity_ = cMinPtsForDensity;
    if ( minptsfordensity_ <= 0 || mIsUdf(minptsfordensity_) )
    {
	setts.set( sKeyMinDPPts(), cMinPtsForDensity ); setts.write();
	minptsfordensity_ = cMinPtsForDensity;
    }

    const int nrpts = plotter_.y2_.axis_ ? uidps.pointSet().nrActive()*2
					 : uidps.pointSet().nrActive();
    const float perc = float( 100/(1 + nrpts/minptsfordensity_) );

    uiLabel* lbl = new uiLabel( 0, uiStrings::sShow() );
    disptb_.addObject( lbl );

    eachfld_ = new uiSpinBox( 0, 2, "Percentage" );
    eachfld_->setSuffix( toUiString("%") );
    eachfld_->setValue( perc );
    eachfld_->setInterval( StepInterval<float>(0.f,100.f,0.1f) );
    eachfld_->setToolTip( tr("Percentage of points displayed") );
    mAttachCB( eachfld_->valueChanged, uiDataPointSetCrossPlotWin::eachChg );
    plotter_.plotperc_ = perc;
    disptb_.addObject( eachfld_ );

    densityplottbid_ = disptb_.addButton( "densityplot",tr("Show density plot"),
		  mCB(this,uiDataPointSetCrossPlotWin,setDensityPlot), true );

    showy2tbid_ = disptb_.addButton( "showy2", tr("Toggle show Y2"),
		  mCB(this,uiDataPointSetCrossPlotWin,showY2), true );
    disptb_.turnOn( showy2tbid_, false );

    colortb_.enableManage( false );
    colortb_.display( false );
    mAttachCB( colortb_.seqChanged, uiDataPointSetCrossPlotWin::colTabChanged );
    mAttachCB( colortb_.scaleChanged,uiDataPointSetCrossPlotWin::colTabChanged);

    plotter_.setColTab( colortb_.colTabSeq() );
    plotter_.setCTMapper( colortb_.colTabMapperSetup() );
    plotter_.initDraw();
    plotter_.drawContent();

    selfld_ = new uiComboBox( 0, "Selection Option" );
    selfld_->addItem( tr("Select only Y1") );
    selfld_->addItem( tr("Select only Y2") );
    selfld_->addItem( tr("Select both") );
    mAttachCB( selfld_->selectionChanged,uiDataPointSetCrossPlotWin::selOption);
    selfld_->setSensitive( false );
    seltb_.addObject( selfld_ );

    setselecttbid_ = seltb_.addButton( "altview", tr("Set selectable"),
		mCB(this,uiDataPointSetCrossPlotWin,setSelectable), true );
    seltb_.turnOn( setselecttbid_, true );

    if ( uidps_.displayMgr() )
    {
	BufferString fnm, tooltip;
	uidps_.displayMgr()->getIconInfo( fnm, tooltip );
	showselptswstbid_ = seltb_.addButton( fnm, mToUiStringTodo(tooltip),
		mCB(this,uiDataPointSetCrossPlotWin,showPtsInWorkSpace) );
    }

    seltb_.turnOn( setselecttbid_, false );

    selmodechgtbid_ = seltb_.addButton( "rectangleselect", tr("Selection mode"),
		mCB(this,uiDataPointSetCrossPlotWin,setSelectionMode) );
    seltb_.turnOn( selmodechgtbid_, plotter_.isRubberBandingOn() );

    clearseltbid_ = seltb_.addButton( "clear", tr("Clear all selections"),
		mCB(this,uiDataPointSetCrossPlotWin,removeSelections) );

    seldeltbid_ = seltb_.addButton( "clearselection", tr("Remove all selected"),
		mCB(this,uiDataPointSetCrossPlotWin,deleteSelections) );

    seltabletbid_ = seltb_.addButton( "seltable",tr("Show selections in table"),
		mCB(this,uiDataPointSetCrossPlotWin,showTableSel) );

    manseltbid_ = seltb_.addButton( "selsettings",
		uiStrings::phrManage( uiStrings::sSelection(mPlural).toLower()),
		mCB(this,uiDataPointSetCrossPlotWin,manageSel) );

    refineseltbid_ = seltb_.addButton( "refinesel", tr("Refine selection"),
		mCB(this,uiDataPointSetCrossPlotWin,setSelectionDomain) );

    maniptb_.insertAction( plotter_.getSaveImageAction() );

    maniptb_.addButton( "xplotprop", uiStrings::sProperties(),
			mCB(this,uiDataPointSetCrossPlotWin,editProps) );
    maniptb_.addButton( "prdfs", uiStrings::sCreateProbDesFunc(),
			mCB(this,uiDataPointSetCrossPlotWin,exportPDF) );
    overlayproptbid_ = maniptb_.addButton( "overlayattr",
			tr("Select Overlay Attribute"),
			mCB(this,uiDataPointSetCrossPlotWin,overlayAttrCB) );
    const int nrgrps = uidps_.groupNames().size();
    if ( nrgrps > 1 )
    {
	uiMenu* mnu = new uiMenu( &maniptb_, tr("View Menu") );
	multicolcodtbid_ = maniptb_.addButton( "colorbar",
		tr("Turn on multicolor coding"),
		mCB(this,uiDataPointSetCrossPlotWin,setMultiColorCB), true );
	uiAction* itm = new uiAction( tr("Change color"),
		mCB(this,uiDataPointSetCrossPlotWin,changeColCB) );
	mnu->insertAction( itm, 0 );
	maniptb_.setButtonMenu( multicolcodtbid_, mnu );

	grpfld_ = new uiComboBox( 0, "Group selection" );
	const uiString alltxt = uiStrings::sAll();
	grpfld_->addItem( alltxt );
	TypeSet<OD::Color> ctseqs;
	for ( int idx=0; idx<uidps_.groupNames().size(); idx++ )
	{
	    grpfld_->addItem( uidps_.groupNames().get(idx) );
	    OD::Color coly1, coly2;
	    for ( int idy=0; idy<2; idy++ )
	    {
		OD::Color& col = idy==0 ? coly1 : coly2;
		do
		{ col = getRandomColor(); }
		while ( !ctseqs.addIfNew(col) );
	    }

	    plotter_.y1grpColors().add( coly1 );
	    plotter_.y2grpColors().add( coly2 );
	}
	grpfld_->setCurrentItem( 0 );
	mAttachCB( grpfld_->selectionChanged,
		   uiDataPointSetCrossPlotWin::grpChg );
	disptb_.addObject( grpfld_ );
    }

    setSelectable( 0 );
    mAttachCB( plotter_.drawTypeChanged,
		uiDataPointSetCrossPlotWin::drawTypeChangedCB );
    mAttachCB( plotter_.coltabRgChanged,
		uiDataPointSetCrossPlotWin::coltabRgChangedCB );
    plotter_.setPrefWidth( 700 );
    plotter_.setPrefHeight( 500 );
}


uiDataPointSetCrossPlotWin::~uiDataPointSetCrossPlotWin()
{
    detachAllNotifiers();

    delete removeToolBar( &colortb_ );
    delete refineseldlg_;
    delete propdlg_;
    delete selgrpdlg_;
}


void uiDataPointSetCrossPlotWin::setPercDisp( float perc )
{
    eachfld_->setValue( perc );
    plotter_.plotperc_ = perc;
    plotter_.getRandRowids();
}


void uiDataPointSetCrossPlotWin::handleAxisChg( uiDataPointSet::TColID xcol,
		uiDataPointSet::TColID ycol,uiDataPointSet::TColID y2col )
{
    plotter().setCols( uiPointSet().dColID(xcol),
	    uiPointSet().dColID(ycol), uiPointSet().dColID(y2col) );
    setButtonStatus();
    deleteAndZeroPtr( propdlg_ );
}


void uiDataPointSetCrossPlotWin::setDensityPlot( CallBacker* )
{
    const bool ison = disptb_.isOn( densityplottbid_ );

    disptb_.setToolTip( densityplottbid_, ison ? tr("Show normal plot")
					       : tr("Show density plot") );
    disptb_.setIcon( densityplottbid_,ison ? "xplot" : "densityplot" );
    eachfld_->setSensitive( !ison );
    if ( ison && plotter_.isY2Shown() )
	uiMSG().message( tr("Y2 cannot be displayed in density plot") );

    ison ?  eachfld_->setValue( 100 ) : eachfld_->setValue( plotter_.plotperc_);
    eachfld_->setSensitive( !ison );
    plotter_.setDensityPlot( ison, disptb_.isOn(showy2tbid_) );

    disptb_.setSensitive( showy2tbid_, !ison );
    maniptb_.setSensitive( overlayproptbid_, !ison );
    if ( multicolcodtbid_ >= 0 )
	maniptb_.setSensitive( multicolcodtbid_, !ison );

    setSelComboSensitive( !ison );
    plotter_.drawContent();
    plotter_.reDrawSelections();
}


void uiDataPointSetCrossPlotWin::drawTypeChangedCB( CallBacker* cb )
{
    mCBCapsuleUnpack( bool, yn , cb );
    colortb_.display( yn );
}


void uiDataPointSetCrossPlotWin::coltabRgChangedCB( CallBacker* cb )
{
    mCBCapsuleUnpack( Interval<float>, range , cb );
    colortb_.setInterval( range );
}


void uiDataPointSetCrossPlotWin::colTabChanged( CallBacker* )
{
    plotter_.setColTab( colortb_.colTabSeq() );
    ColTab::MapperSetup mapsetup = colortb_.colTabMapperSetup();
    mapsetup.maxpts_ = 20000;
    plotter_.setCTMapper( colortb_.colTabMapperSetup() );
    plotter_.drawContent();
    plotter_.reDrawSelections();
}


void uiDataPointSetCrossPlotWin::removeSelections( CallBacker* )
{
    if ( !plotter_.selAreaSize() ) return;

    plotter_.removeSelections();
    plotter_.drawContent();
}


void uiDataPointSetCrossPlotWin::showTableSel( CallBacker* )
{
    uidps_.notifySelectedCell();
}


void uiDataPointSetCrossPlotWin::deleteSelections( CallBacker* )
{
    plotter_.deleteSelections();
    plotter_.removeSelections();
    plotter_.dataChanged();
}


void uiDataPointSetCrossPlotWin::closeNotif( CallBacker* )
{
    defsetup_ = plotter_.setup();
    plotter_.plotperc_ = mUdf(float); // Make sure eachChg knows we are closing
}


void uiDataPointSetCrossPlotWin::setSelectionMode( CallBacker* )
{
    plotter_.setRectSelection( !plotter_.isRectSelection() );
    seltb_.setIcon( selmodechgtbid_,
		       plotter_.isRectSelection() ? "rectangleselect"
						  : "polygonselect" );
    plotter_.setDragMode( plotter_.isRectSelection() ?
				uiGraphicsView::RubberBandDrag :
				uiGraphicsView::NoDrag );
}


class uiSelColorDlg : public uiDialog
{ mODTextTranslationClass(uiSelColorDlg)
public:

uiSelColorDlg( uiParent* p, const BufferStringSet& names,
	TypeSet<OD::Color>& y1cols, TypeSet<OD::Color>& y2cols, bool isy2shwn )
    : uiDialog( p, uiDialog::Setup(tr("Select color for Y1 and Y2"),
				   mNoDlgTitle,mODHelpKey(mSelColorDlgHelpID)) )
    , y1cols_(y1cols)
    , y2cols_(y2cols)
    , names_(names)
    , isy2shown_(isy2shwn)
{
    tbl_ = new uiTable( this, uiTable::Setup(names.size(),isy2shwn ? 2 : 1),"");
    tbl_->leftClicked.notify( mCB(this,uiSelColorDlg,changeColCB) );
    tbl_->setRowLabels( names.getUiStringSet() );
    uiStringSet collabel;
    collabel.add( uiStrings::sY() );
    if ( isy2shwn )
        collabel.add( uiStrings::sY2() );

    tbl_->setColumnLabels( collabel );
    for ( int idx=0; idx<names.size(); idx++ )
    {
	tbl_->setColor( RowCol(idx,0), y1cols[idx] );
	if ( isy2shwn ) tbl_->setColor( RowCol(idx,1), y2cols[idx] );
    }
}

void changeColCB( CallBacker* )
{
    RowCol rc = tbl_->notifiedCell();

    OD::Color newcol = tbl_->getColor( rc );
    if ( selectColor(newcol,this,tr("Marker Color")) )
    {
	rc.col() == 0 ? y1cols_[rc.row()] = newcol : y2cols_[rc.row()] = newcol;
	tbl_->setColor( rc, newcol );
    }
}

bool acceptOk( CallBacker* )
{
    for ( int idx=0; idx<names_.size(); idx++ )
    {
	y1cols_[idx] = tbl_->getColor( RowCol(idx,0) );
	if ( isy2shown_ )
	    y2cols_[idx] = tbl_->getColor( RowCol(idx,1) );
    }

    return true;
}

    uiTable*			tbl_;

    TypeSet<OD::Color>&		y1cols_;
    TypeSet<OD::Color>&		y2cols_;
    BufferStringSet		names_;
    bool			isy2shown_;
};


void uiDataPointSetCrossPlotWin::setSelectionDomain( CallBacker* )
{
    BufferStringSet colnames;
    const DataPointSet& dps = plotter_.dps();
    uiDataPointSet::DColID dcid=-dps.nrFixedCols()+1;
    for ( ; dcid<dps.nrCols(); dcid++ )
	colnames.add( uidps_.userName(dcid) );

    if ( !refineseldlg_ )
	refineseldlg_ = new uiDPSRefineSelDlg( plotter_ );

    refineseldlg_->go();
}


void uiDataPointSetCrossPlotWin::setSelectable( CallBacker* )
{
    const bool isoff = !seltb_.isOn(setselecttbid_ );
    seltb_.setIcon( setselecttbid_, !isoff ? "altview" : "altpick");
    plotter_.setSceneSelectable( isoff );
    selfld_->setSensitive( plotter_.isY2Shown() ? isoff : false );
    seltb_.setSensitive( selmodechgtbid_, isoff );
    seltb_.setSensitive( refineseltbid_, isoff );
    seltb_.setSensitive( manseltbid_, isoff );
    seltb_.setSensitive( seltabletbid_, isoff );
    seltb_.setSensitive( seldeltbid_, isoff );
    seltb_.setSensitive( clearseltbid_, isoff );
    plotter_.setDragMode(  plotter_.isSceneSelectable()
				? ( plotter_.isRectSelection()
				    ? uiGraphicsView::RubberBandDrag
				    : uiGraphicsView::NoDrag )
				: uiGraphicsView::ScrollHandDrag );
    disptb_.turnOn( showy2tbid_, plotter_.isY2Shown() );
}


void uiDataPointSetCrossPlotWin::showY2( CallBacker* )
{
    MouseCursorChanger cursorlock( MouseCursor::Wait );
    if ( plotter_.y2_.axis_ )
    {
	plotter_.showY2( disptb_.isOn(showy2tbid_) );
	setSelComboSensitive( disptb_.isOn(showy2tbid_) );
    }
}


void uiDataPointSetCrossPlotWin::showPtsInWorkSpace( CallBacker* )
{
    uidps_.selPtsToBeShown.trigger();
}


void uiDataPointSetCrossPlotWin::selOption( CallBacker* )
{
    const int selitem = selfld_->currentItem();
    if ( selitem == 0 )
	plotter_.setSelectable( true, false );
    else if ( selitem == 1 )
	plotter_.setSelectable( false, true );
    else
	plotter_.setSelectable( true, true );
}


void uiDataPointSetCrossPlotWin::eachChg( CallBacker* )
{
    MouseCursorChanger cursorchanger( MouseCursor::Wait );
    if ( mIsUdf(plotter_.plotperc_) ) return; // window is closing

    float newperc = eachfld_->getFValue();
    if ( plotter_.isADensityPlot() )
    {
	if ( !mIsEqual(newperc,100,mDefEps) )
	{
	    uiMSG().message( tr("Density Plot will always display all data") );
	    eachfld_->setValue( 100.f );
	}
	return;
    }

    const float prevperc = plotter_.plotperc_;
    if ( mIsEqual(plotter_.plotperc_,newperc,mDefEps) )
	return;

    plotter_.plotperc_ = newperc;

    const int estpts =
	mNINT32( ((!disptb_.isOn(showy2tbid_) && !plotter_.y2_.axis_)
	    ? uidps_.pointSet().nrActive()
	    : uidps_.pointSet().nrActive()*2)*plotter_.plotperc_/100.f );

    Settings& setts = Settings::common();
    if ( !setts.get(sKeyMinDPPts(),minptsfordensity_) )
	minptsfordensity_ = cMinPtsForDensity;
    if ( minptsfordensity_ <= 0 || mIsUdf(minptsfordensity_) )
    {
	setts.set( sKeyMinDPPts(), cMinPtsForDensity ); setts.write();
	minptsfordensity_ = cMinPtsForDensity;
    }

    if ( estpts > minptsfordensity_ && !plotter_.isADensityPlot() )
    {
	uiString msg = tr("This is a time consuming process; it may freeze the "
			  "application due to the large dataset size.\n\n"
			  "Do you want to go ahead with normal plot or have a "
			  "density plot which would be faster?"
			  "\nNote: a density plot cannot have a Y2 axis");
	const int res =
	    uiMSG().askGoOnAfter( msg, uiStrings::sCancel(), tr("Normal Plot"),
                                  tr("Density Plot"));
	if ( res==1 )
	    wantnormalplot_ = true;
	else if ( res==0 )
	    wantnormalplot_ = false;
	else
	{
	    eachfld_->setValue( prevperc );
	    eachfld_->setSensitive( true );
	    return;
	}

	disptb_.turnOn( densityplottbid_, !wantnormalplot_ );
	const bool ison = disptb_.isOn( densityplottbid_ );
	disptb_.setToolTip( densityplottbid_, ison ? tr("Show normal plot")
						   : tr("Show density plot") );
	eachfld_->setSensitive( !ison );
	if ( ison )
	    eachfld_->setValue( 100 );

	eachfld_->setSensitive( !ison );
	plotter_.setDensityPlot( !wantnormalplot_, disptb_.isOn(showy2tbid_) );
    }

    plotter_.getRandRowids();
    plotter_.drawContent( false );
    plotter_.reDrawSelections();
}


void uiDataPointSetCrossPlotWin::grpChg( CallBacker* )
{
    if ( !grpfld_ ) return;
    plotter_.curgrp_ = grpfld_->currentItem();
    plotter_.dataChanged();
}


void uiDataPointSetCrossPlotWin::setSelComboSensitive( bool yn )
{
    bool status = plotter_.isSceneSelectable() && !plotter_.isADensityPlot() &&
		  plotter_.isY2Shown() ? yn : false;
    if ( !status )
    {
	selfld_->setCurrentItem( 0 );
	plotter_.setSelectable( true, false );
    }

    selfld_->setSensitive( status );
}


void uiDataPointSetCrossPlotWin::exportPDF( CallBacker* )
{
    BufferStringSet colnames;
    const DataPointSet& dps = plotter_.dps();
    uiDataPointSet::DColID dcid=-dps.nrFixedCols()+1;
    for ( ; dcid<dps.nrCols(); dcid++ )
	colnames.add( uidps_.userName(dcid) );

    uiCreateDPSPDF dlg( this, &plotter_ );
    dlg.go();
}


void uiDataPointSetCrossPlotWin::manageSel( CallBacker* )
{
    BufferStringSet colnames;
    const DataPointSet& dps = plotter_.dps();
    uiDataPointSet::DColID dcid=-dps.nrFixedCols()+1;
    for ( ; dcid<dps.nrCols(); dcid++ )
	colnames.add( uidps_.userName(dcid) );
    if ( !selgrpdlg_ )
	selgrpdlg_ = new uiDPSSelGrpDlg( plotter_, colnames );
    selgrpdlg_->go();
}


void uiDataPointSetCrossPlotWin::overlayAttrCB( CallBacker* )
{
    if ( !plotter_.axisHandler(0) || !plotter_.axisHandler(1) ) return;
    uiDPSOverlayPropDlg dlg( this, plotter_ );
    dlg.go();
}


void uiDataPointSetCrossPlotWin::editProps( CallBacker* )
{
    if ( !plotter_.axisHandler(0) || !plotter_.axisHandler(1) ) return;
    if ( !propdlg_ )
	propdlg_ = new uiDataPointSetCrossPlotterPropDlg( &plotter_ );
    propdlg_->show();
    propdlg_->raise();
}


void uiDataPointSetCrossPlotWin::setGrpColors()
{
    if ( !plotter_.axisData(1).axis_ || (plotter_.isY2Shown() &&
	 !plotter_.isMultiColMode() && !plotter_.axisData(2).axis_) )
	return;

    for ( int idx=0; idx<uidps_.groupNames().size(); idx++ )
    {
	OD::Color coly1 = plotter_.isMultiColMode()
	    ? plotter_.y1grpColors()[idx]
	    : plotter_.axisData(1).axis_->setup().style_.color_;
	OD::Color coly2 = !plotter_.isY2Shown()
	    ? coly1 : plotter_.isMultiColMode()
	    ? plotter_.y2grpColors()[idx]
	    : plotter_.axisData(2).axis_->setup().style_.color_;
	ColTab::Sequence ctseq;
	ctseq.setColor( 0, coly1.r(), coly1.g(), coly1.b() );
	ctseq.setColor( 1, coly2.r(), coly2.g(), coly2.b() );
	ctseq.setNrSegments( 2 );
	uiPixmap pixmap( 20, 20 );
	pixmap.fill( ctseq, true );
	if ( grpfld_ )
	    grpfld_->setPixmap( idx+1, pixmap );
    }
}


void uiDataPointSetCrossPlotWin::setMultiColorCB( CallBacker* )
{
    const bool ison = maniptb_.isOn( multicolcodtbid_ );
    plotter_.setMultiColMode( ison );

    setGrpColors();
    uiString tooltip = ison ? tr("Turn off color coding")
			        : tr("Turn on color coding");
    maniptb_.setToolTip( multicolcodtbid_, tooltip );
    plotter_.drawContent( false );
    plotter_.reDrawSelections();
}


void uiDataPointSetCrossPlotWin::changeColCB( CallBacker* )
{
    const bool ison = maniptb_.isOn( multicolcodtbid_ );
    if ( ison )
    {
	uiSelColorDlg seldlg( this, uidps_.groupNames(), plotter_.y1grpColors(),
			      plotter_.y2grpColors(), plotter_.isY2Shown() );
	if ( !seldlg.go() )
	    return;

	setGrpColors();
	plotter_.drawContent( false );
	plotter_.reDrawSelections();
    }
    else
	uiMSG().message( tr("Cannot change color in this mode.") );
}

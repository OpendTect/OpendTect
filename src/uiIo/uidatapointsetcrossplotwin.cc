/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki Maitra
 Date:          August 2009
 RCS:           $Id: uidatapointsetcrossplotwin.cc,v 1.32 2010-12-09 11:41:11 cvsnanne Exp $: 
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uidatapointsetcrossplotwin.cc,v 1.32 2010-12-09 11:41:11 cvsnanne Exp $";

#include "uidatapointsetcrossplotwin.h"

#include "uitoolbutton.h"
#include "uicreatedpspdf.h"
#include "uidpscrossplotpropdlg.h"
#include "uidpsoverlayattrdlg.h"
#include "uidatapointsetcrossplot.h"
#include "uidatapointset.h"
#include "uidialog.h"
#include "uigeninput.h"
#include "uicolor.h"
#include "uicolortable.h"
#include "uicombobox.h"
#include "uidlggroup.h"
#include "uigeninput.h"
#include "uigeninputdlg.h"
#include "uigraphicsscene.h"
#include "uiimpexpselgrp.h"
#include "uilabel.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uispinbox.h"
#include "uistatusbar.h"
#include "uitable.h"
#include "uitoolbar.h"
#include "pixmap.h"

#include "arraynd.h"
#include "arrayndimpl.h"
#include "coltabsequence.h"
#include "datapointset.h"
#include "datainpspec.h"
#include "dpsdispmgr.h"
#include "envvars.h"
#include "linear.h"
#include "mathexpression.h"
#include "mousecursor.h"
#include "randcolor.h"
#include "rowcol.h"
#include "sorting.h"
#include "settings.h"

static const int cMinPtsForDensity = 20000;

uiDataPointSetCrossPlotter::Setup uiDataPointSetCrossPlotWin::defsetup_;

uiDataPointSetCrossPlotWin::uiDataPointSetCrossPlotWin( uiDataPointSet& uidps )
    : uiMainWin(&uidps,BufferString(uidps.pointSet().name()," Cross-plot"),
	    			    2,false)
    , uidps_(uidps)
    , plotter_(*new uiDataPointSetCrossPlotter(this,uidps,defsetup_))
    , disptb_(*new uiToolBar(this,"Crossplot display toolbar"))
    , seltb_(*new uiToolBar(this,"Crossplot selection toolbar"))
    , maniptb_(*new uiToolBar(this,"Crossplot manipulation toolbar",
			      uiToolBar::Left))
    , colortb_(*new uiToolBar(this,"DensityPlot Colorbar",uiToolBar::Top,true))
    , grpfld_(0)
    , selsettdlg_(0)
    , propdlg_(0)
    , wantnormalplot_(false)
{
    windowClosed.notify( mCB(this,uiDataPointSetCrossPlotWin,closeNotif) );

    Settings& setts = Settings::common();
    if ( !setts.get(sKeyMinDPPts(),minptsfordensity_) )
	minptsfordensity_ = cMinPtsForDensity;
    const int nrpts = plotter_.y2_.axis_ ? uidps.pointSet().nrActive()*2
					 : uidps.pointSet().nrActive();
    const float perc = (float)( 100/(1 + nrpts/minptsfordensity_) );
    uiGroup* dispgrp = new uiGroup( &disptb_, "Display grp" );
    
    eachfld_ = new uiSpinBox( dispgrp, 2, "Percentage" );
    eachfld_->setValue( perc );
    eachfld_->setInterval( StepInterval<float>((float)0,(float)100,0.10) );
    eachfld_->valueChanged.notify(
	    		mCB(this,uiDataPointSetCrossPlotWin,eachChg) );
    plotter_.plotperc_ = perc;
    
    uiLabel* eachlabel = new uiLabel( dispgrp, "% points displayed", eachfld_ );
    disptb_.addObject( dispgrp->attachObj() );
    
    densityplottbid_ = disptb_.addButton( "densityplot.png","Show density plot",
	    	  mCB(this,uiDataPointSetCrossPlotWin,setDensityPlot), true );

    showy2tbid_ = disptb_.addButton( "showy2.png", "Toggle show Y2",
	    	  mCB(this,uiDataPointSetCrossPlotWin,showY2), true );
    disptb_.turnOn( showy2tbid_, false );

    coltabfld_ = new uiColorTable( &colortb_,
	    			   ColTab::Sequence("Rainbow"), false);
    coltabfld_->setEnabManage( false );
    coltabfld_->seqChanged.notify(
	    mCB(this,uiDataPointSetCrossPlotWin,colTabChanged) );
    coltabfld_->scaleChanged.notify(
	    mCB(this,uiDataPointSetCrossPlotWin,colTabChanged) );
    colortb_.addObject( coltabfld_->attachObj() );
    plotter_.setColTab( coltabfld_->colTabSeq() );
    plotter_.setCTMapper( coltabfld_->colTabMapperSetup() );
    plotter_.initDraw();
    plotter_.drawContent();
    
    uiGroup* selgrp = new uiGroup( &seltb_, "Each grp" );
    selfld_ = new uiComboBox( selgrp, "Selection Option" );
    selfld_->addItem( "Select only Y1" );
    selfld_->addItem( "Select only Y2" );
    selfld_->addItem( "Select both" );
    selfld_->selectionChanged.notify( mCB(this,uiDataPointSetCrossPlotWin,
					  selOption) );
    selfld_->setSensitive( false );
    seltb_.addObject( selgrp->attachObj() );

    setselecttbid_ = seltb_.addButton( "altview.png", "Set selectable",
	    	  mCB(this,uiDataPointSetCrossPlotWin,setSelectable), true );
    seltb_.turnOn( setselecttbid_, true );

    if ( uidps_.displayMgr() )
    {
	BufferString fnm, tooltip;
	uidps_.displayMgr()->getIconInfo( fnm, tooltip );
	showselptswstbid_ = seltb_.addButton( fnm, tooltip,
		      mCB(this,uiDataPointSetCrossPlotWin,showPtsInWorkSpace) );
    }
    
    seltb_.turnOn( setselecttbid_, false );

    selmodechgtbid_ = seltb_.addButton( "rectangleselect.png", "Selection mode",
	   mCB(this,uiDataPointSetCrossPlotWin,setSelectionMode) );
    seltb_.turnOn( selmodechgtbid_, plotter_.isRubberBandingOn() );

    clearseltbid_ = seltb_.addButton( "clearselection.png",
			"Remove all selections",
			mCB(this,uiDataPointSetCrossPlotWin,removeSelections) );
    
    seldeltbid_ = seltb_.addButton( "trashcan.png", "Delete all selected",
	    mCB(this,uiDataPointSetCrossPlotWin,deleteSelections) );

    seltabletbid_ = seltb_.addButton( "seltable.png",
	    "Show selections in table",
	    mCB(this,uiDataPointSetCrossPlotWin,showTableSel) ); 

    selsettingstbid_ = seltb_.addButton( "settings.png", "Selection settings",
	    mCB(this,uiDataPointSetCrossPlotWin,setSelectionDomain) );

    maniptb_.addObject( plotter_.getSaveImageButton(&maniptb_) );

    maniptb_.addButton( "xplotprop.png", "Properties",
			mCB(this,uiDataPointSetCrossPlotWin,editProps) );
    maniptb_.addButton( "prdfs.png", "Create Probabily Density Function",
			mCB(this,uiDataPointSetCrossPlotWin,exportPDF) );
    maniptb_.addButton( "overlayattr.png", "Select Overlay Attribute",
			mCB(this,uiDataPointSetCrossPlotWin,overlayAttrCB) );

    const int nrgrps = uidps_.groupNames().size();
    if ( nrgrps > 1 )
    {
	uiPopupMenu* mnu = new uiPopupMenu( &maniptb_, "View Menu" );
	multicolcodtbid_ = maniptb_.addButton( "colorbar.png",
		"Turn on multicolor coding",
		mCB(this,uiDataPointSetCrossPlotWin,setMultiColorCB), true );
	uiMenuItem* itm = new uiMenuItem( "Change color",
		mCB(this,uiDataPointSetCrossPlotWin,changeColCB) );
	mnu->insertItem( itm, 0 );
	maniptb_.setButtonMenu( multicolcodtbid_, mnu );

	grpfld_ = new uiComboBox( dispgrp, "Group selection" );
	BufferString txt( nrgrps == 2 ? "Both " : "All " );
	txt += uidps_.groupType(); txt += "s";
	grpfld_->addItem( txt );
	TypeSet<Color> ctseqs;
	for ( int idx=0; idx<uidps_.groupNames().size(); idx++ )
	{
	    grpfld_->addItem( uidps_.groupNames().get(idx) );
	    Color coly1, coly2;
	    for ( int idy=0; idy<2; idy++ )
	    {
		Color& col = idy==0 ? coly1 : coly2;
		do 
		{ col = getRandomColor(); }
		while ( !ctseqs.addIfNew(col) );
	    }

	    plotter_.y1grpColors().add( coly1 );
	    plotter_.y2grpColors().add( coly2 );
	}

	grpfld_->attach( rightOf, eachfld_ );
	grpfld_->setCurrentItem( 0 );
	grpfld_->selectionChanged.notify(
			    mCB(this,uiDataPointSetCrossPlotWin,grpChg) );
    }

    setSelectable( 0 );
    plotter_.drawTypeChanged.notify(
	    mCB(this,uiDataPointSetCrossPlotWin,drawTypeChangedCB) );
    plotter_.coltabRgChanged.notify(
	    mCB(this,uiDataPointSetCrossPlotWin,coltabRgChangedCB) );
    plotter_.setPrefWidth( 700 );
    plotter_.setPrefHeight( 500 );
}


void uiDataPointSetCrossPlotWin::setPercDisp( float perc )
{
    eachfld_->setValue( perc );
    plotter_.plotperc_ = perc;
    plotter_.getRandRowids();
}


void uiDataPointSetCrossPlotWin::setDensityPlot( CallBacker* cb )
{
    const bool ison = disptb_.isOn( densityplottbid_ );
    disptb_.setToolTip( densityplottbid_, ison ? "Show normal plot"
	    				       : "Show density plot" );
    disptb_.setPixmap( densityplottbid_,ison ? "xplot.png" : "densityplot.png");
    eachfld_->setSensitive( !ison );
    if ( ison && plotter_.isY2Shown() )
	uiMSG().message( "Y2 cannot be displayed in density plot" );

    ison ?  eachfld_->setValue( 100 ) : eachfld_->setValue( plotter_.plotperc_);
    eachfld_->setSensitive( !ison );
    plotter_.setDensityPlot( ison, disptb_.isOn(showy2tbid_) );
    disptb_.setSensitive( showy2tbid_, !ison );
    setSelComboSensitive( !ison );
    plotter_.drawContent();
}


void uiDataPointSetCrossPlotWin::drawTypeChangedCB( CallBacker* cb )
{
    mCBCapsuleUnpack( bool, yn , cb );
    colortb_.display( yn );
}


void uiDataPointSetCrossPlotWin::coltabRgChangedCB( CallBacker* cb )
{
    mCBCapsuleUnpack( Interval<float>, range , cb );
    coltabfld_->setInterval( range );
}


void uiDataPointSetCrossPlotWin::colTabChanged( CallBacker* )
{
    plotter_.setColTab( coltabfld_->colTabSeq() );
    ColTab::MapperSetup mapsetup = coltabfld_->colTabMapperSetup();
    mapsetup.maxpts_ = 20000;
    plotter_.setCTMapper( coltabfld_->colTabMapperSetup() );
    plotter_.drawContent();
}


void uiDataPointSetCrossPlotWin::removeSelections( CallBacker* )
{
    if ( !plotter_.selAreaSize() ) return;

    plotter_.removeSelections();
    plotter_.drawContent();
}


void uiDataPointSetCrossPlotWin::showTableSel( CallBacker* )
{
    if ( plotter_.isADensityPlot() )
    {
	if ( !plotter_.selAreaSize() ) return;

	MouseCursorChanger cursorlock( MouseCursor::Wait );
	Array2D<float>* data =
	    new Array2DImpl<float>( plotter_.arrArea().width() + 1,
				    plotter_.arrArea().height() +1 );
	data->setAll( (float)0 );

	plotter_.setTRMsg( "Showing selected points in table" );
	plotter_.calculateDensity( data, true );
    }

    uidps_.notifySelectedCell();
}


void uiDataPointSetCrossPlotWin::deleteSelections( CallBacker* )
{
    plotter_.deleteSelections();
    plotter_.removeSelections();
}


void uiDataPointSetCrossPlotWin::closeNotif( CallBacker* )
{
    defsetup_ = plotter_.setup();
    plotter_.plotperc_ = mUdf(int); // Make sure eachChg knows we are closing
}


void uiDataPointSetCrossPlotWin::setSelectionMode( CallBacker* )
{
    plotter_.setRectSelection( !plotter_.isRectSelection() );
    seltb_.setPixmap( selmodechgtbid_,
	   	       plotter_.isRectSelection() ? "rectangleselect.png"
			      			  : "polygonselect.png" );
    plotter_.setDragMode( plotter_.isRectSelection() ?
	    			uiGraphicsView::RubberBandDrag :
	    			uiGraphicsView::NoDrag );
    plotter_.scene().setMouseEventActive( true );
}


class uiSelColorDlg : public uiDialog
{
public:

uiSelColorDlg( uiParent* p, const BufferStringSet& names,
	       TypeSet<Color>& y1cols, TypeSet<Color>& y2cols, bool isy2shwn )
    : uiDialog( p, uiDialog::Setup("Select Color for Y1 & Y2","","111.0.6") )
    , names_( names )
    , y1cols_( y1cols )
    , y2cols_( y2cols )
    , isy2shown_( isy2shwn )
{
    tbl_ = new uiTable( this, uiTable::Setup(names.size(),isy2shwn ? 2 : 1),"");
    tbl_->leftClicked.notify( mCB(this,uiSelColorDlg,changeColCB) );
    tbl_->setRowLabels( names );
    BufferStringSet collabel;
    collabel.add( "Y1" );
    if ( isy2shwn ) collabel.add( "Y2" );

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

    Color newcol = tbl_->getColor( rc );
    if ( selectColor(newcol,this,"Marker color") )
    {
	rc.col == 0 ? y1cols_[rc.row] = newcol : y2cols_[rc.row] = newcol;
	tbl_->setColor( rc, newcol );
    }
}

bool acceptOk( CallBacker* )
{
    for ( int idx=0; idx<names_.size(); idx++ )
    {
	y1cols_[idx] = tbl_->getColor( RowCol(idx,0) );
	if ( isy2shown_ ) y2cols_[idx] = tbl_->getColor( RowCol(idx,1) );
    }

    return true;
}
    uiTable*			tbl_;

    TypeSet<Color>&		y1cols_;
    TypeSet<Color>&		y2cols_;
    BufferStringSet		names_;
    bool			isy2shown_;
};


class uiSetSelGrpTab : public uiDlgGroup
{
public:
uiSetSelGrpTab( uiTabStackDlg* p, uiDataPointSetCrossPlotter& plotter )
    : uiDlgGroup( p->tabParent(), "Manage selection group" )
    , selgrps_(plotter.selectionGrps())
    , plotter_(plotter)
    , curselgrp_(0)
    , selGrpChanged(this)
    , selGrpRemoved(this)
{
    uiTable::Setup su( selgrps_.size(), 2 );
    su.rowdesc("Selection Group").selmode(uiTable::Single);
    tbl_ = new uiTable( this, su, "Selection Groups" );
    tbl_->leftClicked.notify( mCB(this,uiSetSelGrpTab,changeColCB) );
    tbl_->rowInserted.notify( mCB(this,uiSetSelGrpTab,addSelGrp) );
    tbl_->valueChanged.notify( mCB(this,uiSetSelGrpTab,changeSelGrbNm) );
    tbl_->selectionChanged.notify( mCB(this,uiSetSelGrpTab,setCurSelGrp) );
    tbl_->setColumnLabel( 0, "Name" );
    tbl_->setColumnLabel( 1, "Color" );
    for ( int idx=0; idx<selgrps_.size(); idx++ )
    {
	tbl_->setText( RowCol(idx,0), selgrps_[idx]->name() );
	tbl_->setColor( RowCol(idx,1), selgrps_[idx]->col_ );
    }

    curselgrp_ = tbl_->currentRow() < 0 ? 0 : tbl_->currentRow();
    uiPushButton* addgrpbut = new uiPushButton( this, "Add group", 
	    mCB(this,uiSetSelGrpTab,addSelGrp), true );
    addgrpbut->attach( alignedBelow, tbl_ );
    
    uiPushButton* remgrpbut = new uiPushButton( this, "Remove group", 
	    mCB(this,uiSetSelGrpTab,remSelGrp), true );
    remgrpbut->attach( rightTo, addgrpbut );
    
    uiPushButton* expgrpbut = new uiPushButton( this, "Export groups", 
	    mCB(this,uiSetSelGrpTab,exportSelectionGrps), true );
    expgrpbut->attach( rightTo, remgrpbut );
    
    uiPushButton* impgrpbut = new uiPushButton( this, "Import groups", 
	    mCB(this,uiSetSelGrpTab,importSelectionGrps), true );
    impgrpbut->attach( rightTo, expgrpbut );
}


void setCurSelGrp( CallBacker* )
{
    if ( tbl_->currentRow() < 0 ) return;

    curselgrp_ = tbl_->currentRow();
    selGrpChanged.trigger();
}


void changeSelGrbNm( CallBacker* )
{
    if ( tbl_->currentRow() < 0 || !selgrps_.validIdx(tbl_->currentRow()) )
	return;

    for ( int idx=0; idx<tbl_->nrRows(); idx++ )
    {
	SelectionGrp* selgrp = selgrps_[ idx ];
	selgrp->setName( tbl_->text(RowCol(idx,0)) );
	selgrp->col_ = tbl_->getColor( RowCol(idx,1) );
    }

    selGrpChanged.trigger();
}


void addSelGrp( CallBacker* cb )
{
    tbl_->insertRows( tbl_->nrRows(), 1 );
    RowCol newcell = RowCol( tbl_->nrRows()-1, 1 );
    tbl_->setColor( RowCol(newcell.row,1), getRandomColor() );
    BufferString selgrpnm( "No " );
    selgrpnm += tbl_->nrRows();
    tbl_->setText( RowCol(newcell.row,0), selgrpnm );
    selgrps_ +=
	new SelectionGrp( selgrpnm, tbl_->getColor(RowCol(newcell.row,1)) );
    curselgrp_ = tbl_->currentRow();
    selGrpChanged.trigger();
}

void importSelectionGrps( CallBacker* )
{
    uiReadSelGrp dlg( this, plotter_ );
    NotifyStopper ns( tbl_->valueChanged );
    if ( dlg.go() )
    {
	while ( tbl_->nrRows() )
	    tbl_->removeRow(0);

	tbl_->setNrRows( selgrps_.size() );
	for ( int idx=0; idx<selgrps_.size(); idx++ )
	{
	    BufferString temp(selgrps_[idx]->name());
	    tbl_->setText( RowCol(idx,0), temp.buf() );
	    tbl_->setColor( RowCol(idx,1), selgrps_[idx]->col_ );
	}
    }
}


void exportSelectionGrps( CallBacker* )
{
    const bool showboth =
	plotter_.isY2Selectable() && plotter_.isY1Selectable();
    uiExpSelectionArea::Setup setup( plotter_.axisHandler(0)->name().buf(), 
				     plotter_.axisHandler(1)->name().buf(),
				     plotter_.axisHandler(2)
				     ? plotter_.axisHandler(2)->name().buf():0);
    uiExpSelectionArea dlg( this, plotter_.selectionGrps(), setup );
    dlg.go();
}


void remSelGrp( CallBacker* )
{
    if ( tbl_->nrRows() <= 1 ) return;

    selgrps_.remove( tbl_->currentRow() );
    tbl_->removeRow( tbl_->currentRow() );
    curselgrp_ = tbl_->currentRow();
    selGrpRemoved.trigger();
}


void changeColCB( CallBacker* )
{
    if ( tbl_->currentRow() < 0 ) return;

    RowCol rc = tbl_->notifiedCell();
    if ( !rc.col ) return;

    Color newcol = tbl_->getColor( rc );
    if ( selectColor(newcol,this,"Marker color") )
    {
	selgrps_[rc.row]->col_ = newcol;
	tbl_->setColor( rc, newcol );
    }
}

    int							curselgrp_;
    uiTable*						tbl_;
    uiDataPointSetCrossPlotter&				plotter_;
    ObjectSet<SelectionGrp>&				selgrps_;

    Notifier<uiSetSelGrpTab>				selGrpChanged;
    Notifier<uiSetSelGrpTab>				selGrpRemoved;
};

class uiSetSelDomainTab : public uiDlgGroup
{
public:

struct DataColInfo
{
    			DataColInfo( const BufferStringSet& colnames,
				     const TypeSet<int>& colids )
			    : colnms_(colnames), colids_(colids) {}

    BufferStringSet	colnms_;
    TypeSet<int>	colids_;
};

uiSetSelDomainTab( uiTabStackDlg* p , DataColInfo* info,
		   const BufferString& mathobjstr, const TypeSet<int>& colids,
       		   bool isdensityplot )
    : uiDlgGroup( p->tabParent(), "Refine Selection" )
    , mathobj_(0)
    , datainfo_(info)
    , dcolids_(colids)
{
    uiLabel* label =
	new uiLabel( this, "Ranges (e.g. 0>x0 && x0>1.5 && -6125<x1)" );

    inpfld_ = new uiGenInput( this, "Enter Ranges" );
    inpfld_->setElemSzPol( uiObject::WideMax );
    inpfld_->updateRequested.notify( mCB(this,uiSetSelDomainTab,parsePush) );
    label->attach( leftAlignedAbove, inpfld_ ); 

    setbut_ = new uiPushButton( this, "Set", true );
    setbut_->activated.notify( mCB(this,uiSetSelDomainTab,parsePush) );
    setbut_->attach( rightTo, inpfld_ );

    vartable_ = new uiTable( this,uiTable::Setup().rowdesc("X")
					.minrowhgt(1.5) .maxrowhgt(2)
					.mincolwdt(3*uiObject::baseFldSize())
					.maxcolwdt(3.5*uiObject::baseFldSize())
					.defrowlbl("") .fillcol(true)
					.fillrow(true) .defrowstartidx(0),
					"Variable X attribute table" );
    const char* xcollbls[] = { "Select input for", 0 };
    vartable_->setColumnLabels( xcollbls );
    vartable_->setNrRows( 2 );
    vartable_->setStretch( 2, 0 );
    vartable_->setRowResizeMode( uiTable::Fixed );
    vartable_->setColumnResizeMode( uiTable::Fixed );
    vartable_->attach( alignedBelow, inpfld_ );
    vartable_->display( false );
    if ( !mathobjstr.isEmpty() )
    {
	inpfld_->setText( mathobjstr );
	parsePush(0);
    }
}


~uiSetSelDomainTab()
{
    delete datainfo_;
}


void parsePush( CallBacker* )
{
    mathexprstring_ = inpfld_->text();
    MathExpressionParser mep( mathexprstring_ );
    mathobj_ = mep.parse();
    if ( !mathobj_ )
    {
	if ( mep.errMsg() ) uiMSG().error( mep.errMsg() );
	dcolids_.erase();
	vartable_->display( false );
	return;
    }

    updateDisplay();
}


int cColIds( int dcolid )
{ return dcolid + 3; }


void updateDisplay()
{
    const int nrvars = mathobj_->nrVariables();
    vartable_->setNrRows( nrvars );
    for ( int idx=0; idx<nrvars; idx++ )
    {
	uiComboBox* varsel = new uiComboBox( 0, datainfo_->colnms_, "Variable");
	if ( !dcolids_.isEmpty() && dcolids_.validIdx(idx) )
	    varsel->setCurrentItem( cColIds(dcolids_[idx]) );
	vartable_->setRowLabel( idx, mathobj_->uniqueVarName(idx) );
	vartable_->setCellObject( RowCol(idx,0), varsel );
    }

    vartable_->display( true );
}


bool acceptOK()
{
    dcolids_.erase();
    if ( !mathobj_ )
	return true;

    int nrvars = mathobj_->nrVariables();
    for ( int idx=0; idx<nrvars; idx++ )
    {
	uiObject* obj = vartable_->getCellObject( RowCol(idx,0) );
	mDynamicCastGet( uiComboBox*, box, obj );
	if ( !box )
	    continue;

	dcolids_ += datainfo_->colids_[box->currentItem()];
    }

    PtrMan<MathExpression> testexpr = mathobj_->clone();
    nrvars = testexpr->nrVariables();
    for ( int idx=0; idx<nrvars; idx++ )
	testexpr->setVariableValue( idx, 100 );

    if ( !mIsZero(testexpr->getValue(),mDefEps) &&
	 !mIsZero(testexpr->getValue()-1,mDefEps) )
    {
	uiMSG().error( "Equation should return true or false" );
	return false;
    }

    return true;
}
    
    BufferString	mathexprstring_;
    DataColInfo*	datainfo_;
    MathExpression*	mathobj_;
    TypeSet<int>	dcolids_;
    
    MathExpression*	mathObject()		{ return mathobj_; }
    
    uiGenInput*		inpfld_;
    uiPushButton*	setbut_;
    uiTable*		vartable_;
};


class uiSelectionSettDlg : public uiTabStackDlg
{
public:
uiSelectionSettDlg( uiDataPointSetCrossPlotter& p,
		    const BufferStringSet& colnames )
    : uiTabStackDlg( p.parent(), uiDialog::Setup("Selection Settings",0,
						 "111.0.4")
				    .savebutton(!p.isADensityPlot())
				    .savetext("Select on Ok").modal(false) )
    , plotter_( p )
{
    TypeSet<int> colids;
    const DataPointSet& dps = plotter_.dps();

    uiDataPointSet::DColID dcid=-dps.nrFixedCols()+1;
    for ( ; dcid<dps.nrCols(); dcid++ )
	colids += dcid;

    uiSetSelDomainTab::DataColInfo* datainfo =
	new uiSetSelDomainTab::DataColInfo( colnames, colids );
    refseltab_ = new uiSetSelDomainTab( this, datainfo, plotter_.mathObjStr(),
				       	plotter_.modifiedColIds(),
				        plotter_.isADensityPlot() );
    addGroup( refseltab_ );

    selgrptab_ = new uiSetSelGrpTab( this, plotter_ );
    addGroup( selgrptab_ );
    selgrptab_->selGrpChanged.notify(
	    mCB(this,uiSelectionSettDlg,selGrpChangedCB) );
    selgrptab_->selGrpRemoved.notify(
	    mCB(this,uiSelectionSettDlg,selGrpRemovedCB) );
}


void selGrpChangedCB( CallBacker* )
{
    plotter_.setCurSelGrp(
	    selgrptab_->curselgrp_ < 0 ? 0 : selgrptab_->curselgrp_ );
}


void selGrpRemovedCB( CallBacker* )
{
    plotter_.setCurSelGrp(
	    selgrptab_->curselgrp_ < 0 ? 0 : selgrptab_->curselgrp_ );
    plotter_.reDrawSelections();
}


bool acceptOK( CallBacker* )
{
    if ( refseltab_->acceptOK() )
    {
	plotter_.setMathObj( refseltab_->mathObject() );
	plotter_.setMathObjStr( refseltab_->mathexprstring_ );
	plotter_.setModifiedColIds( refseltab_->dcolids_ );
	if ( !plotter_.isADensityPlot() && saveButtonChecked() )
	{
	    plotter_.removeSelections( true );
	    if ( !refseltab_->mathObject() )
		return false;

	    SelectionArea sa( uiRect(0,0,plotter_.width(),plotter_.height()) );
	    plotter_.selectionGrps()[ plotter_.curSelGrp() ]->addSelection( sa);
	    plotter_.reDrawSelections();
	}
    }

    plotter_.reDrawSelections();

    return true;
}

    uiDataPointSetCrossPlotter& 	plotter_;
    uiSetSelDomainTab*			refseltab_;
    uiSetSelGrpTab*			selgrptab_;
};


uiDataPointSetCrossPlotWin::~uiDataPointSetCrossPlotWin()
{
    delete selsettdlg_;
    delete propdlg_;
}


void uiDataPointSetCrossPlotWin::setSelectionDomain( CallBacker* )
{
    BufferStringSet colnames;
    const DataPointSet& dps = plotter_.dps();
    uiDataPointSet::DColID dcid=-dps.nrFixedCols()+1;
    for ( ; dcid<dps.nrCols(); dcid++ )
	colnames.add( uidps_.userName(dcid) );

    if ( !selsettdlg_ )
	selsettdlg_ = new uiSelectionSettDlg( plotter_, colnames );

    selsettdlg_->go();
}


void uiDataPointSetCrossPlotWin::setSelectable( CallBacker* cb )
{
    const bool isoff = !seltb_.isOn(setselecttbid_ );
    seltb_.setPixmap( setselecttbid_, !isoff ? "altview.png" : "altpick.png");
    plotter_.setSceneSelectable( isoff );
    selfld_->setSensitive( plotter_.isY2Shown() ? isoff : false );
    seltb_.setSensitive( selmodechgtbid_, isoff );
    seltb_.setSensitive( selsettingstbid_, isoff );
    seltb_.setSensitive( seltabletbid_, isoff );
    seltb_.setSensitive( seldeltbid_, isoff );
    seltb_.setSensitive( clearseltbid_, isoff );
    plotter_.setDragMode(  plotter_.isSceneSelectable()
	    			? ( plotter_.isRectSelection()
				    ? uiGraphicsView::RubberBandDrag
				    : uiGraphicsView::NoDrag )
				: uiGraphicsView::ScrollHandDrag );
    disptb_.turnOn( showy2tbid_, plotter_.isY2Shown() );
    plotter_.scene().setMouseEventActive( true );
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
    if ( plotter_.isADensityPlot() )
    {
	if ( !plotter_.selAreaSize() )
	{
	    uidps_.selPtsToBeShown.trigger();
	    return;
	}

	Array2D<float>* data =
	    new Array2DImpl<float>( plotter_.arrArea().width() + 1,
				    plotter_.arrArea().height() +1 );
	data->setAll( (float)0 );

	plotter_.setTRMsg( "Showing selected points in workspace" );
	plotter_.calculateDensity( data, true );
    }

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
	    uiMSG().message( "Density Plot will always display all data" );
	    eachfld_->setValue( (float)100 );
	}

	return;
    }
    
    const float prevperc = plotter_.plotperc_;
    if ( mIsEqual(plotter_.plotperc_,newperc,mDefEps) )
	return;

    plotter_.plotperc_ = newperc;

    const int estpts =
	mNINT( ((!disptb_.isOn(showy2tbid_) && !plotter_.y2_.axis_)
	    ? uidps_.pointSet().nrActive()
	    : uidps_.pointSet().nrActive()*2)*plotter_.plotperc_/(float)100 );
    
    Settings& setts = Settings::common();
    if ( !setts.get(sKeyMinDPPts(),minptsfordensity_) )
	minptsfordensity_ = cMinPtsForDensity;

    if ( estpts > minptsfordensity_ && !plotter_.isADensityPlot() )
    {
	BufferString msg( "It is a time consuming process & might freeze the ",
			  "application due to large dataset size. " );
	msg += "Do you want to go ahead with normal plot or have a ";
	msg += "density plot which would be faster ?";
	msg += "\nNote: Density plot cannot have Y2 axis";
	const int res =
	    uiMSG().askGoOnAfter( msg, "Cancel", "Normal Plot", "Density Plot");
	if ( !res )
	    wantnormalplot_ = true;
	else if ( res ==1 )
	    wantnormalplot_ = false;
	else
	{
	    eachfld_->setValue( prevperc );
	    eachfld_->setSensitive( true );
	    return;
	}

	disptb_.turnOn( densityplottbid_, !wantnormalplot_ );
	const bool ison = disptb_.isOn( densityplottbid_ );
	disptb_.setToolTip( densityplottbid_, ison ? "Show normal plot"
						   : "Show density plot" );
	eachfld_->setSensitive( !ison );
	if ( ison )
	    eachfld_->setValue( 100 );

	eachfld_->setSensitive( !ison );
	plotter_.setDensityPlot( !wantnormalplot_, disptb_.isOn(showy2tbid_) );
    }

    plotter_.getRandRowids();
    plotter_.drawContent( false );
}


void uiDataPointSetCrossPlotWin::grpChg( CallBacker* )
{
    if ( !grpfld_ ) return;

    plotter_.curgrp_ = grpfld_->currentItem();
    plotter_.dataChanged();
}


void uiDataPointSetCrossPlotWin::setSelComboSensitive( bool yn )
{
    if ( !yn )
    {
	selfld_->setCurrentItem( 0 );
	plotter_.setSelectable( true, false );
    }

    bool status = plotter_.isSceneSelectable() &&
		  !plotter_.isADensityPlot() ? yn : false;
    selfld_->setSensitive( status );
}


void uiDataPointSetCrossPlotWin::exportPDF( CallBacker* )
{
    BufferStringSet colnames;
    const DataPointSet& dps = plotter_.dps();
    uiDataPointSet::DColID dcid=-dps.nrFixedCols()+1;
    for ( ; dcid<dps.nrCols(); dcid++ )
	colnames.add( uidps_.userName(dcid) );

    uiCreateDPSPDF dlg( this, plotter_, colnames );
    dlg.go();
}


void uiDataPointSetCrossPlotWin::overlayAttrCB( CallBacker* )
{
    uiDPSOverlayPropDlg dlg( this, plotter_ );
    dlg.go();
}


void uiDataPointSetCrossPlotWin::editProps( CallBacker* )
{
    if ( !propdlg_ )
	propdlg_ = new uiDataPointSetCrossPlotterPropDlg( &plotter_ );

    propdlg_->go();
}


void uiDataPointSetCrossPlotWin::setGrpColors()
{
    if ( !plotter_.axisData(1).axis_ || (plotter_.isY2Shown() &&
	 !plotter_.isMultiColMode() && !plotter_.axisData(2).axis_) )
	return;

    for ( int idx=0; idx<uidps_.groupNames().size(); idx++ )
    {
	Color coly1 = plotter_.isMultiColMode()
	    ? plotter_.y1grpColors()[idx]
	    : plotter_.axisData(1).axis_->setup().style_.color_;
	Color coly2 = !plotter_.isY2Shown()
	    ? coly1 : plotter_.isMultiColMode()
	    ? plotter_.y2grpColors()[idx]
	    : plotter_.axisData(2).axis_->setup().style_.color_;
	ColTab::Sequence ctseq;
	ctseq.setColor( 0, coly1.r(), coly1.g(), coly1.b() );
	ctseq.setColor( 1, coly2.r(), coly2.g(), coly2.b() ); 
	ctseq.setNrSegments( 2 );
	ioPixmap pixmap( ctseq, 20, 20, true );
	if ( grpfld_ )
	    grpfld_->setPixmap( pixmap, idx+1 );
    }
}


void uiDataPointSetCrossPlotWin::setMultiColorCB( CallBacker* )
{
    const bool ison = maniptb_.isOn( multicolcodtbid_ );
    plotter_.setMultiColMode( ison );

    setGrpColors();
    BufferString tooltip ( ison ? "Turn off color coding"
	    		        : "Turn on color coding" );
    maniptb_.setToolTip( multicolcodtbid_, tooltip );
    plotter_.drawContent( false );
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
    }
    else
	uiMSG().message( "Cannot change color in this mode." );
}

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki Maitra
 Date:          August 2009
 RCS:           $Id: 
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uidatapointsetcrossplotwin.cc";

#include "uidatapointsetcrossplotwin.h"

#include "uibutton.h"
#include "uidpscrossplotpropdlg.h"
#include "uidatapointsetcrossplot.h"
#include "uidatapointset.h"
#include "uidialog.h"
#include "uilineedit.h"
#include "uicolortable.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uigraphicsscene.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uispinbox.h"
#include "uistatusbar.h"
#include "uitable.h"
#include "uitoolbar.h"

#include "arraynd.h"
#include "arrayndimpl.h"
#include "coltabsequence.h"
#include "datapointset.h"
#include "datainpspec.h"
#include "envvars.h"
#include "linear.h"
#include "pixmap.h"
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
    , showSelPts(this)
{
    windowClosed.notify( mCB(this,uiDataPointSetCrossPlotWin,closeNotif) );

    const int nrpts = uidps.pointSet().size();
    const int eachrow = 1 + nrpts / cMinPtsForDensity;
    uiGroup* dispgrp = new uiGroup( &disptb_, "Display grp" );
    
    eachfld_ = new uiSpinBox( dispgrp, 0, "Each" );
    eachfld_->setValue( eachrow );
    eachfld_->setInterval( 1, mUdf(int), 1 );
    eachfld_->valueChanged.notify(
	    		mCB(this,uiDataPointSetCrossPlotWin,eachChg) );
    eachfld_->setSensitive( uidps_.pointSet().nrActive() < cMinPtsForDensity );
    plotter_.eachrow_ = eachrow;
    
    uiLabel* eachlabel = new uiLabel( dispgrp, "Plot each", eachfld_ );
    disptb_.addObject( dispgrp->attachObj() );
    
    showy2tbid_ = disptb_.addButton( "showy2.png",
	    	  mCB(this,uiDataPointSetCrossPlotWin,showY2),
		  "Toggle show Y2", true );
    disptb_.turnOn( showy2tbid_, false );

    coltabfld_ = new uiColorTable( dispgrp, ColTab::Sequence(0), false );
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

    setselecttbid_ = seltb_.addButton( "altview.png",
	    	  mCB(this,uiDataPointSetCrossPlotWin,setSelectable),
		  "Set selectable", true );
    seltb_.turnOn( setselecttbid_, true );


    showselptswstbid_ = seltb_.addButton( "picks.png",
	    	  mCB(this,uiDataPointSetCrossPlotWin,showPtsInWorkSpace),
		  "Show selected points in workSpace", false );
    seltb_.turnOn( setselecttbid_, true );

    selmodechgtbid_ = seltb_.addButton( "rectangleselect.png",
	   mCB(this,uiDataPointSetCrossPlotWin,setSelectionMode) ,
	   "Selection mode" ); 
    seltb_.turnOn( selmodechgtbid_, plotter_.isRubberBandingOn() );

    clearseltbid_ = seltb_.addButton( "clearselection.png",
	    mCB(this,uiDataPointSetCrossPlotWin,removeSelections), 
	    "Remove all selections" );
    
    seldeltbid_ = seltb_.addButton( "trashcan.png",
	    mCB(this,uiDataPointSetCrossPlotWin,deleteSelections), 
	    "Delete all selections" );

    seltabletbid_ = seltb_.addButton( "seltable.png",
	    mCB(this,uiDataPointSetCrossPlotWin,showTableSel), 
	    "Show selections in table" );

    selsettingstbid_ = seltb_.addButton( "settings.png",
	    mCB(this,uiDataPointSetCrossPlotWin,setSelectionDomain), 
	    "Selection settings" );

    maniptb_.addObject( plotter_.getSaveImageButton() );

    maniptb_.addButton( "xplotprop.png",
	    mCB(this,uiDataPointSetCrossPlotWin,editProps),"Properties",false );

    const int nrgrps = uidps_.groupNames().size();
    if ( nrgrps > 1 )
    {
	grpfld_ = new uiComboBox( dispgrp, "Group selection" );
	BufferString txt( nrgrps == 2 ? "Both " : "All " );
	txt += uidps_.groupType(); txt += "s";
	grpfld_->addItem( txt );
	for ( int idx=0; idx<uidps_.groupNames().size(); idx++ )
	    grpfld_->addItem( uidps_.groupNames().get(idx) );
	grpfld_->attach( rightOf, eachfld_ );
	grpfld_->setCurrentItem( 0 );
	grpfld_->selectionChanged.notify(
			    mCB(this,uiDataPointSetCrossPlotWin,grpChg) );
    }

    seltb_.turnOn( setselecttbid_, false );
    setSelectable( 0 );
    plotter_.drawTypeChanged.notify(
	    mCB(this,uiDataPointSetCrossPlotWin,drawTypeChangedCB) );
    plotter_.coltabRgChanged.notify(
	    mCB(this,uiDataPointSetCrossPlotWin,coltabRgChangedCB) );
    plotter_.setPrefWidth( 700 );
    plotter_.setPrefHeight( 500 );
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
    plotter_.removeSelections();
    plotter_.drawContent();
}


void uiDataPointSetCrossPlotWin::showTableSel( CallBacker* )
{
    if ( plotter_.isADensityPlot() )
    {
	MouseCursorChanger cursorlock( MouseCursor::Wait );
	Array2D<float>* data =
	    new Array2DImpl<float>( plotter_.arrArea().width() + 1,
				    plotter_.arrArea().height() +1 );
	data->setAll( (float)0 );

	plotter_.setTRMsg( "Showing selected points in table" );
	plotter_.calcDensity( data, true );
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
    plotter_.eachrow_ = mUdf(int); // Make sure eachChg knows we are closing
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


class uiSetSelDomainDlg : public uiDialog
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


uiSetSelDomainDlg( uiParent* p , DataColInfo& info,
		   const BufferString& mathobjstr, const TypeSet<int>& colids,
       		   bool isdensityplot )
    : uiDialog( p, uiDialog::Setup("Refine Selection","Set selectable domain",
				    mNoHelpID).savebutton(!isdensityplot)
	    				      .savetext("Select on Ok") )
    , mathobj_(0)
    , datainfo_(info)
    , dcolids_(colids)
{
    uiLabel* label =
	new uiLabel( this, "Ranges (e.g. 0>x0 && x0>1.5 && -6125<x1)" );
    uiLabel* linelabel = new uiLabel( this, "Enter Ranges" );
    linelabel->attach( leftAlignedBelow, label );
    inpfld_ = new uiLineEdit( this, "Enter Ranges" );
    inpfld_->setHSzPol( uiObject::WideMax );
    inpfld_->attach( rightTo, linelabel );

    setbut_ = new uiPushButton( this, "Set", true );
    setbut_->activated.notify( mCB(this,uiSetSelDomainDlg,parsePush) );
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
	inpfld_->setvalue_( mathobjstr );
	parsePush(0);
    }
}


void parsePush( CallBacker* )
{
    mathexprstring_ = inpfld_->getvalue_();
    MathExpressionParser mep( mathexprstring_ );
    mathobj_ = mep.parse();
    if ( !mathobj_ )
    {
	if ( *mep.errMsg() ) uiMSG().error( mep.errMsg() );
	dcolids_.erase();
	vartable_->display( false );
	return;
    }
    updateDisplay();
}


void updateDisplay()
{
    const int nrvars = mathobj_->nrVariables();
    vartable_->setNrRows( nrvars );
    for ( int idx=0; idx<nrvars; idx++ )
    {
	uiComboBox* varsel = new uiComboBox( 0, datainfo_.colnms_, "Variable");
	if ( !dcolids_.isEmpty() )
	    varsel->setCurrentItem( dcolids_[idx] );
	vartable_->setCellObject( RowCol(idx,0), varsel );
    }
    vartable_->display( true );
}


bool acceptOK( CallBacker* )
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
	dcolids_ += datainfo_.colids_[box->currentItem()];
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
    DataColInfo&	datainfo_;
    MathExpression*	mathobj_;
    TypeSet<int>	dcolids_;
    
    MathExpression*	mathObject()		{ return mathobj_; }
    
    uiLineEdit*		inpfld_;
    uiPushButton*	setbut_;
    uiTable*		vartable_;
};


void uiDataPointSetCrossPlotWin::setSelectionDomain( CallBacker* )
{
    BufferStringSet colnames;
    TypeSet<int> colids;
    const DataPointSet& dps = plotter_.dps();
    for ( uiDataPointSet::DColID dcid=0; dcid<dps.nrCols(); dcid++ )
    {
	colids += dcid;
	colnames.add( dps.colName(dcid) ); 
    }
    uiSetSelDomainDlg::DataColInfo datainfo( colnames, colids );
    uiSetSelDomainDlg dlg( this, datainfo, plotter_.mathObjStr(),
	    		   plotter_.modifiedColIds(),
			   plotter_.isADensityPlot() );
    if ( dlg.go() )
    {
	plotter_.setMathObj( dlg.mathObject() );
	plotter_.setMathObjStr( dlg.mathexprstring_ );
	plotter_.setModifiedColIds( dlg.dcolids_ );
	if ( !plotter_.isADensityPlot() && dlg.saveButtonChecked() )
	{
	    plotter_.removeSelections();
	    if ( !dlg.mathObject() )
		return;
	    ObjectSet<uiDataPointSetCrossPlotter::SelectionArea> selareas;
	    selareas += new uiDataPointSetCrossPlotter::SelectionArea(
		    new uiRect(0,0,plotter_.width(),plotter_.height()) );
	    plotter_.setSelectionAreas( selareas );
	}
    }
}


void uiDataPointSetCrossPlotWin::setSelectable( CallBacker* cb )
{
    const bool isoff = !seltb_.isOn(setselecttbid_ );
    seltb_.setPixmap( setselecttbid_, !isoff ? "altview.png" : "altpick.png");
    plotter_.setSceneSelectable( isoff );
    selfld_->setSensitive( plotter_.isY2Shown() ? isoff : false );
    seltb_.setSensitive( selmodechgtbid_, isoff );
    seltb_.setSensitive( showselptswstbid_, isoff );
    seltb_.setSensitive( selsettingstbid_, isoff );
    seltb_.setSensitive( seltabletbid_, isoff );
    seltb_.setSensitive( seldeltbid_, isoff );
    seltb_.setSensitive( clearseltbid_, isoff );
    plotter_.setDragMode(  plotter_.isSceneSelectable()
	    			? ( plotter_.isRectSelection()
				    ? uiGraphicsView::RubberBandDrag
				    : uiGraphicsView::NoDrag )
				: uiGraphicsView::ScrollHandDrag );
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
	Array2D<float>* data =
	    new Array2DImpl<float>( plotter_.arrArea().width() + 1,
				    plotter_.arrArea().height() +1 );
	data->setAll( (float)0 );

	plotter_.setTRMsg( "Showing selected points in workspace" );
	plotter_.calcDensity( data, true );
    }
    showSelPts.trigger();
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
    MouseCursorChanger cursorchanger(MouseCursor::Wait);
    if ( mIsUdf(plotter_.eachrow_) ) return; // window is closing

    int neweachrow = eachfld_->getValue();
    if ( neweachrow < 1 ) neweachrow = 1;
    if ( plotter_.eachrow_ == neweachrow )
	return;
    plotter_.eachrow_ = neweachrow;
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
    seltb_.setSensitive( showy2tbid_, status );
    selfld_->setSensitive( status );
}


void uiDataPointSetCrossPlotWin::editProps( CallBacker* )
{
    uiDataPointSetCrossPlotterPropDlg dlg( &plotter_ );
    dlg.go();
}

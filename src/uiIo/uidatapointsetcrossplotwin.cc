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
#include "uicolor.h"
#include "uicolortable.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uigeninputdlg.h"
#include "uigraphicsscene.h"
#include "uilabel.h"
#include "uimenu.h"
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
#include "dpsdispmgr.h"
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
    
    densityplottbid_ = disptb_.addButton( "densityplot.png",
	    	  mCB(this,uiDataPointSetCrossPlotWin,setDensityPlot),
		  "Show density plot", true );

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


    if ( uidps_.displayMgr() )
    {
	BufferString fnm, tooltip;
	uidps_.displayMgr()->getIconInfo( fnm, tooltip );
	showselptswstbid_ = seltb_.addButton( fnm,
		      mCB(this,uiDataPointSetCrossPlotWin,showPtsInWorkSpace),
		      tooltip, false );
    }
    
    seltb_.turnOn( setselecttbid_, false );

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
	uiPopupMenu* mnu = new uiPopupMenu( &maniptb_, "View Menu" );
	multicolcodtbid_ = maniptb_.addButton( "colorbar.png",
		mCB(this,uiDataPointSetCrossPlotWin,setMultiColorCB),
		"Turn on multicolor coding",true );
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
	       TypeSet<Color>& y1cols, TypeSet<Color>& y2cols )
    : uiDialog( p, uiDialog::Setup("Select Color for Y1 & Y2","","") )
    , names_( names )
    , y1cols_( y1cols )
    , y2cols_( y2cols )
{
    tbl_ = new uiTable( this, uiTable::Setup(names.size(),2), "" );
    tbl_->leftClicked.notify( mCB(this,uiSelColorDlg,changeColCB) );
    tbl_->setRowLabels( names );
    BufferStringSet collabel;
    collabel.add( "Y1" );
    collabel.add( "Y2" );
    tbl_->setColumnLabels( collabel );
    for ( int idx=0; idx<names.size(); idx++ )
    {
	tbl_->setColor( RowCol(idx,0), y1cols[idx] );
	tbl_->setColor( RowCol(idx,1), y2cols[idx] );
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
	y2cols_[idx] = tbl_->getColor( RowCol(idx,1) );
    }
    return true;
}
    uiTable*			tbl_;

    TypeSet<Color>&		y1cols_;
    TypeSet<Color>&		y2cols_;
    BufferStringSet		names_;
};


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
	Array2D<float>* data =
	    new Array2DImpl<float>( plotter_.arrArea().width() + 1,
				    plotter_.arrArea().height() +1 );
	data->setAll( (float)0 );

	plotter_.setTRMsg( "Showing selected points in workspace" );
	plotter_.calcDensity( data, true );
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
    MouseCursorChanger cursorchanger(MouseCursor::Wait);
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


void uiDataPointSetCrossPlotWin::editProps( CallBacker* )
{
    uiDataPointSetCrossPlotterPropDlg dlg( &plotter_ );
    dlg.go();
}


void uiDataPointSetCrossPlotWin::setGrpColors()
{
    for ( int idx=0; idx<uidps_.groupNames().size(); idx++ )
    {
	Color coly1 = plotter_.isMultiColMode()
	    ? plotter_.y1grpColors()[idx]
	    : plotter_.axisData(1).axis_->setup().style_.color_;
	Color coly2 = plotter_.isMultiColMode()
	    ? plotter_.y2grpColors()[idx]
	    : plotter_.axisData(2).axis_->setup().style_.color_;
	ColTab::Sequence ctseq;
	ctseq.setColor( 0, coly1.r(), coly1.g(), coly1.b() );
	ctseq.setColor( 1, coly2.r(), coly2.g(), coly2.b() ); 
	ctseq.setNrSegments( 2 );
	ioPixmap pixmap( ctseq, 20, 20, true );
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
			      plotter_.y2grpColors() );
	if ( !seldlg.go() )
	{
	    return;
	}
	
	setGrpColors();
	plotter_.drawContent( false );
    }
    else
	uiMSG().message( "Cannot change color in this mode." );
}

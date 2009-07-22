/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          August 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwindowfuncseldlg.cc,v 1.14 2009-07-22 16:01:43 cvsbert Exp $";


#include "uiwindowfuncseldlg.h"

#include "uiaxishandler.h"
#include "uicanvas.h"
#include "uigeninput.h"
#include "uigraphicsview.h"
#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
#include "uilistbox.h"
#include "uiworld2ui.h"
#include "iodraw.h"
#include "randcolor.h"
#include "windowfunction.h"

#define mTransHeight    250
#define mTransWidth     500

uiWindowFuncSelDlg::uiWindowFuncSelDlg( uiParent* p, const char* windowname,
	float variable )
    : uiDialog( p, uiDialog::Setup("Window/Taper display",0,mNoHelpID) )
    , transform_(new uiWorld2Ui())
    , variable_(variable)
    , polyitemgrp_(0)
    , borderrectitem_(0)
{
    setCtrlStyle( LeaveOnly );
    
    taperlistfld_ = new uiListBox( this );
    taperlistfld_->attach( topBorder, 0 );
    taperlistfld_->setMultiSelect();
    
    BufferStringSet funcnames = WinFuncs().getNames();
    for ( int idx=0; idx<funcnames.size(); idx++ )
    {
	winfunc_ += WinFuncs().create( funcnames[idx]->buf() );
	linesetcolor_ += Color::stdDrawColor( idx );
	taperlistfld_->addItem( funcnames[idx]->buf(), linesetcolor_[idx] );
    }

    taperlistfld_->setCurrentItem( windowname );

    varinpfld_ = new uiGenInput( this, "Taper Length (%)", FloatInpSpec() );
    varinpfld_->attach( rightAlignedBelow, taperlistfld_ );
    !strcmp(windowname,"CosTaper") ? varinpfld_->display( true ) : 
				     varinpfld_->display( false );
    varinpfld_->setValue( variable_ * 100 );
    varinpfld_->valuechanged.notify(
	    mCB(this,uiWindowFuncSelDlg,taperSelChg) );

    view_ = new uiGraphicsView( this, "Window/Taper view" );
    view_->setPrefHeight( mTransHeight );
    view_->setPrefWidth( mTransWidth );
    view_->setStretch(0,0);
    view_->attach( rightOf, taperlistfld_ );
    taperlistfld_->selectionChanged.notify( 
	mCB(this,uiWindowFuncSelDlg,taperSelChg) );
    transform_->set( uiRect( 35, 5, mTransWidth-5 , mTransHeight-25 ),
		     uiWorldRect(-1.2,1,1.2,0) );

    uiBorder border(10,10,10,10);
    
    uiAxisHandler::Setup asu( uiRect::Bottom, view_->width(), view_->height() );
    asu.style( LineStyle::None );
    asu.border_ = border;

    xax_ = new uiAxisHandler( &view_->scene(), asu );

    asu.side( uiRect::Left );
    yax_ = new uiAxisHandler( &view_->scene(), asu );
    xax_->setBegin( yax_ ); yax_->setBegin( xax_ );
    float annotstart = -1;
    xax_->setRange( StepInterval<float>(-1.2,1.2,0.25), &annotstart );
    yax_->setRange( StepInterval<float>(0,1,0.25),0 );

    setCurrentWindowFunc( windowname, variable );
    taperSelChg(0);
}


uiWindowFuncSelDlg::~uiWindowFuncSelDlg()
{
    delete transform_;
    pointlistset_.erase();
    linesetcolor_.erase();
}


void uiWindowFuncSelDlg::createLine(const WindowFunction& winfunc,bool replace)
{
    TypeSet<uiPoint> pointlist;

    uiRect borderrect( xax_->pixBefore(), 10, mTransWidth - 10,
	    	       mTransHeight - yax_->pixBefore() );
    transform_->resetUiRect( borderrect );
    const StepInterval<float> xrg( -1.2, 1.2, 0.01 );
    for ( int idx=0; idx<xrg.nrSteps(); idx++ )
    {
	const float x = xrg.atIndex( idx );
	const float y = winfunc.getValue( x );
	pointlist += uiPoint( transform_->transform(uiWorldPoint(x,y)) );
    }

    if ( replace )
    {
	for ( int idx=0; idx<winfunc_.size(); idx++ )
	{
	    if ( !strcmp(winfunc_[idx]->name(),winfunc.name()) )
	    {
		pointlistset_[ idx ] = pointlist;
	    }
	}
    }
    else
	pointlistset_ += pointlist;

}


void uiWindowFuncSelDlg::draw()
{
    xax_->setNewDevSize( mTransWidth, mTransHeight );
    yax_->setNewDevSize( mTransHeight , mTransWidth );
    xax_->plotAxis();
    yax_->plotAxis();
    uiRect borderrect( xax_->pixBefore(), 5, mTransWidth - 5,
	    	       mTransHeight - yax_->pixBefore() );
    if ( !borderrectitem_ )
	borderrectitem_ = view_->scene().addRect(
		borderrect.left(), borderrect.top(), borderrect.width(),
		borderrect.height() );
    else
	borderrectitem_->setRect( borderrect.left(), borderrect.top(),
				  borderrect.width(), borderrect.height() );
    borderrectitem_->setPenStyle( LineStyle() );
    const int selsz = pointlistset_.size();
    TypeSet<int> selecteditems;
    if ( !polyitemgrp_ )
    {
	polyitemgrp_ = new uiGraphicsItemGroup();
	view_->scene().addItemGrp( polyitemgrp_ );
    }
    else
	polyitemgrp_->removeAll( true );
    taperlistfld_->getSelectedItems( selecteditems );
    for ( int idx=0; idx<pointlistset_.size(); idx++ )
    {
	uiPolyLineItem* polyitem = new uiPolyLineItem();
	polyitem->setPolyLine( pointlistset_[idx] );
	LineStyle ls;
	ls.width_ = 2;
	if ( selsz == 1 )
	{
	    Color col( linesetcolor_[taperlistfld_->currentItem()] );
	    ls.color_ = col;
	    polyitem->setPenStyle( ls );
	}
	else
	{
	    Color col( linesetcolor_[selecteditems[idx]] );
	    ls.color_ = col;
	    polyitem->setPenStyle( ls );
	}
	polyitemgrp_->add( polyitem );
    }
}


void uiWindowFuncSelDlg::setCurrentWindowFunc( const char* nm, float variable )
{
    variable_ = variable;
    taperlistfld_->setCurrentItem( nm );
    taperSelChg(0);
}


bool uiWindowFuncSelDlg::getCurrentWindowName( BufferString& windowname )
{
    if ( taperlistfld_->nrSelected() == 1 )
    {
	windowname = taperlistfld_->textOfItem( taperlistfld_->currentItem() );
	return true;
    }
    else
	return false;
}


float uiWindowFuncSelDlg::getVariable()
{
    BufferString winname;
    getCurrentWindowName( winname );
    for ( int idx=0; idx<winfunc_.size(); idx++ )
    {
	if( !strcmp(winname.buf(),winfunc_[idx]->name()) )
	{
	    if( winfunc_[idx]->hasVariable() ) 
		return variable_;
	}
    }
    return mUdf(float);
}


void uiWindowFuncSelDlg::taperSelChg( CallBacker* )
{
    pointlistset_.erase();
    bool isvartappresent = false;
    for ( int idx=0; idx<taperlistfld_->size(); idx++ )
    {
	if ( !taperlistfld_->isSelected(idx) )
	    continue;

	if ( winfunc_[ idx ]->hasVariable() )
	{
	    isvartappresent = true;
	    float prevvariable = variable_;
	    mIsUdf(variable_) ? variable_ = 0.05 : 
		 		variable_ = varinpfld_->getfValue(0)/100;
	    variable_ > 1 ? varinpfld_->setValue( prevvariable * 100 ) :
			      varinpfld_->setValue( variable_ * 100 );
	    if ( variable_ > 1 )
	       	variable_ = prevvariable; 
	    winfunc_[ idx ]->setVariable( 1.0 - variable_ );
	    varinpfld_->setValue( variable_ * 100 );
	}
	createLine( *winfunc_[idx] );
    }

    isvartappresent ? varinpfld_->display( true ) :
		      varinpfld_->display( false );
    //canvas_->update();
    draw();
}


void uiWindowFuncSelDlg::setVariable( float variable )
{
    float prevvariable = variable_;
    variable_ = variable;
    variable_ > 1 ? varinpfld_->setValue( prevvariable * 100 ) :
       		    varinpfld_->setValue( variable_ * 100 );
    if ( variable_ > 1 )
	variable_ = prevvariable; 
    taperSelChg(0);
}

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki Maitra
 Date:          August 2007
 RCS:           $Id: uiwindowfuncseldlg.cc,v 1.9 2008-09-09 10:52:11 cvsbert Exp $
________________________________________________________________________

-*/


#include "uiwindowfuncseldlg.h"

#include "uiaxishandler.h"
#include "uicanvas.h"
#include "uigeninput.h"
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

    canvas_ = new uiCanvas( this, Color::White, "Window/Taper canvas" );
    canvas_->setPrefHeight( mTransHeight );
    canvas_->setPrefWidth( mTransWidth );
    canvas_->setStretch(0,0);
    canvas_->attach( rightOf, taperlistfld_ );
    canvas_->postDraw.notify( mCB(this,uiWindowFuncSelDlg,reDraw) );
    taperlistfld_->selectionChanged.notify( 
	mCB(this,uiWindowFuncSelDlg,taperSelChg) );
    transform_->set( uiRect( 35, 5, mTransWidth-5 , mTransHeight-25 ),
		     uiWorldRect(-1.2,1,1.2,0) );

    uiBorder border(5,5,5,5);
    
    uiAxisHandler::Setup asu( uiRect::Bottom );
    asu.style( LineStyle::None );
    asu.border_ = border;

    xax_ = new uiAxisHandler( canvas_->drawTool(), asu );

    asu.side( uiRect::Left );
    yax_ = new uiAxisHandler( canvas_->drawTool(), asu );
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


void uiWindowFuncSelDlg::reDraw( CallBacker* )
{
    xax_->newDevSize();
    yax_->newDevSize();
    xax_->plotAxis();
    yax_->plotAxis();
    const int selsz = pointlistset_.size();
    TypeSet<int> selecteditems;
    taperlistfld_->getSelectedItems( selecteditems );
    ioDrawTool& dt = canvas_->drawTool();
    for ( int idx=0; idx<pointlistset_.size(); idx++ )
    {
	if ( selsz == 1 )
	{
	    Color col( linesetcolor_[taperlistfld_->currentItem()] );
	    dt.setPenColor( col );
	}
	else
	{
	    Color col( linesetcolor_[selecteditems[idx]] );
	    dt.setPenColor( col );
	}
	dt.setPenWidth( 2 );
	dt.drawPolyline( pointlistset_[idx] );
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
    canvas_->update();
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
/*

bool uiWindowFuncSelDlg::rejectOK( CallBacker* )
{
    varinpfld_->valuechanged.remove( mCB(this,uiWindowFuncSelDlg,taperSelChg) );
    taperlistfld_->selectionChanged.remove( mCB(this,uiWindowFuncSelDlg,
						taperSelChg) );
    return true;
}*/

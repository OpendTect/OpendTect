/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          August 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwindowfuncseldlg.cc,v 1.15 2009-09-11 13:17:28 cvsbruno Exp $";


#include "uiwindowfuncseldlg.h"

#include "uiaxishandler.h"
#include "uicanvas.h"
#include "uigeninput.h"
#include "uigraphicsview.h"
#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
#include "uigroup.h"
#include "uilistbox.h"
#include "uiworld2ui.h"
#include "iodraw.h"
#include "randcolor.h"
#include "windowfunction.h"

#define mTransHeight    250
#define mTransWidth     500

uiFuncSelDraw::uiFuncSelDraw( uiParent* p, const char* funcnm )
    : uiGroup(p)
    , transform_(new uiWorld2Ui())
    , polyitemgrp_(0)
    , borderrectitem_(0)
    , funclistselChged(this)
{
    funclistfld_ = new uiListBox( this );
    funclistfld_->attach( topBorder, 0 );
    funclistfld_->setMultiSelect();
    
    view_ = new uiGraphicsView( this, "Function view" );
    view_->setPrefHeight( mTransHeight );
    view_->setPrefWidth( mTransWidth );
    view_->setStretch(0,0);
    view_->attach( rightOf, funclistfld_ );
    
    funclistfld_->selectionChanged.notify( mCB(this,uiFuncSelDraw,funcSelChg) );

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

}


uiFuncSelDraw::~uiFuncSelDraw()
{
    delete transform_;
    pointlistset_.erase();
    linesetcolor_.erase();
}


void uiFuncSelDraw::addToList( const char* fcname )
{
    const int curidx = funclistfld_->size();
    linesetcolor_ += Color::stdDrawColor( curidx );
    funclistfld_->addItem( fcname, linesetcolor_[curidx] );
}


void uiFuncSelDraw::addToListAsCurrent( const char* fcname )
{
    funclistfld_->setCurrentItem( fcname );
}


void uiFuncSelDraw::createLine( const FloatMathFunction& mathfunc )
{
    TypeSet<uiPoint> pointlist;

    uiRect borderrect( xax_->pixBefore(), 10, mTransWidth - 10,
	    	       mTransHeight - yax_->pixBefore() );
    transform_->resetUiRect( borderrect );
    const StepInterval<float> xrg( -1.2, 1.2, 0.01 );
    for ( int idx=0; idx<xrg.nrSteps(); idx++ )
    {
	const float x = xrg.atIndex( idx );
	const float y = mathfunc.getValue( x );
	pointlist += uiPoint( transform_->transform(uiWorldPoint(x,y)) );
    }

    pointlistset_ += pointlist;
}


void uiFuncSelDraw::draw()
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
    funclistfld_->getSelectedItems( selecteditems );
    for ( int idx=0; idx<pointlistset_.size(); idx++ )
    {
	uiPolyLineItem* polyitem = new uiPolyLineItem();
	polyitem->setPolyLine( pointlistset_[idx] );
	LineStyle ls;
	ls.width_ = 2;
	ls.color_ = linesetcolor_[ selsz==1 ? funclistfld_->currentItem() 
					    : selecteditems[idx]];
	polyitem->setPenStyle( ls );
	polyitemgrp_->add( polyitem );
    }
}


void uiFuncSelDraw::funcSelChg( CallBacker* )
{
    funclistselChged.trigger();
    pointlistset_.erase();
    for ( int idx=0; idx<funclistfld_->size(); idx++ )
    {
	if ( !funclistfld_->isSelected(idx) )
	    continue;
	createLine( *mathfunc_[idx] );
    }

    draw();
}


void uiFuncSelDraw::addFunction( FloatMathFunction* mfunc )
{ 
    if (!mfunc ) return; 
    mathfunc_ += mfunc; 
}


int uiFuncSelDraw::getCurrentListSize() const
{ 
    return funclistfld_->size();
}


const char* uiFuncSelDraw::getCurrentListName() const
{
    if ( funclistfld_->nrSelected() == 1 )
	return funclistfld_->textOfItem( funclistfld_->currentItem() );
    return 0;
}



uiWindowFuncSelDlg::uiWindowFuncSelDlg( uiParent* p, const char* windowname,
					float variable )
    : uiDialog( p, uiDialog::Setup("Window/Taper display",0,mNoHelpID) )
    , variable_(variable)
    , funcdrawer_(0)			 
{
    setCtrlStyle( LeaveOnly );
   
    funcdrawer_ = new uiFuncSelDraw( this, windowname );
    BufferStringSet funcnames = WinFuncs().getNames();
    for ( int idx=0; idx<funcnames.size(); idx++ )
    {
	winfunc_ += WinFuncs().create( funcnames[idx]->buf() );
	funcdrawer_->addToList( funcnames[idx]->buf());
	funcdrawer_->addFunction( winfunc_[idx]  );
    }

    funcdrawer_->funclistselChged.notify(mCB(this,uiWindowFuncSelDlg,funcSelChg));

    varinpfld_ = new uiGenInput( this, "Taper Length (%)", FloatInpSpec() );
    varinpfld_->attach( leftAlignedBelow, funcdrawer_ );
    varinpfld_->setValue( variable_ * 100 );
    varinpfld_->valuechanged.notify( mCB(this,uiWindowFuncSelDlg,funcSelChg) );

    setCurrentWindowFunc( windowname, variable );
    funcSelChg(0);
}


uiWindowFuncSelDlg::~uiWindowFuncSelDlg()
{
    delete funcdrawer_;
}


float uiWindowFuncSelDlg::getVariable()
{
    WindowFunction* wf = getCurrentWindowFunc();
    if ( wf && wf->hasVariable() )
	return variable_;
    return -1;
}


void uiWindowFuncSelDlg::funcSelChg( CallBacker* )
{
    NotifyStopper nsf( funcdrawer_->funclistselChged );
    bool isvartappresent = false;

    WindowFunction* wf = getCurrentWindowFunc();
    if ( wf && wf->hasVariable() )
    {
	isvartappresent = true;
	float prevvariable = variable_;
	variable_ = mIsUdf(variable_) ? 0.05 : varinpfld_->getfValue(0)/100;
	if ( variable_ > 1 || mIsUdf(variable_) )
	    variable_ = prevvariable; 
	wf->setVariable( 1.0 - variable_ );
	varinpfld_->setValue( variable_ * 100 );
    }

    varinpfld_->display( isvartappresent );
    funcdrawer_->funcSelChg(0);
    //canvas_->update();
}


void uiWindowFuncSelDlg::setVariable( float variable )
{
    float prevvariable = variable_;
    variable_ = variable;
    variable_ > 1 ? varinpfld_->setValue( prevvariable * 100 ) :
       		    varinpfld_->setValue( variable_ * 100 );
    if ( variable_ > 1 )
	variable_ = prevvariable; 
    funcSelChg(0);
}


void uiWindowFuncSelDlg::setCurrentWindowFunc( const char* nm, float variable )
{
    variable_ = variable;
    funcdrawer_->addToListAsCurrent( nm );
    funcSelChg(0);
}


WindowFunction* uiWindowFuncSelDlg::getCurrentWindowFunc()
{
    BufferString winname = getCurrentWindowName();
    BufferStringSet funcnames = WinFuncs().getNames();
    if ( winname )
	return winfunc_[funcnames.indexOf(winname)];
    return 0;

}


const char* uiWindowFuncSelDlg::getCurrentWindowName() const
{
    return funcdrawer_->getCurrentListName();
}


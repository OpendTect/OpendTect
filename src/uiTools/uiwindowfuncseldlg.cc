/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          August 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwindowfuncseldlg.cc,v 1.21 2009-10-05 15:12:03 cvsbruno Exp $";


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

uiFuncSelDraw::uiFuncSelDraw( uiParent* p, const Setup& su )
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
		     uiWorldRect( su.xaxrg_.start, su.yaxrg_.stop, 
				  su.xaxrg_.stop, su.yaxrg_.start ) );

    uiBorder border(10,10,10,10);
    
    uiAxisHandler::Setup asu( uiRect::Bottom, view_->width(), view_->height() );
    asu.style( LineStyle::None );

    asu.maxnumberdigitsprecision_ = 2;
    asu.maxnumberdigitsprecision_ = 2;
    asu.epsaroundzero_ = 1e-3;
    asu.epsaroundzero_ = 1e-3;

    asu.border_ = border;

    xax_ = new uiAxisHandler( &view_->scene(), asu );
    float annotstart = -1;
    xax_->setRange( su.xaxrg_, &annotstart );

    asu.side( uiRect::Left );
    yax_ = new uiAxisHandler( &view_->scene(), asu );
    yax_->setRange( su.yaxrg_, 0 );

    xax_->setBegin( yax_ ); 		yax_->setBegin( xax_ );
    xax_->setBounds( su.xaxrg_ ); 	yax_->setBounds( su.yaxrg_ );
}


uiFuncSelDraw::~uiFuncSelDraw()
{
    delete transform_;
    pointlistset_.erase();
    linesetcolor_.erase();
}


void uiFuncSelDraw::addToList( const char* fcname, bool withcolor )
{
    const int curidx = funclistfld_->size();
    linesetcolor_ += withcolor ? Color::stdDrawColor( curidx ) : Color::Black();
    funclistfld_->addItem( fcname, linesetcolor_[curidx] );
}


int uiFuncSelDraw::getListSize() const
{
    return funclistfld_->size();
}


int uiFuncSelDraw::getNrSel() const
{
    return funclistfld_->nrSelected();
}


void uiFuncSelDraw::setAsCurrent( const char* fcname )
{
    funclistfld_->setCurrentItem( fcname );
}


int uiFuncSelDraw::removeLastItem()
{
    const int curidx = funclistfld_->size()-1;
    funclistfld_->removeItem( curidx );
    mathfunc_.remove( curidx );
    return curidx;
}


void uiFuncSelDraw::removeItem( int idx )
{
    funclistfld_->removeItem( idx );
    mathfunc_.remove( idx );
}


void uiFuncSelDraw::createLine( const FloatMathFunction* mathfunc )
{
    if ( !mathfunc ) return;
    TypeSet<uiPoint> pointlist;

    uiRect borderrect( xax_->pixBefore(), 10, mTransWidth - 10,
	    	       mTransHeight - yax_->pixBefore() );
    transform_->resetUiRect( borderrect );

    StepInterval<float> xrg( xax_->range() );
    xrg.step = 0.01;
    for ( int idx=0; idx<xrg.nrSteps(); idx++ )
    {
	const float x = xrg.atIndex( idx );
	const float y = mathfunc->getValue( x );
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
	createLine( mathfunc_[idx] );
    }

    draw();
}


void uiFuncSelDraw::addFunction( FloatMathFunction* mfunc )
{ 
    if (!mfunc ) return; 
    mathfunc_ += mfunc; 
}


void uiFuncSelDraw::getSelectedItems( TypeSet<int>& selitems ) const
{ 
    return funclistfld_->getSelectedItems( selitems );
}


bool uiFuncSelDraw::isSelected( int idx) const
{ 
    return funclistfld_->isSelected(idx);
}


void uiFuncSelDraw::setSelected( int idx ) 
{ 
    funclistfld_->setSelected(idx);
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
  
    uiFuncSelDraw::Setup su; su.name_ = windowname;  
    funcdrawer_ = new uiFuncSelDraw( this, su );
    funcnames_ = WinFuncs().getNames();
    for ( int idx=0; idx<funcnames_.size(); idx++ )
    {
	winfunc_ += WinFuncs().create( funcnames_[idx]->buf() );
	funcdrawer_->addToList( funcnames_[idx]->buf());
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


void uiWindowFuncSelDlg::funcSelChg( CallBacker* )
{
    NotifyStopper nsf( funcdrawer_->funclistselChged );
    bool isvartappresent = false;

    TypeSet<int> selitems; 
    funcdrawer_->getSelectedItems( selitems );
    for ( int idx=0; idx<selitems.size(); idx++ )
    {
	const BufferString& winname = funcnames_[selitems[idx]]->buf();
 	WindowFunction* wf = getWindowFuncByName( winname );
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
    }

    varinpfld_->display( isvartappresent );
    funcdrawer_->funcSelChg(0);
    //canvas_->update();
}


float uiWindowFuncSelDlg::getVariable()
{
    const WindowFunction* wf = getWindowFuncByName( getCurrentWindowName() );
    if ( wf && wf->hasVariable() )
	return variable_;
    return -1;
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
    funcdrawer_->setAsCurrent( nm );
    funcSelChg(0); 
}


WindowFunction* uiWindowFuncSelDlg::getWindowFuncByName( const char* nm )
{
    const int idx = funcnames_.indexOf(nm);
    if ( idx >=0 )
	return winfunc_[funcnames_.indexOf(nm)];
    return 0;
}


const char* uiWindowFuncSelDlg::getCurrentWindowName() const
{
    return funcdrawer_->getCurrentListName();
}


/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          August 2007
________________________________________________________________________

-*/


#include "uiwindowfuncseldlg.h"
#include "uiaxishandler.h"
#include "uigeninput.h"
#include "uigraphicsview.h"
#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
#include "uilistbox.h"
#include "uiworld2ui.h"

#include "arrayndimpl.h"
#include "arrayndalgo.h"
#include "randcolor.h"
#include "scaler.h"
#include "windowfunction.h"


#define mTransHeight    250
#define mTransWidth     500

uiFunctionDrawer::uiFunctionDrawer( uiParent* p, const Setup& su )
    : uiGraphicsView( p, "" )
    , transform_(new uiWorld2Ui())
    , polyitemgrp_(0)
    , borderrectitem_(0)
    , funcrg_(su.funcrg_)
{
    setPrefHeight( mTransHeight );
    setPrefWidth( mTransWidth );
    setStretch( 2, 2 );

    transform_->set( uiRect( 35, 5, mTransWidth-5 , mTransHeight-25 ),
		     uiWorldRect( su.xaxrg_.start, su.yaxrg_.stop,
				  su.xaxrg_.stop, su.yaxrg_.start ) );

    uiAxisHandler::Setup asu( uiRect::Bottom,viewWidth(), viewHeight() );
    asu.style( OD::LineStyle::None );
    asu.maxnrchars_ = 8;
    asu.border_ = uiBorder(10,10,10,10);

    float annotstart = -1;
    xax_ = new uiAxisHandler( &scene(), asu );
    xax_->setRange( su.xaxrg_, &annotstart );

    asu.side( uiRect::Left ); asu.islog_ = false;
    yax_ = new uiAxisHandler( &scene(), asu );
    yax_->setRange( StepInterval<float>(0,1,0.25),0 );

    xax_->setBegin( yax_ );		yax_->setBegin( xax_ );
    xax_->setBounds( su.xaxrg_ );	yax_->setBounds( su.yaxrg_ );
    xax_->setCaption( su.xaxcaption_ ); yax_->setCaption( su.yaxcaption_ );

    reSize.notify( mCB( this, uiFunctionDrawer, draw ) );
}


void uiFunctionDrawer::setUpAxis()
{
    xax_->updateDevSize();
    yax_->updateDevSize();
    xax_->updateScene();
    yax_->updateScene();
}


uiFunctionDrawer::~uiFunctionDrawer()
{
    delete transform_;
    clearFunctions();
}


void uiFunctionDrawer::setFrame()
{
    uiRect borderrect( xax_->getPix( xax_->range().start ),
		       yax_->getPix( yax_->range().stop ),
		       xax_->getPix( xax_->range().stop ),
		       yax_->getPix( yax_->range().start ) );
    if ( !borderrectitem_ )
	borderrectitem_ = scene().addRect(
	    mCast(float,borderrect.left()), mCast(float,borderrect.top()),
	    mCast(float,borderrect.width()), mCast(float,borderrect.height()) );
    else
	borderrectitem_->setRect( borderrect.left(), borderrect.top(),
				  borderrect.width(), borderrect.height() );
    borderrectitem_->setPenStyle( OD::LineStyle() );
    borderrect.setTop( borderrect.top() + 3 );
    transform_->resetUiRect( borderrect );
}


void uiFunctionDrawer::clearFunction( int idx )
{
    delete functions_.removeSingle(idx);
}


void uiFunctionDrawer::draw( CallBacker* )
{
    setUpAxis();
    setFrame();

    if ( !polyitemgrp_ )
    {
	polyitemgrp_ = new uiGraphicsItemGroup();
	scene().addItemGrp( polyitemgrp_ );
    }
    else
	polyitemgrp_->removeAll( true );

    if ( !selitemsidx_.size() && functions_.size() )
	selitemsidx_ += 0;

    for ( int idx=0; idx<selitemsidx_.size(); idx++ )
    {
	const int selidx = selitemsidx_[idx];
	DrawFunction* func =
		functions_.validIdx(selidx) ? functions_[selidx] : 0;
	if ( !func ) return;

	createLine( func );
	uiPolyLineItem* polyitem = new uiPolyLineItem();
	polyitem->setPolyLine( func->pointlist_ );
	OD::LineStyle ls;
	ls.width_ = 2;
	ls.color_ = func->color_;
	polyitem->setPenStyle( ls );
	polyitemgrp_->add( polyitem );
    }
}


void uiFunctionDrawer::createLine( DrawFunction* func )
{
    if ( !func ) return;
    TypeSet<uiPoint>& pointlist = func->pointlist_;
    pointlist.erase();

    LinScaler scaler( funcrg_.start, xax_->range().start,
		      funcrg_.stop, xax_->range().stop );

    StepInterval<float> xrg( funcrg_ );
    xrg.step = 0.0001;
    for ( int idx=0; idx<xrg.nrSteps(); idx++ )
    {
	float x = xrg.atIndex( idx );
	const float y = func->mathfunc_->getValue( x );
	x = (float) ( scaler.scale( x ) );
	pointlist += uiPoint( transform_->transform( uiWorldPoint(x,y) ) );
    }
}



uiFuncSelDraw::uiFuncSelDraw( uiParent* p, const uiFunctionDrawer::Setup& su )
    : uiGroup(p)
    , funclistselChged(this)
{
    funclistfld_ = new uiListBox( this, "Function", OD::ChooseAtLeastOne );
    funclistfld_->attach( topBorder, 0 );
    funclistfld_->setHSzPol( uiObject::MedVar );
    funclistfld_->selectionChanged.notify( mCB(this,uiFuncSelDraw,funcSelChg) );
    funclistfld_->itemChosen.notify( mCB(this,uiFuncSelDraw,funcCheckChg) );

    view_ = new uiFunctionDrawer( this, su );
    view_->attach( rightOf, funclistfld_ );
}


int uiFuncSelDraw::getListSize() const
{
    return funclistfld_->size();
}


int uiFuncSelDraw::getNrSel() const
{
    return funclistfld_->nrChosen();
}


void uiFuncSelDraw::setAsCurrent( const char* fcname )
{
    funclistfld_->setCurrentItem( fcname );
}


int uiFuncSelDraw::removeLastItem()
{
    const int curidx = funclistfld_->size()-1;
    funclistfld_->removeItem( curidx );
    mathfunc_.removeSingle( curidx );
    view_->clearFunction( curidx );
    return curidx;
}


void uiFuncSelDraw::removeItem( int idx )
{
    funclistfld_->removeItem( idx );
    mathfunc_.removeSingle( idx );
    view_->clearFunction( idx );
}


void uiFuncSelDraw::funcCheckChg( CallBacker* cb )
{
    funclistselChged.trigger();

    TypeSet<int> selecteditems;
    funclistfld_->getChosen( selecteditems );

    view_->setSelItems( selecteditems );
    view_->draw( cb );
}


void uiFuncSelDraw::funcSelChg(CallBacker *)
{
    const int nrchecked = funclistfld_->nrChosen();
    if ( nrchecked > 1 ) return;

    NotifyStopper ns1( funclistfld_->selectionChanged );
    funclistfld_->chooseAll( false );
    funclistfld_->setChosen( funclistfld_->currentItem() );
}


void uiFuncSelDraw::addFunction( const char* fcname, FloatMathFunction* mfunc,
				 bool withcolor )
{
    if ( !mfunc ) return;
    mathfunc_ += mfunc;

    const int curidx = funclistfld_->size();
    const Color& col = withcolor ? Color::stdDrawColor( curidx )
				 : Color::Black();
    colors_ += col;
    funclistfld_->addItem( mToUiStringTodo(fcname), col );

    uiFunctionDrawer::DrawFunction* drawfunction =
			new uiFunctionDrawer::DrawFunction( mfunc );
    drawfunction->color_ = colors_[curidx];
    view_->addFunction( drawfunction );
}


void uiFuncSelDraw::getSelectedItems( TypeSet<int>& selitems ) const
{
    funclistfld_->getChosen( selitems );
}


void uiFuncSelDraw::setSelectedItems( const TypeSet<int>& selitems )
{
    funclistfld_->setChosen( selitems );
}


bool uiFuncSelDraw::isSelected( int idx) const
{
    return funclistfld_->isChosen(idx);
}


void uiFuncSelDraw::setSelected( int idx )
{
    funclistfld_->setChosen(idx);
}


const char* uiFuncSelDraw::getCurrentListName() const
{
    if ( funclistfld_->nrChosen() == 1 )
	return funclistfld_->textOfItem( funclistfld_->currentItem() );
    return 0;
}



uiWindowFuncSelDlg::uiWindowFuncSelDlg( uiParent* p, const char* winname,
					float variable )
    : uiDialog( p, uiDialog::Setup(tr("Window/Taper display"),
				   uiStrings::sEmptyString(),mNoHelpKey) )
    , variable_(variable)
    , funcdrawer_(0)
{
    setCtrlStyle( CloseOnly );

    uiFunctionDrawer::Setup su;
    funcdrawer_ = new uiFuncSelDraw( this, su );
    funcnames_ = WINFUNCS().getNames();

    for ( int idx=0; idx<funcnames_.size(); idx++ )
    {
	winfunc_ += WINFUNCS().create( funcnames_[idx]->buf() );
	funcdrawer_->addFunction( funcnames_[idx]->buf(), winfunc_[idx] );
    }

    funcdrawer_->funclistselChged.notify(
	    mCB(this,uiWindowFuncSelDlg,funcSelChg) );

    uiString tapertxt( tr("Taper Length (%)") );
    varinpfld_ = new uiGenInput( this, tapertxt, FloatInpSpec() );
    varinpfld_->attach( leftAlignedBelow, funcdrawer_ );
    varinpfld_->setValue( variable_ * 100 );
    varinpfld_->valuechanged.notify( mCB(this,uiWindowFuncSelDlg,funcSelChg) );

    setCurrentWindowFunc( winname, variable_ );
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
	    variable_ = mIsUdf(variable_) ? 0.05f
					  : varinpfld_->getFValue(0)/100;
	    if ( variable_ > 1 || mIsUdf(variable_) )
		variable_ = prevvariable;
	    wf->setVariable( 1.0f - variable_ );
	    varinpfld_->setValue( variable_ *100 );
	}
    }

    varinpfld_->display( isvartappresent );
    funcdrawer_->funcSelChg(0);
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
    if ( variable_ > 1 )
    {
	varinpfld_->setValue( prevvariable * 100 );
	variable_ = prevvariable;
    }
    else
	varinpfld_->setValue( variable_ * 100 );

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

/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwindowfuncseldlg.h"
#include "uiaxishandler.h"
#include "uifunctiondisplayserver.h"
#include "uigeninput.h"
#include "uigraphicsview.h"
#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
#include "uilistbox.h"
#include "uiworld2ui.h"

#include "scaler.h"
#include "windowfunction.h"


#define mTransHeight    250
#define mTransWidth     500

uiFunctionDrawer::uiFunctionDrawer( uiParent* p, const Setup& su )
    : uiFuncDrawerBase(su)
    , uiGraphicsView( p, "" )
    , transform_(new uiWorld2Ui())
{
    setPrefHeight( su.canvasheight_ );
    setPrefWidth( su.canvaswidth_ );
    setStretch( 2, 2 );

    transform_->set( uiRect( 35, 5, mTransWidth-5 , mTransHeight-25 ),
		     uiWorldRect( su.xaxrg_.start_, su.yaxrg_.stop_,
				  su.xaxrg_.stop_, su.yaxrg_.start_ ) );

    uiAxisHandler::Setup asu( uiRect::Bottom,viewWidth(), viewHeight() );
    asu.maxnrchars_ = 8;
    asu.border_ = uiBorder(10,10,10,10);

    float annotstart = -1;
    auto* xax = new uiAxisHandler( &scene(), asu );
    xax->setRange( su.xaxrg_, &annotstart );

    asu.side( uiRect::Left ); asu.islog_ = false;
    auto* yax = new uiAxisHandler( &scene(), asu );
    yax->setRange( su.yaxrg_ );

    xax->setBegin( yax );
    yax->setBegin( xax );

    xax->setBounds( su.xaxrg_ );
    yax->setBounds( su.yaxrg_ );

    xax->setCaption( su.xaxcaption_ );
    yax->setCaption( su.yaxcaption_ );

    xax_ = xax;
    yax_ = yax;

    mAttachCB( reSize, uiFunctionDrawer::draw );
}


uiFunctionDrawer::~uiFunctionDrawer()
{
    detachAllNotifiers();

    delete transform_;
    delete xax_;
    delete yax_;
}


uiAxisHandler* uiFunctionDrawer::xAxis() const
{
    mDynamicCastGet(uiAxisHandler*, xax, xax_);
    return xax;
}


uiAxisHandler* uiFunctionDrawer::yAxis() const
{
    mDynamicCastGet(uiAxisHandler*, yax, yax_);
    return yax;
}


void uiFunctionDrawer::setUpAxis()
{
    xAxis()->updateDevSize();
    yAxis()->updateDevSize();
    xAxis()->updateScene();
    yAxis()->updateScene();
}


void uiFunctionDrawer::setFrame()
{
    uiAxisHandler* xax = xAxis();
    uiAxisHandler* yax = yAxis();
    uiRect borderrect( xax->getPix( xax->range().start_ ),
		       yax->getPix( yax->range().stop_ ),
		       xax->getPix( xax->range().stop_ ),
		       yax->getPix( yax->range().start_ ) );
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
	ls.width_ = setup().width_;
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

    LinScaler scaler( funcrg_.start_, xax_->range().start_,
		      funcrg_.stop_, xax_->range().stop_ );

    StepInterval<float> xrg( funcrg_ );
    xrg.step_ = 0.0001;
    for ( int idx=0; idx<xrg.nrSteps(); idx++ )
    {
	float x = xrg.atIndex( idx );
	const float y = func->mathfunc_->getValue( x );
	x = (float) ( scaler.scale( x ) );
	pointlist += uiPoint( transform_->transform( uiWorldPoint(x,y) ) );
    }
}



// uiFuncSelDraw
uiFuncSelDraw::uiFuncSelDraw( uiParent* p, const uiFuncDrawerBase::Setup& su )
    : uiGroup(p)
    , funclistselChged(this)
{
    funclistfld_ = new uiListBox( this, "Function", OD:: ChooseZeroOrMore);
    funclistfld_->attach( topBorder, 0 );
    funclistfld_->setHSzPol( uiObject::MedVar );
    mAttachCB( funclistfld_->selectionChanged, uiFuncSelDraw::funcSelChg );
    mAttachCB( funclistfld_->itemChosen, uiFuncSelDraw::funcCheckChg );

    view_ = GetFunctionDisplayServer().createFunctionDrawer( this, su );
    view_->uiobj()->attach( rightOf, funclistfld_ );
}


uiFuncSelDraw::~uiFuncSelDraw()
{
    detachAllNotifiers();
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
    TypeSet<int> selecteditems;
    funclistfld_->getChosen( selecteditems );

    view_->setSelItems( selecteditems );
    view_->draw( cb );

    funclistselChged.trigger();
}


void uiFuncSelDraw::funcSelChg( CallBacker* cb )
{
    const int nrchecked = funclistfld_->nrChosen();
    if ( nrchecked > 0 )
	return;

    NotifyStopper ns1( funclistfld_->selectionChanged );
    const int curitm = funclistfld_->currentItem();
    if ( curitm < 0 )
	return;

    TypeSet<int> selecteditems;
    selecteditems += curitm;
    view_->setSelItems( selecteditems );
    view_->draw( cb );

    funclistselChged.trigger();
}


void uiFuncSelDraw::addFunction( const char* fcname, FloatMathFunction* mfunc,
				 bool withcolor )
{
    if ( !mfunc ) return;
    mathfunc_ += mfunc;

    const int curidx = funclistfld_->size();
    const OD::Color& col = withcolor ? OD::Color::stdDrawColor( curidx )
				 : OD::Color::Black();
    colors_ += col;
    funclistfld_->addItem( toUiString(fcname), col );

    uiFuncDrawerBase::DrawFunction* drawfunction =
			new uiFuncDrawerBase::DrawFunction( mfunc );
    drawfunction->color_ = colors_[curidx];
    view_->addFunction( drawfunction );
}


void uiFuncSelDraw::getSelectedItems( TypeSet<int>& selitems ) const
{
    funclistfld_->getChosen( selitems );
    if ( !selitems.isEmpty() )
	return;

    const int curitm = funclistfld_->currentItem();
    if ( curitm >= 0 )
	selitems += curitm;
}


void uiFuncSelDraw::setSelectedItems( const TypeSet<int>& selitems )
{
    funclistfld_->setChosen( selitems );
}


bool uiFuncSelDraw::isSelected( int idx ) const
{
    return funclistfld_->isChosen(idx);
}


void uiFuncSelDraw::setSelected( int idx )
{
    funclistfld_->setChosen(idx);
}


const char* uiFuncSelDraw::getCurrentListName() const
{
    TypeSet<int> selitems;
    getSelectedItems( selitems );
    return selitems.isEmpty() ? nullptr
		: funclistfld_->textOfItem( selitems.first() );
}



uiWindowFuncSelDlg::uiWindowFuncSelDlg( uiParent* p, const char* winname,
					float variable )
    : uiDialog(p,Setup(tr("Window/Taper display"),mNoHelpKey))
    , variable_(variable)
{
    setCtrlStyle( CloseOnly );

    uiFuncDrawerBase::Setup su;
    su.canvasheight( mTransHeight ).canvaswidth( mTransWidth );
    su.xaxrg( StepInterval<float>(-1.2f,1.2f,0.25f) );
    su.yaxrg( StepInterval<float>(0.f,1.2f,0.25f) );
    su.funcrg( Interval<float>(-1.2, 1.2) );
    funcdrawer_ = new uiFuncSelDraw( this, su );
    funcnames_ = WINFUNCS().getNames();

    for ( int idx=0; idx<funcnames_.size(); idx++ )
    {
	winfunc_ += WINFUNCS().create( funcnames_[idx]->buf() );
	funcdrawer_->addFunction( funcnames_[idx]->buf(), winfunc_[idx] );
    }

    mAttachCB( funcdrawer_->funclistselChged, uiWindowFuncSelDlg::funcSelChg );

    uiString tapertxt( tr("Taper Length (%)") );
    varinpfld_ = new uiGenInput( this, tapertxt, FloatInpSpec() );
    varinpfld_->attach( leftAlignedBelow, funcdrawer_ );
    varinpfld_->setValue( variable_ * 100 );
    mAttachCB( varinpfld_->valueChanged, uiWindowFuncSelDlg::funcSelChg );

    setCurrentWindowFunc( winname, variable_ );
}


uiWindowFuncSelDlg::~uiWindowFuncSelDlg()
{
    detachAllNotifiers();
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

/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uistratdisplay.h"

#include "uiaxishandler.h"
#include "uifont.h"
#include "uigeninput.h"
#include "uigraphicsitemimpl.h"
#include "uigraphicsscene.h"
#include "uimenu.h"
#include "uistratreftree.h"
#include "uitoolbar.h"
#include "uitoolbutton.h"

#include "keyboardevent.h"
#include "scaler.h"


// uiStratDisplay

uiStratDisplay::uiStratDisplay( uiParent* p, uiStratRefTree& uitree )
    : uiGraphicsView(p,"Stratigraphy viewer")
    , drawer_(uiStratDrawer(scene(),data_))
    , uidatawriter_(uiStratDispToTree(uitree))
    , uidatagather_(nullptr)
    , uicontrol_(nullptr)
    , islocked_(false)
    , maxrg_(Interval<float>(0,2e3))
{
    uiCrossHairItem* chitm = new uiCrossHairItem( *this );
    chitm->showLine( OD::Vertical, false );

    uidatagather_ = new uiStratTreeToDisp( data_ );
    mAttachCB( uidatagather_->newtreeRead, uiStratDisplay::reDraw );

    MouseEventHandler& meh = getMouseEventHandler();
    mAttachCB( meh.buttonReleased, uiStratDisplay::usrClickCB );
    mAttachCB( meh.doubleClick, uiStratDisplay::doubleClickCB );
    mAttachCB( meh.movement, uiStratDisplay::mouseMoveCB );
    mAttachCB( reSize, uiStratDisplay::reDraw );

    disableScrollZoom();
    setDragMode( uiGraphicsView::NoDrag );
    setSceneBorder( 2 );
    setPrefWidth( 650 );
    setPrefHeight( 400 );
    createDispParamGrp();
    setRange();
    reDraw( nullptr );
}


uiStratDisplay::~uiStratDisplay()
{
    detachAllNotifiers();
    delete uicontrol_;
    delete uidatagather_;
}


void uiStratDisplay::setTree()
{
    uidatagather_->setTree();
    setRange();
}


void uiStratDisplay::setRange()
{
    if ( data_.nrCols() && data_.nrUnits(0) )
    {
	const StratDispData::Unit& unstart = *data_.getUnit( 0, 0 );
	const StratDispData::Unit& unstop =*data_.getUnit(0,data_.nrUnits(0)-1);
	Interval<float> viewrg( unstart.zrg_.start, unstop.zrg_.stop );
	float wdth = viewrg.width(); wdth /= 10.f;
	if ( wdth <= 0 )
	    wdth = 10.f;
	viewrg.stop += wdth;
	setZRange( viewrg );
    }
    else
	setZRange( maxrg_ );
}


void uiStratDisplay::addControl( uiToolBar* tb )
{
    mDynamicCastGet(uiGraphicsView*,v,const_cast<uiStratDisplay*>(this))
    uiStratViewControl::Setup su( maxrg_ ); su.tb_ = tb;
    uicontrol_ = new uiStratViewControl( *v, su );
    mAttachCB( uicontrol_->rangeChanged, uiStratDisplay::controlRange );
    uicontrol_->setRange( rangefld_->getFInterval() );
}


void uiStratDisplay::controlRange( CallBacker* )
{
    if ( uicontrol_ )
    {
	rangefld_->setValue( uicontrol_->range() );
	rangefld_->setNrDecimals( 2 );
	dispParamChgd(nullptr);
    }
}


void uiStratDisplay::createDispParamGrp()
{
    dispparamgrp_ = new uiGroup( parent(), "display Params Group" );
    dispparamgrp_->attach( centeredBelow, this );
    rangefld_ = new uiGenInput( dispparamgrp_, tr("Display between Ages (Ma)"),
		FloatInpIntervalSpec()
		    .setName(BufferString("range start"),0)
		    .setName(BufferString("range stop"),1) );
    mAttachCB( rangefld_->valuechanged, uiStratDisplay::dispParamChgd );

    viewcolbutton_ = new uiPushButton( dispparamgrp_, tr("Columns"),
				       mCB(this,uiStratDisplay,selCols), false);
    viewcolbutton_->attach( rightOf, rangefld_ );
}


class uiColViewerDlg : public uiDialog
{ mODTextTranslationClass(uiColViewerDlg)
public :
    uiColViewerDlg( uiParent* p, uiStratDrawer& drawer, StratDispData& ad )
	: uiDialog(p,uiDialog::Setup(tr("View Columns"),
				     uiString::emptyString(), mNoHelpKey))
	, drawer_(drawer)
	, data_(ad)
    {
	setCtrlStyle( CloseOnly );

	BufferStringSet colnms;
	for ( int idx=0; idx<data_.nrCols(); idx++ )
	    colnms.add( data_.getCol( idx )->name_ );

	for ( int idx=0; idx<colnms.size(); idx++ )
	{
	    uiCheckBox* box = new uiCheckBox(this, toUiString(colnms.get(idx)));
	    box->setChecked( data_.getCol( idx )->isdisplayed_ );
	    mAttachCB( box->activated, uiColViewerDlg::selChg );
	    colboxflds_ += box;
	    if ( idx ) box->attach( alignedBelow, colboxflds_[idx-1] );
	}
	if ( colnms.size() )
	{
	    allboxfld_ = new uiCheckBox( this, uiStrings::sAll() );
	    allboxfld_->attach( alignedAbove, colboxflds_[0] );
	    mAttachCB( allboxfld_->activated, uiColViewerDlg::selAll );
	}
    }

    ~uiColViewerDlg()
    {
	detachAllNotifiers();
    }

    void selAll( CallBacker* )
    {
	bool allsel = allboxfld_->isChecked();
	for ( int idx=0; idx<colboxflds_.size(); idx++ )
	    colboxflds_[idx]->setChecked( allsel );
    }

    void selChg( CallBacker* cb )
    {
	mDynamicCastGet(uiCheckBox*,box,cb)
	if ( !box ) return;

	for ( int idbox=0; idbox<colboxflds_.size(); idbox++ )
	{
	    NotifyStopper ns( colboxflds_[idbox]->activated );
	    bool ison = false;
	    ison = colboxflds_[idbox]->isChecked();
	    data_.getCol( idbox )->isdisplayed_ = ison;
	}
	drawer_.draw();
    }

protected:

    ObjectSet<uiCheckBox>	colboxflds_;
    uiCheckBox*			allboxfld_;
    uiStratDrawer&		drawer_;
    StratDispData&		data_;
};


void uiStratDisplay::selCols( CallBacker* )
{
    uiColViewerDlg dlg( parent(), drawer_, data_ );
    dlg.go();
}


void uiStratDisplay::reDraw( CallBacker* )
{
    drawer_.draw();
}


void uiStratDisplay::setZRange( const Interval<float>& zrgin )
{
    Interval<float> zrg = zrgin;
    rangefld_->setValue( zrg );
    rangefld_->setNrDecimals( 2 );
    if ( uicontrol_ )
	uicontrol_->setRange( zrg );
    zrg.sort(false);
    drawer_.setZRange( zrg );
    drawer_.draw();
}


void uiStratDisplay::display( bool yn, bool shrk, bool maximize )
{
    uiGraphicsView::display( yn );
    dispparamgrp_->display( yn );
}


void uiStratDisplay::dispParamChgd( CallBacker* )
{
    Interval<float> rg = rangefld_->getFInterval();
    if ( rg.start < maxrg_.start || rg.stop > maxrg_.stop
	    || rg.stop <= rg.start || rg.stop <= 0 )
	rg = maxrg_;

    setZRange( rg );
}


void uiStratDisplay::usrClickCB( CallBacker* cb )
{
    mDynamicCastGet(MouseEventHandler*,mevh,cb)
    if ( !mevh || !mevh->hasEvent() || mevh->isHandled() )
	return;

    mevh->setHandled( handleUserClick(mevh->event()) );
}


void uiStratDisplay::doubleClickCB( CallBacker* )
{
    const StratDispData::Unit* unit = getUnitFromPos();
    if ( !unit || islocked_ )
	return;

    uidatawriter_.handleUnitProperties( unit->fullCode() );
}


void uiStratDisplay::mouseMoveCB( CallBacker* )
{
    if ( !mainwin() )
	return;

    const Interval<float> agerg = rangefld_->getFInterval();
    const float age = getPos().y;
    BufferString agestr; agestr.set( age, 3 );
    uiString agetxt = agerg.includes(age,false) ? tr("Age: %1 Ma").arg( agestr )
						: uiStrings::sEmptyString();
    mainwin()->toStatusBar( agetxt, 0 );

    uiString unitstr;
    const StratDispData::Unit* unit = getUnitFromPos();
    if ( unit )
	unitstr = toUiString( unit->fullCode() );

    uiString levelstr;
    const StratDispData::Level* lvl = getLevelFromPos();
    if ( lvl )
	levelstr = toUiString( lvl->name_ );

    mainwin()->toStatusBar( unitstr, 1 );
    mainwin()->toStatusBar( levelstr, 2 );
}


bool uiStratDisplay::handleUserClick( const MouseEvent& ev )
{
    if ( ev.rightButton() && !ev.ctrlStatus() && !ev.shiftStatus() &&
	    !ev.altStatus() && !islocked_ )
    {
	if ( getColIdxFromPos() == uidatagather_->levelColIdx() )
	{
	    const StratDispData::Level* lvl = getLevelFromPos();
	    if ( !lvl )
		return false;

	    uiAction* assmnuitm = new uiAction( tr("Assign marker boundary") );
	    uiMenu menu( parent(), uiStrings::sAction() );
	    menu.insertAction( assmnuitm, 1 );
	    const int mnuid = menu.exec();
	    if ( mnuid<0 )
		return false;
	    else if ( mnuid == 1 )
		uidatawriter_.setUnitLvl( lvl->unitcode_ );
	}
	else if ( getUnitFromPos() )
	{
	    uidatawriter_.handleUnitMenu( getUnitFromPos()->fullCode() );
	}
	else if ( getParentUnitFromPos() || getColIdxFromPos() == 0 )
	{
	    const StratDispData::Unit* unit = getColIdxFromPos() > 0 ?
						getParentUnitFromPos() : 0;
	    bool addunit = data_.nrUnits( 0 ) == 0;
	    if ( !addunit )
	    {
		uiAction* assmnuitm = new uiAction( tr("Add Unit") );
		uiMenu menu( parent(), uiStrings::sAction() );
		menu.insertAction( assmnuitm, 0 );
		const int mnuid = menu.exec();
		addunit = mnuid == 0;
	    }
	    if ( addunit )
		uidatawriter_.addUnit( unit ? unit->fullCode() : 0 );
	}
	return true;
    }
    else if ( ev.leftButton() && getUnitFromPos() )
	return uidatawriter_.setCurrentTreeItem( getUnitFromPos()->fullCode() );

    return false;
}


Geom::Point2D<float> uiStratDisplay::getPos() const
{
    uiStratDisplay* self = const_cast<uiStratDisplay*>( this );
    const float xpos = drawer_.xAxis()->getVal(
			self->getMouseEventHandler().event().pos().x );
    const float ypos = drawer_.yAxis()->getVal(
			self->getMouseEventHandler().event().pos().y );
    return Geom::Point2D<float>( xpos, ypos );
}


int uiStratDisplay::getColIdxFromPos() const
{
    float xpos = getPos().x;
    Interval<int> borders(0,0);
    for ( int idx=0; idx<data_.nrCols(); idx++ )
    {
	borders.stop += drawer_.colItem(idx).size_;
	if ( borders.includes( xpos, true ) )
	    return idx;
	borders.start = borders.stop;
    }
    return -1;
}


const StratDispData::Unit* uiStratDisplay::getUnitFromPos() const
{
    return getUnitFromPos( getColIdxFromPos() );
}


const StratDispData::Unit* uiStratDisplay::getParentUnitFromPos() const
{
    return getUnitFromPos( getColIdxFromPos()-1 );
}


const StratDispData::Unit* uiStratDisplay::getUnitFromPos( int cidx ) const
{
    if ( cidx >=0 && cidx<data_.nrCols() )
    {
	Geom::Point2D<float> pos = getPos();
	for ( int idunit=0; idunit<data_.nrUnits(cidx); idunit++ )
	{
	    const StratDispData::Unit* unit = data_.getUnit( cidx, idunit );
	    if ( pos.y < unit->zrg_.stop && pos.y >= unit->zrg_.start )
		return unit;
	}
    }

    return nullptr;
}


#define mEps drawer_.yAxis()->range().width()/100
const StratDispData::Level* uiStratDisplay::getLevelFromPos() const
{
    const int cidx = getColIdxFromPos();
    if ( cidx >=0 && cidx<data_.nrCols() )
    {
	Geom::Point2D<float> pos = getPos();
	for ( int idlvl=0; idlvl<data_.nrLevels(cidx); idlvl++ )
	{
	    const StratDispData::Level* lvl= data_.getLevel( cidx, idlvl );
	    if ( pos.y < (lvl->zpos_+mEps)  && pos.y > (lvl->zpos_-mEps) )
		return lvl;
	}
    }

    return nullptr;
}


// uiStratDrawer::ColumnItem

uiStratDrawer::ColumnItem::ColumnItem( const char* nm )
    : name_(nm)
{
}


uiStratDrawer::ColumnItem::~ColumnItem()
{
}


// uiStratDrawer

static int border = 20;

uiStratDrawer::uiStratDrawer( uiGraphicsScene& sc, const StratDispData& ad )
    : data_(ad)
    , scene_(sc)
{
    initAxis();
}


void uiStratDrawer::initAxis()
{
    uiAxisHandler::Setup xsu( uiRect::Top );
    auto* xaxis = new uiAxisHandler( &scene_, xsu );
    xaxis->setBounds( Interval<float>( 0, 100 ) );
    setNewAxis( xaxis, true );
    uiBorder uiborder = uiBorder( border );
    uiAxisHandler::Setup ysu( uiRect::Left );
    ysu.border( uiborder ).nogridline( true );
    auto* yaxis = new uiAxisHandler( &scene_, ysu );
    setNewAxis( yaxis, false );
}


uiStratDrawer::~uiStratDrawer()
{
    eraseAll();
}


void uiStratDrawer::setZRange( const StepInterval<float>& rg )
{
    if ( yax_ )
	yax_->setBounds( rg );
}


void uiStratDrawer::setNewAxis( uiAxisHandler* axis, bool isxaxis )
{
    if ( isxaxis )
	{ delete xax_; xax_ = axis; }
    else
	{ delete yax_; yax_ = axis; }

    if ( xax_ && yax_ )
    {
	xax_->setBegin( yax_ );
	yax_->setBegin( xax_ );
    }
}


void uiStratDrawer::updateAxis()
{
    xax_->updateDevSize();
    yax_->updateDevSize();
    yax_->updateScene();
}


void uiStratDrawer::draw()
{
    eraseAll();
    updateAxis();
    drawColumns();
}


void uiStratDrawer::drawColumns()
{
    eraseAll();
    int pos = 0;
    const int nrcols = data_.nrCols();

    for ( int idcol=0; idcol<nrcols; idcol++ )
    {
	ColumnItem* colitm = new ColumnItem( data_.getCol(idcol)->name_ );
	colitms_ += colitm;

	if ( !data_.getCol(idcol)->isdisplayed_ )
	    continue;

	colitm->pos_ = pos;
	colitm->size_ = (int)xax_->getVal( (int)(scene_.width()+10) )
		      /( data_.nrDisplayedCols() ) ;
	if ( colitm->size_ <0 )
	    colitm->size_ = 0;
	drawBorders( *colitm );
	drawLevels( *colitm );
	drawUnits( *colitm );
	pos ++;
    }

    if ( nrcols && data_.nrUnits(0) == 0 )
	drawEmptyText();
    else
	deleteAndNullPtr( emptyitm_ );
}


void uiStratDrawer::eraseAll()
{
    for ( int idx = colitms_.size()-1; idx>=0; idx-- )
    {
	ColumnItem* colitm = colitms_[idx];

	deleteAndNullPtr( colitm->borderitm_ );
	deleteAndNullPtr( colitm->bordertxtitm_ );
	colitm->txtitms_.erase();
	colitm->lvlitms_.erase();
	colitm->unititms_.erase();
    }
    deepErase( colitms_ );
}


void uiStratDrawer::drawBorders( ColumnItem& colitm )
{
    int x1 = xax_->getPix( mCast( float, (colitm.pos_)*colitm.size_ ) );
    int x2 = xax_->getPix( mCast( float, (colitm.pos_+1)*colitm.size_ ) );
    int y1 = yax_->getPix( yax_->range().stop );
    int y2 = yax_->getPix( yax_->range().start );

    TypeSet<uiPoint> rectpts;
    rectpts += uiPoint( x1, y1 );
    rectpts += uiPoint( x2, y1  );
    rectpts += uiPoint( x2, y2  );
    rectpts += uiPoint( x1, y2  );
    rectpts += uiPoint( x1, y1  );
    uiPolyLineItem* pli = scene_.addItem( new uiPolyLineItem(rectpts) );
    pli->setPenStyle(
	OD::LineStyle(OD::LineStyle::Solid,1,OD::Color::LightGrey()) );
    colitm.borderitm_ = pli;

    uiTextItem* ti = scene_.addItem( new uiTextItem(toUiString(colitm.name_)) );
    ti->setTextColor( OD::Color::Black() );
    ti->setPos( float((x1+x2)/2), float(y1-5) );
    const Alignment al( Alignment::HCenter, Alignment::Bottom );
    ti->setAlignment( al );
    ti->setZValue( 2 );
    colitm.bordertxtitm_ = ti;
}


void uiStratDrawer::drawLevels( ColumnItem& colitm )
{
    if ( !colitm.lvlitms_.isEmpty() )
    {
	colitm.lvlitms_.erase();
	colitm.txtitms_.erase();
    }

    const int colidx = colitms_.indexOf( &colitm );
    if ( colidx < 0 )
	return;

    Interval<float> rg = yax_->range();
    rg.sort();
    for ( int idx=0; idx<data_.getCol(colidx)->levels_.size(); idx++ )
    {
	const StratDispData::Level& lvl = *data_.getCol(colidx)->levels_[idx];
	if ( !rg.includes(lvl.zpos_,false) )
	    continue;

	const int x1 = xax_->getPix( mCast(float,(colitm.pos_)*colitm.size_) );
	const int x2 = xax_->getPix( mCast(float,(colitm.pos_+1)*colitm.size_));
	const int y = yax_->getPix( lvl.zpos_ );

	uiLineItem* li = scene_.addItem( new uiLineItem(x1,y,x2,y) );

	OD::LineStyle::Type lst = lvl.name_.isEmpty() ? OD::LineStyle::Dot
						  : OD::LineStyle::Solid;
	li->setPenStyle( OD::LineStyle(lst,2,lvl.color_) );
	uiTextItem* ti = scene_.addItem( new uiTextItem(toUiString(lvl.name_)));
	ti->setPos( mCast(float,x1+(x2-x1)/2), mCast(float,y) );
	ti->setZValue( 4 );
	ti->setTextColor( lvl.color_ );

	colitm.txtitms_ += ti;
	colitm.lvlitms_ += li;
    }
}


void uiStratDrawer::drawEmptyText()
{
    deleteAndNullPtr( emptyitm_ );

    int x = xax_->getPix( 0 );
    if ( !colitms_.isEmpty() )
	x += colitms_[0]->size_/2;

    const int y1 = yax_->getPix( yax_->range().stop );
    const int y2 = yax_->getPix( yax_->range().start );

    auto* ti = new uiTextItem( tr("Right-click to add unit") );
    scene_.addItem( ti );
    ti->setTextColor( OD::Color::Red() );
    ti->setPos( float(x+10), float(y2 - abs((y2-y1)/2)-10) );
    ti->setZValue( 3 );
    emptyitm_ = ti;
}


void uiStratDrawer::drawUnits( ColumnItem& colitm )
{
    colitm.txtitms_.erase(); colitm.unititms_.erase();
    const int colidx = colitms_.indexOf( &colitm );
    if ( colidx < 0 ) return;

    const Interval<float> rg = yax_->range();

    for ( int unidx=0; unidx<data_.getCol(colidx)->units_.size(); unidx++ )
    {
	const StratDispData::Unit& unit = *data_.getCol(colidx)->units_[unidx];
	Interval<float> unitrg = unit.zrg_;
	if ( ( ( !rg.includes(unitrg.start,true) &&
		 !rg.includes(unitrg.stop,true) )
	    && ( !unitrg.includes(rg.start,true) &&
		 !unitrg.includes(rg.stop,true) ) )
		|| !unit.isdisplayed_ ) continue;
	unitrg.limitTo( rg );

	int x1 = xax_->getPix( mCast(float,(colitm.pos_)*colitm.size_) );
	int x2 = xax_->getPix( mCast(float,(colitm.pos_+1)*colitm.size_) );
	bool ztop = ( unitrg.start < rg.stop );
	bool zbase = ( unitrg.stop > rg.start );
	int y1 = yax_->getPix( ztop ? rg.stop : unitrg.start );
	int y2 = yax_->getPix( zbase ? rg.start : unitrg.stop );

	TypeSet<uiPoint> rectpts;
	rectpts += uiPoint( x1, y1 );
	rectpts += uiPoint( x2, y1 );
	rectpts += uiPoint( x2, y2 );
	rectpts += uiPoint( x1, y2 );
	rectpts += uiPoint( x1, y1 );
	uiPolygonItem* pli = scene_.addPolygon( rectpts, true );
	pli->setZValue( 3 );
	pli->setPenColor( OD::Color::Black() );
	if ( unit.color_ != OD::Color::White() )
	    pli->setFillColor( unit.color_, true );

	BufferString unm( unit.name() );
	for ( int idx=1; idx<unm.size(); idx++ )
	{
	    BufferString tmpnm = unm; tmpnm[idx] = '\0';
	    if ( FontList().get().width( toUiString(tmpnm) ) > ( x2-x1 ) )
		{ unm[idx-1] = '\0'; break; }
	}

	uiTextItem* ti = scene_.addItem( new uiTextItem( toUiString(unm )) );
	ti->setTextColor( OD::Color::Black() );
	ti->setPos( mCast(float,(x1+x2)/2), mCast(float,y2-abs((y2-y1)/2)-10) );
	ti->setAlignment( Alignment::HCenter );
	ti->setZValue( 3 );
	colitm.txtitms_ += ti;
	colitm.unititms_ += pli;
    }
}


// uiStratViewControl::Setup

uiStratViewControl::Setup::Setup( const Interval<float>& rg )
    : maxrg_(rg)
    , tb_(nullptr)
{
}


uiStratViewControl::Setup::~Setup()
{
}


// uiStratViewControl

#define mDefBut(but,fnm,cbnm,tt) \
    but = new uiToolButton( tb_, fnm, tt, mCB(this,uiStratViewControl,cbnm) ); \
    tb_->addObject( but );

uiStratViewControl::uiStratViewControl( uiGraphicsView& v, Setup& su )
    : viewer_(v)
    , rangeChanged(this)
    , tb_(su.tb_)
    , boundingrange_(su.maxrg_)
{
    if ( !tb_ )
	tb_ = new uiToolBar( v.parent(), toUiString("Viewer toolbar"),
			     uiToolBar::Top );
    else if ( !tb_->isEmpty() )
	tb_->addSeparator();

    mDefBut(rubbandzoombut_,"rubbandzoom",dragModeCB,tr("Rubberband zoom"));
    mDefBut(vertzoominbut_,"vertzoomin",zoomCB,tr("Zoom in"));
    mDefBut(vertzoomoutbut_,"vertzoomout",zoomCB,tr("Zoom out"));
    mDefBut(cancelzoombut_,"cancelzoom",cancelZoomCB,tr("Cancel zoom"));
    rubbandzoombut_->setToggleButton( true );

    mAttachCB( viewer_.getKeyboardEventHandler().keyPressed,
	       uiStratViewControl::keyPressed );

    MouseEventHandler& meh = mouseEventHandler();
    mAttachCB( meh.wheelMove, uiStratViewControl::wheelMoveCB );
    mAttachCB( meh.buttonPressed, uiStratViewControl::handDragStarted );
    mAttachCB( meh.buttonReleased, uiStratViewControl::handDragged );
    mAttachCB( meh.movement, uiStratViewControl::handDragging );
    mAttachCB( viewer_.rubberBandUsed, uiStratViewControl::rubBandCB );
}


uiStratViewControl::~uiStratViewControl()
{
    detachAllNotifiers();
}


void uiStratViewControl::setSensitive( bool yn )
{
    rubbandzoombut_->setSensitive( yn );
    vertzoominbut_->setSensitive( yn );
    vertzoomoutbut_->setSensitive( yn );
    cancelzoombut_->setSensitive( yn );
}


static float zoomfwdfac = 0.8;

void uiStratViewControl::zoomCB( CallBacker* but )
{
    const bool zoomin = but == vertzoominbut_;
    const Interval<float>& brge = boundingrange_;
    if ( (!zoomin && range_==brge) || (zoomin && range_.width()<=0.1) ) return;

    const MouseEventHandler& meh = mouseEventHandler();
    const uiRect& allarea = viewer_.getSceneRect();
    LinScaler scaler( allarea.top()+border, range_.start,
		      allarea.bottom()-border, range_.stop );
    float rgpos = meh.hasEvent() ? (float)scaler.scale(meh.event().pos().y)
				 : range_.center();
    const float zoomfac = zoomin ? zoomfwdfac : 1/zoomfwdfac;
    const float twdth = (rgpos-range_.start) * zoomfac;
    const float bwdth = (range_.stop-rgpos) * zoomfac;

    if ( rgpos - twdth < brge.start )	rgpos = brge.start + twdth;
    if ( rgpos + bwdth > brge.stop )	rgpos = brge.stop - bwdth;
    range_.set( rgpos - twdth, rgpos + bwdth );
    rangeChanged.trigger();

    updatePosButtonStates();
}


void uiStratViewControl::cancelZoomCB( CallBacker* )
{
    range_ = boundingrange_;
    rangeChanged.trigger();
    updatePosButtonStates();
}


void uiStratViewControl::updatePosButtonStates()
{
    const bool iszoomatstart = range_==boundingrange_;
    vertzoomoutbut_->setSensitive( !iszoomatstart );
    cancelzoombut_->setSensitive( !iszoomatstart );
}


MouseEventHandler& uiStratViewControl::mouseEventHandler()
{
    return viewer_.getNavigationMouseEventHandler();
}


void uiStratViewControl::wheelMoveCB( CallBacker* )
{
    const MouseEventHandler& mvh = mouseEventHandler();
    const MouseEvent& ev = mvh.event();
    if ( mIsZero(ev.angle(),0.01) )
	return;

    const bool zoomin = viewer_.getMouseWheelReversal() ?
	ev.angle() < 0 : ev.angle() > 0;
    zoomCB( zoomin ? vertzoominbut_ : vertzoomoutbut_ );
}


void uiStratViewControl::dragModeCB( CallBacker* )
{
    viewer_.setDragMode( rubbandzoombut_->isOn() ?
			 uiGraphicsViewBase::RubberBandDrag :
			 uiGraphicsViewBase::NoDrag );
}


void uiStratViewControl::keyPressed( CallBacker* )
{
    const KeyboardEvent& ev = viewer_.getKeyboardEventHandler().event();
    if ( ev.key_ == OD::KB_Escape )
    {
	rubbandzoombut_->setOn( !rubbandzoombut_->isOn() );
	dragModeCB( rubbandzoombut_ );
    }
}


void uiStratViewControl::handDragStarted( CallBacker* )
{
    if ( mouseEventHandler().event().rightButton() )
	return;
    mousepressed_ = true;
    startdragpos_ = mCast( float, mouseEventHandler().event().pos().y );
}


void uiStratViewControl::handDragging( CallBacker* )
{
    if ( viewer_.dragMode() != uiGraphicsViewBase::ScrollHandDrag
	|| !mousepressed_ ) return;

    const float newpos = mCast(float,mouseEventHandler().event().pos().y);
    const uiRect& allarea = viewer_.getSceneRect();
    LinScaler scaler( allarea.top()+border, range_.start,
		      allarea.bottom()-border, range_.stop );
    const float shift=(float)(scaler.scale(newpos)-scaler.scale(startdragpos_));
    startdragpos_ = newpos;

    Interval<float> rg( range_.start - shift, range_.stop - shift );
    if ( rg.start < boundingrange_.start )
	rg.set( boundingrange_.start, range_.stop );
    if ( rg.stop > boundingrange_.stop )
	rg.set( range_.start, boundingrange_.stop );
    range_ = rg;

    rangeChanged.trigger();
}


void uiStratViewControl::handDragged( CallBacker* cb )
{
    handDragging( cb );
    mousepressed_ = false;
}


void uiStratViewControl::rubBandCB( CallBacker* )
{
    const uiRect* selarea = viewer_.getSelectedArea();
    if ( !selarea || (selarea->topLeft() == selarea->bottomRight())
	    || (selarea->width()<5 && selarea->height()<5) )
	return;

    const uiRect& allarea = viewer_.getSceneRect();
    LinScaler scaler( allarea.top()+border, range_.start,
		      allarea.bottom()-border, range_.stop );
    const Interval<float> rg( mCast(float,scaler.scale(selarea->top())),
			      mCast(float,scaler.scale(selarea->bottom())) );
    if ( rg.width()<=0.1 ) return;

    range_ = rg;
    range_.sort();
    range_.limitTo( boundingrange_ );
    rangeChanged.trigger();

    rubbandzoombut_->setOn( false );
    dragModeCB( rubbandzoombut_ );
    updatePosButtonStates();
}

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uistratsimplelaymoddisp.h"
#include "uistratlaymodtools.h"
#include "uistrateditlayer.h"
#include "uigraphicsitemimpl.h"
#include "uigraphicsscene.h"
#include "uigraphicsview.h"
#include "uigeninput.h"
#include "uifiledlg.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiaxishandler.h"
#include "stratlevel.h"
#include "stratlayermodel.h"
#include "stratlayersequence.h"
#include "stratreftree.h"
#include "strmprov.h"
#include "survinfo.h"
#include "property.h"
#include "keystrs.h"

#define mGetConvZ(var,conv) \
    if ( SI().depthsInFeet() ) var *= conv
#define mGetRealZ(var) mGetConvZ(var,mFromFeetFactorF)
#define mGetDispZ(var) mGetConvZ(var,mToFeetFactorF)
#define mGetDispZrg(var) \
    Interval<float> var( zrg_ ); \
    if ( SI().depthsInFeet() ) \
	var.scale( mToFeetFactorF )


uiStratLayerModelDisp::uiStratLayerModelDisp( uiStratLayModEditTools& t,
					  const Strat::LayerModelProvider& lmp )
    : uiGroup(t.parent(),"LayerModel display")
    , tools_(t)
    , lmp_(lmp)
    , zoomwr_(mUdf(double),0,0,0)
    , selseqidx_(-1)
    , flattened_(false)
    , fluidreplon_(false)
    , zrg_(0,1)
    , sequenceSelected(this)
    , genNewModelNeeded(this)
    , rangeChanged(this)   
    , modelEdited(this)   
    , infoChanged(this)   
{
}


uiStratLayerModelDisp::~uiStratLayerModelDisp()
{
}


const Strat::LayerModel& uiStratLayerModelDisp::layerModel() const
{
    return lmp_.get();
}


void uiStratLayerModelDisp::selectSequence( int selidx )
{
    selseqidx_ = selidx;
    drawSelectedSequence();
}


void uiStratLayerModelDisp::setFlattened( bool yn )
{
    if ( flattened_ != yn )
    {
	flattened_ = yn;
	setZoomBox(uiWorldRect(mUdf(double),0,0,0));
	modelChanged();
    }
}


bool uiStratLayerModelDisp::haveAnyZoom() const
{
    const int nrseqs = layerModel().size();
    mGetDispZrg(dispzrg);
    uiWorldRect wr( 1, dispzrg.start, nrseqs + 1, dispzrg.stop );
    return zoomwr_.isInside( wr, 1e-5 );
}


float uiStratLayerModelDisp::getLayerPropValue( const Strat::Layer& lay,
						const PropertyRef* pr,
       						int propidx ) const
{
    if ( propidx < lay.nrValues() )
    {
	if ( isFluidReplOn() )
	{
	    const int nrunits = frpars_.size() / 2;
	    BufferString namestr;
	    if ( pr->isKnownAs("PVel") || pr->isKnownAs("SVel")
		    		       || pr->isKnownAs("Den") )
	    {
		float vp, vs, den;
		for ( int idx=0; idx<nrunits; idx++ )
		{
		    frpars_.get( IOPar::compKey(toString(idx),sKey::Name()),
			    	 namestr );
		    if ( !strcmp( namestr.buf(), lay.name() ) )
		    {
			frpars_.get(IOPar::compKey(toString(idx),sKey::Value()),
				    vp, vs, den );
			return pr->isKnownAs("PVel")
				    ? vp : pr->isKnownAs("SVel") ? vs : den;
		    }
		}
	    }
	}
	return lay.value( propidx );
    }

    return mUdf(float);
}


uiStratSimpleLayerModelDisp::uiStratSimpleLayerModelDisp(
		uiStratLayModEditTools& t, const Strat::LayerModelProvider& l )
    : uiStratLayerModelDisp(t,l)
    , emptyitm_(0)
    , zoomboxitm_(0)
    , dispprop_(1)
    , dispeach_(1)
    , fillmdls_(true)
    , uselithcols_(true)
    , showzoomed_(true)
    , vrg_(0,1)
    , logblckitms_(*new uiGraphicsItemSet)
    , lvlitms_(*new uiGraphicsItemSet)
    , contitms_(*new uiGraphicsItemSet)
    , selseqitm_(0)
    , selectedlevel_(-1)
    , selectedcontent_(0)
    , allcontents_(false)
{
    gv_ = new uiGraphicsView( this, "LayerModel display" );
    gv_->setPrefWidth( 800 ); gv_->setPrefHeight( 300 );
    gv_->getMouseEventHandler().buttonReleased.notify(
			mCB(this,uiStratSimpleLayerModelDisp,usrClicked) );
    gv_->getMouseEventHandler().doubleClick.notify(
			mCB(this,uiStratSimpleLayerModelDisp,doubleClicked) );
    gv_->getMouseEventHandler().movement.notify(
			mCB(this,uiStratSimpleLayerModelDisp,mouseMoved) );

    const uiBorder border( 10 );
    uiAxisHandler::Setup xahsu( uiRect::Top );
    xahsu.border( border ).nogridline( true );
    xax_ = new uiAxisHandler( &scene(), xahsu );
    uiAxisHandler::Setup yahsu( uiRect::Left );
    const BufferString zlbl( "Depth (", SI().depthsInFeet() ? "ft" : "m", ")" );
    yahsu.border( border ).name( zlbl );
    yax_ = new uiAxisHandler( &scene(), yahsu );
    yax_->setEnd( xax_ );
    xax_->setBegin( yax_ );

    const CallBack redrawcb( mCB(this,uiStratSimpleLayerModelDisp,reDrawCB) );
    gv_->reSize.notify( redrawcb );
    gv_->reDrawNeeded.notify( redrawcb );
    tools_.selPropChg.notify( redrawcb );
    tools_.selLevelChg.notify( redrawcb );
    tools_.selContentChg.notify( redrawcb );
    tools_.dispEachChg.notify( redrawcb );
    tools_.dispZoomedChg.notify( redrawcb );
    tools_.dispLithChg.notify( redrawcb );
}


uiStratSimpleLayerModelDisp::~uiStratSimpleLayerModelDisp()
{
    eraseAll();
    delete &lvlitms_;
    delete &logblckitms_;
}


void uiStratSimpleLayerModelDisp::eraseAll()
{
    logblckitms_.erase();
    lvlitms_.erase();
    lvldpths_.erase();
    delete selseqitm_; selseqitm_ = 0;
    delete emptyitm_; emptyitm_ = 0;
}


uiGraphicsScene& uiStratSimpleLayerModelDisp::scene()
{
    return gv_->scene();
}


int uiStratSimpleLayerModelDisp::getClickedModelNr() const
{
    MouseEventHandler& mevh = gv_->getMouseEventHandler();
    if ( layerModel().isEmpty() || !mevh.hasEvent() || mevh.isHandled() )
	return -1;
    const MouseEvent& mev = mevh.event();
    const float xsel = xax_->getVal( mev.pos().x );
    int selidx = mNINT32( xsel ) - 1;
    if ( selidx < 0 || selidx >= layerModel().size() )
	selidx = -1;
    return selidx;
}


void uiStratSimpleLayerModelDisp::mouseMoved( CallBacker* )
{
    IOPar statusbarmsg;
    statusbarmsg.set( "Model Number", getClickedModelNr() );
    const MouseEvent& mev = gv_->getMouseEventHandler().event();
    const float depth = yax_->getVal( mev.pos().y );
    statusbarmsg.set( "Depth", depth );
    infoChanged.trigger( statusbarmsg, this );
}


void uiStratSimpleLayerModelDisp::usrClicked( CallBacker* )
{
    const int selidx = getClickedModelNr();
    if ( selidx < 0 ) return;

    MouseEventHandler& mevh = gv_->getMouseEventHandler();
    if ( OD::rightMouseButton(mevh.event().buttonState() ) )
	handleRightClick(selidx);
    else
    {
	selectSequence( selidx );
	sequenceSelected.trigger();
	mevh.setHandled( true );
    }
}


void uiStratSimpleLayerModelDisp::handleRightClick( int selidx )
{
    if ( selidx < 0 || selidx >= layerModel().size() )
	return;

    Strat::LayerSequence& ls = const_cast<Strat::LayerSequence&>(
	    				layerModel().sequence( selidx ) );
    ObjectSet<Strat::Layer>& lays = ls.layers();
    MouseEventHandler& mevh = gv_->getMouseEventHandler();
    float zsel = yax_->getVal( mevh.event().pos().y );
    mGetRealZ( zsel );
    mevh.setHandled( true );
    if ( flattened_ )
    {
	const float lvlz = lvldpths_[selidx];
	if ( mIsUdf(lvlz) )
	    return;
	zsel += lvlz;
    }

    const int layidx = ls.layerIdxAtZ( zsel );
    if ( lays.isEmpty() || layidx < 0 )
	return;

    uiPopupMenu mnu( parent(), "Action" );
    mnu.insertItem( new uiMenuItem("&Properties ..."), 0 );
    mnu.insertItem( new uiMenuItem("&Remove layer ..."), 1 );
    mnu.insertItem( new uiMenuItem("Remove this &Well"), 2 );
    mnu.insertItem( new uiMenuItem("&Dump all wells to file ..."), 3 );
    mnu.insertItem( new uiMenuItem("&Add dumped wells from file ..."), 4 );
    const int mnuid = mnu.exec();
    if ( mnuid < 0 ) return;

    Strat::Layer& lay = *ls.layers()[layidx];
    if ( mnuid == 0 )
    {
	uiStratEditLayer dlg( this, lay, ls, true );
	if ( dlg.go() )
	    forceRedispAll( true );
    }
    else if ( mnuid == 3 || mnuid == 4 )
	doLayModIO( mnuid == 4 );
    else if ( mnuid == 2 )
    {
	const_cast<Strat::LayerModel&>(layerModel()).removeSequence( selidx );
	forceRedispAll( true );
    }
    else
    {

	uiDialog dlg( this, uiDialog::Setup( "Remove a layer",
		    BufferString("Remove '",lay.name(),"'"),mTODOHelpID) );
	uiGenInput* gi = new uiGenInput( &dlg, "Remove", BoolInpSpec(true,
		    "Only this layer","All layers with this ID") );
	if ( dlg.go() )
	    removeLayers( ls, layidx, !gi->getBoolValue() );
    }
}


void uiStratSimpleLayerModelDisp::doLayModIO( bool foradd )
{
    const Strat::LayerModel& lm = layerModel();
    if ( !foradd && lm.isEmpty() )
	{ uiMSG().error("Please generate some layer sequences"); return; }

    uiFileDialog dlg( this, foradd, 0, 0, "Select layer model dump file" );
    if ( !dlg.go() ) return;

    StreamProvider sp( dlg.fileName() );
    StreamData sd( foradd ? sp.makeIStream() : sp.makeOStream() );
    if ( !sd.usable() )
	{ uiMSG().error("Cannot open:\n",dlg.fileName()); return; }

    if ( !foradd )
    {
	if ( !lm.write(*sd.ostrm) )
	    uiMSG().error("Unknown error during write ...");
    }
    else
    {
	Strat::LayerModel newlm;
	if ( !newlm.read(*sd.istrm) )
	    { uiMSG().error("Cannot read layer model from file."
		    "\nFile may not be a layer model file"); return; }

	for ( int ils=0; ils<newlm.size(); ils++ )
	    const_cast<Strat::LayerModel&>(lm)
				.addSequence( newlm.sequence( ils ) );

	forceRedispAll( true );
    }
}


void uiStratSimpleLayerModelDisp::removeLayers( Strat::LayerSequence& seq,
					int layidx, bool doall )
{
    if ( !doall )
    {
	delete seq.layers().removeSingle( layidx );
	seq.prepareUse();
    }
    else
    {
	const Strat::LeafUnitRef& lur = seq.layers()[layidx]->unitRef();
	for ( int ils=0; ils<layerModel().size(); ils++ )
	{
	    Strat::LayerSequence& ls = const_cast<Strat::LayerSequence&>(
						layerModel().sequence( ils ) );
	    bool needprep = false;
	    for ( int ilay=0; ilay<ls.layers().size(); ilay++ )
	    {
		const Strat::Layer& lay = *ls.layers()[ilay];
		if ( &lay.unitRef() == &lur )
		{
		    delete ls.layers().removeSingle( ilay );
		    ilay--; needprep = true;
		}
	    }
	    if ( needprep )
		ls.prepareUse();
	}
    }

    forceRedispAll( true );
}


void uiStratSimpleLayerModelDisp::doubleClicked( CallBacker* )
{
    const int selidx = getClickedModelNr();
    if ( selidx < 0 ) return;

    // Should we do something else than edit?
    handleRightClick(selidx);
}


void uiStratSimpleLayerModelDisp::forceRedispAll( bool modeledited )
{
    reDrawCB( 0 );
    if ( modeledited )
	modelEdited.trigger();
}


void uiStratSimpleLayerModelDisp::reDrawCB( CallBacker* )
{
    eraseAll(); layerModel().prepareUse();
    if ( layerModel().isEmpty() )
    {
	emptyitm_ = scene().addItem( new uiTextItem( "<---empty--->",
				     mAlignment(HCenter,VCenter) ) );
	emptyitm_->setPenColor( Color::Black() );
	emptyitm_->setPos( uiPoint( gv_->width()/2, gv_->height() / 2 ) );
	return;
    }

    doDraw();
}


void uiStratSimpleLayerModelDisp::setZoomBox( const uiWorldRect& wr )
{
    if ( !zoomboxitm_ )
    {
	zoomboxitm_ = scene().addItem( new uiRectItem );
	zoomboxitm_->setPenStyle( LineStyle(LineStyle::Dot,3,Color::Black()) );
	zoomboxitm_->setZValue( 100 );
    }

    // provided rect is always in system [0.5,N+0.5]
    zoomwr_.setLeft( wr.left() + .5 );
    zoomwr_.setRight( wr.right() + .5 );
    zoomwr_.setTop( wr.bottom() );
    zoomwr_.setBottom( wr.top() );
    updZoomBox();
    if ( showzoomed_ )
	forceRedispAll();
}


void uiStratSimpleLayerModelDisp::updZoomBox()
{
    if ( zoomwr_.width() < 0.001 || !zoomboxitm_ || !xax_ )
	{ if ( zoomboxitm_ ) zoomboxitm_->setVisible( false ); return; }

    const int xpix = xax_->getPix( (float)zoomwr_.left() );
    const int ypix = yax_->getPix( (float)zoomwr_.top() );
    const int wdth = xax_->getPix( (float)zoomwr_.right() ) - xpix;
    const int hght = yax_->getPix( (float)zoomwr_.bottom() ) - ypix;
    zoomboxitm_->setRect( xpix, ypix, wdth, hght );
    zoomboxitm_->setVisible( haveAnyZoom() && !showzoomed_ );
}


void uiStratSimpleLayerModelDisp::modelChanged()
{
    forceRedispAll();
}



#define mStartLayLoop(chckdisp,perseqstmt) \
    const int nrseqs = layerModel().size(); \
    for ( int iseq=0; iseq<nrseqs; iseq++ ) \
    { \
	if ( chckdisp && !isDisplayedModel(iseq) ) continue; \
	const float lvldpth = lvldpths_[iseq]; \
	if ( flattened_ && mIsUdf(lvldpth) ) continue; \
	int layzlvl = 0; \
	const Strat::LayerSequence& seq = layerModel().sequence( iseq ); \
	const int nrlays = seq.size(); \
	perseqstmt; \
	for ( int ilay=0; ilay<nrlays; ilay++ ) \
	{ \
	    layzlvl++; \
	    const Strat::Layer& lay = *seq.layers()[ilay]; \
	    float z0 = lay.zTop(); if ( flattened_ ) z0 -= lvldpth; \
	    float z1 = lay.zBot(); if ( flattened_ ) z1 -= lvldpth; \
	    const float val = \
	       getLayerPropValue(lay,seq.propertyRefs()[dispprop_],dispprop_); \

#define mEndLayLoop() \
	} \
    }


void uiStratSimpleLayerModelDisp::getBounds()
{
    lvldpths_.erase();
    const Strat::Level* lvl = tools_.selStratLevel();
    for ( int iseq=0; iseq<layerModel().size(); iseq++ )
    {
	const Strat::LayerSequence& seq = layerModel().sequence( iseq );
	if ( !lvl || seq.isEmpty() )
	    { lvldpths_ += mUdf(float); continue; }

	const int posidx = seq.positionOf( *lvl );
	float zlvl = mUdf(float);
	if ( posidx >= seq.size() )
	    zlvl = seq.layers()[seq.size()-1]->zBot();
	else if ( posidx >= 0 )
	    zlvl = seq.layers()[posidx]->zTop();
	lvldpths_ += zlvl;
    }

    Interval<float> zrg(mUdf(float),mUdf(float)), vrg(mUdf(float),mUdf(float));
    mStartLayLoop( false,  )
#	define mChckBnds(var,op,bnd) \
	if ( (mIsUdf(var) || var op bnd) && !mIsUdf(bnd) ) \
	    var = bnd
	mChckBnds(zrg.start,>,z0);
	mChckBnds(zrg.stop,<,z1);
	mChckBnds(vrg.start,>,val);
	mChckBnds(vrg.stop,<,val);
    mEndLayLoop()
    zrg_ = mIsUdf(zrg.start) ? Interval<float>(0,1) : zrg;
    vrg_ = mIsUdf(vrg.start) ? Interval<float>(0,1) : vrg;

    if ( mIsUdf(zoomwr_.left()) )
    {
	zoomwr_.setLeft( 1 );
	zoomwr_.setRight( nrseqs+1 );
	mGetDispZrg(dispzrg);
	zoomwr_.setTop( dispzrg.stop );
	zoomwr_.setBottom( dispzrg.start );
    }
}


int uiStratSimpleLayerModelDisp::getXPix( int iseq, float relx ) const
{
    static const float margin = 0.05;
    relx = (1-margin) * relx + margin * .5f; // get relx between 0.025 and 0.975
    relx *= dispeach_;
    return xax_->getPix( iseq + 1 + relx );
}


bool uiStratSimpleLayerModelDisp::isDisplayedModel( int iseq ) const
{
    if ( iseq % dispeach_ )
	return false;

    if ( showzoomed_ )
    {
	const int xpix0 = getXPix( iseq, 0 );
	const int xpix1 = getXPix( iseq, 1 );
	if ( xax_->getVal(xpix1) > zoomwr_.right()
	  || xax_->getVal(xpix0) < zoomwr_.left() )
	    return false;
    }
    return true;
}


void uiStratSimpleLayerModelDisp::doDraw()
{
    dispprop_ = tools_.selPropIdx();
    selectedlevel_ = tools_.selLevelIdx();
    dispeach_ = tools_.dispEach();
    showzoomed_ = tools_.dispZoomed();
    uselithcols_ = tools_.dispLith();
    selectedcontent_ = layerModel().refTree().contents()
				.getByName(tools_.selContent());
    allcontents_ = FixedString(tools_.selContent()) == sKey::All();

    getBounds();

    xax_->updateDevSize(); yax_->updateDevSize();
    if ( !showzoomed_ )
    {
	xax_->setBounds( Interval<float>(1,mCast(float,layerModel().size() )));
    	mGetDispZrg(dispzrg);
	yax_->setBounds( Interval<float>(dispzrg.stop,dispzrg.start) );
    }
    else
    {
	xax_->setBounds( Interval<float>((float)zoomwr_.left(),
						(float)zoomwr_.right()) );
	yax_->setBounds( Interval<float>((float)zoomwr_.top(),
						(float)zoomwr_.bottom()) );
    }

    yax_->plotAxis(); xax_->plotAxis();
    const float vwdth = vrg_.width();
    float zfac = 1; mGetDispZ( zfac );

    mStartLayLoop( true, int prevypix1 = -mUdf(int) )

	float dispz0 = z0; float dispz1 = z1;
	mGetConvZ( dispz0, zfac ); mGetConvZ( dispz1, zfac );
	if ( showzoomed_ )
	{
	    if ( dispz0 > zoomwr_.top() || dispz1 < zoomwr_.bottom() )
		continue;
	    if ( dispz1 > zoomwr_.top() )
		dispz1 = (float)zoomwr_.top();
	    if ( dispz0 < zoomwr_.bottom() )
		dispz0 = (float)zoomwr_.bottom();
	}

	int ypix0 = yax_->getPix( dispz0 );
	const int ypix1 = yax_->getPix( dispz1 );
	if ( ypix0 <= prevypix1 ) ypix0 = prevypix1 + 1;
	prevypix1 = ypix1;

	if ( ypix0 < ypix1 && !mIsUdf(val) )
	{
	    const float relx = (val-vrg_.start) / vwdth;
	    const int xpix0 = getXPix( iseq, 0 );
	    const int xpix1 = getXPix( iseq, relx );

	    uiRectItem* it = scene().addRect(
		    mCast(float,xpix0), mCast(float,ypix0),
		    mCast(float,xpix1-xpix0+1), mCast(float,ypix1-ypix0+1) );
	    it->setZValue( layzlvl );

	    const Color laycol = lay.dispColor( uselithcols_ );
	    bool mustannotcont = false;
	    if ( !lay.content().isUnspecified() )
		mustannotcont = allcontents_
		    || (selectedcontent_ && lay.content() == *selectedcontent_);
	    const Color pencol = mustannotcont ? lay.content().color_ : laycol;
	    it->setPenColor( pencol );
	    if ( pencol != laycol )
		it->setPenStyle( LineStyle(LineStyle::Solid,2,pencol) );

	    if ( fillmdls_ )
	    {
		it->setFillColor( laycol );
		if ( mustannotcont )
		    it->setFillPattern( lay.content().pattern_ );
	    }

	    logblckitms_ += it;
	}

    mEndLayLoop()

    drawLevels();
    drawSelectedSequence();
    updZoomBox();
}


void uiStratSimpleLayerModelDisp::drawLevels()
{
    if ( layerModel().isEmpty() )
	return;

    lvlcol_ = tools_.selLevelColor();
    for ( int iseq=0; iseq<lvldpths_.size(); iseq++ )
    {
	float zlvl = lvldpths_[iseq];
	if ( mIsUdf(zlvl) )
	    continue;

	mGetDispZ(zlvl);
	const int ypix = yax_->getPix( flattened_ ? 0 : zlvl );
	const int xpix1 = getXPix( iseq, 0 );
	const int xpix2 = getXPix( iseq, 1 );
	uiLineItem* it = scene().addItem(
	    new uiLineItem( uiPoint(xpix1,ypix), uiPoint(xpix2,ypix), true ) );

	it->setPenStyle( LineStyle(LineStyle::Solid,2,lvlcol_) );
	it->setZValue( 999999 );
	lvlitms_ += it;
    }
}


void uiStratSimpleLayerModelDisp::drawSelectedSequence()
{
    delete selseqitm_; selseqitm_ = 0;
    const int nrseqs = layerModel().size();
    if ( nrseqs < 1 || selseqidx_ > nrseqs || selseqidx_ < 0 ) return;

    const int ypix1 = yax_->getPix( yax_->range().start );
    const int ypix2 = yax_->getPix( yax_->range().stop );
    const float xpix1 = (float)getXPix( selseqidx_, 0 );
    const float xpix2 = (float)getXPix( selseqidx_, 1 );
    const int midpix = (int)( xpix1 + ( xpix2 - xpix1 ) /2 );

    uiLineItem* it = scene().addItem(
	new uiLineItem( uiPoint(midpix,ypix1), uiPoint(midpix,ypix2), true ) );

    it->setPenStyle( LineStyle(LineStyle::Dot,2,Color::Black()) );
    it->setZValue( 9999999 );
    selseqitm_ = it;
}

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2008
________________________________________________________________________

-*/

#include "uiaxishandler.h"
#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
#include "uifont.h"
#include "draw.h"
#include "linear.h"
#include "axislayout.h"

static const float logof2 = logf(2);
#define sDefNrDecimalPlaces 3
#define mFillZValue	setup_.zval_ + 1
#define mSpecialZValue	setup_.zval_ + 2
#define mAuxZValue	setup_.zval_ + 3
#define mAxisZValue	setup_.zval_ + 4
#define mRemoveFromScene(itm) \
    if ( itm ) \
    { \
	delete scene_->removeItem( itm ); \
	itm = nullptr; \
    }


class uiAHPlotAnnot : public PlotAnnotation
{
public:

    enum Type		{ Aux, Special, Fill };
			uiAHPlotAnnot( Type t )
			    : type_(t), txtwdth_(0)		{}
			uiAHPlotAnnot( const PlotAnnotation& pah )
								{ *this = pah; }
			uiAHPlotAnnot( const uiAHPlotAnnot& oth )
								{ *this = oth; }
    uiAHPlotAnnot&	operator =(const PlotAnnotation&);
    uiAHPlotAnnot&	operator =(const uiAHPlotAnnot&);

    Type		type_;
    int			txtwdth_;

    bool		isAux() const		{ return type_ == Aux; }
};


class uiAHPlotAnnotSet : public TypeSet<uiAHPlotAnnot>
{
public:

			uiAHPlotAnnotSet( uiAxisHandler& ah )
			    : axh_(ah), texts_(nullptr), setup_(ah.setup_) {}

			~uiAHPlotAnnotSet()	{ removeAllGraphicsItems(); }

    uiFont&		font()			{ return FontList().get(); }
    void		setVisible(bool);

    void		add(const PlotAnnotation&);
    void		add(float,uiAHPlotAnnot::Type);

    void		allAdded();
    void		createGraphicsItems();

protected:

    void		removeAllGraphicsItems();

    const OD::LineStyle&	getLineStyle(const uiAHPlotAnnot&) const;
    int			getZValue(const uiAHPlotAnnot&) const;

    void		addItems(const uiAHPlotAnnot&,bool);
    void		addGridLineAt(int pix,const uiAHPlotAnnot&);
    void		addAnnotationAt(int pix,const uiAHPlotAnnot&);

    uiAxisHandler&	axh_;
    uiAxisHandler::Setup& setup_;

    int			maxstrwidth_;
    TypeSet< Interval<int> > alreadyfilledpxs_;

    uiGraphicsItemSet*	lines_;
    uiGraphicsItemSet*	texts_;
    uiGraphicsItemSet*	ticks_;

    friend class	uiAxisHandler;

};


uiAHPlotAnnot& uiAHPlotAnnot::operator =( const PlotAnnotation& pah )
{
    PlotAnnotation::operator =( pah );
    type_ = Aux;
    txtwdth_ = FontList().get().width( pah.txt_ );
    return *this;
}


uiAHPlotAnnot& uiAHPlotAnnot::operator =( const uiAHPlotAnnot& oth )
{
    PlotAnnotation::operator =( oth );
    type_ = oth.type_;
    txtwdth_ = oth.txtwdth_;
    return *this;
}


void uiAHPlotAnnotSet::removeAllGraphicsItems()
{
    if ( texts_ )
    {
	delete axh_.scene_->removeItems( lines_ );
	delete axh_.scene_->removeItems( texts_ );
	delete axh_.scene_->removeItems( ticks_ );
	texts_ = nullptr;
    }
}


void uiAHPlotAnnotSet::setVisible( bool yn )
{
    if ( texts_ )
    {
	lines_->setVisible( yn );
	texts_->setVisible( yn );
	ticks_->setVisible( yn );
    }
}


const OD::LineStyle&
	uiAHPlotAnnotSet::getLineStyle( const uiAHPlotAnnot& pah ) const
{
    if ( !pah.isAux() )
	return setup_.gridlinestyle_;

    const bool ishighlighted = pah.linetype_ == PlotAnnotation::HighLighted;
    return ishighlighted ? setup_.auxhllinestyle_ : setup_.auxlinestyle_;
}


int uiAHPlotAnnotSet::getZValue( const uiAHPlotAnnot& pah ) const
{
    if ( pah.type_ == uiAHPlotAnnot::Fill ) return mFillZValue;
    return pah.isAux() ? mAuxZValue : mSpecialZValue;
}


void uiAHPlotAnnotSet::add( const PlotAnnotation& pah )
{
    *this += uiAHPlotAnnot( pah );
}


void uiAHPlotAnnotSet::add( float val, uiAHPlotAnnot::Type type )
{
    uiAHPlotAnnot pah( type );
    pah.pos_ = val;
    pah.txt_ = mToUiStringTodo(toStringLim( val, val < 0 ? axh_.reqnrchars_+1
					 : axh_.reqnrchars_ ));
    pah.txtwdth_ = font().width( pah.txt_ );
    pah.linetype_ = PlotAnnotation::Normal;
    *this += pah;
}


void uiAHPlotAnnotSet::allAdded()
{
    maxstrwidth_ = 0;
    for ( int idx=0; idx<size(); idx++ )
    {
	int strwdth = (*this)[idx].txtwdth_;
	if ( strwdth > maxstrwidth_ )
	    maxstrwidth_ = strwdth;
    }
}


void uiAHPlotAnnotSet::createGraphicsItems()
{
    removeAllGraphicsItems();
    alreadyfilledpxs_.erase();

    lines_ = new uiGraphicsItemSet();
    texts_ = new uiGraphicsItemSet();
    ticks_ = new uiGraphicsItemSet();

    for ( int icoll=0; icoll<2; icoll++ )
    {
	const bool docolldet = icoll == 1;
	for ( int idx=0; idx<size(); idx++ )
	{
	    const uiAHPlotAnnot& pah = (*this)[idx];
	    if ( (pah.type_==uiAHPlotAnnot::Fill) == docolldet )
		addItems( pah, docolldet );
	}
    }

    axh_.scene_->addItems( lines_ );
    axh_.scene_->addItems( texts_ );
    axh_.scene_->addItems( ticks_ );
}


void uiAHPlotAnnotSet::addItems( const uiAHPlotAnnot& pah, bool docolldet )
{
    if ( pah.isAux() && !setup_.showauxannot_ )
	return;

    const int pix = axh_.getPix( pah.pos_ );
    const int nrpixs = setup_.noaxisannot_ ? 0
			: (axh_.isHor() ? pah.txtwdth_ : font().height());
    const Interval<int> pixintv( pix-nrpixs/2, pix+nrpixs/2 );
    if ( docolldet )
    {
	for ( int idx=0; idx<alreadyfilledpxs_.size(); idx++ )
	    if ( alreadyfilledpxs_[idx].overlaps(pixintv,false) )
		return;
    }

    const bool hasbeghndlr =
	axh_.beghndlr_ && !axh_.beghndlr_->setup().noaxisline_;
    const bool hasendhndlr =
	axh_.endhndlr_ && !axh_.endhndlr_->setup().noaxisline_;

    const bool overlapswithotheraxis = pah.isAux() ? false :
		((hasbeghndlr && pix==axh_.getPix(axh_.range().start)) ||
		(hasendhndlr && pix==axh_.getPix(axh_.range().stop)));
    if ( (!setup_.nogridline_ && !overlapswithotheraxis) || pah.isAux() )
	addGridLineAt( pix, pah );

    if ( !setup_.noaxisannot_ || pah.isAux() )
	addAnnotationAt( pix, pah );

    alreadyfilledpxs_ += pixintv;
}


void uiAHPlotAnnotSet::addGridLineAt( int pix, const uiAHPlotAnnot& pah )
{
    uiLineItem* gridlitm = axh_.getGridLine( pix );
    gridlitm->setPenStyle( getLineStyle(pah) );
    gridlitm->setZValue( getZValue(pah) );
    lines_->add( gridlitm );
}


void uiAHPlotAnnotSet::addAnnotationAt( int pix, const uiAHPlotAnnot& pah )
{
    const OD::LineStyle& ls = setup_.style_;
    const int zvalue = getZValue( pah );
    uiLineItem* tickitm = axh_.getTickLine( pix );
    tickitm->setPenColor( ls.color_ );
    tickitm->setZValue( zvalue );
    ticks_->add( tickitm );

    uiTextItem* txtitm;
    const int p1 = axh_.tickEndPix( true );
    if ( axh_.isHor() )
    {
	const Alignment al( Alignment::HCenter, setup_.side_ == uiRect::Top
	    ? (setup_.annotinside_ ? Alignment::Top : Alignment::Bottom)
	    : (setup_.annotinside_ ? Alignment::Bottom : Alignment::Top) );
	txtitm = new uiTextItem( uiPoint(pix,p1), pah.txt_, al );
    }
    else
    {
	const Alignment al( setup_.side_==uiRect::Left ?
		Alignment::Right : Alignment::Left, Alignment::VCenter );
	txtitm = new uiTextItem( uiPoint(p1,pix), pah.txt_, al );
    }

    if ( pah.isNormal() )
	txtitm->setFontData( setup_.fontdata_ );
    else
    {
	FontData fd( setup_.fontdata_ );
	fd.setWeight( FontData::Bold );
	txtitm->setFontData( fd );
    }

    txtitm->setTextColor( Color(75,75,75) );
    txtitm->setZValue( zvalue );
    texts_->add( txtitm );
}


uiAxisHandler::uiAxisHandler( uiGraphicsScene* scene,
			      const uiAxisHandler::Setup& su )
    : annots_(*new uiAHPlotAnnotSet(*this))
    , scene_(scene)
    , setup_(su)
    , height_(su.height_)
    , width_(su.width_)
    , ticsz_(su.ticsz_)
    , beghndlr_(nullptr)
    , endhndlr_(nullptr)
    , annotstart_(0)
    , epsilon_(1e-5f)
    , axsz_(0)
    , devsz_(0)
    , nameitm_(nullptr)
    , endannotitm_(nullptr)
    , axislineitm_(nullptr)
    , ynmtxtvertical_(false)
{
    setRange( StepInterval<float>(0,1,1) );
}


uiAxisHandler::~uiAxisHandler()
{
    mRemoveFromScene( nameitm_ )
    mRemoveFromScene( endannotitm_ )
    mRemoveFromScene( axislineitm_ )
    delete &annots_;
}


void uiAxisHandler::setBounds( Interval<float> rg )
{
    const bool haveudf = mIsUdf(rg.start) || mIsUdf(rg.stop)
			|| mIsUdf(-rg.start) || mIsUdf(-rg.stop);
    if ( haveudf )
	setRange( StepInterval<float>(0.f,1.f,1.f) );
    else
    {
	const bool isrev = rg.start > rg.stop;
	AxisLayout<float> al( rg, setup_.annotinint_ );
	if ( (!isrev && (al.sd_.start < rg.start))
	  || ( isrev && (al.sd_.start > rg.start)) )
	    al.sd_.start += al.sd_.step;

	setRange( StepInterval<float>(rg.start,rg.stop,al.sd_.step),
		  &al.sd_.start );
    }
}


void uiAxisHandler::setCaption( const uiString& caption )
{
    setup_.caption_ = caption;
    updateScene();
}


void uiAxisHandler::setBorder( const uiBorder& b )
{
    setup_.border_ = b;
    updateScene();
}


void uiAxisHandler::setIsLog( bool yn )
{
    setup_.islog_ = yn;
    updateScene();
}


void uiAxisHandler::setRange( const StepInterval<float>& rg, float* astart )
{
    datarg_ = rg;
    annotstart_ = astart ? *astart : datarg_.start;

    float fsteps = (datarg_.stop - datarg_.start) / datarg_.step;
    if ( fsteps < 0 )
	datarg_.step = -datarg_.step;
    if ( mIsZero(fsteps,1e-6) )
	datarg_.widen( datarg_.step * 1.5f );
    fsteps = (datarg_.stop - datarg_.start) / datarg_.step;
    if ( fsteps > 50 )
	datarg_.step /= (fsteps / 50);

    rgisrev_ = datarg_.start > datarg_.stop;
    rgwidth_ = datarg_.width();
    epsilon_ = 1e-5f * rgwidth_;

    reCalcAnnotation();
}


int uiAxisHandler::getNrAnnotCharsForDisp() const
{
    if ( setup_.maxnrchars_ ) return setup_.maxnrchars_;
    const int widthlogval = mIsZero(datarg_.width(),epsilon_)
	? 0 : mNINT32( Math::Log10(fabs(datarg_.width())) );
    const int startlogval = mIsZero(datarg_.start,epsilon_)
	? 0 : mNINT32( Math::Log10(fabs(datarg_.start)) );
    const int stoplogval = mIsZero(datarg_.stop,epsilon_)
	? 0 : mNINT32( Math::Log10(fabs(datarg_.stop)) );
    int nrofpredecimalchars = mMAX(stoplogval,startlogval) + 1;
    // number of chars needed for pre decimal part for maximum value
    if ( nrofpredecimalchars < 1 )
	nrofpredecimalchars = 1;

    int nrofpostdecimalchars = sDefNrDecimalPlaces - widthlogval;
    // number of chars needed for decimal places on the basis of range
    if ( setup_.annotinint_ || nrofpostdecimalchars < 0 )
	nrofpostdecimalchars = 0;
    else
	nrofpostdecimalchars += 1; // +1 for the decimal itself

    const int nrannotchars = nrofpredecimalchars + nrofpostdecimalchars;
    return setup_.maxnrchars_ && nrannotchars>setup_.maxnrchars_
		? setup_.maxnrchars_ : nrannotchars;
}


void uiAxisHandler::newDevSize()
{
    devsz_ = isHor() ? width_ : height_;
    axsz_ = devsz_ - pixBefore() - pixAfter();
}


void uiAxisHandler::updateDevSize()
{
    setNewDevSize( (int)(isHor() ? scene_->width() : scene_->height()),
		   (int)(isHor() ? scene_->height() : scene_->width() ));
}


void uiAxisHandler::setNewDevSize( int devsz, int otherdim )
{
    devsz_ = devsz;
    axsz_ = devsz_ - pixBefore() - pixAfter();
    (isHor() ? width_ : height_) = devsz_;
    (isHor() ? height_ : width_) = otherdim ;
}


float uiAxisHandler::getVal( int pix ) const
{
    float relpix;
    if ( isHor() )
    {
	pix -= rgisrev_ ? pixAfter() : pixBefore();
	relpix = mCast( float, pix );
    }
    else
    {
	pix -= rgisrev_ ? pixBefore() : pixAfter();
	relpix = mCast( float, axsz_-pix );
    }
    relpix /= axsz_;

    if ( setup_.islog_ )
	relpix = expf( relpix * logof2 );

    return datarg_.start + (rgisrev_?-1:1) * rgwidth_ * relpix;
}


float uiAxisHandler::getRelPos( float v ) const
{
    float relv = (rgisrev_ ? datarg_.start - v : v - datarg_.start) / rgwidth_;
    if ( !setup_.islog_ )
	return relv;

    if ( relv < -0.9f ) relv = -0.9f;
    return log( relv + 1 ) / logof2;
}


int uiAxisHandler::getRelPosPix( float relpos ) const
{
    if ( isHor() )
	return mNINT32( pixBefore() + (axsz_ * relpos) + .5 );
    else
	return mNINT32( pixAfter() + (axsz_ * (1 - relpos)) +.5 );
}


int uiAxisHandler::getPix( float pos ) const
{
    return getRelPosPix( getRelPos(pos) );
}


int uiAxisHandler::getPix( double pos ) const
{
    return getRelPosPix( getRelPos( (float) pos) );
}


int uiAxisHandler::getPix( int pos ) const
{
    return getRelPosPix( getRelPos( (float) pos) );
}


int uiAxisHandler::pixToEdge( bool withborder ) const
{
    int ret = withborder ? setup_.border_.get(setup_.side_) : 0;
    if ( setup_.noaxisannot_ || setup_.annotinside_ || setup_.fixedborder_ )
	return ret;

    ret += ticSz() + pxsizeinotherdir_;
    return ret;
}


int uiAxisHandler::pixBefore() const
{
    int pixbefore  = 0;
    if ( beghndlr_ )
	pixbefore = beghndlr_->pixToEdge();
    else
    {
	uiRect::Side beforeside;
	if ( endhndlr_ )
	    beforeside = uiRect::across( endhndlr_->setup_.side_ );
	else
	    beforeside = isHor() ? uiRect::Left : uiRect::Bottom;
	pixbefore = setup_.border_.get( beforeside );
    }

    return pixbefore;
}


int uiAxisHandler::pixAfter() const
{
    int pixafter = 0;
    if ( endhndlr_ )
	pixafter =  endhndlr_->pixToEdge();
    else
    {
	uiRect::Side afterside = isHor() ? uiRect::Right : uiRect::Top;
	if ( beghndlr_ )
	    afterside = uiRect::across( beghndlr_->setup_.side_ );
	pixafter = setup_.border_.get( afterside );
    }

    return pixafter;
}


Interval<int> uiAxisHandler::pixRange() const
{
    if ( isHor() )
	return Interval<int>( pixBefore(), devsz_ - pixAfter() );
    else
	return Interval<int>( pixAfter(), devsz_ - pixBefore() );
}


void uiAxisHandler::setVisible( bool yn )
{
    annots_.setVisible( yn );
    if ( nameitm_ ) nameitm_->setVisible( yn );
    if ( endannotitm_ ) endannotitm_->setVisible( yn );
    if ( axislineitm_ ) axislineitm_->setVisible( yn );
}


void uiAxisHandler::updateScene()
{
    updateAxisLine();
    if ( reCalcAnnotation() )
	annots_.createGraphicsItems();

    updateName();
}


void uiAxisHandler::updateAxisLine()
{
    if ( setup_.noaxisline_ )
	{ mRemoveFromScene( axislineitm_ ); return; }

    OD::LineStyle ls( setup_.style_ );
    ls.type_ = OD::LineStyle::Solid;

    const int edgepix = pixToEdge();
    if ( isHor() )
    {
	const int startpix = pixBefore();
	const int endpix = devsz_ - pixAfter();
	const int pixpos = setup_.side_ == uiRect::Top
			 ? edgepix : height_ - edgepix;
	if ( !axislineitm_ )
	    axislineitm_ = scene_->addItem( new uiLineItem() );

	axislineitm_->setLine( startpix, pixpos,
			       endpix, pixpos );
    }
    else
    {
	const int startpix = pixAfter();
	const int endpix = devsz_ - pixBefore();
	const int pixpos = setup_.side_ == uiRect::Left
			 ? edgepix : width_ - edgepix;
	if ( !axislineitm_ )
	    axislineitm_ = scene_->addItem( new uiLineItem() );

	axislineitm_->setLine( pixpos, startpix,
			       pixpos, endpix );
    }

    axislineitm_->setPenStyle( ls );
    axislineitm_->setZValue( mAxisZValue );
}


bool uiAxisHandler::reCalcAnnotation()
{
    annots_.erase();
    if ( setup_.nogridline_ && setup_.noaxisannot_ && !setup_.showauxannot_ )
	return true;

    annotrg_ = datarg_;
    annotrg_.start = annotstart_;

    const bool allocspaceforname = !setup_.noaxisannot_
				&& !setup_.annotinside_
//				&& !setup_.fixedborder_
				&& !setup_.caption_.isEmpty();
    pxsizeinotherdir_ = 0;
    if ( allocspaceforname )
	pxsizeinotherdir_ += annots_.font().height();

    nrsteps_ = annotrg_.nrSteps();
    if ( !datarg_.includes(annotrg_.atIndex(nrsteps_),false) )
	nrsteps_--;

    reqnrchars_ = getNrAnnotCharsForDisp();

    for ( int idx=0; idx<auxannots_.size(); idx++ )
	if ( datarg_.includes(auxannots_[idx].pos_,rgisrev_) )
	    annots_.add( auxannots_[idx] );

    const bool showsplval = setup_.showSpecialValue();
    if ( showsplval && datarg_.includes(setup_.specialvalue_,rgisrev_) )
	annots_.add( setup_.specialvalue_, uiAHPlotAnnot::Special );

    const bool showstartstop = false;
    if ( showstartstop )
    {
	if ( !showsplval ||
	     !mIsEqual(datarg_.start,setup_.specialvalue_,epsilon_) )
	    annots_.add( datarg_.start, uiAHPlotAnnot::Special );

	if ( (!showsplval ||
	      !mIsEqual(datarg_.stop,setup_.specialvalue_,epsilon_))
		&& rgwidth_>epsilon_ )
	    annots_.add( datarg_.stop, uiAHPlotAnnot::Special );
    }

    for ( int idx=0; idx<=nrsteps_; idx++ )
    {
	float val = annotrg_.atIndex( idx );
	if ( datarg_.includes(val,rgisrev_) )
	    annots_.add( val, uiAHPlotAnnot::Fill );
    }

    annots_.allAdded();
    pxsizeinotherdir_ += isHor() ? annots_.font().height()
				 : annots_.maxstrwidth_;
    endpos_ = setup_.islog_ ? logof2 : 1;
    newDevSize();
    return true;
}


void uiAxisHandler::updateName()
{
    if ( setup_.noaxisannot_ || setup_.caption_.isEmpty() )
	{ mRemoveFromScene( nameitm_ ); return; }

    uiString caption = toUiString( "  %1  " ).arg( setup_.caption_ );
    if ( !nameitm_ )
    {
	nameitm_ = scene_->addItem( new uiTextItem(caption) );
	ynmtxtvertical_ = false;
    }
    else
	nameitm_->setText( caption );

    Alignment al( Alignment::HCenter, Alignment::VCenter );
    uiPoint pt;
    const int namepos = pixToEdge() - ticSz() - pxsizeinotherdir_;
    if ( isHor() )
    {
	const bool istop = setup_.side_ == uiRect::Top;
	pt.x = pixBefore() + axsz_/2;
	pt.y = istop ? namepos : height_-namepos;
	al.set( istop ? Alignment::Top : Alignment::Bottom );
    }
    else
    {
	const bool isleft = setup_.side_ == uiRect::Left;
	pt.x = isleft ? namepos : width_-namepos;
	pt.y = pixAfter() + axsz_/2;

	if ( !ynmtxtvertical_ )
	    nameitm_->setRotation( isleft ? -90.f : 90.f );
	ynmtxtvertical_ = true;
    }

    nameitm_->setPos( pt );
    nameitm_->setAlignment( al );
    nameitm_->setZValue( mAxisZValue );
    const Color col = setup_.nmcolor_ == Color::NoColor()
		    ? setup_.style_.color_ : setup_.nmcolor_;
    nameitm_->setTextColor( col );
    nameitm_->setFontData( setup_.fontdata_ );
}


int uiAxisHandler::tickEndPix( bool farend ) const
{
    const int pix2edge = pixToEdge();
    if ( isHor() )
    {
	const bool istop = setup_.side_ == uiRect::Top;
	const int y0 = istop ? pix2edge : height_ - pix2edge;
	if ( !farend )
	    return y0;

	const int y1 = istop ? (setup_.annotinside_
				? y0 + ticSz() + pxsizeinotherdir_
				: y0 - ticSz() )
			     : (setup_.annotinside_
				? y0 - ticSz() - pxsizeinotherdir_
				: y0 + ticSz() );
	return y1;
    }
    else
    {
	const bool isleft = setup_.side_ == uiRect::Left;
	const int x0 = isleft ? pix2edge : width_ - pix2edge;
	if ( !farend )
	    return x0;

	const int x1 = isleft ? (setup_.annotinside_
				    ? x0+ticSz() + pxsizeinotherdir_
				    : x0-ticSz() )
			      : (setup_.annotinside_
				    ? x0-ticSz() - pxsizeinotherdir_
				    : x0+ticSz() );
	return x1;
    }
}


uiLineItem* uiAxisHandler::getTickLine( int pix )
{
    uiLineItem* litm = new uiLineItem();
    const int p0 = tickEndPix( false );
    const int p1 = tickEndPix( true );
    if ( isHor() )
	litm->setLine( pix, p0, pix, p1 );
    else
	litm->setLine( p0, pix, p1, pix );

    return litm;
}


uiLineItem* uiAxisHandler::getGridLine( int pix )
{
    const uiAxisHandler* hndlr = beghndlr_ ? beghndlr_ : endhndlr_;
    int endpix = setup_.border_.get( uiRect::across(setup_.side_) );
    if ( hndlr )
    {
	if ( isHor() )
	    endpix = setup_.side_==uiRect::Top ? hndlr->pixBefore()
					       : hndlr->pixAfter();
	else
	    endpix = setup_.side_==uiRect::Left ? hndlr->pixAfter()
						: hndlr->pixBefore();
    }

    const int startpix = pixToEdge();

    uiLineItem* lineitem = new uiLineItem();
    const int length = isHor() ? height_ : width_;
    switch ( setup_.side_ )
    {
	case uiRect::Top:
	    lineitem->setLine( pix, startpix, pix, length - endpix );
	    break;
	case uiRect::Bottom:
	    lineitem->setLine( pix, endpix, pix, length - startpix );
	    break;
	case uiRect::Left:
	    lineitem->setLine( startpix, pix, length - endpix, pix );
	    break;
	case uiRect::Right:
	    lineitem->setLine( endpix, pix, length - startpix, pix );
	    break;
    }

    return lineitem;
}


void uiAxisHandler::annotAtEnd( const uiString& txt )
{
    if ( txt.isEmpty() )
	{ mRemoveFromScene( endannotitm_ ); return; }

    const int pix2edge = pixToEdge();
    int xpix, ypix; Alignment al;
    if ( isHor() )
    {
	xpix = devsz_ - pixAfter() - 2;
	ypix = setup_.side_ == uiRect::Top ? pix2edge  : height_ - pix2edge - 2;
	al.set( Alignment::Left,
		setup_.side_==uiRect::Top ? Alignment::Bottom : Alignment::Top);
    }
    else
    {
	xpix = setup_.side_ == uiRect::Left  ? pix2edge + 5
					     : width_ - pix2edge - 5;
	ypix = pixBefore() + 5;
	al.set( setup_.side_==uiRect::Left ? Alignment::Left : Alignment::Right,
		Alignment::VCenter );
    }

    if ( !endannotitm_ )
	endannotitm_ = scene_->addItem( new uiTextItem(txt,al) );
    else
	endannotitm_->setText( txt );
    endannotitm_->setPos( uiPoint(xpix,ypix) );
    endannotitm_->setZValue( mAxisZValue );
    endannotitm_->setFontData( setup_.fontdata_ );
}


void setLine( uiLineItem* lineitm, const LinePars& lp,
	       const uiAxisHandler* xah, const uiAxisHandler* yah,
	       const Interval<float>* extxvalrg )
{
    if ( !xah || !yah || !lineitm )
	return;

    const Interval<int> ypixrg( yah->pixRange() );
    const Interval<float> yvalrg( yah->getVal(ypixrg.start),
				  yah->getVal(ypixrg.stop) );
    Interval<int> xpixrg( xah->pixRange() );
    Interval<float> xvalrg( xah->getVal(xpixrg.start),
			    xah->getVal(xpixrg.stop) );
    if ( extxvalrg )
    {
	xvalrg = *extxvalrg;
	xpixrg.start = xah->getPix( xvalrg.start );
	xpixrg.stop = xah->getPix( xvalrg.stop );
	xpixrg.sort();
	xvalrg.start = xah->getVal(xpixrg.start);
	xvalrg.stop = xah->getVal(xpixrg.stop);
    }

    uiPoint from(xpixrg.start,ypixrg.start), to(xpixrg.stop,ypixrg.stop);
    if ( lp.ax == 0 )
    {
	const int ypix = yah->getPix( lp.a0 );
	if ( !ypixrg.includes( ypix,true ) ) return;
	from.x = xpixrg.start; to.x = xpixrg.stop;
	from.y = to.y = ypix;
    }
    else
    {
	const float xx0 = xvalrg.start; const float yx0 = lp.getValue( xx0 );
	const float xx1 = xvalrg.stop; const float yx1 = lp.getValue( xx1 );
	const float yy0 = yvalrg.start; const float xy0 = lp.getXValue( yy0 );
	const float yy1 = yvalrg.stop; const float xy1 = lp.getXValue( yy1 );
	const bool yx0ok = yvalrg.includes( yx0,true );
	const bool yx1ok = yvalrg.includes( yx1,true );
	const bool xy0ok = xvalrg.includes( xy0,true );
	const bool xy1ok = xvalrg.includes( xy1,true );

	if ( !yx0ok && !yx1ok && !xy0ok && !xy1ok )
	    return;

	if ( yx0ok )
	{
	    from.x = xah->getPix( xx0 ); from.y = yah->getPix( yx0 );
	    if ( yx1ok )
		{ to.x = xah->getPix( xx1 ); to.y = yah->getPix( yx1 ); }
	    else if ( xy0ok )
		{ to.x = xah->getPix( xy0 ); to.y = yah->getPix( yy0 ); }
	    else if ( xy1ok )
		{ to.x = xah->getPix( xy1 ); to.y = yah->getPix( yy1 ); }
	    else
		return;
	}
	else if ( yx1ok )
	{
	    from.x = xah->getPix( xx1 ); from.y = yah->getPix( yx1 );
	    if ( xy0ok )
		{ to.x = xah->getPix( xy0 ); to.y = yah->getPix( yy0 ); }
	    else if ( xy1ok )
		{ to.x = xah->getPix( xy1 ); to.y = yah->getPix( yy1 ); }
	    else
		return;
	}
	else if ( xy0ok )
	{
	    from.x = xah->getPix( xy0 ); from.y = yah->getPix( yy0 );
	    if ( xy1ok )
		{ to.x = xah->getPix( xy1 ); to.y = yah->getPix( yy1 ); }
	    else
		return;
	}
    }

    lineitm->setLine( from, to );
}

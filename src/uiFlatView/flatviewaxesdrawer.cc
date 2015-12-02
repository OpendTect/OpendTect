/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2007
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "flatviewaxesdrawer.h"
#include "flatposdata.h"
#include "uiflatviewer.h"
#include "uigraphicsview.h"
#include "uigraphicsscene.h"

#define mRemoveAnnotItem( item )\
{ view_.scene().removeItem( item ); delete item; item = 0; }

AxesDrawer::AxesDrawer( uiFlatViewer& vwr )
    : uiGraphicsSceneAxisMgr(vwr.rgbCanvas())
    , vwr_(vwr)
    , altdim0_(-1)
    , rectitem_(0)
    , axis1nm_(0)
    , axis2nm_(0)
    , arrowitem1_(0)
    , arrowitem2_(0)
    , titletxt_(0)
{}


AxesDrawer::~AxesDrawer()
{
    mRemoveAnnotItem( rectitem_ );
    mRemoveAnnotItem( arrowitem1_ );
    mRemoveAnnotItem( axis1nm_ );
    mRemoveAnnotItem( arrowitem2_ );
    mRemoveAnnotItem( axis2nm_ );
    mRemoveAnnotItem( titletxt_ );
}


void AxesDrawer::updateScene()
{
    const FlatView::Annotation& annot  = vwr_.appearance().annot_;
    setAnnotInInt( true, annot.x1_.annotinint_ );
    setAnnotInInt( false, annot.x2_.annotinint_ );
    xaxis_->setup().noannot( !annot.x1_.showannot_ );
    yaxis_->setup().noannot( !annot.x2_.showannot_ );
    xaxis_->setup().nogridline( !annot.x1_.showgridlines_ );
    yaxis_->setup().nogridline( !annot.x2_.showgridlines_ );
    updateViewRect();
    uiGraphicsSceneAxisMgr::updateScene();
}


void AxesDrawer::setZValue( int z )
{
    uiGraphicsSceneAxisMgr::setZValue( z );
    if ( rectitem_ ) rectitem_->setZValue( z+1 );
    if ( axis1nm_ ) axis1nm_->setZValue( z+1 );
    if ( arrowitem1_ ) arrowitem1_->setZValue( z+1 );
    if ( axis2nm_ ) axis2nm_->setZValue( z+1 );
    if ( arrowitem2_ ) arrowitem2_->setZValue( z+1 );
    if ( titletxt_ ) titletxt_->setZValue( z+1 );
}


uiBorder AxesDrawer::getAnnotBorder( bool withextraborders ) const
{
    int l = withextraborders ? extraborder_.left() : 0;
    int r = withextraborders ? extraborder_.right() : 0;
    int t = withextraborders ? extraborder_.top() : 0;
    int b = withextraborders ? extraborder_.bottom() : 0;
    const FlatView::Annotation& annot = vwr_.appearance().annot_;
    const int axisheight = getNeededHeight();
    const int axiswidth = getNeededWidth();
    if ( annot.haveTitle() ) t += axisheight;
    if ( annot.haveAxisAnnot(false) )
    { l += axiswidth; r += 5; }
    if ( annot.haveAxisAnnot(true) )
    { b += axisheight;	t += axisheight; }
    uiBorder annotborder(l,t,r,b);
    return annotborder;
}


uiRect AxesDrawer::getViewRect( bool withextraborders ) const
{
    const uiBorder annotborder( getAnnotBorder(withextraborders) );
    uiRect viewrect = view_.getSceneRect();
    const int sceneborder = view_.getSceneBorder();
    viewrect.translate( -uiPoint(sceneborder,sceneborder) );
    return annotborder.getRect( viewrect );
}


void AxesDrawer::setExtraBorder( const uiBorder& border )
{
    extraborder_ = border;
}


void AxesDrawer::updateViewRect()
{
    const uiRect rect = getViewRect();
    setViewRect( rect );
    setBorder( getAnnotBorder() );
    const FlatView::Annotation& annot  = vwr_.appearance().annot_;
    const FlatView::Annotation::AxisData& ad1 = annot.x1_;
    const FlatView::Annotation::AxisData& ad2 = annot.x2_;
    const bool showx1annot = ad1.showannot_ || ad1.showgridlines_;
    const bool showx2annot = ad2.showannot_ || ad2.showgridlines_;

    if ( !showx1annot && !showx2annot && annot.title_.isEmpty() )
    {
	mRemoveAnnotItem( rectitem_ );
	mRemoveAnnotItem( arrowitem1_ );
	mRemoveAnnotItem( axis1nm_ );
	mRemoveAnnotItem( arrowitem2_ );
	mRemoveAnnotItem( axis2nm_ );
	mRemoveAnnotItem( titletxt_ );
	return;
    }

    if ( !rectitem_ )
	rectitem_ =  view_.scene().addRect( mCast(float,rect.left()),
				            mCast(float,rect.top()),
					    mCast(float,rect.width()),
					    mCast(float,rect.height()) );
    else
	rectitem_->setRect( rect.left(), rect.top(),
			    rect.width(), rect.height() );
    rectitem_->setPenStyle( OD::LineStyle(OD::LineStyle::Solid, 3, annot.color_) );
    
    OD::ArrowStyle arrowstyle;
    arrowstyle.headstyle_.type_ = OD::ArrowHeadStyle::Triangle;
    if ( showx1annot && !ad1.name_.isEmpty() && ad1.name_ != " " )
    {
	const int right = rect.right();
	const int bottom = rect.bottom();
    	uiPoint from( right-10, bottom+9 );
    	uiPoint to( right, bottom+9 );

	if ( ad1.reversed_ ) Swap( from, to );    	
    	if ( !arrowitem1_ )
	    arrowitem1_ = view_.scene().addItem(
		    new uiArrowItem(from,to,arrowstyle) );
	arrowitem1_->setVisible( true );
    	arrowitem1_->setPenStyle( OD::LineStyle(OD::LineStyle::Solid,1,annot.color_) );
    	arrowitem1_->setTailHeadPos( from, to );
    	
    	if ( !axis1nm_ )
	    axis1nm_ = view_.scene().addItem(
		    new uiTextItem(mToUiStringTodo(ad1.name_),
				  mAlignment(Right,Top)) );
    	else
    	    axis1nm_->setText( mToUiStringTodo(ad1.name_) );

	axis1nm_->setVisible( true );
	axis1nm_->setTextColor( annot.color_ );
	axis1nm_->setPos( uiPoint(right-11,bottom-1) );
    }
    else
    {
	if ( arrowitem1_ ) arrowitem1_->setVisible( false );
	if ( axis1nm_ ) axis1nm_->setVisible( false );
    }

    if ( showx2annot && !ad2.name_.isEmpty() && ad2.name_ != " " )
    {
	const int left = rect.left();
	const int bottom = rect.bottom();
	uiPoint from( left , bottom+13 );
	uiPoint to( left, bottom+3 );

	if ( ad2.reversed_ ) Swap( from, to );
	if ( !arrowitem2_ )
	    arrowitem2_ = view_.scene().addItem(
		    new uiArrowItem(from,to,arrowstyle) );
	arrowitem2_->setVisible( true );
	arrowitem2_->setPenColor( annot.color_ );
	arrowitem2_->setTailHeadPos( from, to );
	
	if ( !axis2nm_ )
	    axis2nm_ = view_.scene().addItem(
		    new uiTextItem(mToUiStringTodo(ad2.name_),
				   mAlignment(Left,Top)) );
	else
	    axis2nm_->setText( mToUiStringTodo(ad2.name_) );

	axis2nm_->setVisible( true );
	axis2nm_->setTextColor( annot.color_ );
	axis2nm_->setPos( uiPoint(left+4,bottom-1) );
    }
    else
    {
	if ( arrowitem2_ ) arrowitem2_->setVisible( false );
	if ( axis2nm_ ) axis2nm_->setVisible( false );
    }

    if ( !annot.title_.isEmpty() && annot.title_ != " " )
    {
	if ( !titletxt_ )
	{
	    titletxt_ = view_.scene().addItem(
		    new uiTextItem(mToUiStringTodo(annot.title_),
				   mAlignment(HCenter,Top)) );
	    titletxt_->setTextColor( annot.color_ );
	}
	else
	    titletxt_->setText( mToUiStringTodo(annot.title_) );
	
	titletxt_->setVisible( true );
	const uiRect scenerect = view_.getViewArea();
	titletxt_->setPos( uiPoint(rect.centre().x,scenerect.top()) );
    }
    else if ( titletxt_ )
	titletxt_->setVisible( false );

    setZValue( uiGraphicsSceneAxisMgr::getZValue() );
}


void AxesDrawer::setWorldCoords( const uiWorldRect& wr )
{
    const bool usewva = !vwr_.isVisible( false );
    ConstDataPackRef<FlatDataPack> fdp = vwr_.obtainPack( usewva, true );
    if ( !fdp || altdim0_<0 )
    {
	uiGraphicsSceneAxisMgr::setWorldCoords( wr );
	return;
    }

    const StepInterval<double> dim0rg1 = fdp->posData().range( true );
    const double altdim0start = fdp->getAltDim0Value( altdim0_, 0 );
    const double altdim0stop =
	fdp->getAltDim0Value( altdim0_,dim0rg1.nrSteps() );
    const double altdimdiff = altdim0stop - altdim0start;
    const double altdim0step =
	mIsZero(altdimdiff,mDefEps) || mIsZero(dim0rg1.nrSteps(),mDefEps)
	? 1.0 : altdimdiff/dim0rg1.nrSteps();
    StepInterval<double> dim0rg2( altdim0start, altdim0stop, altdim0step );
    const float startindex = dim0rg1.getfIndex( wr.left() );
    const float stopindex = dim0rg1.getfIndex( wr.right() );
    dim0rg2.start = altdim0start + dim0rg2.step*startindex;
    dim0rg2.stop = altdim0start + dim0rg2.step*stopindex;

    const uiWorldRect altwr( dim0rg2.start,wr.top(),dim0rg2.stop,wr.bottom() );
    uiGraphicsSceneAxisMgr::setWorldCoords( altwr );
}

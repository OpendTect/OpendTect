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
#include "uigraphicsscalebar.h"
#include "survinfo.h"
#include "zaxistransform.h"

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
    , scalebaritem_(0)
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
    mRemoveAnnotItem( scalebaritem_ );
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
    if ( scalebaritem_ ) scalebaritem_->setZValue( z+1 );
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
    { l += axiswidth; r += 10; }
    if ( annot.haveAxisAnnot(true) )
    { b += axisheight;	t += axisheight; }
    if ( annot.showscalebar_ && scalebaritem_ )
	b += scalebaritem_->getPxHeight()*4;
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


static bool isVertical( const uiFlatViewer& vwr )
{
    const bool usewva = !vwr.isVisible( false );
    ConstDataPackRef<FlatDataPack> fdp = vwr.obtainPack( usewva, true );
    if ( !fdp )
	return true;

    FixedString x2dimnm( fdp->dimName(false) );
    FixedString vwrzdomstr( vwr.zDomain().userName().getFullString() );
    return x2dimnm == vwrzdomstr ||
	   stringStartsWithCI("Time",x2dimnm.buf()) ||
	   stringStartsWithCI("TWT",x2dimnm.buf()) ||
	   stringStartsWithCI("Z",x2dimnm.buf());
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

    rectitem_->setPenStyle( OD::LineStyle(OD::LineStyle::Solid,3,annot.color_));

    ArrowStyle arrowstyle;
    arrowstyle.headstyle_.type_ = ArrowHeadStyle::Triangle;
    uiString userfacstr = vwr_.zDomain().uiUnitStr(true);
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
	arrowitem1_->setPenStyle(
		OD::LineStyle(OD::LineStyle::Solid,1,annot.color_) );
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

	uiString x2axisstr( mToUiStringTodo(ad2.name_) );
	if ( isVertical(vwr_) )
	    x2axisstr.append( userfacstr );

	if ( !axis2nm_ )
	    axis2nm_ = view_.scene().addItem(
		    new uiTextItem( x2axisstr, mAlignment(Left,Top)) );
	else
	    axis2nm_->setText( x2axisstr );

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

    if ( annot.showscalebar_ )
    {
	if ( !scalebaritem_ )
	    scalebaritem_ = view_.scene().addItem( new uiScaleBarItem(150) );
	scalebaritem_->setVisible( true );
	scalebaritem_->setPos( view_.mapToScene(uiPoint(view_.width()/2+30,
							view_.height()-20)) );
	scalebaritem_->update();
    }
    else if ( scalebaritem_ )
	scalebaritem_->setVisible( false );

    setZValue( uiGraphicsSceneAxisMgr::getZValue() );
}


void AxesDrawer::transformAndSetAuxAnnotation( bool forx1 )
{
    const bool usewva = !vwr_.isVisible( false );
    ConstDataPackRef<FlatDataPack> fdp = vwr_.obtainPack( usewva, true );
    if ( !fdp )
	return;

    const float userfac = mCast(float,vwr_.zDomain().userFactor());
    const TypeSet<PlotAnnotation>& xannot =
	forx1 ? vwr_.appearance().annot_.x1_.auxannot_
	      : vwr_.appearance().annot_.x2_.auxannot_;
    TypeSet<PlotAnnotation> auxannot = xannot;
    const StepInterval<double> xrg = fdp->posData().range( forx1 );
    for ( int idx=0; idx<xannot.size(); idx++ )
    {
	const int annotposidx = xrg.getIndex( xannot[idx].pos_ );
	auxannot[idx].pos_ = altdim0_>=0 && forx1
	    ? mCast(float,fdp->getAltDim0Value(altdim0_,annotposidx))
	    : xannot[idx].pos_;

	if ( isVertical(vwr_) && !forx1 )
	    auxannot[idx].pos_ *= userfac;
    }

    setAuxAnnotPositions( auxannot, forx1 );
}


void AxesDrawer::setWorldCoords( const uiWorldRect& wr )
{
    const bool usewva = !vwr_.isVisible( false );
    ConstDataPackRef<FlatDataPack> fdp = vwr_.obtainPack( usewva, true );
    transformAndSetAuxAnnotation( true );
    transformAndSetAuxAnnotation( false );
    setScaleBarWorld2UI( wr );
    const float userfac = mCast(float,vwr_.zDomain().userFactor());

    if ( !fdp || altdim0_<0 )
    {
	uiWorldRect altwr = wr;
	if ( fdp && isVertical(vwr_) )
	{
	    altwr.setTop( altwr.top() * userfac );
	    altwr.setBottom( altwr.bottom() * userfac );
	}

	uiGraphicsSceneAxisMgr::setWorldCoords( altwr );
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
    Interval<double> dim1rg( wr.top(), wr.bottom() );
    if ( isVertical(vwr_) )
    {
	dim1rg.start *= userfac;
	dim1rg.stop *= userfac;
    }


    const uiWorldRect altwr( dim0rg2.start, dim1rg.start,
			     dim0rg2.stop, dim1rg.stop );
    uiGraphicsSceneAxisMgr::setWorldCoords( altwr );
}

void AxesDrawer::setScaleBarWorld2UI( const uiWorldRect& wr )
{
    if ( !vwr_.appearance().annot_.showscalebar_ )
	return;

    ConstDataPackRef<FlatDataPack> fdp =
	vwr_.obtainPack( !vwr_.isVisible(false), true );
    if ( !fdp )
	return;

    const StepInterval<double> dim0rg1 = fdp->posData().range( true );
    const float startindex = dim0rg1.getfIndex( wr.left() );
    const float stopindex = dim0rg1.getfIndex( wr.right() );
    const float pos0diststart = fdp->getPosDistance( true, startindex );
    const float pos0diststop = fdp->getPosDistance( true, stopindex );
    uiWorldRect scalebarwr( wr );
    scalebarwr.setLeft( pos0diststart );
    scalebarwr.setRight( pos0diststop );
    uiWorld2Ui scalebarw2ui( vwr_.getViewRect(), scalebarwr );
    if ( !scalebaritem_ )
	    scalebaritem_ = view_.scene().addItem( new uiScaleBarItem(150) );
    scalebaritem_->setVisible( true );
    scalebaritem_->setPos( view_.mapToScene(uiPoint(view_.width()/2+30,
						    view_.height()-20)) );
    scalebaritem_->setWorld2Ui( scalebarw2ui );
    scalebaritem_->update();
}

/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "flatviewaxesdrawer.h"
#include "flatposdata.h"
#include "survinfo.h"
#include "zaxistransform.h"

#include "uiaxishandler.h"
#include "uiflatviewer.h"
#include "uifont.h"
#include "uigraphicscoltab.h"
#include "uigraphicsscalebar.h"
#include "uigraphicsscene.h"
#include "uigraphicsview.h"


static bool isVertical( const uiFlatViewer& vwr )
{
    const bool usewva = !vwr.isVisible( false );
    const WeakPtr<FlatDataPack> datapack = vwr.getPack( usewva, true );
    if ( !datapack )
	return true;

    ConstRefMan<FlatDataPack> fdp = datapack.get();
    StringView x2dimnm( fdp->dimName(false) );
    StringView vwrzdomstr( vwr.zDomain().userName().getFullString().buf() );
    return x2dimnm == vwrzdomstr ||
	   stringStartsWithCI("Time",x2dimnm.buf()) ||
	   stringStartsWithCI("TWT",x2dimnm.buf()) ||
	   stringStartsWithCI("Z",x2dimnm.buf());
}



AxesDrawer::AxesDrawer( uiFlatViewer& vwr )
    : uiGraphicsSceneAxisMgr(vwr.rgbCanvas())
    , vwr_(vwr)
    , rectitem_(nullptr)
    , titletxt_(nullptr)
    , scalebaritem_(nullptr)
    , colorbaritem_(nullptr)
{}


#define mRemoveAnnotItem( item )\
{ view_.scene().removeItem( item ); deleteAndNullPtr( item ); }

AxesDrawer::~AxesDrawer()
{
    mRemoveAnnotItem( rectitem_ );
    mRemoveAnnotItem( titletxt_ );
    mRemoveAnnotItem( scalebaritem_ );
    mRemoveAnnotItem( colorbaritem_ );
}


void AxesDrawer::updateScene()
{
    const FlatView::Annotation& annot  = vwr_.appearance().annot_;
    setAnnotInInt( true, annot.x1_.annotinint_ );
    setAnnotInInt( false, annot.x2_.annotinint_ );
    axis(OD::Top)->setup().noannot( !annot.x1_.showannot_ );
    axis(OD::Left)->setup().noannot( !annot.x2_.showannot_ );
    axis(OD::Right)->setup().noannot( !annot.x2_.showannot_ );
    axis(OD::Top)->setup().nogridline( !annot.x1_.showgridlines_ );
    axis(OD::Left)->setup().nogridline( !annot.x2_.showgridlines_ );
    axis(OD::Right)->setup().nogridline( !annot.x2_.showgridlines_ );

    uiString x2axisstr( toUiString(annot.x2_.name_) );
    if ( isVertical(vwr_) )
	x2axisstr.addSpace().append( vwr_.zDomain().uiUnitStr(true) );
    axis(OD::Left)->setup().caption( x2axisstr );

    if ( annot.x1_.name_.isEmpty() )
    {
// Hack
	axis(OD::Bottom)->setup().noannot( true );
	axis(OD::Bottom)->setup().nogridline( true );
    }
    else
    {
	axis(OD::Bottom)->setup().caption( toUiString(annot.x1_.name_) );
	axis(OD::Bottom)->setup().noannot( !annot.x1_.showannot_ );
	axis(OD::Bottom)->setup().nogridline( !annot.x1_.showgridlines_ );
    }

    updateViewRect();
    uiGraphicsSceneAxisMgr::updateScene();
}


void AxesDrawer::setZValue( int z )
{
    uiGraphicsSceneAxisMgr::setZValue( z );
    if ( rectitem_ ) rectitem_->setZValue( z+1 );
    if ( titletxt_ ) titletxt_->setZValue( z+1 );
    if ( scalebaritem_ ) scalebaritem_->setZValue( z+1 );
    if ( colorbaritem_ ) colorbaritem_->setZValue( z+1 );
}


uiBorder AxesDrawer::getAnnotBorder( bool withextraborders ) const
{
    int l = withextraborders ? extraborder_.left() : 0;
    int r = withextraborders ? extraborder_.right() : 0;
    int t = withextraborders ? extraborder_.top() : 0;
    int b = withextraborders ? extraborder_.bottom() : 0;
    const int axisheight = getNeededHeight();
    const int axiswidth = getNeededWidth();

    const FlatView::Annotation& annot = vwr_.appearance().annot_;
    if ( annot.x2_.hasannot_ )
    {
	l += axiswidth;
	r += axiswidth;
    }

    if ( annot.x1_.hasannot_ )
    {
	t += axisheight*3; // Should be enough space for title

	if ( !annot.x1_.name_.isEmpty() )
	    b += axisheight*3;
    }

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


void AxesDrawer::setTitleFont( const FontData& fd )
{
    if ( titletxt_ )
	titletxt_->setFontData( fd );
}


void AxesDrawer::updateViewRect()
{
    const uiRect rect = getViewRect();
    if ( rect.right() < rect.left() || rect.bottom() < rect.top() )
	return;

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

    if ( !annot.title_.isEmpty() && annot.title_ != " " )
    {
	// Add some spaces to avoid text truncations
	const uiString titlestr = toUiString( "   %1   " ).arg( annot.title_ );
	if ( !titletxt_ )
	{
	    titletxt_ = view_.scene().addItem(
		    new uiTextItem(titlestr,mAlignment(HCenter,Top)) );
	    titletxt_->setTextColor( annot.color_ );
	    FontData fd = FontList().get( FontData::Graphics2D ).fontData();
	    fd.setPointSize( fd.pointSize()+2 );
	    fd.setWeight( FontData::Bold );
	    titletxt_->setFontData( fd );
	}
	else
	    titletxt_->setText( titlestr );

	titletxt_->setVisible( true );
	const uiRect scenerect = view_.getViewArea();
	titletxt_->setPos( uiPoint(rect.centre().x,scenerect.top()) );
    }
    else if ( titletxt_ )
	titletxt_->setVisible( false );

    if ( annot.showscalebar_ )
    {
	const int sbwidth = 150;
	if ( !scalebaritem_ )
	    scalebaritem_ = view_.scene().addItem(new uiScaleBarItem(sbwidth));

	const int sbheight = scalebaritem_->getPxHeight();
	scalebaritem_->setPos( uiPoint(rect.right(),rect.top()-sbheight*6) );
    }

    if ( scalebaritem_ )
    {
	scalebaritem_->setVisible( annot.showscalebar_ );
	scalebaritem_->update();
    }

    if ( annot.showcolorbar_ )
    {
	if ( !colorbaritem_ )
	{
	    uiColTabItem::Setup itmsetup( false );
	    colorbaritem_ = view_.scene().addItem( new uiColTabItem(itmsetup) );
	}

	colorbaritem_->setPos( uiPoint(rect.left(),rect.top()-20) );
    }
    if ( colorbaritem_ )
    {
	colorbaritem_->setVisible( annot.showcolorbar_ );
    }

    setZValue( uiGraphicsSceneAxisMgr::getZValue() );
}


void AxesDrawer::transformAndSetAuxAnnotation( bool forx1 )
{
    const bool usewva = !vwr_.isVisible( false );
    const WeakPtr<FlatDataPack> datapack = vwr_.getPack( usewva, true );
    if ( !datapack )
	return;

    ConstRefMan<FlatDataPack> fdp = datapack.get();
    const float userfac = mCast(float,vwr_.zDomain().userFactor());
    const TypeSet<PlotAnnotation>& xannot =
	forx1 ? vwr_.appearance().annot_.x1_.auxannot_
	      : vwr_.appearance().annot_.x2_.auxannot_;
    TypeSet<PlotAnnotation> auxannot = xannot;
    const StepInterval<double> xrg = fdp->posData().range( forx1 );
    for ( int idx=0; idx<xannot.size(); idx++ )
    {
	if ( !xrg.includes(xannot[idx].pos_,false) )
	    continue;

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
    ConstRefMan<FlatDataPack> fdp = vwr_.getPack( usewva, true ).get();
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

    const WeakPtr<FlatDataPack> datapack =
				vwr_.getPack( !vwr_.isVisible(false), true );
    if ( !datapack )
	return;

    ConstRefMan<FlatDataPack> fdp = datapack.get();
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
    scalebaritem_->setPos( view_.mapToScene(uiPoint(view_.viewWidth()/2+30,
						    view_.viewHeight()-20)) );
    scalebaritem_->setWorld2Ui( scalebarw2ui );
    scalebaritem_->update();
}

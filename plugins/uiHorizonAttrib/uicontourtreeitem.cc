/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Raman Singh
 Date:          Feb 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uicontourtreeitem.cc,v 1.3 2009-03-10 06:02:20 cvsraman Exp $";


#include "uicontourtreeitem.h"

#include "arrayndimpl.h"
#include "attribsel.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "isocontourtracer.h"
#include "linear.h"
#include "polygon.h"
#include "survinfo.h"

#include "mousecursor.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uilistview.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uioddisplaytreeitem.h"
#include "uiodscenemgr.h"
#include "uisellinest.h"
#include "uivispartserv.h"

#include "viscoord.h"
#include "visdrawstyle.h"
#include "vishorizondisplay.h"
#include "vismaterial.h"
#include "vispolyline.h"
#include "vistext.h"
#include "vistransform.h"


static const int cMinNrNodesForLbl = 25;

mClass uiContourParsDlg : public LineStyleDlg
{
public:
uiContourParsDlg( uiParent* p, const Interval<float>& rg,
       		  const SamplingData<int>& sd, const LineStyle& ls )
    : LineStyleDlg(p,ls,"Specify Contour Parameters",false,true,true)
    , rg_(rg)
    , sd_(sd)
    , propertyChanged(this)
{
    setCaption( "Contour Display Options" );
    setTitleText( mNoDlgTitle );
    BufferString lbltxt = "Z Range "; lbltxt += SI().getZUnitString();
    lbltxt += ": "; lbltxt += mNINT( rg.start );
    lbltxt += " to "; lbltxt += mNINT( rg.stop );
    uiLabel* lbl = new uiLabel( this, lbltxt.buf() );

    stepfld_ = new uiGenInput( this, "Contour step", IntInpSpec(sd.step) );
    stepfld_->valuechanged.notify( mCB(this,uiContourParsDlg,stepChanged) );
    lbl->attach( leftAlignedAbove, stepfld_ );

    startfld_ = new uiGenInput( this, "First contour", IntInpSpec(sd.start) );
    startfld_->valuechanged.notify( mCB(this,uiContourParsDlg,startChanged) );
    startfld_->attach( alignedBelow, stepfld_ );

    nrcontourlbl_ = new uiLabel( this, "Number of contours" );
    nrcontourlbl_->setPrefWidthInChar( 25 );
    nrcontourlbl_->attach( leftAlignedBelow, startfld_ );

    lsfld->attach( alignedBelow, startfld_ );
    lsfld->attach( ensureBelow, nrcontourlbl_ );
    lsfld->changed.notify( mCB(this,uiContourParsDlg,lsChanged) );

    stepChanged( 0 );
}

SamplingData<int> getContourSampling() const
{
    SamplingData<int> sd( startfld_->getIntValue(), stepfld_->getIntValue() );
    return sd;
}

    Notifier<uiContourParsDlg>	propertyChanged;

protected:

void lsChanged( CallBacker* )
{
    propertyChanged.trigger();
}

void stepChanged( CallBacker* cb )
{
    const int step = stepfld_->getIntValue();
    if ( step < 1 || step > (int)rg_.width() )
    {
	if ( cb )
	    uiMSG().error( "Please specify a valid value for step" );

	stepfld_->setValue( sd_.step );
	return;
    }

    const int start = startfld_->getIntValue();
    const int nrcontours = ( (int)rg_.stop - start ) / step + 1;
    BufferString lbltxt( "Number of contours: " );
    lbltxt += nrcontours;
    nrcontourlbl_->setText( lbltxt.buf() );
    sd_.step = step;
}

void startChanged( CallBacker* )
{
    const int start = startfld_->getIntValue();
    if ( start < rg_.start )
	startfld_->setValue( ceil(rg_.start) );
    else if ( start > rg_.stop )
	startfld_->setValue( (int)rg_.stop );

    stepChanged( 0 );
}

    Interval<float>	rg_;
    SamplingData<int>	sd_;
    uiGenInput*		startfld_;
    uiGenInput*		stepfld_;
    uiLabel*		nrcontourlbl_;
};


mClass visContourLabels : public visBase::VisualObjectImpl
{
public:

visContourLabels()
    : VisualObjectImpl(false)
    , transformation_(0)
{}

~visContourLabels()
{
    if ( transformation_ )
	transformation_->unRef();
}

void addLabel( visBase::Text2* label )
{
    label->setDisplayTransformation( transformation_ );
    addChild( label->getInventorNode() );
}

void setDisplayTransformation( visBase::Transformation* nt )
{
    if ( transformation_ ) transformation_->unRef();
    transformation_ = nt;
    if ( transformation_ ) transformation_->ref();
}

    visBase::Transformation*	transformation_;
};


void uiContourTreeItem::initClass()
{ uiODDataTreeItem::factory().addCreator( create, 0 ); }

uiContourTreeItem::uiContourTreeItem( const char* parenttype )
    : uiODDataTreeItem( parenttype )
    , optionsmenuitem_( "Options ..." )
    , lines_( 0 )
    , drawstyle_( 0 )
    , material_(0)
    , labelgrp_(0)
    , linewidth_( 1 )
    , arr_( 0 )
    , rg_(mUdf(float),-mUdf(float))
    , contoursampling_(-1,-1)
{
    ODMainWin()->applMgr().visServer()->removeAllNotifier().notify(
	    mCB(this,uiContourTreeItem,visClosingCB) );
}


uiContourTreeItem::~uiContourTreeItem()
{
    delete arr_;

    applMgr()->visServer()->removeAllNotifier().remove(
	    mCB(this,uiContourTreeItem,visClosingCB) );

    if ( lines_ || drawstyle_ )
	pErrMsg("prepareForShutdown not run");

    if ( !parent_ )
	return;

    parent_->checkStatusChange()->remove(mCB(this,uiContourTreeItem,checkCB));
}


void uiContourTreeItem::prepareForShutdown()
{
    visClosingCB( 0 );
}


bool uiContourTreeItem::init()
{
    if ( !uiODDataTreeItem::init() )
	return false;

    uilistviewitem_->setChecked( true );
    parent_->checkStatusChange()->notify(mCB(this,uiContourTreeItem,checkCB));
    return true;
}


uiODDataTreeItem* uiContourTreeItem::create( const Attrib::SelSpec& as,
					       const char* parenttype )
{
    return new uiContourTreeItem(parenttype);
}


void uiContourTreeItem::checkCB(CallBacker*)
{
    bool newstatus = uilistviewitem_->isChecked();
    if ( newstatus && parent_ )
	newstatus = parent_->isChecked();

    if ( lines_ ) lines_->turnOn( newstatus );
    if ( labelgrp_ ) labelgrp_->turnOn( newstatus );
}


void uiContourTreeItem::visClosingCB( CallBacker* )
{
    removeAll();
}


void uiContourTreeItem::removeAll()
{
    if ( lines_ )
    {
	applMgr()->visServer()->removeObject( lines_, sceneID() );

	lines_->unRef();
	lines_ = 0;
    }

    if ( drawstyle_ )
    {
	drawstyle_->unRef();
	drawstyle_ = 0;
    }

    if ( material_ )
    {
	material_->unRef();
	material_ = 0;
    }

    removeLabels();
}


void uiContourTreeItem::removeLabels()
{
    uiVisPartServer* visserv = applMgr()->visServer();
    if ( labelgrp_ )
    {
	visserv->removeObject( labelgrp_, sceneID() );
	labelgrp_->unRef();
	labelgrp_ = 0;
    }

    deepUnRef( labels_ );
    labels_.erase();
}


void uiContourTreeItem::createMenuCB( CallBacker* cb )
{
    uiODDataTreeItem::createMenuCB( cb );
    mDynamicCastGet(MenuHandler*,menu,cb);

    mAddMenuItem( menu, &optionsmenuitem_, lines_, false );
}


void uiContourTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDataTreeItem::handleMenuCB( cb );
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(MenuHandler*,menu,caller);
    if ( mnuid==-1 || menu->isHandled() || mnuid!=optionsmenuitem_.id )
	return;

    if ( !lines_ )
	return;

    menu->setIsHandled( true );
    const float fac = SI().zFactor();
    Interval<float> range( rg_.start*fac, rg_.stop*fac );
    uiContourParsDlg dlg( ODMainWin(), range, contoursampling_,
	    		  LineStyle(LineStyle::Solid,linewidth_,color_) );
    dlg.propertyChanged.notify( mCB(this,uiContourTreeItem,propChangeCB) );
    const bool res = dlg.go();
    dlg.propertyChanged.remove( mCB(this,uiContourTreeItem,propChangeCB) );
    if ( !res ) return;

    SamplingData<int> sd = dlg.getContourSampling();
    if ( sd != contoursampling_ )
    {
	contoursampling_ = sd;
	computeContours();
    }
}


void uiContourTreeItem::propChangeCB( CallBacker* cb )
{
    mDynamicCastGet(uiContourParsDlg*,dlg,cb);
    if ( !dlg || !lines_ ) return;

    LineStyle ls( dlg->getLineStyle() );
    drawstyle_->setLineStyle( ls );
    material_->setColor( ls.color_ );
    color_ = ls.color_;
    linewidth_ = ls.width_;
}


void uiContourTreeItem::computeContours()
{
    uiVisPartServer* visserv = applMgr()->visServer();
    mDynamicCastGet(visSurvey::HorizonDisplay*,hd,
	    	    visserv->getObject(displayID()))
    if ( !hd )
	return;

    MouseCursorChanger cursorchanger( MouseCursor::Wait );
    StepInterval<int> rowrg = hd->displayedRowRange();
    StepInterval<int> colrg = hd->displayedColRange();
    const int nrbids = ( rowrg.nrSteps() + 1 ) * ( colrg.nrSteps() + 1 );

    createLines();
    if ( !lines_ ) return;

    EM::ObjectID emid = hd->getObjectID();
    mDynamicCastGet(EM::Horizon3D*,hor,EM::EMM().getObject(emid));
    if ( !hor ) return;

    EM::SectionID sid = hor->sectionID( 0 );
    Array2D<float>* field = hor->geometry().sectionGeometry(sid)->getArray();
    if ( hd->getDataTransform() )
	field = hor->createArray2D( sid, hd->getDataTransform() );

    if ( !field )
	return;

    const float fac = SI().zFactor();
    IsoContourTracer ictracer( *field );
    ictracer.setSampling( hor->geometry().rowRange(sid),
	    		  hor->geometry().colRange(sid) );
    if ( mIsUdf(rg_.start) )
    {
	for ( int idx=0; idx<field->info().getSize(0); idx++ )
	{
	    for ( int idy=0; idy<field->info().getSize(1); idy++ )
	    {
		const float val = field->get( idx, idy );
		if ( !mIsUdf(val) ) rg_.include( val, false );
	    }
	}
	
	if ( mIsUdf(rg_.start) )
	    return;

	AxisLayout al( rg_ );
	SamplingData<float> sd = al.sd;
	sd.step /= 5;
	const float offset = ( sd.start - rg_.start ) / sd.step;
	if ( offset < 0 || offset > 1 )
	{
	    const int nrsteps = mNINT( floor(offset) );
	    sd.start -= nrsteps * sd.step;
	}

	contoursampling_.start = mNINT( sd.start * fac );
	contoursampling_.step = mNINT( sd.step * fac );
    }

    if ( contoursampling_.start < 0 || contoursampling_.step < 0 )
	return;

    int cii = 0;
    float contourval = contoursampling_.start;
    lines_->getCoordinates()->removeAfter( -1 );
    lines_->removeCoordIndexAfter( -1 );
    removeLabels();
    while ( contourval < rg_.stop*fac )
    {
	ObjectSet<ODPolygon<float> > isocontours;
	ictracer.getContours( isocontours, contourval/fac, false );
	for ( int cidx=0; cidx<isocontours.size(); cidx++ )
	{
	    const ODPolygon<float>& ic = *isocontours[cidx];
	    for ( int vidx=0; vidx<ic.size(); vidx++ )
	    {
		const Geom::Point2D<float> vertex = ic.getVertex( vidx );
		Coord vrtxcoord( vertex.x, vertex.y );
		vrtxcoord = SI().binID2Coord().transform( vrtxcoord );
		const Coord3 pos( vrtxcoord, contourval/fac );
		const int posidx = lines_->getCoordinates()->addPos( pos );
		lines_->setCoordIndex( cii++, posidx );
		if ( ic.size() > cMinNrNodesForLbl  && vidx == ic.size()/2 )
		    addText( pos, getStringFromInt(mNINT(contourval)) );
	    }
	    
	    if ( ic.isClosed() )
	    {
		const int posidx = lines_->getCoordIndex( cii-ic.size() );
		lines_->setCoordIndex( cii++, posidx );
	    }
	    
	    lines_->setCoordIndex( cii++, -1 );
	}

	deepErase( isocontours );
	contourval += contoursampling_.step;
    }

    lines_->getCoordinates()->removeAfter( cii-1 );
    lines_->removeCoordIndexAfter( cii-1 );
    if ( hd->getDataTransform() )
	delete field;
}


void uiContourTreeItem::createLines()
{
    if ( lines_ )
	return;

    lines_ = visBase::IndexedPolyLine::create();
    lines_->ref();
    applMgr()->visServer()->addObject( lines_, sceneID(), false );
    if ( !drawstyle_ )
    {
	drawstyle_ = visBase::DrawStyle::create();
	drawstyle_->ref();
	if ( lines_ ) lines_->insertNode( drawstyle_->getInventorNode() );
    }

    if ( !material_ )
    {
	material_ = visBase::Material::create();
	material_->ref();
	if ( lines_ ) lines_->insertNode( material_->getInventorNode() );
    }
}


void uiContourTreeItem::addText( const Coord3& pos, const char* txt )
{
    visBase::Text2* label = visBase::Text2::create();
    label->setText( txt );
    label->setPosition( pos );
    label->setSize( 12 );
    label->setMaterial( material_ );
    label->ref();
    if ( !labelgrp_ )
    {
	labelgrp_ = new visContourLabels;
	labelgrp_->ref();
	applMgr()->visServer()->addObject( labelgrp_, sceneID(), false );
    }

    labelgrp_->addLabel( label );
    labels_ += label;
}


void uiContourTreeItem::updateColumnText( int col )
{
    uiODDataTreeItem::updateColumnText( col );
    if ( !col && !lines_ )
	computeContours();
}


BufferString uiContourTreeItem::createDisplayName() const
{
    return BufferString( "Contours" );
}

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman Singh
 Date:          Feb 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uicontourtreeitem.cc,v 1.15 2011-02-09 16:01:42 cvskris Exp $";


#include "uicontourtreeitem.h"

#include "arrayndimpl.h"
#include "attribsel.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "isocontourtracer.h"
#include "axislayout.h"
#include "mousecursor.h"
#include "polygon.h"
#include "survinfo.h"

#include "uidialog.h"
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

class uiContourParsDlg : public uiDialog
{
public:
uiContourParsDlg( uiParent* p, const Interval<float>& rg,
       		  const StepInterval<float>& intv, const LineStyle& ls )
    : uiDialog(p,Setup("Contour Display Options",mNoDlgTitle,"104.3.2")
		 .nrstatusflds(1))
    , rg_(rg)
    , contourintv_(intv)
    , propertyChanged(this)
{
    BufferString lbltxt = "Total Z Range "; lbltxt += SI().getZUnitString();
    lbltxt += ": "; lbltxt += rg.start;
    lbltxt += " - "; lbltxt += rg.stop;
    uiLabel* lbl = new uiLabel( this, lbltxt.buf() );

    lbltxt = "Contour range "; lbltxt += SI().getZUnitString();
    intvfld_ = new uiGenInput( this, lbltxt, FloatInpIntervalSpec(intv) );
    intvfld_->valuechanged.notify( mCB(this,uiContourParsDlg,intvChanged) );
    intvfld_->attach( leftAlignedBelow, lbl );

    lsfld_ = new uiSelLineStyle( this, ls, 0, false, true, true );
    lsfld_->attach( alignedBelow, intvfld_ );
    lsfld_->changed.notify( mCB(this,uiContourParsDlg,lsChanged) );

    intvChanged( 0 );
}

const LineStyle& getLineStyle() const
{ return lsfld_->getStyle(); }

StepInterval<float> getContourInterval() const
{ return intvfld_->getFStepInterval(); }

    Notifier<uiContourParsDlg>	propertyChanged;

protected:

void lsChanged( CallBacker* )
{
    propertyChanged.trigger();
}

void intvChanged( CallBacker* cb )
{
    StepInterval<float> intv = intvfld_->getFStepInterval();
    if ( intv.start < rg_.start || intv.start > rg_.stop )
	intvfld_->setValue( contourintv_.start, 0 );
    if ( intv.stop < rg_.start || intv.stop > rg_.stop )
	intvfld_->setValue( contourintv_.stop, 1 );

    if ( intv.step <= 0 || intv.step > rg_.width() )
    {
	if ( cb )
	    uiMSG().error( "Please specify a valid value for step" );

	intvfld_->setValue( contourintv_.step, 2 );
	return;
    }

    intv = intvfld_->getFStepInterval();
    contourintv_.step = intv.step;

    BufferString txt( "Number of contours: ", intv.nrSteps()+1 );
    toStatusBar( txt );
}

    Interval<float>	rg_;
    StepInterval<float>	contourintv_;
    uiGenInput*		intvfld_;
    uiSelLineStyle*	lsfld_;
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



const char* uiContourTreeItem::sKeyContourDefString()
{ return "Countour Display"; }

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
{
    ODMainWin()->applMgr().visServer()->removeAllNotifier().notify(
	    mCB(this,uiContourTreeItem,visClosingCB) );
}


uiContourTreeItem::~uiContourTreeItem()
{
    delete arr_;
    
    mDynamicCastGet( visSurvey::HorizonDisplay*, hd,
	    applMgr()->visServer()->getObject(displayID()))
    if ( hd )
	hd->getMovementNotifier()->remove(mCB(this,uiContourTreeItem,checkCB));

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
    BufferString defstr = as.defString();
    return defstr == sKeyContourDefString() ? new uiContourTreeItem(parenttype)
					    : 0;
}


void uiContourTreeItem::checkCB(CallBacker*)
{
    bool newstatus = uilistviewitem_->isChecked();
    if ( newstatus && parent_ )
	newstatus = parent_->isChecked();

    mDynamicCastGet( visSurvey::HorizonDisplay*, hd,
	    applMgr()->visServer()->getObject( displayID() ) );
    const bool display = newstatus && hd && !hd->getOnlyAtSectionsDisplay();
    
    if ( lines_ ) lines_->turnOn( display );
    if ( labelgrp_ ) labelgrp_->turnOn( display );
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
    Interval<float> range( rg_.start, rg_.stop ); range.scale( fac );
    StepInterval<float> intv( contourintv_ ); intv.scale( fac );
    uiContourParsDlg dlg( ODMainWin(), range, intv,
	    		  LineStyle(LineStyle::Solid,linewidth_,color_) );
    dlg.propertyChanged.notify( mCB(this,uiContourTreeItem,propChangeCB) );
    const bool res = dlg.go();
    dlg.propertyChanged.remove( mCB(this,uiContourTreeItem,propChangeCB) );
    if ( !res ) return;

    intv = dlg.getContourInterval();
    intv.scale( 1/fac );
    if ( intv != contourintv_ )
    {
	contourintv_ = intv;
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

    hd->getMovementNotifier()->notify( mCB(this,uiContourTreeItem,checkCB) );

    MouseCursorChanger cursorchanger( MouseCursor::Wait );
    StepInterval<int> rowrg = hd->geometryRowRange();
    StepInterval<int> colrg = hd->geometryColRange();
    const int nrbids = ( rowrg.nrSteps() + 1 ) * ( colrg.nrSteps() + 1 );

    createLines();
    if ( !lines_ ) return;

    EM::ObjectID emid = hd->getObjectID();
    mDynamicCastGet(EM::Horizon3D*,hor,EM::EMM().getObject(emid));
    if ( !hor ) return;

    EM::SectionID sid = hor->sectionID( 0 );
    Array2D<float>* field = hor->geometry().sectionGeometry(sid)->getArray();
    if ( hd->getZAxisTransform() )
	field = hor->createArray2D( sid, hd->getZAxisTransform() );

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

	contourintv_.start = sd.start;
	contourintv_.stop = rg_.stop;
	contourintv_.step = sd.step;
	const int nrsteps = contourintv_.nrSteps();
	contourintv_.stop = sd.start + nrsteps*sd.step;
    }

    if ( contourintv_.step <= 0 )
	return;

    const char* fmt = SI().zIsTime() ? "%g" : "%f";
    int cii = 0;
    float contourval = contourintv_.start;
    lines_->getCoordinates()->removeAfter( -1 );
    lines_->removeCoordIndexAfter( -1 );
    removeLabels();
    const float maxcontourval = mMIN(contourintv_.stop,rg_.stop);
    while ( contourval < maxcontourval+mDefEps )
    {
	ObjectSet<ODPolygon<float> > isocontours;
	ictracer.getContours( isocontours, contourval, false );
	for ( int cidx=0; cidx<isocontours.size(); cidx++ )
	{
	    const ODPolygon<float>& ic = *isocontours[cidx];
	    char buf[255];
	    for ( int vidx=0; vidx<ic.size(); vidx++ )
	    {
		const Geom::Point2D<float> vertex = ic.getVertex( vidx );
		Coord vrtxcoord( vertex.x, vertex.y );
		vrtxcoord = SI().binID2Coord().transform( vrtxcoord );
		const Coord3 pos( vrtxcoord, contourval );
		const int posidx = lines_->getCoordinates()->addPos( pos );
		lines_->setCoordIndex( cii++, posidx );
		if ( ic.size() > cMinNrNodesForLbl  && vidx == ic.size()/2 )
		    addText( pos, getStringFromFloat(fmt,contourval*fac, buf) );
	    }
	    
	    if ( ic.isClosed() )
	    {
		const int posidx = lines_->getCoordIndex( cii-ic.size() );
		lines_->setCoordIndex( cii++, posidx );
	    }
	    
	    lines_->setCoordIndex( cii++, -1 );
	}

	deepErase( isocontours );
	contourval += contourintv_.step;
    }

    lines_->getCoordinates()->removeAfter( cii-1 );
    lines_->removeCoordIndexAfter( cii-1 );
    if ( hd->getZAxisTransform() )
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
    label->setFontData( FontData(12) );
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

    uiVisPartServer* visserv = applMgr()->visServer();
    mDynamicCastGet(const visSurvey::HorizonDisplay*,hd,
		    visserv->getObject(displayID()))
    if ( !hd || !lines_ || !labelgrp_ ) return;

    const bool solomode = visserv->isSoloMode();
    const bool turnon = !hd->getOnlyAtSectionsDisplay() &&
       ( (solomode && hd->isOn()) || (!solomode && hd->isOn() && isChecked()) );
    lines_->turnOn( turnon );
    labelgrp_->turnOn( turnon );
}


BufferString uiContourTreeItem::createDisplayName() const
{
    return BufferString( "Contours" );
}

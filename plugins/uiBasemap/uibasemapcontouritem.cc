#include "uibasemapcontouritem.h"

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Henrique Mageste
 Date:		February 2014
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";


#include "basemapcontour.h"

#include "uibasemap.h"
#include "uiioobjselgrp.h"
#include "uigeninput.h"
#include "uimenu.h"
#include "uisellinest.h"
#include "uistrings.h"
#include "uitaskrunner.h"

#include "emhorizon3d.h"
#include "emioobjinfo.h"
#include "emmanager.h"
#include "emsurfacetr.h"

#include "axislayout.h"
#include "ctxtioobj.h"
#include "ioman.h"
#include "survinfo.h"

const char* sKeySpacing()   { return "Spacing"; }

// uiBasemapContourGroup
uiBasemapContourGroup::uiBasemapContourGroup( uiParent* p, bool isadd )
    : uiBasemapGroup(p)
    , mid_(MultiID::udf())
{
    if ( isadd )
    {
	ioobjfld_ = new uiIOObjSelGrp( this, mIOObjContext(EMHorizon3D));
	ioobjfld_->selectionChanged.notify(
				mCB(this,uiBasemapContourGroup,selChg));
    }

    BufferString lbltxt( "Contour range " );
    lbltxt.add( SI().getZUnitString() );
    spacingfld_ = new uiGenInput( this, lbltxt, FloatInpIntervalSpec(true) );

    if ( isadd )
	spacingfld_->attach( alignedBelow, ioobjfld_ );

    uiSelLineStyle::Setup stu; stu.drawstyle( false );
    LineStyle lst( LineStyle::Solid, 1, Color(0,170,0,0) );
    lsfld_ = new uiSelLineStyle( this, lst, stu );
    lsfld_->attach( alignedBelow, spacingfld_ );

    addNameField();
}


uiBasemapContourGroup::~uiBasemapContourGroup()
{
}


void uiBasemapContourGroup::selChg(CallBacker *)
{
    mid_ = ioobjfld_->currentID();

    setParameters();
}


void uiBasemapContourGroup::setParameters()
{
    setItemName( IOM().nameOf(mid_) );
    EM::IOObjInfo eminfo( mid_ );
    const float userfac = mCast(float,SI().zDomain().userFactor());
    Interval<float> zrange = eminfo.getZRange();
    zrange.scale( userfac );
    AxisLayout<float> al( zrange, false, true );
    spacingfld_->setValue( al.getSampling() );
}


bool uiBasemapContourGroup::acceptOK()
{
    const bool res = uiBasemapGroup::acceptOK();

    return ( res || !mid_.isUdf() );
}


bool uiBasemapContourGroup::fillPar( IOPar& par ) const
{
    par.set( sKey::NrItems(), 1 );

    IOPar ipar;
    const bool res = uiBasemapGroup::fillPar( ipar );

    ipar.set( sKey::Horizon(), mid_ );

    ipar.set( sKeyNrObjs(), 1 );
    StepInterval<float> intv = spacingfld_->getFStepInterval();
    intv.scale( 1.f/SI().zDomain().userFactor() );
    ipar.set( sKeySpacing(), intv );

    BufferString lsstr;
    lsfld_->getStyle().toString( lsstr );
    ipar.set( sKey::LineStyle(), lsstr );

    const BufferString key = IOPar::compKey( sKeyItem(), 0 );
    par.mergeComp( ipar, key );

    return res;
}


bool uiBasemapContourGroup::usePar( const IOPar& par )
{
    const bool res = uiBasemapGroup::usePar( par );

    par.get( sKey::Horizon(), mid_ );

    StepInterval<float> spacing;
    par.get( sKeySpacing(), spacing );
    spacing.scale( SI().zDomain().userFactor() );
    spacingfld_->setValue( spacing );

    BufferString lsstr;
    par.get( sKey::LineStyle(), lsstr );
    LineStyle ls;
    ls.fromString( lsstr );
    lsfld_->setStyle( ls );

    return res;
}


uiObject* uiBasemapContourGroup::lastObject()
{ return lsfld_->attachObj(); }


// uiBasemapContourTreeItem
uiBasemapContourTreeItem::uiBasemapContourTreeItem( const char* nm )
    : uiBasemapTreeItem(nm)
{
}


uiBasemapContourTreeItem::~uiBasemapContourTreeItem()
{
}


bool hasChanged( const IOPar& prevpar, const IOPar& par, const char* key )
{
    return prevpar[key] != par[key];
}


bool uiBasemapContourTreeItem::usePar( const IOPar& par )
{
    const IOPar prevpar = pars_;
    uiBasemapTreeItem::usePar( par );

    StepInterval<float> spacing;
    par.get( sKeySpacing(), spacing );

    BufferString lsstr;
    par.get( sKey::LineStyle(), lsstr );
    LineStyle ls;
    ls.fromString( lsstr );

    //const bool lschgd = ls != prevls;

    MultiID mid;
    par.get( sKey::Horizon(), mid );
    uiTaskRunner uitr( &BMM().getBasemap() );
    if ( basemapobjs_.isEmpty() )
    {
	Basemap::ContourObject* obj = new Basemap::ContourObject();
	obj->setMultiID( mid, &uitr );
	addBasemapObject( *obj );
    }

    mDynamicCastGet(Basemap::ContourObject*,obj,basemapobjs_[0])
    if ( obj )
    {
	obj->setContours( spacing, ls, &uitr );
	obj->updateGeometry();
    }



    return true;
}


bool uiBasemapContourTreeItem::showSubMenu()
{
    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    mnu.insertItem( new uiAction(uiStrings::sEdit(false)), 0 );
    const int mnuid = mnu.exec();
    return handleSubMenu( mnuid );
}


bool uiBasemapContourTreeItem::handleSubMenu( int mnuid )
{
    if ( mnuid==0 )
	BMM().edit( getFamilyID(), ID() );
    else
	return false;

    return true;
}

// uiBasemapContourItem
const char* uiBasemapContourItem::iconName() const
{ return "basemap-contours"; }

uiBasemapGroup* uiBasemapContourItem::createGroup( uiParent* p, bool isadd )
{ return new uiBasemapContourGroup( p, isadd ); }

uiBasemapTreeItem* uiBasemapContourItem::createTreeItem( const char* nm )
{ return new uiBasemapContourTreeItem( nm ); }

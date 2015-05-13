/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Henrique Mageste
 Date:		November 2014
 RCS:		$Id$
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uibasemapseisoutlineitem.h"

#include "basemapseisoutline.h"

#include "uicolor.h"
#include "uigeninput.h"
#include "uimenu.h"
#include "uisellinest.h"
#include "uistrings.h"

#include "axislayout.h"
#include "draw.h"
#include "seistrctr.h"
#include "survinfo.h"

static const char* sKeyHasLines()	{ return "Has Lines"; }
static const char* sKeyFilled()		{ return "Filled"; }

// uiBasemapSeisOutlineGroup
uiBasemapSeisOutlineGroup::uiBasemapSeisOutlineGroup( uiParent* p, bool isadd )
    : uiBasemapIOObjGroup(p,*Seis::getIOObjContext(Seis::Vol,true),isadd)
{
    uiSelLineStyle::Setup stu; stu.drawstyle( false );
    LineStyle lst;
    lsfld_ = new uiSelLineStyle( this, lst, stu );
    if ( uiBasemapIOObjGroup::lastObject() )
	lsfld_->attach( alignedBelow, uiBasemapIOObjGroup::lastObject() );

    uiColorInput::Setup colstu( Color::White() );
    colstu.lbltxt( "Fill with" ).withcheck( true )
	  .transp(uiColorInput::Setup::Separate);
    fillcolfld_ = new uiColorInput( this, colstu );
    fillcolfld_->attach( alignedBelow, lsfld_->attachObj() );
    fillcolfld_->setDoDraw( false );

    linespacingfld_ = new uiGenInput( this, "Display In-lines",
				      IntInpIntervalSpec(true) );
    linespacingfld_->attach( alignedBelow, fillcolfld_ );
    linespacingfld_->setWithCheck( true );

    setLineSpacing();
}


uiBasemapSeisOutlineGroup::~uiBasemapSeisOutlineGroup()
{
}


bool uiBasemapSeisOutlineGroup::acceptOK()
{
    const bool res = uiBasemapIOObjGroup::acceptOK();
    return res;
}


void uiBasemapSeisOutlineGroup::setLineSpacing()
{
    AxisLayout<int> al( SI().inlRange(false), true, true );
    linespacingfld_->setValue( al.getSampling() );
}


bool uiBasemapSeisOutlineGroup::fillPar( IOPar& par ) const
{
    const bool res = uiBasemapIOObjGroup::fillPar( par );

    for ( int idx=0; idx<nrItems(); idx++ )
    {
	IOPar ipar;
	BufferString lsstr;
	lsfld_->getStyle().toString( lsstr );
	ipar.set( sKey::LineStyle(), lsstr );

	ipar.setYN( sKeyHasLines(), linespacingfld_->isChecked() );
	ipar.set( sKey::InlRange(), linespacingfld_->getIStepInterval() );

	ipar.setYN( sKeyFilled(), fillcolfld_->doDraw() );
	ipar.set( sKey::Color(), fillcolfld_->color() );

	par.mergeComp( ipar, IOPar::compKey(sKeyItem(),idx) );
    }

    return res;
}


bool uiBasemapSeisOutlineGroup::usePar( const IOPar& par )
{
    const bool res = uiBasemapIOObjGroup::usePar( par );

    BufferString lsstr;
    par.get( sKey::LineStyle(), lsstr );
    LineStyle ls; ls.fromString( lsstr );
    lsfld_->setStyle( ls );

    bool haslines = false;
    StepInterval<int> linespacing;
    par.getYN( sKeyHasLines(), haslines );
    par.get( sKey::InlRange(), linespacing );
    linespacingfld_->setValue( linespacing );
    linespacingfld_->setChecked( haslines );

    bool isfilled = false;
    par.getYN( sKeyFilled(), isfilled );

    Color color;
    par.get( sKey::Color(), color );
    fillcolfld_->setColor( color );
    fillcolfld_->setDoDraw( isfilled );

    return res;
}


uiObject* uiBasemapSeisOutlineGroup::lastObject()
{ return linespacingfld_->attachObj(); }



// uiBasemapSeisOutlineParentTreeItem
const char* uiBasemapSeisOutlineParentTreeItem::iconName() const
{ return "basemap-seisoutline"; }



// uiBasemapSeisOutlineTreeItem
uiBasemapSeisOutlineTreeItem::uiBasemapSeisOutlineTreeItem( const char* nm )
    : uiBasemapTreeItem(nm)
{
}


uiBasemapSeisOutlineTreeItem::~uiBasemapSeisOutlineTreeItem()
{
}


bool uiBasemapSeisOutlineTreeItem::usePar( const IOPar& par )
{
    const IOPar prevpar = pars_;
    uiBasemapTreeItem::usePar( par );

    BufferString lsstr;
    par.get( sKey::LineStyle(), lsstr );
    LineStyle ls;
    ls.fromString( lsstr );

    bool haslines = false;
    StepInterval<int> linespacing;
    par.getYN( sKeyHasLines(), haslines );
    par.get( sKey::InlRange(), linespacing );

    bool isfilled = false;
    BufferString colorstr;
    par.getYN( sKeyFilled(), isfilled );

    Color color;
    par.get( sKey::Color(), color );

    MultiID mid;
    if ( !par.get(sKey::ID(),mid) )
	return false;

    if ( basemapobjs_.isEmpty() )
	addBasemapObject( *new Basemap::SeisOutlineObject() );

    mDynamicCastGet(Basemap::SeisOutlineObject*,obj,basemapobjs_[0])
    if ( !obj ) return false;

    if ( hasParChanged(prevpar,par,sKey::LineStyle()) )
	obj->setLineStyle( 0, ls );

    if ( hasParChanged(prevpar,par,sKey::Color()) ||
	 hasParChanged(prevpar,par,sKeyFilled()) )
	    obj->setFillColor( 0, isfilled ? color : Color::NoColor() );

    if ( hasSubParChanged(prevpar,par,sKey::ID()) )
    {
	obj->setMultiID( mid );
	obj->extractPolygons();
	obj->updateGeometry();
    }

    if ( hasParChanged(prevpar,par,sKey::InlRange()) ||
	 hasParChanged(prevpar,par,sKeyHasLines()) ||
	 hasSubParChanged(prevpar,par,sKey::ID()) )
    {
	obj->setInsideLines( haslines ? linespacing : StepInterval<int>::udf());
	obj->extractSegments();
	obj->updateGeometry();
    }

    return true;
}


bool uiBasemapSeisOutlineTreeItem::showSubMenu()
{
    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    mnu.insertItem( new uiAction(uiStrings::sEdit(false)), sEditID() );
    mnu.insertItem( new uiAction(uiStrings::sRemove(true)), sRemoveID() );
    const int mnuid = mnu.exec();
    return handleSubMenu( mnuid );
}


bool uiBasemapSeisOutlineTreeItem::handleSubMenu( int mnuid )
{
    return uiBasemapTreeItem::handleSubMenu( mnuid );
}


const char* uiBasemapSeisOutlineTreeItem::parentType() const
{ return typeid(uiBasemapSeisOutlineParentTreeItem).name(); }



// uiBasemapSeisOutlineItem
int uiBasemapSeisOutlineItem::defaultZValue() const
{ return 100; }

uiBasemapGroup* uiBasemapSeisOutlineItem::createGroup( uiParent* p, bool isadd )
{ return new uiBasemapSeisOutlineGroup( p, isadd ); }

uiBasemapParentTreeItem* uiBasemapSeisOutlineItem::createParentTreeItem()
{ return new uiBasemapSeisOutlineParentTreeItem( ID() ); }

uiBasemapTreeItem* uiBasemapSeisOutlineItem::createTreeItem( const char* nm )
{ return new uiBasemapSeisOutlineTreeItem( nm ); }

#include "uibasemapgriditem.h"

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Henrique Mageste
 Date:		October 2014
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";


#include "basemapgrid.h"

#include "axislayout.h"
#include "ioman.h"
#include "ioobj.h"
#include "ptrman.h"

#include "uibasemap.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uigraphicsview.h"
#include "uimenu.h"
#include "uisellinest.h"
#include "uistrings.h"
#include "uiworld2ui.h"

#include "visplanedatadisplay.h"
#include "visgridlines.h"


static const char* sKeyGridType()    { return "Grid Type"; }
static const char* sKeyInlXSpacing() { return "Inl/X Spacing"; }
static const char* sKeyCrlYSpacing() { return "Crl/y Spacing"; }
static const char* sKeyShowInlX()    { return "Show Inl/X Grid"; }
static const char* sKeyShowCrlY()    { return "Show Crl/Y Grid"; }
static const char* sKeyInlCrl()      { return "Inl/Crl"; }
static const char* sKeyXY()	     { return "X/Y"; }

// uiBasemapGridGroup
uiBasemapGridGroup::uiBasemapGridGroup( uiParent* p, bool isadd )
    : uiBasemapGroup(p)
    , icxycheck_( 0 )
{
    if ( isadd )
    {
	icxycheck_ = new uiGenInput( this, "Grid system",
				     BoolInpSpec(false,"Inl/Crl","X/Y") );
	icxycheck_->valuechanged.notify( mCB(this,uiBasemapGridGroup,icxyCB) );
    }

    inlxfld_ = new uiCheckBox( this, "",
			       mCB(this,uiBasemapGridGroup,showGridLineCB) );
    inlxfld_->setHSzPol( uiObject::Wide );

    inlxspacingfld_ = new uiGenInput( this, "Spacing (Start/Stop)", \
				      DoubleInpIntervalSpec(true) );
    inlxspacingfld_->attach( alignedBelow, inlxfld_);

    crlyfld_ = new uiCheckBox( this, "",
			      mCB(this,uiBasemapGridGroup,showGridLineCB) );
    crlyfld_->setHSzPol( uiObject::Wide );
    crlyfld_->attach( alignedBelow, inlxspacingfld_ );

    crlyspacingfld_ = new uiGenInput( this, "Spacing (Start/Stop)", \
				     DoubleInpIntervalSpec(true) );
    crlyspacingfld_->attach( alignedBelow, crlyfld_ );

    LineStyle lst( LineStyle::Solid, 1, Color(0,170,0,0) );
    lsfld_ = new uiSelLineStyle( this, lst, "Line style" );
    lsfld_->attach( alignedBelow, crlyspacingfld_ );

    if ( icxycheck_ )
    {
	inlxfld_->attach( alignedBelow, icxycheck_ );
	icxyCB( 0 );
    }

    inlxfld_->setChecked( true );
    crlyfld_->setChecked( true );

    setCheckBoxLabel();
}


uiBasemapGridGroup::~uiBasemapGridGroup()
{
}


void uiBasemapGridGroup::icxyCB( CallBacker* )
{
    defaultname_ = icxycheck_->getBoolValue() ? sKeyInlCrl() : sKeyXY();

    setCheckBoxLabel();
    setParameters();
}


void uiBasemapGridGroup::showGridLineCB( CallBacker* cb )
{
    if ( cb == inlxfld_ )
	inlxspacingfld_->setSensitive( inlxfld_->isChecked() );
    else
	crlyspacingfld_->setSensitive( crlyfld_->isChecked() );
}


void uiBasemapGridGroup::setCheckBoxLabel()
{
    BufferString lbl( "Show " );
    lbl.add( defaultname_ == sKeyInlCrl() ? "Inl" : "X" ).add( " lines:" );
    inlxfld_->setText( lbl );

    lbl.set( "Show " ).add( defaultname_ == sKeyInlCrl() ? "Crl" : "Y"). \
    add(" lines:");
    crlyfld_->setText( lbl );
}


bool uiBasemapGridGroup::acceptOK()
{
    const bool res = uiBasemapGroup::acceptOK();

    return res;
}


void uiBasemapGridGroup::setParameters()
{
    if ( defaultname_ == sKeyInlCrl() )
    {
	AxisLayout<int> al( SI().inlRange(false), true, true );
	inlxspacingfld_->setValue( al.getSampling() );

	al.setDataRange( SI().crlRange(false) );
	crlyspacingfld_->setValue( al.getSampling() );
    }
    else
    {
	double min = SI().minCoord( false ).x;
	double max = SI().maxCoord( false ).x;
	AxisLayout<double> al( Interval<double>(min,max), false, true );
	inlxspacingfld_->setValue( al.getSampling() );

	min = SI().minCoord( false ).y;
	max = SI().maxCoord( false ).y;
	al.setDataRange( Interval<double>(min,max) );
	crlyspacingfld_->setValue( al.getSampling() );
    }
}


bool uiBasemapGridGroup::fillPar( IOPar& par ) const
{
    const bool res = uiBasemapGroup::fillPar( par );

    par.set( sKey::NrItems(), 1 );

    IOPar ipar;
    ipar.set( sKey::Name(), itemName() );
    ipar.set( sKeyGridType(), defaultname_ );
    ipar.set( sKeyNrObjs(), 1 );
    ipar.setYN( sKeyShowInlX(), inlxfld_->isChecked() );
    ipar.setYN( sKeyShowCrlY(), crlyfld_->isChecked() );

    if ( defaultname_ == sKeyInlCrl() )
    {
	ipar.set( sKeyInlXSpacing(), inlxspacingfld_->getIStepInterval() );
	ipar.set( sKeyCrlYSpacing(), crlyspacingfld_->getIStepInterval() );
    }
    else
    {
	ipar.set( sKeyInlXSpacing(), inlxspacingfld_->getDStepInterval() );
	ipar.set( sKeyCrlYSpacing(), crlyspacingfld_->getDStepInterval() );
    }

    BufferString lsstr;
    lsfld_->getStyle().toString( lsstr );
    ipar.set( sKey::LineStyle(), lsstr );

    const BufferString key = IOPar::compKey( sKeyItem(), 0 );
    par.mergeComp( ipar, key );

    return res;
}


bool uiBasemapGridGroup::usePar( const IOPar& par )
{
    par.get( sKeyGridType(), defaultname_ );

    bool doshow( true );
    par.getYN( sKeyShowInlX(), doshow );
    inlxfld_->setChecked( doshow );
    par.getYN( sKeyShowCrlY(), doshow );
    crlyfld_->setChecked( doshow );

    setCheckBoxLabel();

    if ( defaultname_ == sKeyInlCrl() )
    {
	StepInterval<int> inlcrlspacing;
	par.get( sKeyInlXSpacing(), inlcrlspacing );
	inlxspacingfld_->setValue( inlcrlspacing );

	par.get( sKeyCrlYSpacing(), inlcrlspacing );
	crlyspacingfld_->setValue( inlcrlspacing );
    }
    else
    {
	StepInterval<double> xyspacing;
	par.get( sKeyInlXSpacing(), xyspacing );
	inlxspacingfld_->setValue( xyspacing );

	par.get( sKeyCrlYSpacing(), xyspacing );
	crlyspacingfld_->setValue( xyspacing );
    }

    BufferString lsstr;
    par.get( sKey::LineStyle(), lsstr );
    LineStyle ls;
    ls.fromString( lsstr );
    lsfld_->setStyle( ls );

    const bool res = uiBasemapGroup::usePar( par );
    return res;
}


uiObject* uiBasemapGridGroup::lastObject()
{ return crlyspacingfld_->attachObj(); }


// uiBasemapGridTreeItem
uiBasemapGridTreeItem::uiBasemapGridTreeItem( const char* nm )
    : uiBasemapTreeItem(nm)
{
}


uiBasemapGridTreeItem::~uiBasemapGridTreeItem()
{
}


bool uiBasemapGridTreeItem::usePar( const IOPar& par )
{
    const IOPar prevpar = pars_;
    uiBasemapTreeItem::usePar( par );

    BufferString lsstr;
    par.get( sKey::LineStyle(), lsstr );
    LineStyle ls;
    ls.fromString( lsstr );

    bool inlxchecked = false, crlychecked = false;
    StepInterval<double> inlxspacing;
    StepInterval<double> crlyspacing;
    BufferString gridtype;

    par.getYN( sKeyShowInlX(), inlxchecked );
    par.getYN( sKeyShowCrlY(), crlychecked );
    par.get( sKeyInlXSpacing(), inlxspacing );
    par.get( sKeyCrlYSpacing(), crlyspacing );
    par.get( sKeyGridType(), gridtype );

    const bool isinlcrl = gridtype == sKeyInlCrl();

    uiBaseMap& uibm = BMM().getBasemap();
    const uiRect area = uibm.view().getViewArea();
    const uiWorldRect xyarea = uibm.getWorld2Ui().transform( area );

    if ( basemapobjs_.isEmpty() )
    {
	Basemap::GridObject* obj = new Basemap::GridObject();
	addBasemapObject( *obj );
    }
    mDynamicCastGet(Basemap::GridObject*,obj,basemapobjs_[0])
    if ( !obj ) return false;

    if ( hasParChanged(prevpar,par,sKey::LineStyle()) )
	obj->setLineStyle( 0, ls );

    if ( hasParChanged(prevpar,par,sKeyInlXSpacing()) ||
	 hasParChanged(prevpar,par,sKeyCrlYSpacing()) )
    {
	if ( isinlcrl )
	    obj->setInlCrlGrid( inlxspacing, crlyspacing,
				       inlxchecked, crlychecked );
	if ( !isinlcrl )
	    obj->setXYGrid( inlxspacing, crlyspacing, xyarea,
				   inlxchecked, crlychecked );
	obj->updateGeometry();
    }

    return true;
}


bool uiBasemapGridTreeItem::showSubMenu()
{
    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    mnu.insertItem( new uiAction(uiStrings::sEdit(false)), sEditID() );
    mnu.insertItem( new uiAction(uiStrings::sRemove(true)), sRemoveID() );
    const int mnuid = mnu.exec();
    return handleSubMenu( mnuid );
}


bool uiBasemapGridTreeItem::handleSubMenu( int mnuid )
{
    return uiBasemapTreeItem::handleSubMenu(mnuid);
}


// uiBasemapGridItem
int uiBasemapGridItem::defaultZValue() const
{ return 100; }

const char* uiBasemapGridItem::iconName() const
{ return "basemap-gridlines"; }

uiBasemapGroup* uiBasemapGridItem::createGroup( uiParent* p, bool isadd )
{ return new uiBasemapGridGroup( p, isadd ); }

uiBasemapTreeItem* uiBasemapGridItem::createTreeItem( const char* nm )
{ return new uiBasemapGridTreeItem( nm ); }

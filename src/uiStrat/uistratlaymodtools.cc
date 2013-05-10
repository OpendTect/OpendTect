/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2012
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uistratlaymodtools.h"


#include "keystrs.h"
#include "propertyref.h"
#include "stratlevel.h"
#include "stratlayermodel.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uispinbox.h"
#include "uitoolbutton.h"

const char* uiStratGenDescTools::sKeyNrModels()
{ return "Nr models"; }

const char* uiStratLayModEditTools::sKeyDisplayedProp()
{return "Displayed property";}

const char* uiStratLayModEditTools::sKeyDecimation()
{ return "Decimation"; }

const char* uiStratLayModEditTools::sKeySelectedLevel()
{ return "Selected level"; }

const char* uiStratLayModEditTools::sKeySelectedContent()
{return "Selected content";}

const char* uiStratLayModEditTools::sKeyZoomToggle()
{ return "Allow zoom"; }

const char* uiStratLayModEditTools::sKeyDispLith()
{ return "Display lithology"; }


const char* uiStratLayModEditTools::sKeyShowFlattened()
{ return "Display flattened";}


uiStratGenDescTools::uiStratGenDescTools( uiParent* p )
    : uiGroup(p,"Gen Desc Tools")
    , openReq(this)
    , saveReq(this)
    , propEdReq(this)
    , genReq(this)
{
    uiGroup* leftgrp = new uiGroup( this, "Left group" );
    uiToolButton* opentb = new uiToolButton( leftgrp, "open",
				"Open stored generation description",
				mCB(this,uiStratGenDescTools,openCB) );
    savetb_ = new uiToolButton( leftgrp, "save",
	    			"Save generation description",
				mCB(this,uiStratGenDescTools,saveCB) );
    savetb_->attach( rightOf, opentb );
    uiToolButton* proptb = new uiToolButton( leftgrp, "defprops",
	    			"Select layer properties",
				mCB(this,uiStratGenDescTools,propEdCB) );
    proptb->attach( rightOf, savetb_ );

    uiGroup* rightgrp = new uiGroup( this, "Right group" );
    const CallBack gocb( mCB(this,uiStratGenDescTools,genCB) );
    nrmodlsfld_ = new uiGenInput( rightgrp, "",
	    			  IntInpSpec(25).setLimits(Interval<int>(1,10000)) );
    nrmodlsfld_->setElemSzPol( uiObject::Small );
    nrmodlsfld_->setStretch( 0, 0 );
    nrmodlsfld_->setToolTip( "Number of models to generate", 0 );
    nrmodlsfld_->updateRequested.notify( gocb );
    uiToolButton* gotb = new uiToolButton( rightgrp, "go",
	    			"Generate this amount of models", gocb );
    nrmodlsfld_->attach( leftOf, gotb );
    rightgrp->attach( ensureRightOf, leftgrp );
    rightgrp->setFrame( true );
}


int uiStratGenDescTools::nrModels() const
{
    return nrmodlsfld_->getIntValue();
}


void uiStratGenDescTools::enableSave( bool yn )
{
    savetb_->setSensitive( yn );
}

void uiStratGenDescTools::fillPar( IOPar& par ) const
{
    par.set( sKeyNrModels(), nrModels() );
}


bool uiStratGenDescTools::usePar( const IOPar& par )
{
    int nrmodels;
    if ( par.get( sKeyNrModels(), nrmodels ) )
	nrmodlsfld_->setValue( nrmodels );
	
    return true;
}



uiStratLayModEditTools::uiStratLayModEditTools( uiParent* p )
    : uiGroup(p,"Lay Mod Edit Tools")
    , selPropChg(this)
    , selLevelChg(this)
    , selContentChg(this)
    , dispEachChg(this)
    , dispZoomedChg(this)
    , dispLithChg(this)
    , flattenChg(this)
{
    uiGroup* leftgrp = new uiGroup( this, "Left group" );
    propfld_ = new uiComboBox( leftgrp, "Display property" );
    propfld_->setToolTip( "Displayed property" );
    propfld_->selectionChanged.notify(
	    			mCB(this,uiStratLayModEditTools,selPropCB) );

    lvlfld_ = new uiComboBox( leftgrp, "Level" );
    lvlfld_->setToolTip( "Selected stratigraphic level" );
    lvlfld_->attach( rightOf, propfld_ );
    lvlfld_->selectionChanged.notify(
	    			mCB(this,uiStratLayModEditTools,selLevelCB) );

    contfld_ = new uiComboBox( leftgrp, "Content" );
    contfld_->setToolTip( "Marked content" );
    contfld_->attach( rightOf, lvlfld_ );
    contfld_->setHSzPol( uiObject::Small );
    contfld_->selectionChanged.notify(
	    			mCB(this,uiStratLayModEditTools,selContentCB) );

    eachlbl_ = new uiLabel( leftgrp, "each" );
    eachlbl_->attach( rightOf, contfld_ );
    eachfld_ = new uiSpinBox( leftgrp, 0, "DispEach" );
    eachfld_->setInterval( 1, 1000 );
    eachfld_->attach( rightOf, eachlbl_ );
    eachfld_->valueChanging.notify(
				mCB(this,uiStratLayModEditTools,dispEachCB) );

    uiGroup* rightgrp = new uiGroup( this, "Right group" );
    flattenedtb_ = new uiToolButton( rightgrp, "flattenseis",
			"Show flattened when on",
			mCB(this,uiStratLayModEditTools,showFlatCB) );
    flattenedtb_->setToggleButton( true );
    flattenedtb_->setOn( false );
    lithtb_ = new uiToolButton( rightgrp, "lithologies",
			"Show lithology colors when on",
			mCB(this,uiStratLayModEditTools,dispLithCB) );
    lithtb_->setToggleButton( true );
    lithtb_->setOn( true );
    lithtb_->attach( leftOf, flattenedtb_ );
    zoomtb_ = new uiToolButton( rightgrp, "toggzooming",
			"Do not zoom into models when on",
			mCB(this,uiStratLayModEditTools,dispZoomedCB) );
    zoomtb_->setToggleButton( true );
    zoomtb_->setOn( false );
    zoomtb_->attach( leftOf, lithtb_ );
    rightgrp->attach( rightTo, leftgrp );
    rightgrp->attach( rightBorder );
}


void uiStratLayModEditTools::setNoDispEachFld()
{
    eachlbl_->display( false ); eachfld_->display( false );
    eachfld_ = 0;
}


static void setFldNms( uiComboBox* cb, const BufferStringSet& nms, bool wnone,
			bool wall, int def )
{
    const BufferString selnm( cb->text() );
    cb->setEmpty();
    if ( wnone )
	cb->addItem( "---" );
    if ( nms.isEmpty() ) return;

    cb->addItems( nms );
    if ( wall )
	cb->addItem( sKey::All() );
    if ( !selnm.isEmpty() ) 
	def = nms.indexOf( selnm );
    if ( wnone ) def++;
    if ( def > cb->size() ) def = cb->size() - 1;
    cb->setCurrentItem( def );
}


void uiStratLayModEditTools::setProps( const BufferStringSet& nms )
{ setFldNms( propfld_, nms, false, false, 0 ); }
void uiStratLayModEditTools::setLevelNames( const BufferStringSet& nms )
{ setFldNms( lvlfld_, nms, true, false, 0 ); }
void uiStratLayModEditTools::setContentNames( const BufferStringSet& nms )
{ setFldNms( contfld_, nms, true, true, -1 ); }


const char* uiStratLayModEditTools::selProp() const
{
    return propfld_->isEmpty() ? 0 : propfld_->text();
}


int uiStratLayModEditTools::selPropIdx() const
{
    return propfld_->isEmpty() ? 0 : propfld_->getIntValue() + 1;
}


const char* uiStratLayModEditTools::selLevel() const
{
    return lvlfld_->isEmpty() ? 0 : lvlfld_->text();
}


int uiStratLayModEditTools::selLevelIdx() const
{
    return lvlfld_->isEmpty() ? -1 : lvlfld_->currentItem() - 1;
}


const char* uiStratLayModEditTools::selContent() const
{
    return contfld_->isEmpty() ? 0 : contfld_->text();
}


const Strat::Level* uiStratLayModEditTools::selStratLevel() const
{
    const int lvlidx = selLevelIdx();
    return lvlidx < 0 ? 0 : Strat::LVLS().levels()[lvlidx];
}


Color uiStratLayModEditTools::selLevelColor() const
{
    const Strat::Level* lvl = selStratLevel();
    return lvl ? lvl->color() : Color::NoColor();
}


int uiStratLayModEditTools::dispEach() const
{
    return eachfld_ ? eachfld_->getValue() : 1;
}


bool uiStratLayModEditTools::dispZoomed() const
{
    return !zoomtb_->isOn();
}


bool uiStratLayModEditTools::dispLith() const
{
    return lithtb_->isOn();
}


bool uiStratLayModEditTools::showFlattened() const
{
    return flattenedtb_->isOn() && (bool)selStratLevel();
}


void uiStratLayModEditTools::setSelProp( const char* sel )
{
    propfld_->setText( sel );
}


void uiStratLayModEditTools::setSelLevel( const char* sel )
{
    lvlfld_->setText( sel );
}


void uiStratLayModEditTools::setSelContent( const char* sel )
{
    contfld_->setText( sel );
}


void uiStratLayModEditTools::setDispEach( int nr )
{
    if ( eachfld_ ) eachfld_->setValue( nr );
}


void uiStratLayModEditTools::setDispZoomed( bool yn )
{
    zoomtb_->setOn( !yn );
}


void uiStratLayModEditTools::setDispLith( bool yn )
{
    lithtb_->setOn( yn );
}


void uiStratLayModEditTools::setFlatTBSensitive( bool yn )
{
    flattenedtb_->setSensitive( yn );
}


void uiStratLayModEditTools::setShowFlattened( bool yn )
{
    flattenedtb_->setOn( yn );
}

#define mGetProp( func, key ) \
if ( func ) \
par.set( key, func )

void uiStratLayModEditTools::fillPar( IOPar& par ) const
{
    mGetProp( selProp(), sKeyDisplayedProp() );
    par.set( sKeyDecimation(), dispEach() );

    mGetProp( selLevel(), sKeySelectedLevel() );
    mGetProp( selContent(), sKeySelectedContent() );
    
    par.setYN( sKeyZoomToggle(), dispZoomed() );
    par.setYN( sKeyDispLith(), dispLith() );
    par.setYN( sKeyShowFlattened(), showFlattened() );
}

#define mSetProp( fld, key ) \
{ \
    FixedString selprop = par[key()]; \
    if ( selprop && fld->isPresent( selprop ) ) \
    { \
	fld->setCurrentItem( selprop ); \
	fld->selectionChanged.trigger(); \
    } \
}

#define mSetYN( func, key, cb ) \
{ \
    bool yn; \
    if ( par.getYN( key(), yn ) ) \
    { \
	func( yn ); \
	cb( 0 ); \
    } \
}




bool uiStratLayModEditTools::usePar( const IOPar& par )
{
    mSetProp( propfld_, sKeyDisplayedProp );
    mSetProp( lvlfld_, sKeySelectedLevel );
    mSetProp( contfld_, sKeySelectedContent );

    int decimation;
    if ( par.get( sKeyDecimation(), decimation ) )
    {
	setDispEach( decimation );
	dispEachCB( 0 );
    }
    
    mSetYN( setDispZoomed, sKeyZoomToggle, dispZoomedCB );
    mSetYN( setDispLith, sKeyDispLith, dispLithCB );
    mSetYN( setShowFlattened, sKeyShowFlattened, showFlatCB );

    return true;
}


//-----------------------------------------------------------------------------

#define mCreatePropSelFld( propnm, txt, prop, prevbox ) \
    uiLabeledComboBox* lblbox##propnm = new uiLabeledComboBox( this, txt ); \
    propnm##fld_ = lblbox##propnm->box(); \
    PropertyRefSelection subsel##propnm = proprefsel.subselect( prop );\
    for ( int idx=0; idx<subsel##propnm.size(); idx++ )\
	if ( subsel##propnm[idx] )\
	    propnm##fld_->addItem( subsel##propnm[idx]->name() );\
    if ( prevbox )\
	lblbox##propnm->attach( alignedBelow, prevbox );


uiStratLayModFRPropSelector::uiStratLayModFRPropSelector( uiParent* p,
					const PropertyRefSelection& proprefsel )
	: uiDialog(p,uiDialog::Setup("Property Selector",
		    		     "There are multiple properties referenced"
				     " with the same type. \n" 
				     "Please specify which one to use as: ",
				     mTODOHelpID) )
{
    mCreatePropSelFld( den, "Reference for Density", PropertyRef::Den, 0 );
    mCreatePropSelFld( vp, "Reference for Vp", PropertyRef::Vel, lblboxden );
    mCreatePropSelFld( vs, "Reference for Vs", PropertyRef::Vel, lblboxvp );
}


bool uiStratLayModFRPropSelector::needsDisplay() const
{
    if ( vpfld_->size() ==2 && vsfld_->size() ==2
	    && vsfld_->isPresent(Strat::LayerModel::defSVelStr()) )
    {
	vsfld_->setCurrentItem( Strat::LayerModel::defSVelStr() );
	return false;
    }
    
    return vpfld_->size()>1 || vsfld_->size()>1 || denfld_->size()>1;
}


const char* uiStratLayModFRPropSelector::getSelVPName() const
{
    return vpfld_->text();
}


const char* uiStratLayModFRPropSelector::getSelVSName() const
{
    return vsfld_->text();
}


const char* uiStratLayModFRPropSelector::getSelDenName() const
{
    return denfld_->text();
}

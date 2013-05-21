/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          May 2010
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uit2dconvsel.h"

#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uicombobox.h"
#include "ioobj.h"
#include "survinfo.h"
#include "veldesc.h"
#include "zdomain.h"


mImplFactory1Param(uiT2DConvSelGroup,uiParent*,uiT2DConvSelGroup::factory);

uiT2DConvSel::Setup::Setup( uiIOObjSel* tied, bool opt )
    : tiedto_(tied)
    , optional_(opt)
    , ist2d_(SI().zIsTime())
{
}


uiT2DConvSel::uiT2DConvSel( uiParent* p, const Setup& su )
    : uiGroup(p)
    , setup_(su)
{
    BufferString fldtext;
    if ( setup_.fldtext_.isEmpty() )
    {
	const FixedString zunit = setup_.ist2d_
	    ? sKey::Depth()
	    : sKey::Time();
	
	if ( setup_.optional_ )
	    fldtext.add( "Convert to " ).add( zunit );
	else
	    fldtext.add( zunit ).add( " conversion");
    }

    uiLabeledComboBox* lcb = new uiLabeledComboBox( this, fldtext.buf() );
    choicefld_ = lcb->box();
    choicefld_->setHSzPol( uiObject::SmallVar );
    if ( setup_.optional_ )
	choicefld_->addItem( "No" );

    const BufferStringSet& factnms( uiT2DConvSelGroup::factory().getNames() );
    for ( int idx=0; idx<factnms.size(); idx++ )
    {
	const char* nm = factnms.get( idx );
	choicefld_->addItem( nm );
	uiT2DConvSelGroup* grp = uiT2DConvSelGroup::factory().create(nm,this);
	grps_ += grp;
	if ( idx == 0 )
	    setHAlignObj( grp );

	grp->attach( rightOf, lcb );
    }

    if ( setup_.tiedto_ )
    {
	const CallBack cb( mCB(this,uiT2DConvSel,inpSel) );
	setup_.tiedto_->selectionDone.notify( cb );
	postFinalise().notify( cb );
    }

    const CallBack cb( mCB(this,uiT2DConvSel,choiceSel) );
    choicefld_->selectionChanged.notify( cb );
    postFinalise().notify( cb );
}


#define mGetGroupIdx \
    const int grpidx = choicefld_->currentItem() - (setup_.optional_ ? 1 : 0)


void uiT2DConvSel::choiceSel( CallBacker* cb )
{
    mGetGroupIdx;
    for ( int idx=0; idx<grps_.size(); idx++ )
	grps_[idx]->display( idx == grpidx );
}


void uiT2DConvSel::inpSel( CallBacker* cb )
{
    if ( !setup_.tiedto_ ) return;
    const IOObj* ioobj = setup_.tiedto_->ioobj( true );
    if ( !ioobj ) return;

    choicefld_->setSensitive( ZDomain::isSI(ioobj->pars()) );
}


bool uiT2DConvSel::usePar( const IOPar& iop )
{
    const char* typ = iop.find( sKey::Type() );
    if ( !typ || !*typ || !choicefld_->isPresent(typ) )
	return false;

    int selidx = choicefld_->indexOf( typ );
    if ( selidx < 0 ) return false;

    if ( setup_.optional_ ) selidx--;
    if ( selidx >= 0 )
	grps_[selidx]->usePar( iop );

    choicefld_->setCurrentItem( selidx );
    return true;
}


bool uiT2DConvSel::fillPar( IOPar& iop, bool typeonly ) const
{
    BufferString typestr = choicefld_->text();
    typestr += setup_.ist2d_ ? "T2D" : "D2T";
    iop.set( sKey::Name(), typestr );
    if ( typeonly )
	return true;

    mGetGroupIdx;
    return grpidx < 0 ? false : grps_[grpidx]->fillPar( iop );
}


//uiT2DLinConvSelGroup
uiT2DLinConvSelGroup::uiT2DLinConvSelGroup( uiParent* p )
    : uiT2DConvSelGroup(p,"Linear T2D conv sel")
{
    const float dv = 0;    
    const float v0 = mCast( float, SI().zInFeet() ? 3000 : 1000 );
    
    BufferString text( "V0 " );
    text.add( VelocityDesc::getVelUnit(true) );
    
    const BufferString zentity = SI().zDomain().entityStr();
    
    text.add( ", dV/d" ).add( zentity ).add( " (" );
    BufferString gradientunit;
    if ( SI().zDomain().isDepth() )
    {
	gradientunit = "s^-1";
    }
    else
    {
	gradientunit = SI().depthsInFeet()
	    ? "ft/(s^2)"
	    : "m/(s^2)";
    }
    
    text.add( gradientunit.buf() ).add( " )" );
    fld_ = new uiGenInput( this, text, FloatInpSpec(v0), FloatInpSpec(dv) );
}


#define mDefA0A1 float a0 = fld_->getfValue(0); float a1 = fld_->getfValue(1)

//uiT2DLinConvSelGroup
bool uiT2DLinConvSelGroup::usePar( const IOPar& iop )
{
    //TODO handle Z unit properly
    mDefA0A1;
    if ( iop.get("V0,dV",a0,a1) )
	{ fld_->setValue( a0, 0 ); fld_->setValue( a1, 1 ); }

    return true;
}


bool uiT2DLinConvSelGroup::fillPar( IOPar& iop ) const
{
    //TODO handle Z unit properly
    mDefA0A1;
    iop.set( "V0,dV", a0, a1 );

    return true;
}


void uiT2DLinConvSelGroup::initClass()
{
    uiT2DConvSelGroup::factory().addCreator( create, "Linear" );
}

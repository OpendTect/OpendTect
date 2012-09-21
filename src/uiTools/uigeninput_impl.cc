/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          21/2/2002
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "uigeninput.h"
#include "uigeninput_impl.h"
#include "uibutton.h"
#include "datainpspec.h"


uiGenInputInputFld::uiGenInputInputFld( uiGenInput* p, const DataInpSpec& dis ) 
    : spec_(*dis.clone())
    , p_( p )
{
}


uiGenInputInputFld::~uiGenInputInputFld()
{
    delete &spec_;
}


uiObject* uiGenInputInputFld::elemObj( int idx )
{
    UserInputObj* elem = element(idx);
    if ( !elem ) return 0;
    mDynamicCastGet(uiGroup*,grp,elem)
    if ( grp ) return grp->mainObject();
    mDynamicCastGet(uiObject*,ob,elem)
    return ob;
}


bool uiGenInputInputFld::isUndef( int idx ) const
{
    return mIsUdf(text(idx));
} 


const char* uiGenInputInputFld::text( int idx ) const
{ 
    const UserInputObj* obj = element( idx );
    const char* ret = obj ? obj->text() : 0;
    return ret ? ret : mUdf(const char*);
}


#define mImplGetFn(typ,fn) \
typ uiGenInputInputFld::fn( int idx ) const \
{  \
    const UserInputObj* obj = element( idx ); \
    return obj ? obj->fn() : mUdf(typ); \
}
mImplGetFn(int,getIntValue)
mImplGetFn(float,getfValue)
mImplGetFn(double,getdValue)
mImplGetFn(bool,getBoolValue)


void uiGenInputInputFld::setText( const char* s, int idx )
{
    setValue( s, idx );
}


void uiGenInputInputFld::setValue( bool b, int idx )
{
    if ( element(idx) )
	element(idx)->setValue(b);
}


#define mDoAllElems(fn) \
for( int idx=0; idx<nElems(); idx++ ) \
{ \
    UserInputObj* obj = element( idx ); \
    if ( obj ) obj->fn; \
    else pErrMsg("Found null field"); \
}
#define mDoAllElemObjs(fn) \
for( int idx=0; idx<nElems(); idx++ ) \
{ \
    uiObject* obj = elemObj( idx ); \
    if ( obj ) obj->fn; \
    else pErrMsg("Found null elemObj"); \
}

//! stores current value as clear state.
void uiGenInputInputFld::initClearValue()
{
    mDoAllElems(initClearValue())
}


void uiGenInputInputFld::clear()
{
    mDoAllElems(clear())
}

void uiGenInputInputFld::display( bool yn, int elemidx )
{
    if ( elemidx < 0 )
    {
	mDoAllElemObjs(display(yn))
    }
    else
    {
	uiObject* obj = elemObj( elemidx );
	if ( obj )
	    obj->display( yn );
    }
} 


bool uiGenInputInputFld::isReadOnly( int idx ) const
{ 
    const UserInputObj* obj = element( idx );
    return obj && obj->isReadOnly();
}


void uiGenInputInputFld::setReadOnly( bool yn, int idx )
{
    UserInputObj* obj = element( idx );
    if ( obj ) obj->setReadOnly( yn );
}


void uiGenInputInputFld::setSensitive( bool yn, int elemidx )
{ 
    if ( elemidx < 0 )
	{ mDoAllElemObjs(setSensitive(yn)) }
    else
    {
	uiObject* obj = elemObj( elemidx );
	if ( obj )
	obj->setSensitive( yn );
    }
}


bool uiGenInputInputFld::update( const DataInpSpec& nw )
{
    if ( spec_.type() == nw.type() && update_(nw) )
	{ spec_ = nw; return true; }

    return false;
}


void uiGenInputInputFld::updateSpec()
{ 
    for( int idx=0; idx<nElems()&&element(idx); idx++ )
    spec_.setText( element(idx)->text(), idx );
}


void uiGenInputInputFld::valChangingNotify(CallBacker*)
{
    p_->valuechanging.trigger( *p_ );
}


void uiGenInputInputFld::valChangedNotify(CallBacker*)
{
    p_->valuechanged.trigger( *p_ );
}


void uiGenInputInputFld::updateReqNotify(CallBacker*)
{
    p_->updateRequested.trigger( *p_ );
}


bool uiGenInputInputFld::update_( const DataInpSpec& nw )
{ 
    return nElems() == 1 && element(0) ? element(0)->update(nw) : false; 
} 


void uiGenInputInputFld::init()
{
    update_( spec_ );

    uiObject::SzPolicy hpol =
    p_ ? p_->elemSzPol() : uiObject::Undef;

    if ( hpol == uiObject::Undef )
    {
	int nel = p_ ? p_->nrElements() : nElems();

	switch( spec_.type().rep() )
	{
	case DataType::stringTp:
	    hpol = nel > 1 ? uiObject::SmallVar : uiObject::MedVar;
	break;
	case DataType::boolTp:
	    hpol = nel > 1 ? uiObject::SmallVar : uiObject::MedVar;
	break;
	default:
	    hpol = nel > 1 ? uiObject::Small : uiObject::Medium;
	break;
	}
    }

    mDoAllElemObjs(setHSzPol(hpol))
}



uiGenInputBoolFld::uiGenInputBoolFld( uiParent* p, const char* truetext,
			  const char* falsetext, bool initval, const char* nm)
    : uiGroup( p, nm )
    , UserInputObjImpl<bool>()
    , butgrp( 0 ), checkbox( 0 ), rb1( 0 ), rb2( 0 ), yn( initval )
    , valueChanged( this )
{
    init( p, truetext, falsetext, initval );
}

uiGenInputBoolFld::uiGenInputBoolFld(uiParent* p, const DataInpSpec& spec, const char* nm)
    : uiGroup( p, nm )
    , UserInputObjImpl<bool>()
    , butgrp( 0 ), checkbox( 0 ), rb1( 0 ), rb2( 0 ), yn( false )
    , valueChanged( this )
{
    const BoolInpSpec* spc = dynamic_cast<const BoolInpSpec*>(&spec);
    if ( !spc ) pErrMsg("huh?");

    init( p, spc->trueFalseTxt(true), 
	    spc->trueFalseTxt(false), spc->getBoolValue() );
    if ( rb1 && spec.name(0) ) rb1->setName( spec.name(0) );
    if ( rb2 && spec.name(1) ) rb2->setName( spec.name(1) );
}


void uiGenInputBoolFld::init( uiParent* p, const char* truetext,
			const char* falsetext, bool yn_ )
{
    truetxt = truetext;
    falsetxt = falsetext;
    yn = yn_;

    initClearValue();

    if ( truetxt.isEmpty()  || falsetxt.isEmpty() )
    { 
	checkbox = new uiCheckBox( p, (truetxt=="") ? 
				(const char*) name() : (const char*)truetxt );
	checkbox->activated.notify( mCB(this,uiGenInputBoolFld,selected) );
	setvalue_( yn );
	return; 
    }

    // we have two labelTxt()'s, so we'll make radio buttons
    uiGroup* grp_ = new uiGroup( p, name() ); 
    butgrp = grp_->mainObject();

    rb1 = new uiRadioButton( grp_, truetxt );
    rb1->activated.notify( mCB(this,uiGenInputBoolFld,selected) );
    rb2 = new uiRadioButton( grp_, falsetxt );
    rb2->activated.notify( mCB(this,uiGenInputBoolFld,selected) );

    rb2->attach( rightTo, rb1 );
    grp_->setHAlignObj( rb1 );

    setvalue_( yn );
}


uiObject* uiGenInputBoolFld::mainobject()
{
    if ( checkbox )
	return checkbox;
    return butgrp;
}


void uiGenInputBoolFld::setToolTip( const char* tt )
{
    if ( checkbox )
	checkbox->setToolTip( tt );
    if ( rb1 )
	rb1->setToolTip( tt );
    if ( rb2 )
	rb2->setToolTip( tt );
}


void uiGenInputBoolFld::selected(CallBacker* cber)
{
    bool yn_ = yn;
    if ( cber == rb1 )		{ yn = rb1->isChecked(); }
    else if( cber == rb2 )	{ yn = !rb2->isChecked(); }
    else if( cber == checkbox )	{ yn = checkbox->isChecked(); }
    else return;

    if ( yn != yn_ )		
    {
	valueChanged.trigger(*this);
	setvalue_( yn );
    }
}


void uiGenInputBoolFld::setvalue_( bool b )
{
    yn = b ? true : false; 

    if ( checkbox ) { checkbox->setChecked( yn ); return; }

    if ( !rb1 || !rb2 ) { pErrMsg("Huh?"); return; }

    rb1->setChecked(yn); 
    rb2->setChecked(!yn); 
}


void uiGenInputBoolFld::setReadOnly( bool ro )
{
    if ( checkbox ) { checkbox->setSensitive( !ro ); return; }

    if ( !rb1 || !rb2 ) { pErrMsg("Huh?"); return; }

    rb1->setSensitive(!ro); 
    rb2->setSensitive(!ro); 
}


bool uiGenInputBoolFld::isReadOnly() const
{
    if ( checkbox )		return checkbox->sensitive();

    if ( !rb1 || !rb2 ) { pErrMsg("Huh?"); return false; }

    return !rb1->sensitive() && !rb2->sensitive(); 
}


bool uiGenInputBoolFld::update_( const DataInpSpec& spec )
{
    setvalue_( spec.getBoolValue() );
    return true;
}

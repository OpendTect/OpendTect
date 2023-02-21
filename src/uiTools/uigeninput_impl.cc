/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uigeninput.h"
#include "uigeninput_impl.h"

#include "uibutton.h"
#include "uispinbox.h"

#include "datainpspec.h"
#include <limits.h>


uiGenInputInputFld::uiGenInputInputFld( uiGenInput* p, const DataInpSpec& dis )
    : spec_(*dis.clone())
    , p_( p )
{
}


uiGenInputInputFld::~uiGenInputInputFld()
{
    delete &spec_;
}


const UserInputObj* uiGenInputInputFld::element( int idx ) const
{ return const_cast<uiGenInputInputFld*>(this)->element(idx); }


uiObject* uiGenInputInputFld::elemObj( int idx )
{
    UserInputObj* elem = element(idx);
    if ( !elem ) return 0;
    mDynamicCastGet(uiGroup*,grp,elem)
    if ( grp ) return grp->mainObject();
    mDynamicCastGet(uiObject*,ob,elem)
    return ob;
}


const uiObject* uiGenInputInputFld::elemObj( int idx ) const
{ return const_cast<uiGenInputInputFld*>(this)->elemObj( idx ); }


bool uiGenInputInputFld::isUndef( int idx ) const
{
    return mIsUdf(text(idx));
}


const char* uiGenInputInputFld::text( int idx ) const
{
    const UserInputObj* obj = element( idx );
    if ( !obj )
    {
	mDynamicCastGet(const uiSpinBox*,sb,elemObj(idx))
	if ( sb ) return sb->text();
    }

    const char* ret = obj ? obj->text() : 0;
    return ret ? ret : mUdf(const char*);
}


int uiGenInputInputFld::getIntValue( int idx ) const
{
    const UserInputObj* obj = element( idx );
    if ( !obj )
    {
	mDynamicCastGet(const uiSpinBox*,sb,elemObj(idx))
	if ( sb ) return sb->getIntValue();
    }
    return obj ? obj->getIntValue() : mUdf(int);
}


od_int64 uiGenInputInputFld::getInt64Value( int idx ) const
{
    const UserInputObj* obj = element( idx );
    if ( !obj )
    {
	mDynamicCastGet(const uiSpinBox*,sb,elemObj(idx))
	if ( sb ) return sb->getInt64Value();
    }
    return obj ? obj->getInt64Value() : mUdf(od_int64);
}


#define mImplGetFn(typ,fn) \
typ uiGenInputInputFld::fn( int idx ) const \
{  \
    const UserInputObj* obj = element( idx ); \
    if ( !obj ) \
    { \
	mDynamicCastGet(const uiSpinBox*,sb,elemObj(idx)) \
	if ( sb ) return sb->fn(); \
    } \
    return obj ? obj->fn() : mUdf(typ); \
}
//mImplGetFn(int,getIntValue)
mImplGetFn(float,getFValue)
mImplGetFn(double,getDValue)
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
    else { pErrMsg("Found null field"); } \
}
#define mDoAllElemObjs(fn) \
for( int idx=0; idx<nElems(); idx++ ) \
{ \
    uiObject* obj = elemObj( idx ); \
    if ( obj ) obj->fn; \
    else { pErrMsg("Found null elemObj"); } \
}



void uiGenInputInputFld::setEmpty()
{
    mDoAllElems(setEmpty())
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


void uiGenInputInputFld::setReadOnly( bool yn, int elemidx )
{
    if ( elemidx < 0 )
	{ mDoAllElems(setReadOnly(yn)) }
    else
    {
	UserInputObj* obj = element( elemidx );
	if ( obj ) obj->setReadOnly( yn );
    }
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


mStartAllowDeprecatedSection

void uiGenInputInputFld::valChangingNotify( CallBacker* )
{
    p_->valueChanging.trigger( *p_ );
    p_->valuechanging.trigger( *p_ );
}


void uiGenInputInputFld::valChangedNotify( CallBacker* )
{
    p_->valueChanged.trigger( *p_ );
    p_->valuechanged.trigger( *p_ );
}

mStopAllowDeprecatedSection


void uiGenInputInputFld::updateReqNotify( CallBacker* )
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

    uiObject::SzPolicy hpol = p_ ? p_->elemSzPol() : uiObject::Undef;
    if ( hpol == uiObject::Undef )
    {
	int nel = p_ ? p_->nrElements() : nElems();

	switch( spec_.type().rep() )
	{
	case DataType::stringTp:
	case DataType::boolTp:
	    hpol = nel > 1 ? uiObject::SmallVar : uiObject::MedVar;
	break;
	case DataType::intTp:
	    hpol = uiObject::Small;
	break;
	default:
	    hpol = nel > 1 ? uiObject::Small : uiObject::Medium;
	break;
	}
    }

    mDoAllElemObjs(setHSzPol(hpol))

    const int hstretch = p_->stretch( true );
    const int vstretch = p_->stretch( false );
    mDoAllElemObjs( setStretch(hstretch,vstretch) )
}



uiGenInputBoolFld::uiGenInputBoolFld( uiParent* p, const uiString& truetext,
		    const uiString& falsetext, bool initval, const char* nm)
    : uiGroup( p, nm )
    , UserInputObjImpl<bool>()
    , butgrp_( 0 ), checkbox_( 0 ), rb1_( 0 ), rb2_( 0 ), yn_( initval )
    , valueChanged( this )
{
    init( p, truetext, falsetext, initval );
}

uiGenInputBoolFld::uiGenInputBoolFld(uiParent* p, const DataInpSpec& spec,
				     const char* nm)
    : uiGroup( p, nm )
    , UserInputObjImpl<bool>()
    , butgrp_( 0 ), checkbox_( 0 ), rb1_( 0 ), rb2_( 0 ), yn_( false )
    , valueChanged( this )
{
    const BoolInpSpec* spc = dynamic_cast<const BoolInpSpec*>(&spec);
    if ( !spc )
	{ pErrMsg("huh?"); init( p, tr("Y"), tr("N"), true ); }
    else
    {
	init( p, spc->trueFalseTxt(true),
		spc->trueFalseTxt(false), spc->getBoolValue() );
	if ( rb1_ && spec.name(0) ) rb1_->setName( spec.name(0) );
	if ( rb2_ && spec.name(1) ) rb2_->setName( spec.name(1) );
    }
}


const char* uiGenInputBoolFld::text() const
{ return yn_ ? truetxt_.getFullString() : falsetxt_.getFullString(); }


void uiGenInputBoolFld::setText( const char* t )
{
    bool newval;
    if ( truetxt_.getFullString() == t ) newval = true;
    else if ( falsetxt_.getFullString()==t ) newval = false;
    else newval = toBool(t);

    setvalue_(newval);
}


void uiGenInputBoolFld::init( uiParent* p, const uiString& truetext,
			const uiString& falsetext, bool yn )
{
    truetxt_ = truetext;
    falsetxt_ = falsetext;
    yn_ = yn;

    if ( truetxt_.isEmpty()  || falsetxt_.isEmpty() )
    {
	checkbox_ = new uiCheckBox( p, (truetxt_.isEmpty()) ?
				mToUiStringTodo(name()) : truetxt_ );
	checkbox_->activated.notify( mCB(this,uiGenInputBoolFld,selected) );
	setvalue_( yn );
	return;
    }

    // we have two labelTxt()'s, so we'll make radio buttons
    uiGroup* grp_ = new uiGroup( p, name() );
    butgrp_ = grp_->mainObject();

    rb1_ = new uiRadioButton( grp_, truetxt_ );
    rb1_->activated.notify( mCB(this,uiGenInputBoolFld,selected) );
    rb2_ = new uiRadioButton( grp_, falsetxt_ );
    rb2_->activated.notify( mCB(this,uiGenInputBoolFld,selected) );

    rb2_->attach( rightTo, rb1_ );
    grp_->setHAlignObj( rb1_ );

    setvalue_( yn_ );
}


uiObject* uiGenInputBoolFld::mainobject()
{
    if ( checkbox_ )
	return checkbox_;
    return butgrp_;
}


void uiGenInputBoolFld::setToolTip( const uiString& tt )
{
    if ( checkbox_ )
	checkbox_->setToolTip( tt );
    if ( rb1_ )
	rb1_->setToolTip( tt );
    if ( rb2_ )
	rb2_->setToolTip( tt );
}


void uiGenInputBoolFld::selected(CallBacker* cber)
{
    bool newyn = false;

    if ( cber == rb1_ )		{ newyn = rb1_->isChecked(); }
    else if( cber == rb2_ )	{ newyn = !rb2_->isChecked(); }
    else if( cber == checkbox_ ){ newyn = checkbox_->isChecked(); }
    else return;

    if ( newyn != yn_ )
    {
	setvalue_( newyn );
	valueChanged.trigger(*this);
    }
}


void uiGenInputBoolFld::setvalue_( bool b )
{
    yn_ = b;
    if ( checkbox_ ) { checkbox_->setChecked( yn_ ); return; }

    if ( !rb1_ || !rb2_ )
	{ pErrMsg("Huh?"); return; }

    rb1_->setChecked( yn_ );
    rb2_->setChecked( !yn_ );
}


void uiGenInputBoolFld::setReadOnly( bool ro )
{
    if ( checkbox_ ) { checkbox_->setSensitive( !ro ); return; }

    if ( !rb1_ || !rb2_ )
	{ pErrMsg("Huh?"); return; }

    rb1_->setSensitive(!ro);
    rb2_->setSensitive(!ro);
}


bool uiGenInputBoolFld::isReadOnly() const
{
    if ( checkbox_ )		return checkbox_->sensitive();

    if ( !rb1_ || !rb2_ )
	{ pErrMsg("Huh?"); return false; }

    return !rb1_->sensitive() && !rb2_->sensitive();
}


bool uiGenInputBoolFld::update_( const DataInpSpec& spec )
{
    mDynamicCastGet(const BoolInpSpec*,boolspec,&spec)
    if ( !boolspec )
	return false;

    const uiString& truetext = boolspec->trueFalseTxt( true );
    const uiString& falsetext = boolspec->trueFalseTxt( false );
    if ( checkbox_ && !truetext.isEmpty() )
	checkbox_->setText( truetext );
    else
    {
	if ( !truetext.isEmpty() )
	    rb1_->setText( truetext );
	if ( !falsetext.isEmpty() )
	    rb2_->setText( falsetext );
    }

    setvalue_( spec.getBoolValue() );
    return true;
}


// uiGenInputIntFld
uiGenInputIntFld::uiGenInputIntFld( uiParent* p, int val, const char* nm )
    : uiSpinBox(p,0,nm)
    , UserInputObjImpl<int>()
{
    setvalue_( val );
}


uiGenInputIntFld::uiGenInputIntFld( uiParent* p, const DataInpSpec& spec,
				    const char* nm )
    : uiSpinBox(p,0,nm)
    , UserInputObjImpl<int>()
{
    mDynamicCastGet(const IntInpSpec*,ispec,&spec)
    if ( spec.hasLimits() && ispec )
	uiSpinBox::setInterval( *ispec->limits() );

    setvalue_( spec.getIntValue() );
}


void uiGenInputIntFld::setReadOnly( bool yn )
{ uiSpinBox::setSensitive( !yn ); }

bool uiGenInputIntFld::isReadOnly() const
{ return !uiSpinBox::sensitive(); }

bool uiGenInputIntFld::update_( const DataInpSpec& spec )
{ setvalue_( spec.getIntValue() ); return true; }

void uiGenInputIntFld::setToolTip( const uiString& tt )
{ uiSpinBox::setToolTip( tt ); }

int uiGenInputIntFld::getvalue_() const
{ return uiSpinBox::getIntValue(); }

void uiGenInputIntFld::setvalue_( int val )
{ uiSpinBox::setValue( val ); }

bool uiGenInputIntFld::notifyValueChanging_( const CallBack& cb )
{ uiSpinBox::valueChanging.notify( cb ); return true; }

bool uiGenInputIntFld::notifyValueChanged_( const CallBack& cb )
{ uiSpinBox::valueChanged.notify( cb ); return true; }

bool uiGenInputIntFld::notifyUpdateRequested_( const CallBack& cb )
{ uiSpinBox::valueChanged.notify( cb ); return true; }


// uiGenInputInt64Fld
uiGenInputInt64Fld::uiGenInputInt64Fld( uiParent* p, od_int64 val,
					const char* nm )
    : uiSpinBox(p,0,nm)
    , UserInputObjImpl<od_int64>()
{
    setvalue_( val );
}


uiGenInputInt64Fld::uiGenInputInt64Fld( uiParent* p, const DataInpSpec& spec,
				    const char* nm )
    : uiSpinBox(p,0,nm)
    , UserInputObjImpl<od_int64>()
{
    mDynamicCastGet(const IntInpSpec*,ispec,&spec)
    if ( spec.hasLimits() && ispec )
	uiSpinBox::setInterval( *ispec->limits() );

    setvalue_( spec.getIntValue() );
}


void uiGenInputInt64Fld::setReadOnly( bool yn )
{ uiSpinBox::setSensitive( !yn ); }

bool uiGenInputInt64Fld::isReadOnly() const
{ return !uiSpinBox::sensitive(); }

bool uiGenInputInt64Fld::update_( const DataInpSpec& spec )
{ setvalue_( spec.getIntValue() ); return true; }

void uiGenInputInt64Fld::setToolTip( const uiString& tt )
{ uiSpinBox::setToolTip( tt ); }

od_int64 uiGenInputInt64Fld::getvalue_() const
{ return uiSpinBox::getInt64Value(); }

void uiGenInputInt64Fld::setvalue_( od_int64 val )
{ uiSpinBox::setValue( val ); }

bool uiGenInputInt64Fld::notifyValueChanging_( const CallBack& cb )
{ uiSpinBox::valueChanging.notify( cb ); return true; }

bool uiGenInputInt64Fld::notifyValueChanged_( const CallBack& cb )
{ uiSpinBox::valueChanged.notify( cb ); return true; }

bool uiGenInputInt64Fld::notifyUpdateRequested_( const CallBack& cb )
{ uiSpinBox::valueChanged.notify( cb ); return true; }

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          01/02/2001
________________________________________________________________________

-*/

#include "uispinbox.h"
#include "i_qspinbox.h"

#include "uilabel.h"
#include "uiobjbodyimpl.h"
#include "uivirtualkeyboard.h"

#include "mouseevent.h"
#include "staticstring.h"

#include "q_uiimpl.h"

#include <QContextMenuEvent>
#include <QLineEdit>
#include <QValidator>
#include <limits.h>
#include <math.h>

mUseQtnamespace

class uiSpinBoxBody : public uiObjBodyImpl<uiSpinBox,QDoubleSpinBox>
{
public:
                        uiSpinBoxBody(uiSpinBox&,uiParent*,const char*);
    virtual		~uiSpinBoxBody();

    virtual int		nrTxtLines() const	{ return 1; }
    void		setNrDecimals(int);
    void		setAlpha(bool yn);
    bool		isAlpha() const		{ return isalpha_; }
    bool		hadFocusChg() const	{ return hadfocuschg_; }
    bool		isModified() const;

    QValidator::State	validate( QString& input, int& posn ) const
			{
			    const double val = input.toDouble();
			    if ( val > maximum() )
				input.setNum( maximum() );
			    return QDoubleSpinBox::validate( input, posn );
			}

    virtual double	valueFromText(const QString&) const;
    virtual QString	textFromValue(double value) const;

    QLineEdit*		getLineEdit() const	{ return lineEdit(); }

protected:
    virtual void	contextMenuEvent(QContextMenuEvent*);
    virtual void	focusOutEvent(QFocusEvent*);


private:

    i_SpinBoxMessenger& messenger_;

    QDoubleValidator*	dval;
    bool		isalpha_;
    bool		hadfocuschg_;

};


uiSpinBoxBody::uiSpinBoxBody( uiSpinBox& hndl, uiParent* p, const char* nm )
    : uiObjBodyImpl<uiSpinBox,QDoubleSpinBox>(hndl,p,nm)
    , messenger_(*new i_SpinBoxMessenger(this,&hndl))
    , dval(new QDoubleValidator(this))
    , isalpha_(false)
    , hadfocuschg_(false)
{
    setHSzPol( uiObject::Small );
    setCorrectionMode( QAbstractSpinBox::CorrectToNearestValue );
}


uiSpinBoxBody::~uiSpinBoxBody()
{
    delete &messenger_;
    delete dval;
}


static const char* letters[] =
{ "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m",
  "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z", 0 };

void uiSpinBoxBody::setAlpha( bool yn )
{
    isalpha_ = yn;
    setMinimum( 0 );
    setMaximum( specialValueText().isEmpty() ? 25 : 26 );
    setSingleStep( 1 );
}


double uiSpinBoxBody::valueFromText( const QString& qtxt ) const
{
    if ( !specialValueText().isEmpty() && qtxt==specialValueText() )
	return handle_.minFValue();

    if ( !isalpha_ )
	return QDoubleSpinBox::valueFromText( qtxt );

    for ( int idx=0; idx<26; idx++ )
    {
	if ( qtxt == letters[idx] )
	    return (double)idx;
    }

    return 0;
}


QString uiSpinBoxBody::textFromValue( double val ) const
{
    if ( !specialValueText().isEmpty() && val==handle_.minFValue() )
	return specialValueText();

    if ( !isalpha_ )
	return QDoubleSpinBox::textFromValue( val );

    QString svtxt = specialValueText();
    int intval = mNINT32(val);
    if ( !svtxt.isEmpty() )
	intval--;

    QString str;
    if ( intval < 0 )
	str = "a";
    else if ( intval > 25 )
	str = "z";
    else
	str = letters[intval];

    return str;
}


void uiSpinBoxBody::setNrDecimals( int dec )
{ dval->setDecimals( dec ); }


bool uiSpinBoxBody::isModified() const
{ return lineEdit()->isModified(); }


void uiSpinBoxBody::contextMenuEvent( QContextMenuEvent* ev )
{
    if ( uiVirtualKeyboard::isVirtualKeyboardEnabled() )
	handle_.popupVirtualKeyboard( ev->globalX(), ev->globalY() );
    else
	QDoubleSpinBox::contextMenuEvent( ev );
}


void uiSpinBoxBody::focusOutEvent( QFocusEvent* ev )
{
    hadfocuschg_ = true;
    QAbstractSpinBox::focusOutEvent( ev );
    hadfocuschg_ = false;
}


//------------------------------------------------------------------------------

uiSpinBox::uiSpinBox( uiParent* p, int dec, const char* nm )
    : uiObject(p,nm,mkbody(p,nm))
    , valueChanged(this)
    , valueChanging(this)
    , dosnap_(false)
    , focuschgtrigger_(true)
{
    setNrDecimals( dec );
    setKeyboardTracking( false );
    valueChanged.notify( mCB(this,uiSpinBox,snapToStep) );
    oldvalue_ = getFValue();

    setMaxValue( INT_MAX );
}


uiSpinBox::~uiSpinBox()
{
    valueChanged.remove( mCB(this,uiSpinBox,snapToStep) );
    delete body_;
}


uiSpinBoxBody& uiSpinBox::mkbody(uiParent* parnt, const char* nm )
{
    body_= new uiSpinBoxBody(*this,parnt, nm);
    return *body_;
}

void uiSpinBox::setSpecialValueText( const char* txt )
{
    body_->setSpecialValueText( txt );
    if ( isAlpha() ) setMaxValue( 26 );
}


void uiSpinBox::setAlpha( bool yn )
{
    mBlockCmdRec;
    body_->setAlpha( yn );
}

bool uiSpinBox::isAlpha() const
{ return body_->isAlpha(); }

void uiSpinBox::setNrDecimals( int dec )
{ body_->setDecimals( dec ); }


void uiSpinBox::snapToStep( CallBacker* )
{
    if ( !dosnap_ ) return;

    const double diff = body_->value() - body_->minimum();
    const double stp = body_->singleStep();
    const int ratio =  mNINT32( diff / stp );
    const float newval = minFValue() + ratio * stp;
    setValue( newval );
}


void uiSpinBox::setInterval( const StepInterval<int>& intv )
{
    setMinValue( intv.start );
    setMaxValue( intv.stop );
    setStep( intv.step );
}


void uiSpinBox::setInterval( const StepInterval<float>& intv )
{
    setMinValue( intv.start );
    setMaxValue( intv.stop );
    setStep( intv.step );
}


void uiSpinBox::setInterval( const StepInterval<double>& intv )
{
    setMinValue( (float)intv.start );
    setMaxValue( (float)intv.stop );
    setStep( (float)intv.step );
}


StepInterval<int> uiSpinBox::getInterval() const
{
    return StepInterval<int>(minValue(),maxValue(),step());
}


StepInterval<float> uiSpinBox::getFInterval() const
{
    return StepInterval<float>( minFValue(), maxFValue(), fstep() );
}


int uiSpinBox::getIntValue() const
{ return mNINT32(body_->value()); }

od_int64 uiSpinBox::getInt64Value() const
{ return mNINT64(body_->value()); }

double uiSpinBox::getDValue() const
{ return body_->value(); }

float uiSpinBox::getFValue() const
{ return (float)body_->value(); }

bool uiSpinBox::getBoolValue() const
{ return getIntValue() != 0; }


const char* uiSpinBox::text() const
{
    mDeclStaticString( res );
    res = body_->textFromValue( getFValue() );
    return res;
}

static bool isNotSet( int val )
{ return mIsUdf(val) || val == INT_MAX; }


void uiSpinBox::setValue( od_int64 val )
{
    //TODO do the right thing
    setValue( (int)val );
}

void uiSpinBox::setValue( int val )
{
    mBlockCmdRec;
    if ( mIsUdf(val) )
    {
	if ( isNotSet(-minValue()) && isNotSet(maxValue()) )
	    val = 0;
	else if ( isNotSet(-minValue()) )
	    val = maxValue();
	else if ( isNotSet(maxValue()) )
	    val = minValue();
    }

    body_->setValue( val );
}


void uiSpinBox::setValue( float val )
{
    mBlockCmdRec;
    if ( mIsUdf(val) )
	val = maxFValue();
    body_->setValue( val );
}


void uiSpinBox::setValue( double val )
{
    setValue( mCast(float,val) );
}


void uiSpinBox::setValue( const char* txt )
{
    mBlockCmdRec;
    body_->setValue( body_->valueFromText(txt) );
}


void uiSpinBox::setMinValue( int val )
{
    mBlockCmdRec;
    body_->setMinimum( val );
}


void uiSpinBox::setMinValue( float val )
{
    mBlockCmdRec;
    body_->setMinimum( val );
}


int uiSpinBox::minValue() const
{ return mNINT32(body_->minimum()); }

float uiSpinBox::minFValue() const
{ return (float)body_->minimum(); }


void uiSpinBox::setMaxValue( int val )
{
    mBlockCmdRec;
    const double maxval = mIsUdf(val) ? mUdf(double) : mCast(double,val);
    body_->setMaximum( maxval );
}


void uiSpinBox::setMaxValue( float val )
{
    mBlockCmdRec;
    const double maxval = mIsUdf(val) ? mUdf(double) : mCast(double,val);
    body_->setMaximum( maxval );
}


void uiSpinBox::setMaxValue( double val )
{
    setMaxValue( mCast(float,val) );
}


int uiSpinBox::maxValue() const
{
    const double maxval = body_->maximum();
    return mIsUdf(maxval) ? mUdf(int) : mNINT32(maxval);
}


float uiSpinBox::maxFValue() const
{
    const double maxval = body_->maximum();
    return mIsUdf(maxval) ? mUdf(float) : mCast(float,maxval);
}


int uiSpinBox::step() const
{ return mNINT32(body_->singleStep()); }

float uiSpinBox::fstep() const
{ return (float)body_->singleStep(); }

void uiSpinBox::stepBy( int nrsteps )
{
    mBlockCmdRec;
    body_->stepBy( nrsteps );
}

void uiSpinBox::setStep( int stp, bool snapcur )
{ setStep( (float)stp, snapcur ); }

void uiSpinBox::setStep( float stp, bool snapcur )
{
    mBlockCmdRec;
    if ( mIsZero(stp,mDefEps) ) stp = 1;
    body_->setSingleStep( stp );
    dosnap_ = snapcur;
    snapToStep(0);
}


void uiSpinBox::setStep( double stp, bool snapcur )
{
    setStep( mCast(float,stp), snapcur );
}


void uiSpinBox::setPrefix( const uiString& pfx )
{
    prefix_ = pfx;
    body_->setPrefix( toQString(pfx) );
}


uiString uiSpinBox::prefix() const
{
    return prefix_;
}


void uiSpinBox::setSuffix( const uiString& sfx )
{
    suffix_ = sfx;
    body_->setSuffix( toQString(sfx) );
}


uiString uiSpinBox::suffix() const
{
    return suffix_;
}


void uiSpinBox::translateText()
{
    body_->setSuffix( toQString(suffix_) );
    body_->setPrefix( toQString(prefix_) );
}


void uiSpinBox::setKeyboardTracking( bool yn )
{ body_->setKeyboardTracking( yn ); }

bool uiSpinBox::keyboardTracking() const
{ return body_->keyboardTracking(); }

void uiSpinBox::setFocusChangeTrigger( bool yn )
{ focuschgtrigger_ = yn; }

bool uiSpinBox::focusChangeTrigger() const
{ return focuschgtrigger_; }


void uiSpinBox::notifyHandler( bool editingfinished )
{
    if ( editingfinished && body_->hadFocusChg() && !focuschgtrigger_ )
	return;

    BufferString msg = toString( oldvalue_ );
    oldvalue_ = getFValue();
    msg += editingfinished ? " valueChanged" : " valueChanging";
    const int refnr = beginCmdRecEvent( msg );

    if ( editingfinished )
	valueChanged.trigger( this );
    else
	valueChanging.trigger( this );

    endCmdRecEvent( refnr, msg );
}


bool uiSpinBox::handleLongTabletPress()
{
    const Geom::Point2D<int> pos = TabletInfo::currentState()->globalpos_;
    popupVirtualKeyboard( pos.x_, pos.y_ );
    return true;
}


void uiSpinBox::popupVirtualKeyboard( int globalx, int globaly )
{
    if ( !hasFocus() )
	return;

    uiVirtualKeyboard virkeyboard( *this, globalx, globaly );
    virkeyboard.show();

    if ( virkeyboard.enterPressed() )
	valueChanged.trigger();
}


//------------------------------------------------------------------------------

uiLabeledSpinBox::uiLabeledSpinBox( uiParent* p, const uiString& txt, int dec,
				    const char* nm )
	: uiGroup(p,"LabeledSpinBox")
{
    BufferString boxnm( nm );
    if ( boxnm.isEmpty() )
	boxnm.set( toString(txt) );
    sb_ = new uiSpinBox( this, dec, boxnm );
    BufferString sblbl;
    lbl_ = new uiLabel( this, txt, sb_ );
    lbl_->setAlignment( OD::Alignment::Right );
    setHAlignObj( sb_ );
}

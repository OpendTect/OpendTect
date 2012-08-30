/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          01/02/2001
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uispinbox.cc,v 1.54 2012-08-30 07:52:52 cvsnageswara Exp $";

#include "uispinbox.h"
#include "uilabel.h"

#include "i_qspinbox.h"
#include "mouseevent.h"
#include "uiobjbody.h"
#include "uivirtualkeyboard.h"

#include <QContextMenuEvent>
#include <QLineEdit>
#include <QValidator>
#include <math.h>

class uiSpinBoxBody : public uiObjBodyImpl<uiSpinBox,mQtclass(QDoubleSpinBox)>
{
public:
                        uiSpinBoxBody(uiSpinBox&,uiParent*,const char*);
    virtual		~uiSpinBoxBody();

    virtual int 	nrTxtLines() const	{ return 1; }
    void		setNrDecimals(int);
    void		setAlpha(bool yn);
    bool		isAlpha() const		{ return isalpha_; }

    mQtclass(QValidator)::State	validate( mQtclass(QString&) input,
	    			          int& posn ) const
			{
			    const double val = input.toDouble();
			    if ( val > maximum() )
				input.setNum( maximum() );
			    return mQtclass(QDoubleSpinBox)::validate( input,
				    				       posn );
			}

    virtual double		valueFromText(const mQtclass(QString&)) const;
    virtual mQtclass(QString)	textFromValue(double value) const;

protected:
    virtual void	contextMenuEvent(mQtclass(QContextMenuEvent*));


private:

    i_SpinBoxMessenger& messenger_;

    mQtclass(QDoubleValidator*)	dval;

    bool		isalpha_;

};


uiSpinBoxBody::uiSpinBoxBody( uiSpinBox& hndl, uiParent* p, const char* nm )
    : uiObjBodyImpl<uiSpinBox,mQtclass(QDoubleSpinBox)>(hndl,p,nm)
    , messenger_(*new i_SpinBoxMessenger(this,&hndl))
    , dval(new mQtclass(QDoubleValidator)(this))
    , isalpha_(false)
{
    setHSzPol( uiObject::Small );
    setCorrectionMode( mQtclass(QAbstractSpinBox)::CorrectToNearestValue );
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


double uiSpinBoxBody::valueFromText( const mQtclass(QString&) qtxt ) const
{
    if ( !specialValueText().isEmpty() && qtxt==specialValueText() )
	return handle_.minFValue();

    if ( !isalpha_ )
	return mQtclass(QDoubleSpinBox)::valueFromText( qtxt );

    for ( int idx=0; idx<26; idx++ )
    {
	if ( qtxt == letters[idx] )
	    return (double)idx;
    }

    return 0;
}


mQtclass(QString) uiSpinBoxBody::textFromValue( double val ) const
{
    if ( !specialValueText().isEmpty() && val==handle_.minFValue() )
	return specialValueText();

    if ( !isalpha_ )
	return mQtclass(QDoubleSpinBox)::textFromValue( val );

    mQtclass(QString) svtxt = specialValueText();
    int intval = mNINT32(val);
    if ( !svtxt.isEmpty() )
	intval--;

    mQtclass(QString) str;
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


void uiSpinBoxBody::contextMenuEvent( mQtclass(QContextMenuEvent*) ev )
{ handle().popupVirtualKeyboard( ev->globalX(), ev->globalY() ); }


//------------------------------------------------------------------------------

uiSpinBox::uiSpinBox( uiParent* p, int dec, const char* nm )
    : uiObject(p,nm,mkbody(p,nm))
    , valueChanged(this)
    , valueChanging(this)
    , dosnap_(false)
{
    setNrDecimals( dec );
    setKeyboardTracking( false );
    valueChanged.notify( mCB(this,uiSpinBox,snapToStep) );
    oldvalue_ = getFValue();
}


uiSpinBox::~uiSpinBox()
{
    valueChanged.remove( mCB(this,uiSpinBox,snapToStep) );
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


int uiSpinBox::getValue() const
{ return mNINT32(body_->value()); }

float uiSpinBox::getFValue() const	
{ return (float)body_->value(); }


const char* uiSpinBox::text() const
{
    static BufferString res;
    res = mQStringToConstChar( body_->textFromValue(getFValue()) );
    return res;
}


void uiSpinBox::setValue( int val )
{
    mBlockCmdRec;
    if ( mIsUdf(val) )
	val = maxValue();
    body_->setValue( val );
}

void uiSpinBox::setValue( float val )
{
    mBlockCmdRec;
    if ( mIsUdf(val) )
	val = maxFValue();
    body_->setValue( val );
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
    body_->setMaximum( val );
}

void uiSpinBox::setMaxValue( float val )
{
    mBlockCmdRec;
    body_->setMaximum( val );
}

int uiSpinBox::maxValue() const
{ return mNINT32(body_->maximum()); }

float uiSpinBox::maxFValue() const
{ return (float)body_->maximum(); }

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


void uiSpinBox::setPrefix( const char* pfx )
{ body_->setPrefix( pfx ); }


const char* uiSpinBox::prefix() const
{
    static BufferString res;
    res = mQStringToConstChar( body_->prefix() );
    return res;
}


void uiSpinBox::setSuffix( const char* sfx )
{ body_->setSuffix( sfx ); }


const char* uiSpinBox::suffix() const
{
    static BufferString res;
    res = mQStringToConstChar( body_->suffix() );
    return res;
}


void uiSpinBox::setKeyboardTracking( bool yn )
{
#if QT_VERSION >= 0x040300
    body_->setKeyboardTracking( yn );
#endif
}


bool uiSpinBox::keyboardTracking() const
{
#if QT_VERSION < 0x040300
    return false;
#else
    return body_->keyboardTracking();
#endif
}


void uiSpinBox::notifyHandler( bool editingfinished )
{
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
    popupVirtualKeyboard( pos.x, pos.y );
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

uiLabeledSpinBox::uiLabeledSpinBox( uiParent* p, const char* txt, int dec,
				    const char* nm )
	: uiGroup(p,"LabeledSpinBox")
{
    sb = new uiSpinBox( this, dec, nm && *nm ? nm : txt );
    BufferString sblbl;
    lbl = new uiLabel( this, txt, sb );
    lbl->setAlignment( Alignment::Right );
    setHAlignObj( sb );
}

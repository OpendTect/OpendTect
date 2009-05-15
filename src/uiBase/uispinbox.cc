/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          01/02/2001
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uispinbox.cc,v 1.37 2009-05-15 16:28:43 cvsjaap Exp $";

#include "uispinbox.h"
#include "uilabel.h"

#include "i_qspinbox.h"
#include "uiobjbody.h"

#include <QApplication>
#include <QEvent>
#include <QLineEdit>
#include <QValidator>
#include <math.h>

class uiSpinBoxBody : public uiObjBodyImpl<uiSpinBox,QDoubleSpinBox>
{
public:

                        uiSpinBoxBody(uiSpinBox&,uiParent*, const char* );

    virtual		~uiSpinBoxBody();

    virtual int 	nrTxtLines() const	{ return 1; }
    void		setNrDecimals(int);

    void		activateField(const char* txt=0,bool enter=true);
    void		activateStep(int nrsteps);
    bool		event(QEvent*);

    QValidator::State	validate( QString& input, int& pos ) const
			{
			    const double val = input.toDouble();
			    if ( val > maximum() )
				input.setNum( maximum() );
			    return QDoubleSpinBox::validate( input, pos );
			}

protected:

    virtual int		mapTextToValue( bool* ok );
    virtual QString	mapValueToText( int v );

    const char*		activatetxt_;
    bool		activateenter_;
    int 		activatesteps_;

private:

    i_SpinBoxMessenger& messenger_;

    QDoubleValidator*	dval;

};


uiSpinBoxBody::uiSpinBoxBody( uiSpinBox& handle, uiParent* p, const char* nm )
    : uiObjBodyImpl<uiSpinBox,QDoubleSpinBox>(handle,p,nm)
    , messenger_(*new i_SpinBoxMessenger(this,&handle))
    , dval(new QDoubleValidator(this))
{
    setHSzPol( uiObject::Small );
    setCorrectionMode( QAbstractSpinBox::CorrectToNearestValue );
}


uiSpinBoxBody::~uiSpinBoxBody()
{
    delete &messenger_;
    delete dval;
}


int uiSpinBoxBody::mapTextToValue( bool* ok )
{
    return (int)(cleanText().toFloat(ok) * handle_.factor_);
}


QString uiSpinBoxBody::mapValueToText( int val )
{
    QString s;
    s.setNum( (float)val / handle_.factor_ );
    return s;
}


void uiSpinBoxBody::setNrDecimals( int dec )
{
    dval->setDecimals( dec );
}


static const QEvent::Type sQEventActField = (QEvent::Type) (QEvent::User+0);
static const QEvent::Type sQEventActStep  = (QEvent::Type) (QEvent::User+1);


void uiSpinBoxBody::activateField( const char* txt, bool enter )
{
    activatetxt_ = txt;
    activateenter_ = enter;
    QEvent* actevent = new QEvent( sQEventActField );
    QApplication::postEvent( this, actevent );
}


void uiSpinBoxBody::activateStep( int nrsteps )
{
    activatesteps_ = nrsteps;
    QEvent* actevent = new QEvent( sQEventActStep );
    QApplication::postEvent( this, actevent );
}


bool uiSpinBoxBody::event( QEvent* ev )
{ 
    if ( ev->type() == sQEventActField )
    {
	if ( activatetxt_ )
	    setValue( valueFromText(QString(activatetxt_ )) );
	if ( activateenter_ )
	    handle_.valueChanged.trigger();
    }
    else if ( ev->type() == sQEventActStep  )
	stepBy( activatesteps_ );
    else
	return QDoubleSpinBox::event( ev );

    handle_.activatedone.trigger();
    return true;
}


//------------------------------------------------------------------------------

uiSpinBox::uiSpinBox( uiParent* p, int dec, const char* nm )
    : uiObject(p,nm,mkbody(p,nm))
    , valueChanged(this)
    , valueChanging(this)
    , activatedone(this)
    , dosnap_(false)
    , factor_(1)
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


void uiSpinBox::activateField(const char* txt, bool enter)
{ body_->activateField( txt, enter ); }


void uiSpinBox::activateStep( int nrsteps )
{ body_->activateStep( nrsteps ); }


void uiSpinBox::setNrDecimals( int dec )
{
    body_->setDecimals( dec );
}


void uiSpinBox::snapToStep( CallBacker* )
{
    if ( !dosnap_ ) return;

    const float diff = getFValue() - minFValue();
    const float step = fstep();
    const int ratio =  int (diff / step);
    const float newval = minFValue() + ratio * step;
    setValue( newval );
}


void uiSpinBox::setInterval( const StepInterval<int>& intv )
{
    setMinValue( intv.start );
    setMaxValue( intv.stop );
    setStep( intv.step ? intv.step : 1 );
}


void uiSpinBox::setInterval( const StepInterval<float>& intv )
{
    setMinValue( intv.start );
    setMaxValue( intv.stop );
    setStep( intv.step ? intv.step : 1 );
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
{ return mNINT(body_->value()); }

float uiSpinBox::getFValue() const	
{ return (float)body_->value(); }

void uiSpinBox::setValue( int val )
{
    if ( mIsUdf(val) )
	val = maxValue();
    body_->setValue( val );
}

void uiSpinBox::setValue( float val )
{
    if ( mIsUdf(val) )
	val = maxFValue();
    body_->setValue( val );
}

void uiSpinBox::setMinValue( int val )
{ body_->setMinimum( val ); }

void uiSpinBox::setMinValue( float val )
{ body_->setMinimum( val ); }

int uiSpinBox::minValue() const
{ return mNINT(body_->minimum()); }

float uiSpinBox::minFValue() const
{ return (float)body_->minimum(); }

void uiSpinBox::setMaxValue( int val )
{ body_->setMaximum( val ); }

void uiSpinBox::setMaxValue( float val )
{ body_->setMaximum( val ); }

int uiSpinBox::maxValue() const
{ return mNINT(body_->maximum()); }

float uiSpinBox::maxFValue() const
{ return (float)body_->maximum(); }

int uiSpinBox::step() const
{ return mNINT(body_->singleStep()); }

float uiSpinBox::fstep() const
{ return (float)body_->singleStep(); }

void uiSpinBox::setStep( int step_, bool snapcur )		
{ setStep( (double)step_, snapcur ); }

void uiSpinBox::setStep( float step_, bool snapcur )
{
    if ( !step_ ) step_ = 1;
    body_->setSingleStep( step_ );
    dosnap_ = snapcur;
    snapToStep(0);
}


void uiSpinBox::setPrefix( const char* suffix )
{ body_->setPrefix( suffix ); }


const char* uiSpinBox::prefix() const
{
    static BufferString res;
    res = mQStringToConstChar( body_->prefix() );
    return res;
}


void uiSpinBox::setSuffix( const char* suffix )
{ body_->setSuffix( suffix ); }


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

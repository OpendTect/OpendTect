/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          25/05/2000
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uilineedit.cc,v 1.34 2009-06-08 08:36:00 cvsjaap Exp $";

#include "uilineedit.h"
#include "i_qlineedit.h"

#include "uibody.h"
#include "uiobjbody.h"
#include "datainpspec.h"

#include <QApplication>
#include <QEvent>
#include <QSize> 
#include <QCompleter>
#include <QIntValidator>
#include <QDoubleValidator>

class uiLineEditBody : public uiObjBodyImpl<uiLineEdit,QLineEdit>
{
public:

                        uiLineEditBody( uiLineEdit& handle,
				   uiParent*, const char* nm="Line Edit body");

    virtual		~uiLineEditBody()		{ delete &messenger_; }

    virtual int 	nrTxtLines() const		{ return 1; }

    void 		activate(const char* txt=0,bool enter=true);
    bool 		event(QEvent*);

protected:
    const char*		activatetxt_;
    bool 		activateenter_;

private:

    i_lineEditMessenger& messenger_;

};


uiLineEditBody::uiLineEditBody( uiLineEdit& handle,uiParent* parnt, 
				const char* nm )
    : uiObjBodyImpl<uiLineEdit,QLineEdit>(handle,parnt,nm)
    , messenger_ ( *new i_lineEditMessenger(this,&handle) )
{ 
    setStretch( 1, 0 ); 
    setHSzPol( uiObject::Medium );
}


static const QEvent::Type sQEventActivate = (QEvent::Type) (QEvent::User+0);

void uiLineEditBody::activate( const char* txt, bool enter )
{
    activatetxt_ = txt;
    activateenter_ = enter;
    QEvent* actevent = new QEvent( sQEventActivate );
    QApplication::postEvent( this, actevent );
}


bool uiLineEditBody::event( QEvent* ev )
{
    if ( ev->type() != sQEventActivate ) 
	return QLineEdit::event( ev );

    if ( !handle_.isReadOnly() )
    {
	if ( activatetxt_ )
	    handle_.setvalue_( activatetxt_ );
	if ( activateenter_ )
	    handle_.returnPressed.trigger();
	if ( activatetxt_ || activateenter_ )
	    handle_.editingFinished.trigger();
    }
    
    handle_.activatedone.trigger();
    return true;
}


//------------------------------------------------------------------------------


uiLineEdit::uiLineEdit( uiParent* parnt, const DataInpSpec& spec,
			const char* nm )
    : uiObject( parnt, nm, mkbody(parnt,nm) )
    , editingFinished(this), returnPressed(this)
    , textChanged(this), activatedone(this)
    , UserInputObjImpl<const char*>()
{
    setText( spec.text() );
}


uiLineEdit::uiLineEdit( uiParent* parnt, const char* nm ) 
    : uiObject( parnt, nm, mkbody(parnt,nm) )
    , editingFinished(this), returnPressed(this)
    , textChanged(this), activatedone(this)
    , UserInputObjImpl<const char*>()
{
    setText( "" );
}


uiLineEditBody& uiLineEdit::mkbody( uiParent* parnt, const char* nm )
{ 
    body_ = new uiLineEditBody(*this,parnt,nm);
    return *body_; 
}


void uiLineEdit::activate( const char* txt, bool enter )
{ body_->activate( txt, enter ); }


const char* uiLineEdit::getvalue_() const
{
    result_ = mQStringToConstChar( body_->text() );
    return result_;
}


void uiLineEdit::setvalue_( const char* t )
{
    body_->setText( mIsUdf(t) ? QString() : QString(t) );
    body_->setCursorPosition( 0 );
    setEdited( false );
}


void uiLineEdit::setPasswordMode()
{
    body_->setEchoMode( QLineEdit::Password );
}


void uiLineEdit::setValidator( const uiIntValidator& val )
{
    body_->setValidator( new QIntValidator(val.bottom_,val.top_,body_) );
}


void uiLineEdit::setValidator( const uiFloatValidator& val )
{
    QDoubleValidator* qdval =
	new QDoubleValidator( val.bottom_, val.top_, val.nrdecimals_, body_ );
    if ( !val.scnotation_ )
	qdval->setNotation( QDoubleValidator::StandardNotation );
    body_->setValidator( qdval );
}


void uiLineEdit::setMaxLength( int maxtxtlength )
{ body_->setMaxLength( maxtxtlength ); }

int uiLineEdit::maxLength() const
{ return body_->maxLength(); }

void uiLineEdit::setEdited( bool yn )
{ body_->setModified( yn ); }

bool uiLineEdit::isEdited() const
{ return body_->isModified(); }

void uiLineEdit::setCompleter( const BufferStringSet& bs, bool cs )
{
    QStringList qsl;
    for ( int idx=0; idx<bs.size(); idx++ )
	qsl << QString( bs.get(idx) );

    QCompleter* qc = new QCompleter( qsl, 0 );
    qc->setCaseSensitivity( cs ? Qt::CaseSensitive : Qt::CaseInsensitive );
    body_->setCompleter( qc );
}


void uiLineEdit::setReadOnly( bool yn )
{ body_->setReadOnly( yn ); }

bool uiLineEdit::isReadOnly() const
{ return body_->isReadOnly(); }

bool uiLineEdit::update_( const DataInpSpec& spec )
{ setText( spec.text() ); return true; }

void uiLineEdit::home()
{ body_->home( false ); }

void uiLineEdit::end()
{ body_->end( false ); }

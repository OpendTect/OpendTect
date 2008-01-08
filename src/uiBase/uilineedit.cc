/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          25/05/2000
 RCS:           $Id: uilineedit.cc,v 1.26 2008-01-08 03:45:00 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uilineedit.h"
#include "i_qlineedit.h"

#include "uibody.h"
#include "uiobjbody.h"
#include "datainpspec.h"

#include <QApplication>
#include <QEvent>
#include <QSize> 


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

    if ( activatetxt_ )
	handle_.setvalue_( activatetxt_ );
    if ( activateenter_ )
	handle_.returnPressed.trigger();
    
    handle_.activatedone.trigger();
    return true;
}


//------------------------------------------------------------------------------

uiLineEdit::uiLineEdit( uiParent* parnt, const char* deftxt, const char* nm ) 
    : uiObject( parnt, nm, mkbody(parnt,nm) )
    , editingFinished(this), returnPressed(this)
    , textChanged(this), activatedone(this)
    , UserInputObjImpl<const char*>()
{
    setText( deftxt ? deftxt : "" );
}

uiLineEdit::uiLineEdit( uiParent* parnt, const DataInpSpec& spec,
			const char* nm )
    : uiObject( parnt, nm, mkbody(parnt,nm) )
    , editingFinished(this), returnPressed(this)
    , textChanged(this), activatedone(this)
    , UserInputObjImpl<const char*>()
{
    setText( spec.text() ? spec.text() : "" );
}


uiLineEditBody& uiLineEdit::mkbody( uiParent* parnt, const char* nm )
{ 
    body_ = new uiLineEditBody(*this,parnt,nm);
    return *body_; 
}


void uiLineEdit::activate(const char* txt, bool enter)
{ body_->activate( txt, enter ); }


const char* uiLineEdit::getvalue_() const
{
    const_cast<uiLineEdit*>(this)->result = (const char*)body_->text();
    return result;
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

//! Sets the maximum permitted length of the text
void uiLineEdit::setMaxLength( int maxtxtlength )
{
    body_->setMaxLength( maxtxtlength );
}


int uiLineEdit::maxLength() const
{
    return body_->maxLength();
}


/*!  Sets the edited flag of this line edit to \a yn.  The edited flag
is never read by uiLineEdit, and is changed to true whenever the user
changes its contents.
\sa isEdited()
*/
void uiLineEdit::setEdited( bool yn )
{
    body_->setEdited( yn );
}


/*!  Returns the edited flag of the line edit.  If this returns false,
the line edit's contents have not been changed since the construction
of the uiLineEdit (or the last call to setEdited( false ), if any).  If
it returns true, the contents have been edited, or setEdited( true )
has been called.

\sa setEdited()
*/
bool uiLineEdit::isEdited() const
{
    return body_->edited();
}


void uiLineEdit::setReadOnly( bool yn )
{
    body_->setReadOnly( yn );
}


bool uiLineEdit::isReadOnly() const
{
    return body_->isReadOnly();
}

bool uiLineEdit::update_( const DataInpSpec& spec )
{
    setText( spec.text() );
    return true;
}

void uiLineEdit::home()
{
    body_->home( false );
}

void uiLineEdit::end()
{
    body_->end( false );
}


/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          25/05/2000
 RCS:           $Id: uilineedit.cc,v 1.13 2002-05-29 15:00:45 arend Exp $
________________________________________________________________________

-*/

#include "uilineedit.h"
#include "uibody.h"
#include "i_qlineedit.h"
#include "uiobjbody.h"
#include <datainpspec.h>

#include <qsize.h> 


class uiLineEditBody : public uiObjBodyImpl<uiLineEdit,QLineEdit>
{
public:

                        uiLineEditBody( uiLineEdit& handle,
				   uiParent*, const char* nm="Line Edit body");

    virtual int 	nrTxtLines() const		{ return 1; }

private:

    i_lineEditMessenger& messenger_;

};




uiLineEditBody::uiLineEditBody( uiLineEdit& handle,uiParent* parnt, 
				const char* nm )
    : uiObjBodyImpl<uiLineEdit,QLineEdit>(handle, parnt, nm)
    , messenger_ ( *new i_lineEditMessenger( this, &handle ))
{ 
    setStretch( 1, 0 ); 
    setHSzPol(uiObject::medium);
}

//------------------------------------------------------------------------------

uiLineEdit::uiLineEdit( uiParent* parnt, const char* deftxt, const char* nm )
    : uiObject( parnt, nm, mkbody(parnt,nm) )
    , returnPressed(this), textChanged(this)
    , UserInputObjImpl<const char*>()
{
    setText( deftxt ? deftxt : "" );
}

uiLineEdit::uiLineEdit( uiParent* parnt, const DataInpSpec& spec,
			const char* nm )
    : uiObject( parnt, nm, mkbody(parnt,nm) )
    , returnPressed(this), textChanged(this)
    , UserInputObjImpl<const char*>()
{
    setText( spec.text() ? spec.text() : "" );
}

uiLineEditBody& uiLineEdit::mkbody( uiParent* parnt, const char* nm)
{ 
    body_ = new uiLineEditBody(*this,parnt,nm);
    return *body_; 
}


const char* uiLineEdit::getvalue_() const
{
    const_cast<uiLineEdit*>(this)->result = (const char*)body_->text();
    return (const char*)result;

    if ( *(const char*)result ) return (const char*)result;

    return isEdited() ? (const char*)result : undefVal<const char*>();
}


void uiLineEdit::setvalue_( const char* t )
{
    body_->setText( isUndefined(t) ? QString() : QString(t) );
    body_->setCursorPosition( 0 );
    setEdited( false );
}


void uiLineEdit::setPasswordMode()
{
    body_->setEchoMode( QLineEdit::Password );
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

    if( yn )
	body_->setBackgroundColor( QColor() );
    else
	body_->setBackgroundMode( QWidget::PaletteBase );

}


bool uiLineEdit::isReadOnly() const
{
    return body_->isReadOnly();
}

bool uiLineEdit::update( const DataInpSpec& spec )
{
    setText( spec.text() );
    return true;
}


/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          25/05/2000
 RCS:           $Id: uilineedit.cc,v 1.1 2000-11-27 10:20:35 bert Exp $
________________________________________________________________________

-*/

#include "uilineedit.h"

#include "i_qlineedit.h"
#include "i_qobjwrap.h"

#include "qsize.h" 

//------------------------------------------------------------------------------

uiLineEdit::uiLineEdit(  uiObject* parnt, const char* deftxt, const char* nm )
	: uiWrapObj<i_QLineEdit>(new i_QLineEdit(*this,parnt,deftxt), parnt, nm)
	, _messenger ( *new i_lineEditMessenger( mQtThing(), this ))
	, textChanged(this)
	, returnPressed(this)
{
    setText( deftxt ? deftxt : "" );
}

const QWidget* 	uiLineEdit::qWidget_() const 	{ return mQtThing(); } 

const char* uiLineEdit::text() const
{
    const_cast<uiLineEdit*>(this)->result = (const char*)mQtThing()->text();
    return (const char*)result;
}


int uiLineEdit::getIntValue() const
{
    const_cast<uiLineEdit*>(this)->result = (const char*)mQtThing()->text();
    return *(const char*)result ? atof(result) : 0;
}


double uiLineEdit::getValue() const
{
    const_cast<uiLineEdit*>(this)->result = (const char*)mQtThing()->text();
    return *(const char*)result ? atof(result) : mUndefValue;
}


void uiLineEdit::setText( const char* t )
{
    mQtThing()->setText( QString( t ));
    mQtThing()->setCursorPosition( 0 );
}


void uiLineEdit::setValue( double d )
{
    BufferString s;
    if ( !mIsUndefined(d) ) s += d;
    setText( s );
}


void uiLineEdit::setValue( float f )
{
    BufferString s;
    if ( !mIsUndefined(f) ) s += f;
    setText( s );
}


/*!  Sets the edited flag of this line edit to \a yn.  The edited flag
is never read by uiLineEdit, and is changed to true whenever the user
changes its contents.
\sa isEdited()
*/
void uiLineEdit::setEdited( bool yn )
{
    mQtThing()->setEdited( yn );
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
    return mQtThing()->edited();
}


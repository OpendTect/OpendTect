/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          25/05/2000
 RCS:           $Id: uilineedit.cc,v 1.9 2002-01-10 11:14:52 arend Exp $
________________________________________________________________________

-*/

#include "uilineedit.h"
#include "uibody.h"
#include "i_qlineedit.h"
#include "uiobjbody.h"

#include <qsize.h> 


class uiLineEditBody : public uiObjBodyImpl<uiLineEdit,QLineEdit>
{
public:

                        uiLineEditBody( uiLineEdit& handle,
				   uiParent*,const char* starttxt=0,
				   const char* nm="Line Edit");

    virtual int 	nrTxtLines() const		{ return 1; }

private:

    i_lineEditMessenger& messenger_;

};




uiLineEditBody::uiLineEditBody( uiLineEdit& handle,uiParent* parnt, 
				const char* deftxt, const char* nm )
    : uiObjBodyImpl<uiLineEdit,QLineEdit>(handle, parnt, nm)
    , messenger_ ( *new i_lineEditMessenger( this, &handle ))
{ 
    setStretch( 1, 0 ); 
    setSzPol( SzPolicySpec().setHSzP(SzPolicySpec::medium) );
}

//------------------------------------------------------------------------------

uiLineEdit::uiLineEdit( uiParent* parnt, const char* deftxt, const char* nm )
    : uiObject( parnt, nm, mkbody(parnt,deftxt,nm) )
    , textChanged(this)
    , returnPressed(this)
{
    setText( deftxt ? deftxt : "" );
}

uiLineEditBody& uiLineEdit::mkbody( uiParent* parnt, const char* deftxt, 
				    const char* nm)
{ 
    body_ = new uiLineEditBody(*this,parnt,deftxt,nm);
    return *body_; 
}


const char* uiLineEdit::text() const
{
    const_cast<uiLineEdit*>(this)->result = (const char*)body_->text();
    return (const char*)result;
}


int uiLineEdit::getIntValue() const
{
    const_cast<uiLineEdit*>(this)->result = (const char*)body_->text();
    return *(const char*)result ? atoi(result) : 0;
}


double uiLineEdit::getValue() const
{
    const_cast<uiLineEdit*>(this)->result = (const char*)body_->text();
    return *(const char*)result ? atof(result) : mUndefValue;
}


void uiLineEdit::setText( const char* t )
{
    body_->setText( QString( t ));
    body_->setCursorPosition( 0 );
}


void uiLineEdit::setValue( int i )
{
    BufferString s=i;
    setText( s );
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


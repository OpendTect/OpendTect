#ifndef uilineedit_h
#define uilineedit_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          21/9/2000
 RCS:           $Id: uilineedit.h,v 1.4 2001-08-23 14:59:17 windev Exp $
________________________________________________________________________

-*/

#include <uiobj.h>

class uiLineEditBody;

class uiLineEdit : public uiObject
{
public:

                        uiLineEdit(uiParent*,const char* starttxt=0,
				   const char* nm="Line Edit");

    const char*		text() const;
    int 		getIntValue() const;
    double 		getValue() const;

    void		setText( const char* );
    void		setValue( int );
    void		setValue( float );
    void		setValue( double );

    void		setEdited( bool = true );
    bool		isEdited() const;

    void		setReadOnly( bool = true );
    bool		isReadOnly() const;

    Notifier<uiLineEdit> textChanged;
    Notifier<uiLineEdit> returnPressed;

private:

    uiLineEditBody*	body_;
    uiLineEditBody&	mkbody(uiParent*, const char*, const char*);

    BufferString	result;

};

#endif

#ifndef uilineedit_h
#define uilineedit_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          21/9/2000
 RCS:           $Id: uilineedit.h,v 1.3 2001-05-03 12:14:24 arend Exp $
________________________________________________________________________

-*/

#include <uiobj.h>
class QLineEdit;
template <class T> class i_QObjWrapper;
mTemplTypeDefT(i_QObjWrapper,QLineEdit,i_QLineEdit)

class	i_lineEditMessenger;

class uiLineEdit : public uiWrapObj<i_QLineEdit>
{
public:

                        uiLineEdit(uiObject*,const char* starttxt=0,
				   const char* nm="Line Edit");

    virtual bool        isSingleLine() const { return true; }

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

protected:

    const QWidget*	qWidget_() const;

private:

    i_lineEditMessenger& _messenger;

    BufferString	result;

};

#endif

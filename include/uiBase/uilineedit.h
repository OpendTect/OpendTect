#ifndef uilineedit_h
#define uilineedit_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          21/9/2000
 RCS:           $Id: uilineedit.h,v 1.14 2005-01-12 16:13:43 arend Exp $
________________________________________________________________________

-*/

#include <uiobj.h>
#include <userinputobj.h>

class uiLineEditBody;

class uiLineEdit : public UserInputObjImpl<const char*>, public uiObject
{
public:
			//! pref_empty : return empty string/ null value
			//  insted of undefined value, when line edit is empty.
                        uiLineEdit(uiParent*,const char* starttxt=0,
				   const char* nm="Line Edit" );

                        uiLineEdit(uiParent*,const DataInpSpec&,
				   const char* nm="Line Edit" );

    void		setEdited( bool = true );
    bool		isEdited() const;

    virtual void	setReadOnly( bool = true );
    virtual bool	isReadOnly() const;
    virtual bool	update_( const DataInpSpec& spec );

    void		setPasswordMode();

    void		setMaxLength(int);
    int			maxLength() const;

			//! Moves the text cursor to the beginning of the line. 
    void		home();
			//! Moves the text cursor to the end of the line. 
    void		end();

    Notifier<uiLineEdit> returnPressed;	
    Notifier<uiLineEdit> textChanged;	

protected:

    virtual const char*	getvalue_() const;
    virtual void	setvalue_( const char* );

    virtual bool	notifyValueChanging_( const CallBack& cb )
			    { textChanged.notify(cb); return true;}
    virtual bool	notifyValueChanged_( const CallBack& cb ) 
			    { returnPressed.notify(cb); return true;}

private:

    uiLineEditBody*	body_;
    uiLineEditBody&	mkbody(uiParent*, const char*);

    BufferString	result;
};
#endif

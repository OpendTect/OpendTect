#ifndef uilineedit_h
#define uilineedit_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          21/9/2000
 RCS:           $Id: uilineedit.h,v 1.6 2002-02-21 17:43:09 arend Exp $
________________________________________________________________________

-*/

#include <userinputobj.h>
#include <uiobj.h>

class uiLineEditBody;

class uiLineEdit : public UserInputObjImpl<const char*>, public uiObject
{
public:

                        uiLineEdit(uiParent*,const char* starttxt=0,
				   const char* nm="Line Edit");

    void		setEdited( bool = true );
    bool		isEdited() const;

    void		setReadOnly( bool = true );
    bool		isReadOnly() const;

    void		setPasswordMode();

    virtual bool	notifyValueChanging( const CallBack& cb )
			    { textChanged.notify(cb); return true;}
    virtual bool	notifyValueChanged( const CallBack& cb ) 
			    { returnPressed.notify(cb); return true;}

    Notifier<uiLineEdit> returnPressed;	
    Notifier<uiLineEdit> textChanged;	

protected:

    virtual void	clear_()			{ setvalue_(""); }
    virtual const char*	getvalue_() const;
    virtual void	setvalue_( const char* );

private:

    uiLineEditBody*	body_;
    uiLineEditBody&	mkbody(uiParent*, const char*, const char*);

    BufferString	result;
};
#endif

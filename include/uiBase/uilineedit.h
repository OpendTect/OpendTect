#ifndef uilineedit_h
#define uilineedit_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          21/9/2000
 RCS:           $Id: uilineedit.h,v 1.7 2002-03-12 12:11:40 arend Exp $
________________________________________________________________________

-*/

#include <uiobj.h>
#include <userinputobj.h>

class uiLineEditBody;

class uiLineEdit : public UserInputObjImpl<const char*>, public uiObject
{
public:

                        uiLineEdit(uiParent*,const char* starttxt=0,
				   const char* nm="Line Edit");

    void		setEdited( bool = true );
    bool		isEdited() const;

    virtual void	setReadOnly( bool = true );
    virtual bool	isReadOnly() const;
    virtual bool	update( const DataInpSpec& spec );

    void		setPasswordMode();

    Notifier<uiLineEdit> returnPressed;	
    Notifier<uiLineEdit> textChanged;	

protected:

    virtual void	clear_()			{ setvalue_(""); }
    virtual const char*	getvalue_() const;
    virtual void	setvalue_( const char* );

    virtual bool	notifyValueChanging_( const CallBack& cb )
			    { textChanged.notify(cb); return true;}
    virtual bool	notifyValueChanged_( const CallBack& cb ) 
			    { returnPressed.notify(cb); return true;}

private:

    uiLineEditBody*	body_;
    uiLineEditBody&	mkbody(uiParent*, const char*, const char*);

    BufferString	result;
};
#endif

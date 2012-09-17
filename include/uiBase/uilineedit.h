#ifndef uilineedit_h
#define uilineedit_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          21/9/2000
 RCS:           $Id: uilineedit.h,v 1.27 2010/10/28 11:09:08 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiobj.h"
#include "userinputobj.h"

class uiLineEditBody;

mClass uiIntValidator
{
public:
    		uiIntValidator()
		    : bottom_(-mUdf(int)), top_(mUdf(int))	{}
    		uiIntValidator( int bot, int top )
		    : bottom_(bot), top_(top)			{}

    int		bottom_;
    int		top_;
};


mClass uiFloatValidator
{
public:
    		uiFloatValidator()
		    : bottom_(-mUdf(float)), top_(mUdf(float))
		    , nrdecimals_(1000), scnotation_(true)	{}
    		uiFloatValidator( float bot, float top )
		    : bottom_(bot), top_(top)
		    , nrdecimals_(1000), scnotation_(true)	{}

    float	bottom_;
    float	top_;
    int		nrdecimals_;
    bool	scnotation_;	// If true, ScientificNotation is used
};


mClass uiLineEdit : public UserInputObjImpl<const char*>, public uiObject
{
public:
			//! pref_empty : return empty string/ null value
			//  insted of undefined value, when line edit is empty.
                        uiLineEdit(uiParent*,const char* nm);
                        uiLineEdit(uiParent*,const DataInpSpec&,const char* nm);

    void		setEdited(bool=true);
    bool		isEdited() const;

    void		setCompleter(const BufferStringSet& bs,
	    			     bool casesensitive=false);

    virtual void	setReadOnly(bool=true);
    virtual bool	isReadOnly() const;
    virtual bool	update_(const DataInpSpec&);

    void		setPasswordMode();
    void		setValidator(const uiIntValidator&);
    void		setValidator(const uiFloatValidator&);

    void		setMaxLength(int);
    int			maxLength() const;
    void		setNrDecimals( int nrdec )	{ nrdecimals_ = nrdec; }

			//! Moves the text cursor to the beginning of the line. 
    void		home();
			//! Moves the text cursor to the end of the line. 
    void		end();

    void		backspace();
    void		del();
    void		cursorBackward(bool mark,int steps=1);
    void		cursorForward(bool mark,int steps=1);
    int			cursorPosition() const;
    void		insert( const char* );

    int			selectionStart() const;
    const char*		selectedText() const;
    void		setSelection(int start,int length);

    bool		handleLongTabletPress();
    void		popupVirtualKeyboard(int globalx=-1,int globaly=-1);

    Notifier<uiLineEdit> editingFinished;	
    Notifier<uiLineEdit> returnPressed;	
    Notifier<uiLineEdit> textChanged;	
    Notifier<uiLineEdit> selectionChanged;	


    virtual const char*	getvalue_() const;
    virtual void	setvalue_( const char* );

    void		setToolTip( const char* tt )
    			{ uiObject::setToolTip(tt); }

protected:
    
    virtual bool	notifyValueChanging_( const CallBack& cb )
			{ textChanged.notify( cb ); return true;}
    virtual bool	notifyValueChanged_( const CallBack& cb ) 
			{ editingFinished.notify( cb ); return true;}
    virtual bool	notifyUpdateRequested_( const CallBack& cb )
    			{ returnPressed.notify( cb ); return true; }

private:

    uiLineEditBody*	body_;
    uiLineEditBody&	mkbody(uiParent*, const char*);

    mutable BufferString result_;
    int			nrdecimals_;

};
#endif

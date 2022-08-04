#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          21/9/2000
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uiobj.h"
#include "userinputobj.h"

#include "bufstringset.h"

class uiLineEditBody;

#define mDefTextValidator "^[^'!'].+$"
#define mTextVlAllCharsAccepted "^.+$"

mExpClass(uiBase) uiIntValidator
{
public:
		uiIntValidator()
		    : bottom_(-mUdf(int)), top_(mUdf(int))	{}
		uiIntValidator( int bot, int top )
		    : bottom_(bot), top_(top)			{}

    int		bottom_;
    int		top_;
};


mExpClass(uiBase) uiFloatValidator
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


mExpClass( uiBase ) uiTextValidator
{
public:
		    uiTextValidator()
			: leastnrocc_(mUdf(int))
			, maxnrocc_(mUdf(int))
			, excludfirstocconly_(true)
		    {
			regexchars_.setEmpty();
		    }
		    uiTextValidator(const BufferStringSet& regexchars,
				    const int leastnrocc=mUdf(int),
				    const int maxnrocc=mUdf(int),
				    bool excludfirstocconly=true)
			: regexchars_(regexchars)
			, leastnrocc_(leastnrocc)
			, maxnrocc_(maxnrocc)
			, excludfirstocconly_(excludfirstocconly)
		    {}
		    uiTextValidator(const uiTextValidator& textvl)
			: regexchars_(textvl.regexchars_)
			, leastnrocc_(textvl.leastnrocc_)
			, maxnrocc_(textvl.maxnrocc_)
			, excludfirstocconly_(textvl.excludfirstocconly_)
		    {}

    BufferStringSet	    regexchars_;
    int			    leastnrocc_;
    int			    maxnrocc_;
    bool		    excludfirstocconly_;

    BufferString	    getRegExString() const;
};


#define mUseDefaultTextValidatorOnField(fld) \
    BufferStringSet regchars; \
    regchars.add( "!" ); \
    uiTextValidator txtvl( regchars ); \
    fld->setTextValidator( txtvl ); \



mExpClass(uiBase) uiLineEdit : public UserInputObjImpl<const char*>,
			       public uiObject
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
    void		setPlaceholderText(const uiString&);

    void		setReadOnly(bool=true) override;
    bool		isReadOnly() const override;
    bool		update_(const DataInpSpec&) override;

    void		setPasswordMode();
    void		setValidator(const uiIntValidator&);
    void		setValidator(const uiFloatValidator&);
    void		setTextValidator(const uiTextValidator&);

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

    bool		handleLongTabletPress() override;
    void		popupVirtualKeyboard(int globalx=-1,int globaly=-1);

    Notifier<uiLineEdit> editingFinished;
    Notifier<uiLineEdit> returnPressed;
    Notifier<uiLineEdit> textChanged;
    Notifier<uiLineEdit> selectionChanged;


    const char*		getvalue_() const override;
    void		setvalue_( const char* ) override;

    void		setToolTip( const uiString& tt ) override
			{ uiObject::setToolTip(tt); }

protected:

    bool	notifyValueChanging_( const CallBack& cb ) override
			{ textChanged.notify( cb ); return true;}
    bool	notifyValueChanged_( const CallBack& cb ) override
			{ editingFinished.notify( cb ); return true;}
    bool	notifyUpdateRequested_( const CallBack& cb ) override
			{ returnPressed.notify( cb ); return true; }

private:

    uiLineEditBody*	body_;
    uiLineEditBody&	mkbody(uiParent*, const char*);

    mutable BufferString result_;
    int			nrdecimals_;

};


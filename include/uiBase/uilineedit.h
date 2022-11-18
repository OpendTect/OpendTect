#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
		uiIntValidator();
		uiIntValidator(int bot,int top);
		~uiIntValidator();

    int		bottom_		= -mUdf(int);
    int		top_		= mUdf(int);
};


mExpClass(uiBase) uiFloatValidator
{
public:
		uiFloatValidator();
		uiFloatValidator(float bot,float top);
		~uiFloatValidator();

    float	bottom_		= -mUdf(float);
    float	top_		= mUdf(float);
    int		nrdecimals_	= 10;
    bool	scnotation_	= true;	// If true, ScientificNotation is used
};


mExpClass(uiBase) uiTextValidator
{
public:
			uiTextValidator();
			uiTextValidator(const BufferStringSet& regexchars,
					const int leastnrocc=mUdf(int),
					const int maxnrocc=mUdf(int),
					bool excludfirstocconly=true);
			uiTextValidator(const uiTextValidator& textvl);
			~uiTextValidator();

    static uiTextValidator getDefault();

    BufferStringSet	regexchars_;
    int			leastnrocc_;
    int			maxnrocc_;
    bool		excludfirstocconly_;

    BufferString	getRegExString() const;
};


mExpClass(uiBase) uiLineEdit : public UserInputObjImpl<const char*>
			     , public uiObject
{
public:
			//! pref_empty : return empty string/ null value
			//  insted of undefined value, when line edit is empty.
			uiLineEdit(uiParent*,const char* nm);
			uiLineEdit(uiParent*,const DataInpSpec&,const char* nm);
			~uiLineEdit();

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
    void		setDefaultTextValidator();

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
    void		setDefaultStyleSheet();

protected:

    bool		notifyValueChanging_( const CallBack& cb ) override
			{ textChanged.notify( cb ); return true;}
    bool		notifyValueChanged_( const CallBack& cb ) override
			{ editingFinished.notify( cb ); return true;}
    bool		notifyUpdateRequested_( const CallBack& cb ) override
			{ returnPressed.notify( cb ); return true; }

private:

    uiLineEditBody*	body_;
    uiLineEditBody&	mkbody(uiParent*, const char*);

    mutable BufferString result_;
    int			nrdecimals_;

};

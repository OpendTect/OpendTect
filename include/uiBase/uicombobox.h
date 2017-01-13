#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          25/05/2000
________________________________________________________________________

-*/
#include "uibasemod.h"
#include "uigroup.h"
#include "userinputobj.h"
#include "enums.h"

class uiLabel;
class uiComboBoxBody;
class BufferStringSet;

/*!\brief Combo box.

  The user can select an item from a drop-down list. Sometimes, you can allow
  the user entering a new string there, use setReadOnly(false). In that case
  the result of text() can be different from textOfItem(currentItem()). Also,
  setText will do something is if the given string is not in the list.

  */

mExpClass(uiBase) uiComboBox : public uiObject, public UserInputObjImpl<int>
{
public:

			uiComboBox(uiParent*,const char* nm);
			uiComboBox(uiParent*,const uiStringSet&,
				   const char* nm);
			uiComboBox(uiParent*,const char**,const char* nm);
			uiComboBox(uiParent*,const uiString*,const char* nm);
			/*!<Similar to const char** Adds strings until an empty
			    string is found. */
			uiComboBox(uiParent*,const EnumDef&,const char* nm);
			/*!<EnumDef is assumed to remain in mem*/
    virtual		~uiComboBox();

    virtual void	setReadOnly(bool yn=true);
    virtual bool	isReadOnly() const;
    void		setEditable(bool yn);
    bool		isEditable() const;

    int			size() const;
    inline bool		isEmpty() const		{ return size() == 0; }
    void		setEmpty();
    bool		isPresent(const char*) const;
    int			indexOf(const uiString&) const;
    int			indexOf(const char*) const;
    inline int		indexOf( const OD::String& str ) const
			{ return indexOf( str.str() ); }

    const char*		text() const;
    void		setText(const char*);
    void		setText(uiString&);
    int			currentItem() const;
    void		setCurrentItem(int);
    void		setCurrentItem(const char*); //!< First match
    void		setCurrentItem(uiString&);
    void		setCurrentItem( const FixedString& fs )
						{ setCurrentItem( fs.str() ); }
    virtual void	addItem(const uiString&);
    void		addItem(const uiString&,int id);
    void		addItems(const uiStringSet&);
    void		addSeparator();
    void		insertItem(const uiString&,int index=-1,int id=-1);
    void		insertItem(const uiPixmap&,const uiString&,
				   int index=-1,int id=-1);

    const char*		itemText(int) const;
    const uiString	textOfItem(int) const;
    void		getItems(BufferStringSet&) const;

    void		setItemText(int,const uiString&);
    void		setPixmap(int index,const uiPixmap&);
    void		setIcon(int index,const char* icon_identifier);

    void		setItemID(int index,int id);
    int			currentItemID() const;
    int			getItemID(int index) const;
    int			getItemIndex(int id) const;

    Notifier<uiComboBox> editTextChanged;
    Notifier<uiComboBox> selectionChanged;

protected:

    virtual void	setvalue_( int i )	{ setCurrentItem(i); }
    virtual int		getvalue_() const	{ return currentItem(); }

    virtual bool	notifyUpdateRequested_(const CallBack&) {return false;}
    virtual bool	notifyValueChanging_(const CallBack&)	{return false;}
    virtual bool	notifyValueChanged_( const CallBack& cb )
			    { selectionChanged.notify(cb); return true; }
    void		translateText();
    mDeprecated		uiComboBox(uiParent*,const BufferStringSet&,
				   const char* nm);
    mDeprecated void	addItems(const BufferStringSet&);
private:

    int			oldnritems_;
    int			oldcuritem_;
    TypeSet<int>	itemids_;
    uiStringSet		itemstrings_;

    mutable BufferString rettxt_;

    uiComboBoxBody*	body_;
    uiComboBoxBody&	mkbody(uiParent*,const char*);
    void		adjustWidth(const uiString&);
    int			curwidth_;
    const EnumDef*	enumdef_;

public:

    void		setToolTip( const uiString& tt )
			{ uiObject::setToolTip(tt); }

    virtual bool	update_( const DataInpSpec& spec );
    void		getItemSize(int,int& h,int& w) const;

    void		notifyHandler(bool selectionchanged);

    bool		handleLongTabletPress();
    void		popupVirtualKeyboard(int globalx=-1,int globaly=-1);

};



mExpClass(uiBase) uiLabeledComboBox : public uiGroup
{
public:
		uiLabeledComboBox(uiParent*,const uiString& lbl,
				  const char* nm=0);
		uiLabeledComboBox(uiParent*,const char**,
				  const uiString& lbl,const char* nm=0);
		uiLabeledComboBox(uiParent*,const uiStringSet&,
				  const uiString& lbl,const char* nm=0);
		uiLabeledComboBox(uiParent*,const EnumDef&,
				  const uiString& lbl,const char* nm=0);

    uiComboBox*	box()		{ return cb_; }
    uiLabel*	label()		{ return labl_; }


protected:

    uiComboBox*	cb_;
    uiLabel*	labl_;
    mDeprecated uiLabeledComboBox(uiParent*,const BufferStringSet&,
				  const uiString& lbl,const char* nm=0);

};

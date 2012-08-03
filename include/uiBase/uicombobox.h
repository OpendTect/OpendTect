#ifndef uicombobox_h
#define uicombobox_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          25/05/2000
 RCS:           $Id: uicombobox.h,v 1.41 2012-08-03 13:00:51 cvskris Exp $
________________________________________________________________________

-*/
#include "uibasemod.h"
#include "uigroup.h"
#include "userinputobj.h"

class uiLabel;
class uiComboBoxBody;
class BufferStringSet;
class BufferString;
template <class T> class ObjectSet;

/*!\brief Combo box.

  The user can select an item from a drop-down list. Sometimes, you can allow
  the user entering a new string there, use setReadOnly(false). In that case
  the result of text() can be different from textOfItem(currentItem()). Also,
  setText will do something is if the given string is not in the list.

  */

mClass(uiBase) uiComboBox : public uiObject, public UserInputObjImpl<int>
{
public:

			uiComboBox(uiParent*,const char* nm);
			uiComboBox(uiParent*,const BufferStringSet&,
				   const char* nm);
			uiComboBox(uiParent*,const char**,const char* nm);
    virtual 		~uiComboBox();

    virtual void        setReadOnly( bool = true );
    virtual bool        isReadOnly() const;

    int			size() const;
    inline bool		isEmpty() const		{ return size() == 0; }
    void		setEmpty();
    bool		isPresent(const char*) const;
    int			indexOf(const char*) const;

    const char*		text() const;
    void		setText(const char*);
    int			currentItem() const;
    void		setCurrentItem(int);
    void		setCurrentItem(const char*); //!< First match

    void		addItem(const wchar_t*,int id=-1);
    virtual void	addItem(const char*);
    void		addItem(const char*,int id);
    void		addItems(const BufferStringSet&);
    void		addSeparator();
    void		insertItem(const char*,int index=-1,int id=-1);
    void		insertItem(const ioPixmap&,const char*,
				   int index=-1,int id=-1);

    const char*		textOfItem(int) const;
    void		setItemText(int,const char*);
    void		setPixmap(const ioPixmap&,int index);

    void		setItemID(int index,int id);
    int			currentItemID() const;
    int			getItemID(int index) const;
    int			getItemIndex(int id) const;

    Notifier<uiComboBox> editTextChanged;
    Notifier<uiComboBox> selectionChanged;

protected:

    virtual void        setvalue_( int i )	{ setCurrentItem(i); }
    virtual int		getvalue_() const	{ return currentItem(); }
    virtual bool	clear_()		{ setEmpty(); return true; }

    virtual bool	notifyUpdateRequested_(const CallBack&) {return false;}
    virtual bool	notifyValueChanging_(const CallBack&)	{return false;}
    virtual bool	notifyValueChanged_( const CallBack& cb )   
			    { selectionChanged.notify(cb); return true; }

private:

    int			oldnritems_;
    int			oldcuritem_;
    TypeSet<int>	itemids_;

    mutable BufferString rettxt_;

    uiComboBoxBody*	body_;
    uiComboBoxBody&	mkbody(uiParent*,const char*);

public:

    void		setToolTip( const char* tt )
    			{ uiObject::setToolTip(tt); }

    virtual bool	update_( const DataInpSpec& spec );
    void		getItemSize(int,int& h,int& w) const;

    void		notifyHandler(bool selectionchanged);

    bool		handleLongTabletPress();
    void		popupVirtualKeyboard(int globalx=-1,int globaly=-1);

};



mClass(uiBase) uiLabeledComboBox : public uiGroup
{
public:
		uiLabeledComboBox(uiParent*,const char* lbl,
				  const char* nm=0);
		uiLabeledComboBox(uiParent*,const BufferStringSet&,
				  const char* lbl,const char* nm=0);
		uiLabeledComboBox(uiParent*,const char**,
				  const char* lbl,const char* nm=0);

    uiComboBox*	box()		{ return cb_; }
    uiLabel*	label()		{ return labl_; }


protected:

    uiComboBox*	cb_;
    uiLabel*	labl_;

};

#endif


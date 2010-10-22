#ifndef uicombobox_h
#define uicombobox_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          25/05/2000
 RCS:           $Id: uicombobox.h,v 1.34 2010-10-22 15:22:22 cvsjaap Exp $
________________________________________________________________________

-*/
#include "uigroup.h"
#include "userinputobj.h"

class uiLabel;
class uiComboBoxBody;
class BufferStringSet;
class BufferString;
template <class T> class ObjectSet;

mClass uiComboBox : public uiObject, public UserInputObjImpl<int>
{
public:

			uiComboBox(uiParent*,const char* nm);
			uiComboBox(uiParent*,const BufferStringSet&,
				   const char* nm);
			uiComboBox(uiParent*,const char**,const char* nm);
    virtual 		~uiComboBox();

			/*!  This is the text that is actually in the current
			     item. This text may differ from
			     textOfItem(currentItem()) when the box is editable.
			*/
    const char*		text() const;
    void		setText(const char*);

    void		setEditText(const char*);

    bool		isPresent(const char*) const;

    void		empty();
    int			size() const;
    inline bool		isEmpty() const		{ return size() == 0; }

    virtual void	addItem(const char*);
    void		addItems(const BufferStringSet&);
    void		insertItem(const char*,int index=-1);
    void		insertItem(const ioPixmap&,const char*,int index=-1);
    void		setPixmap(const ioPixmap&,int index);
    void		getItemSize(int,int& h,int& w) const;

    int			currentItem() const;
    void		setCurrentItem(int);
    void		setCurrentItem(const char*); //!< First match
    const char*		textOfItem(int) const;
    void		setItemText(int,const char*);
    int			indexOf(const char*) const;

    Notifier<uiComboBox> editTextChanged;

			//! Triggered when selection has changed.
    Notifier<uiComboBox> selectionChanged;

    void		notifyHandler(bool selectionchanged);

    virtual void        setReadOnly( bool = true );
    virtual bool        isReadOnly() const;

    virtual bool	update_( const DataInpSpec& spec );

    bool		handleLongTabletPress();
    void		popupVirtualKeyboard(int globalx=-1,int globaly=-1);

protected:

    virtual void        setvalue_( int i )	{ setCurrentItem(i); }
    virtual int		getvalue_() const	{ return currentItem(); }
    virtual bool	clear_()		{ empty(); return true; }

    virtual bool	notifyUpdateRequested_(const CallBack&) {return false;}
    virtual bool	notifyValueChanging_(const CallBack&)	{return false;}
    virtual bool	notifyValueChanged_( const CallBack& cb )   
			    { selectionChanged.notify(cb); return true; }

private:

    int			oldnritems_;
    int			oldcuritem_;

    mutable BufferString rettxt_;

    uiComboBoxBody*	body_;
    uiComboBoxBody&	mkbody(uiParent*,const char*);

};



mClass uiLabeledComboBox : public uiGroup
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

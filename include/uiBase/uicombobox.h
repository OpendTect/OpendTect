#ifndef uiComboBox_H
#define uiComboBox_H

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          25/05/2000
 RCS:           $Id: uicombobox.h,v 1.22 2008-04-16 15:15:39 cvsbert Exp $
________________________________________________________________________

-*/
#include "uigroup.h"
#include "userinputobj.h"

class uiLabel;
class uiComboBoxBody;
class BufferStringSet;
class BufferString;
template <class T> class ObjectSet;

class uiComboBox : public uiObject, public UserInputObjImpl<int>
{
public:

			uiComboBox(uiParent*,const char* nm="Combo Box",
				   bool editable=false);
			uiComboBox(uiParent*,const BufferStringSet&,
				   const char* nm="Combo Box",
				   bool editable=false);
			uiComboBox(uiParent*,const char**,
				   const char* nm="Combo Box",
				   bool editable=false);
    virtual 		~uiComboBox();

			/*!  This is the text that is actually in the current
			     item. This text may differ from
			     textOfItem(currentItem()) when the box is editable.
			*/
    const char*		text() const;
    void		setText(const char*);

    bool		isPresent(const char*) const;

    void		empty();
    int			size() const;


    virtual bool	hasItems()		{ return true; }

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

			//! Triggered when selection has changed.
    Notifier<uiComboBox> selectionChanged;

			//! Force activation in GUI thread
    void		activate(int idx);
    Notifier<uiComboBox> activatedone; 

    virtual void        setReadOnly( bool = true );
    virtual bool        isReadOnly() const;

    virtual bool	update_( const DataInpSpec& spec );

protected:

    virtual void        setvalue_( int i )	{ setCurrentItem(i); }
    virtual int		getvalue_() const	{ return currentItem(); }
    virtual bool	clear_()		{ empty(); return true; }

    virtual bool	notifyValueChanging_( const CallBack& )	{ return false;}
    virtual bool	notifyValueChanged_( const CallBack& cb )   
			    { selectionChanged.notify(cb); return true; }

private:

    BufferString	rettxt;

    uiComboBoxBody*	body_;
    uiComboBoxBody&	mkbody(uiParent*, const char*, bool);

};


class uiLabeledComboBox : public uiGroup
{
public:
		uiLabeledComboBox(uiParent*,const char* txt,
				  const char* nm="Labeled Combobox",
				  bool editable=false);
		uiLabeledComboBox(uiParent*,const BufferStringSet&,
				  const char* txt,bool ed=false);
		uiLabeledComboBox(uiParent*,const char**,
				  const char* txt,bool ed=false);

    uiComboBox*	box()		{ return cb; }
    uiLabel*	label()		{ return labl; }


protected:

    uiComboBox*	cb;
    uiLabel*	labl;

};

#endif

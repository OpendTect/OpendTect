#ifndef uiComboBox_H
#define uiComboBox_H

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          25/05/2000
 RCS:           $Id: uicombobox.h,v 1.10 2001-08-23 14:59:17 windev Exp $
________________________________________________________________________

-*/
#include <uigroup.h>

class PtrUserIDObjectSet;
class uiLabel;
class uiComboBoxBody;

class BufferString;
template <class T> class ObjectSet;

class uiComboBox : public uiObject
{
public:

			uiComboBox(uiParent*,const char* nm="Combo Box",
				   bool editable=false);
			uiComboBox(uiParent*,const PtrUserIDObjectSet&,
				   bool editable=false);
    virtual 		~uiComboBox();

    const char*		getText() const;
			/*!< This is the text that is actually in the current
			     item. This text may differ from
			     textOfItem(currentItem()) when the box is editable.
			*/
    void		setText(const char*);
			/*!< This text will be put instead of what's
			     in the item! */

    bool		isPresent(const char*) const;

    virtual void	clear()			{ setCurrentItem(0); }
    void		empty();
    int			size() const;
    void		addItem(const char*); 
    void		addItems(const char**); 
    void		addItems(const PtrUserIDObjectSet&);
    void		addItems(const ObjectSet<BufferString>&);
    int			currentItem() const;
    void		setCurrentItem(int);
    void		setCurrentItem(const char*); //!< First match
    const char*		textOfItem(int) const;
    void		setItemText(int,const char*);

			//! Triggered when selection has changed.
    Notifier<uiComboBox> selectionChanged;

private:

    BufferString	rettxt;

    uiComboBoxBody*	body_;
    uiComboBoxBody&	mkbody(uiParent*, const char*, bool);

};


class uiLabeledComboBox : public uiGroup
{
public:
		uiLabeledComboBox( uiParent*,const char* txt,
				   const char* nm="Labeled Combobox",
				   bool editable=false);
		uiLabeledComboBox( uiParent*,const PtrUserIDObjectSet&,
					    bool ed=false);

    uiComboBox*	box()		{ return cb; }
    uiLabel*	label()		{ return labl; }


protected:

    uiComboBox*	cb;
    uiLabel*	labl;

};

#endif

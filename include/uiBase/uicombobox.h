#ifndef uiComboBox_H
#define uiComboBox_H

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          25/05/2000
 RCS:           $Id: uicombobox.h,v 1.4 2001-05-01 17:05:26 bert Exp $
________________________________________________________________________

-*/
#include <uigroup.h>

class QComboBox;
class PtrUserIDObjectSet;
class uiLabel;

template <class T> class i_QObjWrapper;
mTemplTypeDefT(i_QObjWrapper, QComboBox, i_QComboBox)

class uiComboBox : public uiWrapObj<i_QComboBox>
{
friend class i_comboMessenger;
friend i_QComboBox;
public:

			uiComboBox(uiObject*,const char* nm="Combo Box",
				   bool editable=false);
			uiComboBox(uiObject*,const PtrUserIDObjectSet&,
				   bool editable=false);
    virtual 		~uiComboBox();

    void                notify( const CallBack& cb ) { notifyCBL += cb; }
			//!< Triggered when selection has changed. 

    const char*		getText() const;
    int			size() const;

    bool		isPresent(const char*) const;
    int			currentItem() const;
    void		setCurrentItem(int);
    void		setCurrentItem(const char*); //!< First match
    const char*		textOfItem(int) const;
    void		setItemText(int,const char*);

    void		clear();
    void		addItem(const char*); 
    void		addItems(const char**); 
    void		addItems(const PtrUserIDObjectSet&);

    virtual bool        isSingleLine() const { return true; }

protected:

    const QWidget*	qWidget_() const;

    virtual void        notifyHandler() //!< Handler called from Qt.
			{ Notifier(); }
    void                Notifier()     { notifyCBL.doCall(this); }
    CallBackList        notifyCBL;

private:

    i_comboMessenger&    _messenger;
    BufferString	rettxt;

};


class uiLabeledComboBox : public uiGroup
{
public:
		uiLabeledComboBox( uiObject*,const char* txt,
				   const char* nm="Labeled Combobox",
				   bool editable=false);
		uiLabeledComboBox(uiObject*,const PtrUserIDObjectSet&,
					    bool ed=false);

    uiComboBox*	box()		{ return cb; }
    uiLabel*	label()		{ return labl; }


protected:

    uiComboBox*	cb;
    uiLabel*	labl;

};

#endif

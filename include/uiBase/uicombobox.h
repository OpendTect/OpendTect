#ifndef uiComboBox_H
#define uiComboBox_H

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          25/05/2000
 RCS:           $Id: uicombobox.h,v 1.1 2000-11-27 10:19:27 bert Exp $
________________________________________________________________________

-*/
#include <uigroup.h>

class QComboBox;
class UserIDSet;
class uiLabel;

template <class T> class i_QObjWrapper;
mTemplTypeDefT(i_QObjWrapper, QComboBox, i_QComboBox)

class uiComboBox : public uiWrapObj<i_QComboBox>
{
friend class i_comboMessenger;
friend i_QComboBox;
public:

			uiComboBox( uiObject*,const char* nm="Combo Box",
				    bool autoComplete=true);
			uiComboBox(uiObject*,const UserIDSet&,bool ac=true);

    virtual 		~uiComboBox();

    virtual bool        isSingleLine() const { return true; }

    void                notify( const CallBack& cb ) { notifyCBL += cb; }
    //!< Triggered when selection has changed. 

    int			currentItem() const;
    void		setCurrentItem(int);
    void		setCurrentItem(const char*); //!< First match
    const char*		textOf(int) const;
    const char*		currentText() const
			{ return textOf( currentItem() ); }
    int			size() const;
    

    int			insertItem( const char* text ); 
    int			insertItems( const char** textList ); 

    int			getCurId() const	{ return cur_id; }
    //!< \return current Id, which equals the number of items in the box - 1.
    // UNLESS items have been removed, but that's not supported.

protected:
    const QWidget*	qWidget_() const;

    virtual void        notifyHandler() //!< Handler called from Qt.
			{ Notifier(); }
    void                Notifier()     { notifyCBL.doCall(this); }
    CallBackList        notifyCBL;

private:

    i_comboMessenger&    _messenger;

    int			cur_id;
    int			getNewId() { return ++cur_id; }

    BufferString	rettxt;

};


class uiLabeledComboBox : public uiGroup
{
public:
		uiLabeledComboBox( uiObject*,const char* txt,
				   const char* nm="Labeled Combobox",
				   bool autoComplete=true);
		uiLabeledComboBox(uiObject*,const UserIDSet&,bool ac=true);

    uiComboBox*	box()		{ return cb; }
    uiLabel*	label()		{ return labl; }


protected:

    uiComboBox*	cb;
    uiLabel*	labl;

};

#endif

#ifndef uilistbox_h
#define uilistbox_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          16/05/2000
 RCS:           $Id: uilistbox.h,v 1.46 2009-05-28 09:08:50 cvsjaap Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
#include "pixmap.h"
#include "keyenum.h"

class BufferStringSet;
class Color;
class uiLabel;
class uiListBoxBody;
class uiPopupMenu;

class QString;


mClass uiListBox : public uiObject
{
friend class i_listMessenger;
friend class uiListBoxBody;
public:

                        uiListBox(uiParent* parnt=0, 
				  const char* nm="uiListBox",
				  bool isMultiSelect=false,
				  int preferredNrLines=0,
				  int preferredFieldWidth=0);

			uiListBox(uiParent*,const BufferStringSet&,
				  const char* txt="uiListBox",
				  bool isMultiSelect=false,
				  int preferredNrLines=0,
				  int preferredFieldWidth=0);

    virtual 		~uiListBox();

/*! \brief set preferred number of lines. 
    If set to 0, then it is determined by the number of items in list.
    If set to 1, then the list has a fixed height of 1 textline and 
    therefore should not be able to grow/shrink vertically.

    adaptVStretch specifies wether or not the vertical stretch should be
    set to 0 if nrlines == 1 or 2 otherwise.
*/
    void 		setLines(int,bool adaptvstretch);
    void		setNotSelectable();
    void		setMultiSelect(bool yn=true);
    int			maxSelectable() const;

    int			size() const;
    inline bool		isEmpty() const		{ return size() == 0; }
    int			nextSelected(int prev=-1) const;
    bool		isPresent(const char*) const;
    bool		isSelected(int) const;
    int			nrSelected() const;
    void		setSelected(int,bool yn=true);
    void		selectAll(bool yn=true);
    virtual void	clear();
    void		sort(bool asc=true);

    void		empty();
    void		removeItem(int);
    void		addItem(const char*,bool embedded=false); 
    			//!< embedded = put [...] around text
    void		addItem(const char*,const ioPixmap&);
    void		addItem(const char*,const Color&);
    void		addItems(const char**); 
    void		addItems(const BufferStringSet&);
    void		insertItem(const char*,int idx=-1,bool embedded=false);
    void		insertItem(const char*,const ioPixmap&,int idx=-1);
    void		insertItem(const char*,const Color&,int);
    void		setPixmap(int,const Color&);
    void		setPixmap(int,const ioPixmap&);
    ioPixmap		pixmap(int) const;

    void		setItemText(int,const char*);
    int			currentItem() const;
    const char*		getText() const	 { return textOfItem(currentItem()); }
    const char*		textOfItem(int,bool disembed=false) const;
    			//!< disembed = remove [...] from text
    bool		isEmbedded(int) const;
    			//!< check for [...] around text
    void                setCurrentItem(int);
    void                setCurrentItem(const char*); //!< First match
    int			indexOf(const char*) const; //!< First match

    void		setItemsCheckable(bool);	//!<Sets all items
    void		setItemCheckable(int,bool);
    void		setItemsChecked(bool);		//!<Sets all items
    void		setItemChecked(int,bool);
    bool		isItemCheckable(int) const;
    bool		isItemChecked(int) const;
    int			nrChecked() const;

    void		setSelectedItems(const BufferStringSet&);
    void		setSelectedItems(const TypeSet<int>&);
    void		setCheckedItems(const BufferStringSet&);
    void		setCheckedItems(const TypeSet<int>&);
    void		getSelectedItems(BufferStringSet&) const;
    void		getSelectedItems(TypeSet<int>&) const;
    void		getCheckedItems(BufferStringSet&) const;
    void		getCheckedItems(TypeSet<int>&) const;

    void		setFieldWidth(int);
    int			optimumFieldWidth(int minwdth=20,int maxwdth=40) const;

    Notifier<uiListBox> selectionChanged;
    Notifier<uiListBox> doubleClicked;
    Notifier<uiListBox> rightButtonClicked;
    Notifier<uiListBox> leftButtonClicked;
    Notifier<uiListBox> deleteButtonPressed;

    void		notifyHandler( const char* notifiername );

    			//! Force activation in GUI thread
    void		activateClick(int idx,bool leftclick=true,
				      bool doubleclick=false);
    void		activateButton(int idx);
    void		activateSelect(const TypeSet<int>&);
    Notifier<uiListBox> activatedone;

protected:

    mutable BufferString	rettxt;
    OD::ButtonState	buttonstate_;
    bool		itemscheckable_;
    uiPopupMenu&	rightclickmnu_;

    void		menuCB(CallBacker*);

private:

    bool		curitemwaschecked_;
    int			oldnrselected_;

    uiListBoxBody*	body_;
    uiListBoxBody&	mkbody(uiParent*,const char*,bool,int,int);

    void		createQString(QString&,const char*,bool) const;
    bool		validIndex(int) const;
};


mClass uiLabeledListBox : public uiGroup
{
public:

    enum LblPos		{ LeftTop, RightTop, LeftMid, RightMid,
			  AboveLeft, AboveMid, AboveRight,
			  BelowLeft, BelowMid, BelowRight };

			uiLabeledListBox(uiParent*,const char* txt,
					 bool multisel=false,LblPos p=LeftTop);
			uiLabeledListBox(uiParent*,const BufferStringSet&,
					 const char* txt,
					 bool multisel=false,LblPos p=LeftTop);

    uiListBox*		box()				{ return lb; }
    int			nrLabels() const		{ return lbls.size(); }
    uiLabel*		label( int nr=0 )		{ return lbls[nr]; }
    const char*		labelText(int nr=0) const;
    void		setLabelText(const char*,int nr=0);


protected:

    uiListBox*		lb;
    ObjectSet<uiLabel>	lbls;

    void		mkRest(const char*,LblPos);

};


#endif

#ifndef uilistbox_h
#define uilistbox_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          16/05/2000
 RCS:           $Id: uilistbox.h,v 1.67 2012-08-01 10:23:50 cvsmahant Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
#include "pixmap.h"
#include "keyenum.h"
#include "draw.h"

class BufferStringSet;
class Color;
class uiLabel;
class uiListBoxBody;
class uiPopupMenu;

class QListWidgetItem;


/*!\brief List Box.

  Can be single- or multi-selection. Items can also be 'checkable'. Then, a
  check box appears in front of each of the items in the list.

  The size of the box is determined automatically (prefNrLines=0) but can be
  overruled by setNrLines().

  You can have some of the items marked. This will be done by surrounding the
  item with curly braces, like: '{the item}'.

*/

mClass uiListBox : public uiObject
{
friend class i_listMessenger;
friend class uiListBoxBody;
public:

                        uiListBox(uiParent*,const char* nm=0,
				  bool multiselect=false,
				  int prefNrLines=0,int prefFieldWidth=0);

			uiListBox(uiParent*,const BufferStringSet&,
				  const char* nm=0,bool multiselect=false,
				  int prefNrLines=0,int prefFieldWidth=0);

    virtual 		~uiListBox();

    enum SelectionMode	{ No, Single, Multi, Extended, Contiguous };
    void		setSelectionMode(SelectionMode);

    void		setMultiSelect(bool yn=true);
    void		setNotSelectable();	//!< display only, no iteraction
    int			maxNrOfSelections() const;

    int			size() const;
    inline bool		isEmpty() const		{ return size() == 0; }
    bool		isPresent(const char*) const;

    bool		isSelected(int) const;
    int			nextSelected(int prev=-1) const;
    int			nrSelected() const;
    void		setSelected(int,bool yn=true);
    void		selectAll(bool yn=true);
    void		clearSelection();

    void		setEmpty();
    void		removeItem(int);
    void		setAllowDuplicates(bool yn);
    void		addItem(const char*,bool marked=false,int id=-1);
    void		addItem(const char*,const ioPixmap&,int id=-1);
    void		addItem(const char*,const Color&,int id=-1);
    void		addItems(const char**);
    void		addItems(const BufferStringSet&);
    void		insertItem(const char*,int idx=-1,
				   bool marked=false,int id=-1);
    void		insertItem(const char*,const ioPixmap&,
				   int idx=-1,int id=-1);
    void		insertItem(const char*,const Color&,
				   int idx=-1,int id=-1);
    void		setPixmap(int,const Color&);
    void		setPixmap(int,const ioPixmap&);
    ioPixmap		pixmap(int) const;
    void		setColor(int,const Color&);
    Color		getColor(int) const;

    void		sortItems(bool asc=true);

    int			currentItem() const;
    const char*		getText() const	 { return textOfItem(currentItem()); }
    void                setCurrentItem(int);
    void                setCurrentItem(const char*);	//!< First match
    int			indexOf(const char*) const;	//!< First match
    const char*		textOfItem(int) const;
    void		setItemText(int,const char*);
    void		getItems(BufferStringSet&) const;

    bool		isMarked(int) const;
    void		setMarked(int,bool);

    void		setItemsCheckable(bool);	//!< Sets all items
    void		setItemCheckable(int,bool);
    void		setAllItemsChecked(bool);
    void		setItemChecked(int,bool);
    void		setItemChecked(const char*,bool);
    bool		isItemCheckable(int) const;
    bool		isItemChecked(int) const;
    bool		isItemChecked(const char*) const;
    int			nrChecked() const;

    void		setItemSelectable(int,bool);
    bool		isItemSelectable(int) const;
    void		setSelectedItems(const BufferStringSet&);
    void		setSelectedItems(const TypeSet<int>&);
    void		setCheckedItems(const BufferStringSet&);
    void		setCheckedItems(const TypeSet<int>&);
    void		getSelectedItems(BufferStringSet&) const;
    void		getSelectedItems(TypeSet<int>&) const;
    void		getCheckedItems(BufferStringSet&) const;
    void		getCheckedItems(TypeSet<int>&) const;

    void		setItemID(int idx,int id);
    int			currentItemID() const;
    int			getItemID(int idx) const;
    int			getItemIdx(int id) const;	//!< First match

    Alignment::HPos	alignment() const		{ return alignment_; }
    void		setAlignment(Alignment::HPos);
    void 		setNrLines(int);
    void		setFieldWidth(int);
    int			optimumFieldWidth(int minwdth=20,int maxwdth=40) const;
    static int		cDefNrLines();		//!< == 7 (July 2011)

    Notifier<uiListBox> selectionChanged;
    CNotifier<uiListBox,int> itemChecked;	//!< or un-checked (of course)
    Notifier<uiListBox> doubleClicked;
    Notifier<uiListBox> rightButtonClicked;
    Notifier<uiListBox> leftButtonClicked;
    Notifier<uiListBox> deleteButtonPressed;

    bool		handleLongTabletPress();

protected:

    mutable BufferString rettxt;
    OD::ButtonState	buttonstate_;
    Alignment::HPos	alignment_;
    bool		itemscheckable_;
    bool 		allowduplicates_;
    uiPopupMenu&	rightclickmnu_;

    void		menuCB(CallBacker*);
    void		handleCheckChange(QListWidgetItem*);

private:

    uiListBoxBody*	body_;
    uiListBoxBody&	mkbody(uiParent*,const char*,bool,int,int);

    bool		validIndex(int) const;

};


mClass uiLabeledListBox : public uiGroup
{
public:

    enum LblPos		{ LeftTop, RightTop, LeftMid, RightMid,
			  AboveLeft, AboveMid, AboveRight,
			  BelowLeft, BelowMid, BelowRight };

			uiLabeledListBox(uiParent*,const char* lbltxt,
					 bool multisel=false,LblPos p=LeftTop);
			uiLabeledListBox(uiParent*,const BufferStringSet&,
					 const char* lbltxt,
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

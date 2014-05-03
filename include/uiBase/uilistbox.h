#ifndef uilistbox_h
#define uilistbox_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          16/05/2000
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uigroup.h"
#include "pixmap.h"
#include "keyenum.h"
#include "draw.h"

class BufferStringSet;
class uiLabel;
class uiListBoxBody;
class uiMenu;
class uiString;

mFDQtclass(QListWidgetItem)


/*!\brief List Box.

  The size of the box is determined automatically (prefNrLines=0) but can be
  overruled by setNrLines().

  If the user can select multiple items, then there is a difference between
  selected and current item. In the old (Windows) style, you get something that
  is like you have many current items. Two main problems make us want to phase
  out this style:
  * The fact that you can choose multiple items is visually hidden.
  * In many styles you don't know what the current item is
  Especially the first problem is a big thing.

  New style multi-select works with a check boxes in front of each item. This
  delivers a challenge because what if the current item is not checked?
  This is the basis for the 'chosen' concept. An item is chosen if it is
  checked. If no box is checked, then you can have the listbox report the
  current item as the chosen item. To switch this off, tell the listbox that
  no chosen is a valid thing, i.e. use setAllowNoneChosen( true ).

  Lastly, you can have some of the items marked. This will be done by
  surrounding the item with curly braces, like: '{the item}'. This will not
  affect selection/checked/chosen status.

*/

mExpClass(uiBase) uiListBox : public uiObject
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
			uiListBox(uiParent*,const TypeSet<uiString>&,
				  const char* nm=0,bool multiselect=false,
				  int prefNrLines=0,int prefFieldWidth=0);

    virtual		~uiListBox();

    void		setMultiChoice( bool yn )     { setItemsCheckable(yn); }
    void		setAllowNoneChosen( bool yn ) { nochosenisvalid_ = yn; }
    void		setNotSelectable();	//!< display only, no iteraction

    int			size() const;
    inline bool		isEmpty() const		{ return size() == 0; }
    bool		isPresent(const char*) const;

    Alignment::HPos	alignment() const		{ return alignment_; }
    void		setAlignment(Alignment::HPos);
    void		setNrLines(int);
    void		setFieldWidth(int);

    void		setEmpty();
    void		removeItem(int);
    void		removeItem(const char*);
    void		removeItem( const FixedString& fs )
						{ removeItem( fs.str() ); }
    void		setAllowDuplicates(bool yn);
    void		addItem(const uiString&,bool marked=false,int id=-1);
    void		addItem(const uiString&,const ioPixmap&,int id=-1);
    void		addItem(const uiString&,const Color&,int id=-1);
    void		addItems(const char**);
    void		addItems(const BufferStringSet&);
    void		addItems(const TypeSet<uiString>&);
    void		insertItem(const uiString&,int idx=-1,
				   bool marked=false,int id=-1);
    void		insertItem(const uiString&,const ioPixmap&,
				   int idx=-1,int id=-1);
    void		insertItem(const uiString&,const Color&,
				   int idx=-1,int id=-1);
    void		setPixmap(int,const Color&);
    void		setPixmap(int,const ioPixmap&);
    ioPixmap		pixmap(int) const;
    void		setColor(int,const Color&);
    Color		getColor(int) const;

    void		sortItems(bool asc=true);

    int			indexOf(const char*) const;	//!< First match
    const char*		textOfItem(int) const;
    void		setItemText(int,const uiString&);
    void		getItems(BufferStringSet&) const;

    int			currentItem() const;
    const char*		getText() const	 { return textOfItem(currentItem()); }
    void                setCurrentItem(int);
    void                setCurrentItem(const char*);	//!< First match
    void		setCurrentItem( const FixedString& fs )
						{ setCurrentItem( fs.str() ); }

    int			nrChosen() const;
    bool		isChosen(int) const;
    int			firstChosen() const;
    void		getChosen(BufferStringSet&) const;
    void		getChosen(TypeSet<int>&) const;
    void		setChosen(int,bool yn=true);
    void		setChosen(const BufferStringSet&);
    void		setChosen(const TypeSet<int>&);
    void		chooseAll(bool yn);

    bool		isMarked(int) const;
    void		setMarked(int,bool);

    void		setItemID(int idx,int id);
    int			currentItemID() const;
    int			getItemID(int idx) const;
    int			getItemIdx(int id) const;	//!< First match

    void		scrollToTop();
    void		scrollToBottom();
    bool		handleLongTabletPress();
    void		disableRightClick(bool yn);

    Notifier<uiListBox> selectionChanged;
    CNotifier<uiListBox,int> itemChosen;  //!< maybe followed by selectionChange
    CNotifier<uiListBox,int> itemChecked; //!< always followed by itemChosen
    Notifier<uiListBox> doubleClicked;
    Notifier<uiListBox> rightButtonClicked;
    Notifier<uiListBox> leftButtonClicked;
    Notifier<uiListBox> deleteButtonPressed;

private:

    void		translateText();

    mutable BufferString rettxt_;
    OD::ButtonState	buttonstate_;
    Alignment::HPos	alignment_;
    bool		itemscheckable_;
    bool		allowduplicates_;
    bool		nochosenisvalid_;
    uiMenu&		rightclickmnu_;

    void		menuCB(CallBacker*);
    void		handleCheckChange(mQtclass(QListWidgetItem*));
    void		selChgCB(CallBacker*);

    uiListBoxBody*	body_;
    uiListBoxBody&	mkbody(uiParent*,const char*,bool,int,int);

    bool		validIndex(int) const;
    int			optimumFieldWidth(int minwdth=20,int maxwdth=40) const;
    static int		cDefNrLines();		//!< == 7

public:

    void		setItemsCheckable(bool);
    void		setItemCheckable(int,bool);
    void		setAllItemsChecked(bool);
    void		setItemChecked(int,bool);
    void		setItemChecked(const char*,bool);
    bool		isItemCheckable(int) const;
    bool		isItemChecked(int) const;
    bool		isItemChecked(const char*) const;
    int			nrChecked() const;
    int			firstChecked() const;
    void		setCheckedItems(const BufferStringSet&);
    void		setCheckedItems(const TypeSet<int>&);
    void		getCheckedItems(BufferStringSet&) const;
    void		getCheckedItems(TypeSet<int>&) const;

    enum SelectionMode	{ No, Single, Multi, Extended, Contiguous };
    void		setSelectionMode(SelectionMode);
    void		setMultiSelect(bool yn=true);
    int			maxNrOfSelections() const;
    bool		isSelected(int) const;
    int			nextSelected(int prev=-1) const;
    int			nrSelected() const;
    void		setSelected(int,bool yn=true);
    void		selectAll(bool yn=true);
    void		clearSelection();
    void		setItemSelectable(int,bool);
    bool		isItemSelectable(int) const;
    void		setSelectedItems(const BufferStringSet&);
    void		setSelectedItems(const TypeSet<int>&);
    void		getSelectedItems(BufferStringSet&) const;
    void		getSelectedItems(TypeSet<int>&) const;

};


mExpClass(uiBase) uiLabeledListBox : public uiGroup
{
public:

    enum LblPos		{ LeftTop, RightTop, LeftMid, RightMid,
			  AboveLeft, AboveMid, AboveRight,
			  BelowLeft, BelowMid, BelowRight };

			uiLabeledListBox(uiParent*,const uiString& lbltxt,
					 bool multisel=false,LblPos p=LeftMid);
			uiLabeledListBox(uiParent*,const BufferStringSet&,
					 const uiString& lbltxt,
					 bool multisel=false,LblPos p=LeftMid);

    uiListBox*		box()				{ return lb_; }
    int			nrLabels() const		{ return lbls_.size(); }
    uiLabel*		label( int nr=0 )		{ return lbls_[nr]; }
    void		setLabelText(const uiString&,int nr=0);

protected:

    uiListBox*		lb_;
    ObjectSet<uiLabel>	lbls_;

    void		mkRest(const uiString&,LblPos);

};


#endif


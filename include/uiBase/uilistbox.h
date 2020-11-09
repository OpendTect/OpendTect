#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          16/05/2000
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uigroup.h"
#include "keyenum.h"
#include "draw.h"

class BufferStringSet;
class uiCheckBox;
class uiLabel;
class uiListBoxBody;
class uiMenu;
class uiString;

mFDQtclass(QListWidgetItem)



/*!\brief List Box.

  The size of the box is determined automatically (prefNrLines=0) but can be
  overruled by setNrLines().

  If the user can select multiple items, then there is a difference between
  selected and current item. In the old (Windows) style, you got something that
  resembles having many current items. Two main problems make us want to phase
  out this style:
  * The fact that the user can in fact choose multiple items is visually hidden,
  * In many styles you don't know what the current item is after selecting
    blocks
  Especially the first problem is a big thing.

  New style multi-select works with a check boxes in front of each item. This
  delivers a challenge because what if the current item is not checked?

  This is the basis for the 'chosen' concept, which works in both single- and
  multi-selection modes. In multi-selection, an item is chosen if it is
  checked. If no box is checked, then you can have the listbox report the
  current item as the chosen item. To switch this off, tell the listbox that
  no chosen is a valid thing, i.e. pick the ZeroOrMore mode.

  Lastly, you can have some of the items marked. This will be done by
  surrounding the item with curly braces, like: '{the item}'. This will not
  affect selection/checked/chosen status.

*/
mExpClass(uiBase) uiListBoxObj : public uiObject
{
public:
			uiListBoxObj(uiParent*,const char* nm,OD::ChoiceMode);
			~uiListBoxObj();

    uiListBoxBody&	body()		{ return *body_; }

    bool		isEmpty() const override;
    void		setEmpty() override;

private:

    uiListBoxBody*	body_;
    uiListBoxBody&	mkbody(uiParent*,const char*,OD::ChoiceMode);

};



mExpClass(uiBase) uiListBox : public uiGroup
{ mODTextTranslationClass(uiListBox)
friend class i_listMessenger;
friend class uiListBoxBody;
public:
   enum LblPos		{ LeftTop, RightTop, LeftMid, RightMid,
			  AboveLeft, AboveMid, AboveRight,
			  BelowLeft, BelowMid, BelowRight };

    mExpClass(uiBase) Setup
    {
    public:
			Setup(OD::ChoiceMode icm=OD::ChooseOnlyOne,
			      const uiString& l=uiString::empty(),
			      uiListBox::LblPos lp=uiListBox::LeftTop)
			    : lbl_(l)
			    , cm_(icm)
			    , prefnrlines_(0)
			    , prefwidth_(0)
			    , lblpos_(lp)
			    {}

	mDefSetupMemb(uiString,lbl)
	mDefSetupMemb(OD::ChoiceMode,cm)
	mDefSetupMemb(int,prefnrlines)
	mDefSetupMemb(int,prefwidth)
	mDefSetupMemb(uiListBox::LblPos,lblpos)

    };

			uiListBox(uiParent*,const char* nm=0,
				  OD::ChoiceMode cm=OD::ChooseOnlyOne);
			uiListBox(uiParent*,const Setup&,const char* nm=0);
    virtual		~uiListBox();

    uiListBoxObj*	box()				{ return lb_; }
    uiGroup*		checkGroup()			{ return checkgrp_; }
    uiCheckBox*		primaryCheckBox()		{ return cb_; }
    int			nrLabels() const		{ return lbls_.size(); }
    uiLabel*		label( int nr=0 )		{ return lbls_[nr]; }
    void		setLabelText(const uiString&,int nr=0);

    inline OD::ChoiceMode choiceMode() const	{ return choicemode_; }
    inline bool		isMultiChoice() const
					{ return ::isMultiChoice(choicemode_); }
    void		setChoiceMode(OD::ChoiceMode);
    void		setMultiChoice(bool yn=true);
    void		setAllowNoneChosen(bool);
    void		setNotSelectable();

    int			size() const;
    bool		isEmpty() const		{ return size() < 1; }
    bool		validIdx(int) const;
    bool		isPresent(const char*) const;
    bool		isPresent(const uiString&) const;
    int			maxNrOfChoices() const;

    OD::Alignment::HPos alignment() const	{ return alignment_; }
    void		setAlignment(OD::Alignment::HPos);
    void		setNrLines(int);
    void		setFieldWidth(int);
    void		setHSzPol(uiObject::SzPolicy);
    void		setVSzPol(uiObject::SzPolicy);

    void		setEmpty();
    void		removeItem(int);
    void		removeItem(const char*);
    void		removeItem( const FixedString& fs )
						{ removeItem( fs.str() ); }
    void		setAllowDuplicates(bool yn);
    void		addItem(const uiString&,bool marked=false,int id=-1);
    void		addItem(const uiString&,const uiPixmap&,int id=-1);
    void		addItem(const uiString&,const Color&,int id=-1);
    void		addItem( const char* str, bool marked=false, int id=-1 )
				{ addItem( toUiString(str), marked, id ); }

    void		addItems(const BufferStringSet&);
    void		addItems(const uiStringSet&);

    void		insertItem(const uiString&,int idx=-1,
				   bool marked=false,int id=-1);
    void		insertItem(const uiString&,const uiPixmap&,
				   int idx=-1,int id=-1);
    void		insertItem(const uiString&,const Color&,
				   int idx=-1,int id=-1);
    void		insertItem( const char* str, int idx=-1,
				    bool marked=false, int id=-1 )
			{ insertItem(toUiString(str),idx,marked,id); }

    void		setPixmap(int,const Color&);
    void		setPixmap(int,const uiPixmap&);
    void		setIcon(int,const char* icon_identifier);
    void		setColor(int,const Color&);
    void		setColorIcon(int,const Color&);

    Color		getColor(int) const;

    void		sortItems(bool asc=true);
    void		sortNmItems(bool asc=true);

    int			indexOf(const char*) const;	//!< First match
    int			indexOf(const uiString&) const;	//!< First match
    const char*		itemText(int) const;
    uiString		textOfItem(int) const;
    void		getItems(BufferStringSet&) const;
    void		setItemText(int,const uiString&);
    void		setItemText( int idx, const char* str )
			{ setItemText( idx, toUiString(str) ); }
    void		setItemText( int idx, const OD::String& str )
			{ setItemText( idx, toUiString(str) ); }

    int			currentItem() const;
    const char*		getText() const  { return itemText(currentItem()); }
    void		setCurrentItem(int);
    void		setCurrentItem(const char*);	//!< First match
    void		setCurrentItem(const uiString&);
    void		setCurrentItem( const OD::String& str )
						{ setCurrentItem( str.str() ); }
    void		setItemSelectable(int,bool);

    int			nrChosen() const;
    bool		isChoosable(int) const;
    bool		isChosen(int) const;
    int			firstChosen() const;
    int			nextChosen(int prev=-1) const;
    void		getChosen(BufferStringSet&) const;
    void		getChosen(uiStringSet&) const;
    void		getChosen(TypeSet<int>&) const;
    void		setChoosable(int,bool yn);
    void		setChosen(int,bool yn=true);
    void		setChosen(Interval<int>,bool yn=true);
    void		chooseAll(bool yn=true);
    void		setChosen(const BufferStringSet&);
    void		setChosen(const TypeSet<int>&);

    void		displayItem(int, bool);

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
    CNotifier<uiListBox,int> itemChosen; /*< if itmidx==-1, many items were
						chosen at once */
    Notifier<uiListBox> doubleClicked;
    Notifier<uiListBox> rightButtonClicked;
    Notifier<uiListBox> leftButtonClicked;
    Notifier<uiListBox> deleteButtonPressed;

    void		offerReadWriteSelection( const CallBack& rcb,
						 const CallBack& wcb )
			{ retrievecb_ = rcb; savecb_ = wcb; }

private:

    void		translateText();

    OD::ChoiceMode	choicemode_;
    OD::Alignment::HPos alignment_;
    bool		allowduplicates_;
    uiMenu&		rightclickmnu_;
    mutable BufferString rettxt_;
    mutable uiString	uirettxt_;
    OD::ButtonState	buttonstate_;
    CallBack		savecb_;
    CallBack		retrievecb_;
    bool		allshown_;

    void		menuCB(CallBacker*);
    void		handleCheckChange(mQtclass(QListWidgetItem*));
    void		usrChooseAll(bool yn=true);

    bool		isNone() const	{ return choicemode_ == OD::ChooseNone;}
    int			optimumFieldWidth(int minwdth=20,int maxwdth=40) const;
    static int		cDefNrLines();		//!< == 7

    void		updateFields2ChoiceMode();
    void		initNewItem(int);
    void		setItemCheckable(int,bool);

    void		setItemsCheckable( bool yn )	{ setMultiChoice(yn); }
    void		setAllItemsChecked(bool);
    void		setItemChecked(int,bool);
    void		setItemChecked(const char*,bool);
    bool		isItemChecked(const char*) const;
    int			nrChecked() const;
    void		setCheckedItems(const BufferStringSet&);
    void		setCheckedItems(const TypeSet<int>&);
    void		getCheckedItems(BufferStringSet&) const;
    void		getCheckedItems(TypeSet<int>&) const;

    uiListBoxObj*	lb_;
    ObjectSet<uiLabel>	lbls_;
    uiGroup*		checkgrp_;
    uiCheckBox*		cb_;

    void		mkLabel(const uiString&,LblPos);
    void		mkCheckGroup();
    void		checkCB(CallBacker*);
    void		updateCheckState();

public:
			//!To be called by CmdDriver only, not for casual use.
    bool		isItemChecked(int) const;
};

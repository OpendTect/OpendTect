#ifndef uiListBox_H
#define uiListBox_H

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          16/05/2000
 RCS:           $Id: uilistbox.h,v 1.16 2001-12-30 23:02:19 bert Exp $
________________________________________________________________________

-*/

#include <uigroup.h>
class PtrUserIDObjectSet;
class uiLabel;
class uiListBoxBody;


class uiListBox : public uiObject
{
friend class i_listMessenger;
public:

                        uiListBox(uiParent* parnt=0, 
				  const char* nm="uiListBox",
				  bool isMultiSelect=false,
				  int preferredNrLines=0,
				  int preferredFieldWidth=0);

			uiListBox(uiParent*,const PtrUserIDObjectSet&,
				  bool isMultiSelect=false,
				  int preferredNrLines=0,
				  int preferredFieldWidth=0);

    virtual 		~uiListBox();

			/*! preferred number of lines. if set to 0, then 
                            it is determined by the number of items in list.
			    If set to 1, then the list can not 
			    grow/shrink vertically.
			*/
    void 		setLines( int prefNrLines );
    void		setNotSelectable();
    void		setMultiSelect(bool yn=true);

    int			size() const;
    bool		isPresent(const char*) const;
    bool		isSelected(int) const;
    void		setSelected(int,bool yn=true);
    void		selAll(bool yn=true);
    virtual void	clear()			{ setCurrentItem(0); }

    void		empty();
    void		addItem(const char*); 
    void		addItems(const char**); 
    void		addItems(const PtrUserIDObjectSet&);
    void		setItemText(int,const char*);
    int			currentItem() const;
    const char*		getText() const	 { return textOfItem(currentItem()); }
    const char*		textOfItem(int) const;
    void                setCurrentItem(int);
    void                setCurrentItem(const char*); //!< First match

    int			lastClicked()		{ return lastClicked_; }

    Notifier<uiListBox> selectionChanged;

			//! sets lastClicked
    Notifier<uiListBox> doubleClicked;

			//! sets lastClicked
    Notifier<uiListBox> rightButtonClicked;

protected:

    mutable BufferString	rettxt;
    int			lastClicked_;

private:

    uiListBoxBody*		body_;
    uiListBoxBody&		mkbody(uiParent*, const char*, bool, int, int);

};


class uiLabeledListBox : public uiGroup
{
public:

    enum LblPos	{ LeftTop, RightTop,
		  AboveLeft, AboveMid, AboveRight,
		  BelowLeft, BelowMid, BelowRight };

		uiLabeledListBox( uiParent*,const char* txt,
				  bool multisel=false,LblPos p=LeftTop);
		uiLabeledListBox( uiParent*,const PtrUserIDObjectSet&,
				  bool multisel=false,LblPos p=LeftTop);

    uiListBox*	box()				{ return lb; }

    int		nrLabels() const		{ return lbls.size(); }
    uiLabel*	label( int nr=0 )		{ return lbls[nr]; }
    const char*	labelText(int nr=0) const;
    void	setLabelText(const char*,int nr=0);


protected:

    uiListBox*		lb;
    ObjectSet<uiLabel>	lbls;

    void		mkRest(const char*,LblPos);

};


#endif

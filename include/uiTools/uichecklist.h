#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"

#include "uigroup.h"
#include "odcommonenums.h"

class uiCheckBox;
class uiLabel;
class BufferStringSet;

/*! \brief Group of check boxes. Ensures a certain policy is honored.

 - Unrel: all boxes can be on or off
 - NotAll: Not all boxes can be on
 - AtLeastOne: Not all boxes can be off
 - OneOnly: radio-behavior (but the items still are check boxes)
 - MaybeOne: also radio, but also allowing no choice at all
 - Chain: next is only available if previous is checked:
   -- Chain1st: 1st check determines availability of all others
   -- ChainAll: Nth check determines availability all > N

 */

mExpClass(uiTools) uiCheckList : public uiGroup
{
public:

    enum Pol		{ Unrel, NotAll, OneMinimum, OneOnly, MaybeOne,
			  Chain1st, ChainAll };

			uiCheckList(uiParent*,Pol=Unrel,
				    OD::Orientation orient=OD::Vertical);
			~uiCheckList();

    void		setLabel(const uiString&);
    uiCheckList&	addItem(const uiString& txt,const char* iconfnm=0);
    uiCheckList&	addItems(const BufferStringSet&);
    uiCheckList&	addItems(const uiStringSet&);
    uiCheckList&	displayIdx(int,bool);

    Pol			pol() const		{ return pol_; }
    OD::Orientation	orientation() const	{ return orientation_; }
    inline bool		isHor() const { return orientation_ == OD::Horizontal; }

    int			size() const		{ return boxs_.size(); }
    bool		isChecked(int) const;
    uiCheckList&	setChecked(int,bool);
    int			firstChecked() const;
    int			lastChecked() const;

    Notifier<uiCheckList> changed;
    uiCheckBox*		clicked()		{ return clicked_; }

protected:

    const Pol		pol_;
    const OD::Orientation orientation_;
    ObjectSet<uiCheckBox> boxs_;
    uiGroup*		grp_;
    uiLabel*		lbl_;
    uiCheckBox*		clicked_;

    void		setBox(int,bool chkd,bool shw=true);

    void		initObj(CallBacker*);
    void		boxChk(CallBacker*);

    void		ensureOne(bool);
    void		handleRadio(bool);
    void		handleChain();

public:

			// use this to do special things, like set tooltips
    uiCheckBox*		box( int idx )		{ return boxs_[idx]; }
    const uiCheckBox*	box( int idx ) const	{ return boxs_[idx]; }

};

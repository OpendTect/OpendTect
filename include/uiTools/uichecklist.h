#ifndef uichecklist_h
#define uigeninput_impl_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Sep 2011
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigroup.h"
class uiCheckBox;
class BufferStringSet;

/*! \brief Group of check boxes. Ensures a certain policy is honored.

 - Unrel: all boxes can be on or off
 - NotAll: Not all boxes can be on
 - AtLeastOne: Not all boxes can be off
 - OneOnly: radio-behaviour (but the items still are check boxes)
 - MaybeOne: also radio, but also allowing no choice at all
 - Chain: next is only available if previous is checked:
   -- Chain1st: 1st check determines availability of all others
   -- ChainAll: Nth check determines availability all > N

 */

mExpClass(uiTools) uiCheckList : public uiGroup
{
public:

    typedef uiObject::Orientation Orientation;

    enum Pol		{ Unrel, NotAll, AtLeastOne, OneOnly, MaybeOne,
			  Chain1st, ChainAll };

			uiCheckList(uiParent*,Pol=Unrel,
				    Orientation orient=uiObject::Vertical);

    uiCheckList&	addItem(const char* txt,const char* iconfnm=0);
    uiCheckList&	addItems(const BufferStringSet&);
    Pol			pol() const		{ return pol_; }
    Orientation		orientation() const	{ return orientation_; }
    inline bool		isHor() const
				{ return orientation_ == uiObject::Horizontal; }

    int			size() const		{ return boxs_.size(); }
    bool		isChecked(int) const;
    uiCheckList&	setChecked(int,bool);
    int			firstChecked() const;
    int			lastChecked() const;

    Notifier<uiCheckList> changed;
    uiCheckBox*		clicked()		{ return clicked_; }

protected:

    const Pol		pol_;
    const Orientation	orientation_;
    ObjectSet<uiCheckBox> boxs_;
    uiCheckBox*		clicked_;

    void		setBox(int,bool chkd,bool shw=true);

    void		initGrp(CallBacker*);
    void		boxChk(CallBacker*);

    void		ensureOne(bool);
    void		handleRadio(bool);
    void		handleChain();

public:

			// use this to do special things, like set tooltips
    uiCheckBox*		box( int idx )		{ return boxs_[idx]; }
    const uiCheckBox*	box( int idx ) const	{ return boxs_[idx]; }

};


#endif

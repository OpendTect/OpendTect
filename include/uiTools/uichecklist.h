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

/*! \brief Group of check boxes
 
 If you construct with 2 strings, then then the layout will be single-line
 (horizontal). With the BufferStringSet, it will be vertical.
 Policies:
 - Unrel: all boxes can be on or off
 - NotAll: Not all boxes can be on
 - AtLeastOne: Not all boxes can be off
 - OneOnly: radio-behaviour (but the items still are check boxes)
 - MaybeOne: also radio, but allowing no choice at all
 - Chain: next is only available if previous is checked:
   -- Chain1st: 1st check determines availability of all others
   -- ChainAll: Nth check determines availability all > N
 
 */

mExpClass(uiTools) uiCheckList : public uiGroup
{
public:

    enum Pol		{ Unrel, NotAll, AtLeastOne, OneOnly, MaybeOne,
			  Chain1st, ChainAll };

			uiCheckList(uiParent*,const BufferStringSet&,Pol=Unrel,
				    bool force_hor=false);
			uiCheckList(uiParent*,
				    const char*,const char*,Pol=Unrel);
    Pol			pol() const		{ return pol_; }

    int			size() const		{ return boxs_.size(); }
    bool		isChecked(int) const;
    void		setChecked(int,bool);
    int			firstChecked() const;
    int			lastChecked() const;

    Notifier<uiCheckList> changed;
    uiCheckBox*		clicked()		{ return clicked_; }

protected:

    const Pol		pol_;
    ObjectSet<uiCheckBox> boxs_;
    uiCheckBox*		clicked_;

    void		addBox(const char*,bool hor=true);
    void		setBox(int,bool chkd,bool shw=true);

    void		initGrp(CallBacker*);
    void		boxChk(CallBacker*);

    void		ensureOne(bool);
    void		handleRadio(bool);
    void		handleChain();

public:

    			// use this to set tooltips etc.
    uiCheckBox*		box( int idx )		{ return boxs_[idx]; }
    const uiCheckBox*	box( int idx ) const	{ return boxs_[idx]; }

};


#endif


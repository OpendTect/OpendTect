#ifndef uichecklist_h
#define uigeninput_impl_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Sep 2011
 RCS:           $Id: uichecklist.h,v 1.3 2011-09-07 12:03:31 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
class uiCheckBox;
class BufferStringSet;

/*! \brief Group of check boxes
 
 If you construct with 2 strings, then then the layout will be single-line
 (horizontal). With the BufferStringSet, it will be vertical.
 Policies:
 - Unrel: all boxes can be on or off
 - NotAll: Not all boxes can be on
 - AtLeastOne: Not all boxes can be off - be sure to set one yourself initially
 - Chain: 2nd is only available if 1st is checked;
   -- Chain1st: 1st check determines availability of all others
   -- ChainAll: Nth check determines availability all > N
 
 */

mClass uiCheckList : public uiGroup
{
public:

    enum Pol		{ Unrel, NotAll, AtLeastOne, Chain1st, ChainAll };

                        uiCheckList(uiParent*,const BufferStringSet&,Pol=Unrel);
                        uiCheckList(uiParent*,
				    const char*,const char*,Pol=Unrel);
    Pol			pol() const		{ return pol_; }

    int			size() const		{ return boxs_.size(); }
    bool		isChecked(int) const;
    void		setChecked(int,bool);

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
    void		handleChain();

};


#endif

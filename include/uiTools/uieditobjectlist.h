#ifndef uieditobjectlist_h
#define uieditobjectlist_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Sep 2012
 RCS:           $Id: uieditobjectlist.h,v 1.1 2012-09-14 10:38:57 cvsbert Exp $
________________________________________________________________________

-*/


#include "uitoolsmod.h"
#include "uigroup.h"
class BufferStringSet;
class uiListBox;
class uiButtonGroup;


/*!\brief Base class for element allowing maintenance of lists of
		(usually small) objects. */

mClass(uiTools) uiEditObjectList : public uiGroup
{
public:

			uiEditObjectList(uiParent*,const char* itmtyp,
					 bool movable,bool compact=false);

    uiButtonGroup*	buttons()		{ return bgrp_; }

    Notifier<uiEditObjectList>	selectionChange;

protected:

    uiListBox*		listfld_;
    uiButtonGroup*	bgrp_;

    virtual void	editReq(bool isadd)	= 0;
    virtual void	removeReq()		= 0;
    virtual void	itemSwitch(bool up)	{}

    int			currentItem() const;
    void		setItems(const BufferStringSet&,int newcur=-1);

    void		addCB(CallBacker*)	{ editReq(true); }
    void		edCB(CallBacker*)	{ editReq(false); }
    void		rmCB(CallBacker*)	{ removeReq(); }
    void		upCB(CallBacker*)	{ itemSwitch(true); }
    void		downCB(CallBacker*)	{ itemSwitch(false); }
    void		selChgCB(CallBacker*)	{ selectionChange.trigger(); }

};


#endif


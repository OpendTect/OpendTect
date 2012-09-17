#ifndef uieditobjectlist_h
#define uieditobjectlist_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Sep 2012
 RCS:           $Id: uieditobjectlist.h,v 1.2 2012/09/17 10:01:31 cvsbert Exp $
________________________________________________________________________

-*/


#include "uigroup.h"
class BufferStringSet;
class uiListBox;
class uiButton;
class uiButtonGroup;


/*!\brief Base class for element allowing maintenance of lists of
		(usually small) objects. */

mClass uiEditObjectList : public uiGroup
{
public:

			uiEditObjectList(uiParent*,const char* itmtyp,
					 bool movable,bool compact=false);

    uiButtonGroup*	buttons()		{ return bgrp_; }

    Notifier<uiEditObjectList>	selectionChange;

protected:

    uiListBox*		listfld_;
    uiButtonGroup*	bgrp_;
    uiButton*		addbut_;
    uiButton*		edbut_;
    uiButton*		rmbut_;
    uiButton*		upbut_;
    uiButton*		downbut_;

    virtual void	editReq(bool isadd)	= 0;
    virtual void	removeReq()		= 0;
    virtual void	itemSwitch(bool up)	{}

    int			currentItem() const;
    void		setItems(const BufferStringSet&,int newcur=-1);
    void		manButSt();

    void	addCB(CallBacker*)	{ editReq(true);	manButSt(); }
    void	edCB(CallBacker*)	{ editReq(false);	manButSt(); }
    void	rmCB(CallBacker*)	{ removeReq();		manButSt(); }
    void	upCB(CallBacker*)	{ itemSwitch(true);	manButSt(); }
    void	downCB(CallBacker*)	{ itemSwitch(false);	manButSt(); }
    void	selChgCB(CallBacker*)	{ selectionChange.trigger();
								manButSt(); }

};


#endif


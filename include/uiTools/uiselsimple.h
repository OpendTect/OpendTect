#ifndef uiselsimple_h
#define uiselsimple_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Dec 2001
 RCS:           $Id: uiselsimple.h,v 1.4 2003-11-07 12:21:54 bert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class uiListBox;
class BufferStringSet;

/*!\brief Select entry from list

Use setTitleText to set a message different from caption.

*/

class uiSelectFromList : public uiDialog
{ 	
public:
			uiSelectFromList(uiParent*,
					 const BufferStringSet&,
					 const char* cur=0,
					 const char* captn="Please select");
			uiSelectFromList(uiParent*,
					 const char**,int nritems=-1,
					 const char* cur=0,
					 const char* captn="Please select");
			~uiSelectFromList()	{}

    int			selection() const	{ return sel_; }
    			//!< -1 = no selection made (cancelled or 0 list items)

    uiListBox*		selfld;

protected:

    int			sel_;

    bool		acceptOK(CallBacker*);

private:

    void		init(const char**,int,const char*);

};


#endif

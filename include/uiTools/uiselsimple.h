#ifndef uiselsimple_h
#define uiselsimple_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Dec 2001
 RCS:           $Id: uiselsimple.h,v 1.5 2007-01-08 17:06:55 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class uiListBox;
class uiGenInput;
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


/*!\brief Get a name from user, whilst displaying names that already exist */

class uiGetObjectName : public uiDialog
{ 	
public:
			uiGetObjectName(uiParent*,const char* txt,
					const BufferStringSet&,
					const char* deflt=0,
				    const char* captn="Please provide a name");
			~uiGetObjectName()	{}

    const char*		text() const;

protected:

    uiGenInput*		inpfld_;
    uiListBox*		listfld_;

    void		selChg(CallBacker*);
    bool		acceptOK(CallBacker*);

};


#endif

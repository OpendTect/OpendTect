#ifndef uiselsimple_h
#define uiselsimple_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          Dec 2001
 RCS:           $Id: uiselsimple.h,v 1.2 2002-05-28 08:40:58 bert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class uiListBox;
class PtrUserIDObjectSet;

/*!\brief Select entry from list

Use setTitleText to set a message different from caption.

*/

class uiSelectFromList : public uiDialog
{ 	
public:
			uiSelectFromList(uiParent*,
					 const ObjectSet<BufferString>&,
					 const char* cur=0,
					 const char* captn="Please select");
			uiSelectFromList(uiParent*,
					 const PtrUserIDObjectSet&,
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

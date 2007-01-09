#ifndef uiselsimple_h
#define uiselsimple_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Dec 2001
 RCS:           $Id: uiselsimple.h,v 1.6 2007-01-09 13:21:06 cvsbert Exp $
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

    class Setup : public uiDialog::Setup
    {
    public:
			Setup(const char* dlgtitl,const BufferStringSet&);

	mDefSetupMemb(BufferString,current);

	const BufferStringSet& items() const	{ return items_; }

    protected:

	const BufferStringSet&	items_;

    };

			uiSelectFromList(uiParent*,const Setup&);
			~uiSelectFromList()	{}

    int			selection() const	{ return sel_; }
    			//!< -1 = no selection made (cancelled or 0 list items)

    uiListBox*		selfld_;

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

    uiGenInput*		inpFld()		{ return inpfld_; }
    			//!< Is the lowest field

protected:

    uiGenInput*		inpfld_;
    uiListBox*		listfld_;

    void		selChg(CallBacker*);
    bool		acceptOK(CallBacker*);

};


#endif

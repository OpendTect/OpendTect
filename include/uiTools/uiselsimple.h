#ifndef uiselsimple_h
#define uiselsimple_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Dec 2001
 RCS:           $Id: uiselsimple.h,v 1.8 2007-01-11 12:37:49 cvsnanne Exp $
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
			Setup( const char* wintitl, const BufferStringSet& its )
			: uiDialog::Setup(wintitl)
			, items_(its)		{}

	mDefSetupMemb(BufferString,current);

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

    class Setup : public uiDialog::Setup
    {
    public:
			Setup( const char* wintitl,const BufferStringSet& its )
			: uiDialog::Setup(wintitl)
			, items_(its)
			, inptxt_("Name")	{}

	mDefSetupMemb(BufferString,deflt);
	mDefSetupMemb(BufferString,inptxt);

	const BufferStringSet&	items_;

    };

			uiGetObjectName(uiParent*,const Setup&);
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

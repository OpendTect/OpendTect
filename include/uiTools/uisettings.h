#ifndef uisettings_h
#define uisettings_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		November 2001
 RCS:		$Id: uisettings.h,v 1.5 2003-11-07 12:21:54 bert Exp $
________________________________________________________________________

-*/


#include "uidialog.h"
#include "bufstringset.h"
class Settings;
class uiGenInput;
class uiLabeledComboBox;


class uiSettings : public uiDialog
{
public:
			uiSettings(uiParent*,const char* titl,
				   const char* settskey=0);
    virtual		~uiSettings();

protected:

    Settings&		setts;
    BufferStringSet	items;
    int			curidx;

    uiLabeledComboBox*	itemfld;
    uiGenInput*		valfld;
    uiGenInput*		ynfld;

    void		itemSel(CallBacker*);
    void		setNew();
    void		commitPrev();
    bool		acceptOK(CallBacker*);

};

#endif

#ifndef uisettings_h
#define uisettings_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Nanne Hemstra
 Date:		November 2001
 RCS:		$Id: uisettings.h,v 1.4 2003-10-21 09:54:31 bert Exp $
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

#ifndef uisettings_h
#define uisettings_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Nanne Hemstra
 Date:		November 2001
 RCS:		$Id: uisettings.h,v 1.3 2003-10-17 14:19:01 bert Exp $
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
			uiSettings(uiParent*,const char*);
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

#ifndef uisettings_h
#define uisettings_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Nanne Hemstra
 Date:		November 2001
 RCS:		$Id: uisettings.h,v 1.1 2001-11-16 14:28:31 nanne Exp $
________________________________________________________________________

-*/


#include "uidialog.h"
#include "uidset.h"
class Settings;
class uiGenInput;
class uiLabeledComboBox;


class uiSettings : public uiDialog
{
public:
			uiSettings(uiParent*,const char*);
			~uiSettings()	{}

protected:

    Settings&		setts;
    UserIDSet		items;
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

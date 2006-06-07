#ifndef uihorizonrelations_h
#define uihorizonrelations_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		April 2006
 RCS:		$Id: uihorizonrelations.h,v 1.2 2006-06-07 06:57:14 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

#include "bufstringset.h"
#include "multiid.h"

class uiLabeledListBox;
class uiPushButton;
class BufferStringSet;

class uiHorizonRelationsDlg : public uiDialog
{
public:
			uiHorizonRelationsDlg(uiParent*);

protected:
    uiLabeledListBox*	relationfld_;
    uiPushButton*	crossbut_;
    uiPushButton*	waterbut_;

    BufferStringSet	hornames_;
    TypeSet<MultiID>	horids_;

    void		fillRelationField(const BufferStringSet&);
    void		read();
    bool		write();

    void		readHorizonCB(CallBacker*);
    void		checkCrossingsCB(CallBacker*);
    void		makeWatertightCB(CallBacker*);
    bool		rejectOK(CallBacker*);
};

#endif

#ifndef uihorizonrelations_h
#define uihorizonrelations_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		April 2006
 RCS:		$Id: uihorizonrelations.h,v 1.1 2006-05-05 06:46:07 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

#include "bufstringset.h"
#include "multiid.h"

class uiLabeledListBox;
class BufferStringSet;

class uiHorizonRelationsDlg : public uiDialog
{
public:
			uiHorizonRelationsDlg(uiParent*);

protected:
    uiLabeledListBox*	relationfld_;

    BufferStringSet	hornames_;
    TypeSet<MultiID>	horids_;

    void		fillRelationField(const BufferStringSet&);
    void		read();
    bool		write();

    void		updateOrderCB(CallBacker*);
    void		checkCrossingsCB(CallBacker*);
    void		makeWatertightCB(CallBacker*);
    bool		rejectOK(CallBacker*);
};

#endif

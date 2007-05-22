#ifndef uihorizonrelations_h
#define uihorizonrelations_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		April 2006
 RCS:		$Id: uihorizonrelations.h,v 1.3 2007-05-22 03:23:22 cvsnanne Exp $
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
			uiHorizonRelationsDlg(uiParent*,bool is2d);

protected:
    uiLabeledListBox*	relationfld_;
    uiPushButton*	crossbut_;
    uiPushButton*	waterbut_;

    BufferStringSet	hornames_;
    TypeSet<MultiID>	horids_;
    bool		is2d_;

    void		fillRelationField(const BufferStringSet&);
    void		read();
    bool		write();

    void		readHorizonCB(CallBacker*);
    void		checkCrossingsCB(CallBacker*);
    void		makeWatertightCB(CallBacker*);
    bool		rejectOK(CallBacker*);
};

#endif

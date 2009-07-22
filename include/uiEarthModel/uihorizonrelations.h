#ifndef uihorizonrelations_h
#define uihorizonrelations_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		April 2006
 RCS:		$Id: uihorizonrelations.h,v 1.5 2009-07-22 16:01:21 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

#include "bufstringset.h"
#include "multiid.h"

class uiLabeledListBox;
class uiPushButton;
class BufferStringSet;

mClass uiHorizonRelationsDlg : public uiDialog
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

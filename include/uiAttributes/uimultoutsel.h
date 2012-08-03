#ifndef uimultoutsel_h
#define uimultoutsel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        H. Huck
 Date:          Jan 2009
 RCS:           $Id: uimultoutsel.h,v 1.4 2012-08-03 13:00:49 cvskris Exp $
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uidialog.h"

class BufferStringSet;
class uiCheckBox;
class uiListBox;
namespace Attrib { class Desc; };

/*! \brief Dialog used to determine which outputs of an attribute will be computed at once and stored as multiple components when creating a volume output */

mClass(uiAttributes) uiMultOutSel : public uiDialog
{
public:
			uiMultOutSel(uiParent*,const Attrib::Desc&);

    void		getSelectedOutputs(TypeSet<int>&) const;
    void		getSelectedOutNames(BufferStringSet&) const;
    bool		doDisp() const;

protected:

    void		fillInAvailOutNames(Attrib::Desc*,
	                                    BufferStringSet&) const;

    void		createMultOutDlg(const BufferStringSet&);
    void		allSel(CallBacker*);

    uiListBox*		outlistfld_;
    uiCheckBox*		outallfld_;

};

#endif


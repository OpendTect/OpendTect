#ifndef uimultoutsel_h
#define uimultoutsel_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Huck
 Date:          Jan 2009
 RCS:           $Id: uimultoutsel.h,v 1.1 2009-01-07 11:19:50 cvshelene Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class BufferStringSet;
class uiListBox;
namespace Attrib { class Desc; };

/*! \brief Dialog used to determine which outputs of an attribute will be computed at once and stored as multiple components when creating a volume output */

mClass uiMultOutSel : public uiDialog
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

    uiListBox*		outlistfld_;

};

#endif

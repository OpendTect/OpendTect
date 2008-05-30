#ifndef uicoltabimport_h
#define uicoltabimport_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki Maitra
 Date:          April 2008
 RCS:           $Id: uicoltabimport.h,v 1.1 2008-05-30 04:10:33 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "sets.h"

namespace ColTab { class Sequence; class SeqMgr; }

class uiFileInput;
class uiGenInput;
class uiLabeledListBox;


class uiColTabImport : public uiDialog
{
public:
				uiColTabImport(uiParent*);
				~uiColTabImport();

    const char*			getCurrentSelColTab() const;

protected:

    uiFileInput*	        homedirfld_;
    uiGenInput*         	dtectusrfld_;
    uiLabeledListBox*   	listfld_;

    ObjectSet<ColTab::Sequence> seqs_;

    void			usrSel(CallBacker*);
    bool			acceptOK(CallBacker*);
};

#endif

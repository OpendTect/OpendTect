#ifndef uicoltabimport_h
#define uicoltabimport_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki Maitra
 Date:          April 2008
 RCS:           $Id: uicoltabimport.h,v 1.2 2009-01-08 07:07:01 cvsranojay Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "sets.h"

namespace ColTab { class Sequence; class SeqMgr; }

class uiFileInput;
class uiGenInput;
class uiLabeledListBox;


mClass uiColTabImport : public uiDialog
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

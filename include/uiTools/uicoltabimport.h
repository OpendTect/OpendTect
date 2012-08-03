#ifndef uicoltabimport_h
#define uicoltabimport_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          April 2008
 RCS:           $Id: uicoltabimport.h,v 1.4 2012-08-03 13:01:12 cvskris Exp $
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uidialog.h"
#include "sets.h"

namespace ColTab { class Sequence; class SeqMgr; }

class uiFileInput;
class uiGenInput;
class uiLabeledListBox;


mClass(uiTools) uiColTabImport : public uiDialog
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


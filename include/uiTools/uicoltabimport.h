#ifndef uicoltabimport_h
#define uicoltabimport_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          April 2008
 RCS:           $Id: uicoltabimport.h,v 1.3 2009/07/22 16:01:23 cvsbert Exp $
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

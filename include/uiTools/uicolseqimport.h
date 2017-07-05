#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          April 2008
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uidialog.h"
#include "sets.h"
#include "uistring.h"

namespace ColTab { class Sequence; }

class uiFileSel;
class uiGenInput;
class uiLabel;
class uiListBox;


mExpClass(uiTools) uiColSeqImport : public uiDialog
{ mODTextTranslationClass(uiColSeqImport)
public:
				uiColSeqImport(uiParent*);
				~uiColSeqImport();

    const char*			currentSeqName() const;

protected:

    uiGenInput*			choicefld_;
    uiFileSel*			dirfld_;
    uiGenInput*			dtectusrfld_;
    uiListBox*			listfld_;
    uiLabel*			messagelbl_;

    ObjectSet<ColTab::Sequence> seqs_;

    void			choiceSel(CallBacker*);
    void			usrSel(CallBacker*);
    bool			acceptOK();

    void			showMessage(const uiString&);
    void			showList();

private :

    static uiString		getLabelText(bool);

};

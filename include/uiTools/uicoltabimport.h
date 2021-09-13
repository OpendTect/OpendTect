#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          April 2008
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uidialog.h"
#include "sets.h"
#include "uistring.h"

namespace ColTab { class Sequence; class SeqMgr; }

class uiFileInput;
class uiGenInput;
class uiLabel;
class uiListBox;


mExpClass(uiTools) uiColTabImport : public uiDialog
{ mODTextTranslationClass(uiColTabImport)
public:
				uiColTabImport(uiParent*);
				~uiColTabImport();

    const char*			getCurrentSelColTab() const;

protected:

    uiGenInput*			choicefld_;
    uiFileInput*		dirfld_;
    uiGenInput*			dtectusrfld_;
    uiListBox*			listfld_;
    uiLabel*			messagelbl_;

    ObjectSet<ColTab::Sequence> seqs_;

    void			choiceSel(CallBacker*);
    void			usrSel(CallBacker*);
    bool			acceptOK(CallBacker*);

    void			showMessage(const uiString&);
    void			showList();

    void			getFromSettingsPar(const IOPar&);
    void			getFromAlutFiles(const BufferStringSet&);

private :
				mDeprecated("Use getLabelStr instead")
    static uiString		getLabel(bool);
    static uiString		getLabelStr(int);
};


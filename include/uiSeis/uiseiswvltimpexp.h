#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2006 / Dec 2009
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uidialog.h"
#include "dbkey.h"

class CtxtIOObj;
class IOObj;
class uiGenInput;
class uiWaveletIOObjSel;
class uiFileSel;
class uiTableImpDataSel;
namespace Table { class FormatDesc; }


mExpClass(uiSeis) uiSeisWvltImp : public uiDialog
{ mODTextTranslationClass(uiSeisWvltImp)
public:
			uiSeisWvltImp(uiParent*);
			~uiSeisWvltImp();

    DBKey		selKey() const;

protected:

    Table::FormatDesc&	fd_;

    uiFileSel*		inpfld_;
    uiTableImpDataSel*	dataselfld_;
    uiGenInput*		scalefld_;
    uiWaveletIOObjSel*	wvltfld_;

    bool		acceptOK();

};


mExpClass(uiSeis) uiSeisWvltExp : public uiDialog
{ mODTextTranslationClass(uiSeisWvltExp)
public:
			uiSeisWvltExp(uiParent*);

protected:

    uiWaveletIOObjSel*	wvltfld_;
    uiFileSel*		outpfld_;
    uiGenInput*		addzfld_;

    bool		acceptOK();

};


mExpClass(uiSeis) uiSeisWvltCopy : public uiDialog
{ mODTextTranslationClass(uiSeisWvltCopy)
public:
			uiSeisWvltCopy(uiParent*,const IOObj*);
			~uiSeisWvltCopy();

    DBKey		getDBKey() const;

protected:

    uiWaveletIOObjSel*	wvltinfld_;
    uiWaveletIOObjSel*	wvltoutfld_;
    uiGenInput*		scalefld_;

    bool		acceptOK();

};

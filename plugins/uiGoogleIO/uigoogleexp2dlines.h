#pragma once
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2009
-*/

#include "uigoogleexpdlg.h"
#include "bufstringset.h"
class uiGenInput;
class uiSelLineStyle;
class uiSeis2DFileMan;
class BufferStringSet;
namespace ODGoogle { class XMLWriter; }


mExpClass(uiGoogleIO) uiGISExport2DSeis : public uiDialog
{ mODTextTranslationClass(uiGISExport2DSeis);
public:
			uiGISExport2DSeis(uiParent*,uiSeis2DFileMan* m=nullptr,
			    const BufferString& linenm=BufferString::empty());
			~uiGISExport2DSeis();

protected:

    uiSeis2DFileMan*	s2dfm_		= nullptr;
    bool		allsel_		= false;
    BufferStringSet	sellnms_;

    uiGenInput*		putlnmfld_;
    uiGenInput*		putallfld_	= nullptr;
    uiSelLineStyle*	lsfld_;
    uiGISExpStdFld*	expfld_;

    void		getCoordsForLine(RefObjectSet<const Pick::Set>&,
					 const BufferString& lnm);
    void		getInitialSelectedLineNames();
    void		getFinalSelectedLineNames();

    bool		acceptOK(CallBacker*);
};

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


mClass(uiGoogleIO) uiGISExport2DSeis : public uiDialog
{ mODTextTranslationClass(uiGoogleExport2DSeis);
public:

			uiGISExport2DSeis(uiSeis2DFileMan*);
			~uiGISExport2DSeis();

protected:

    uiSeis2DFileMan*	s2dfm_;
    bool		allsel_;
    BufferStringSet	sellnms_;

    uiGenInput*		putlnmfld_;
    uiGenInput*		putallfld_;
    uiSelLineStyle*	lsfld_;
    uiGISExpStdFld*	expfld_;

    void		getCoordsForLine(ObjectSet<Pick::Set>&,const char*);
    void		getInitialSelectedLineNames();
    void		getFinalSelectedLineNames();

    bool		acceptOK();


};

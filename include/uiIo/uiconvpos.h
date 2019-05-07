#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2001
________________________________________________________________________

-*/

#include "enums.h"

#include "uiiocommon.h"
#include "uidialog.h"

class uiGenInput;
class uiFileSel;
class SurveyInfo;
class uiPushButton;
class uiComboBox;

namespace Coords { class uiCoordSystemSel; }


mExpClass(uiIo) uiConvertPos : public uiDialog
{ mODTextTranslationClass(uiConvertPos);

public:
				uiConvertPos(uiParent*, const SurveyInfo&,
							    bool modal=true);
				~uiConvertPos();
    enum ConversionType		{ CRS, IC, XY, LL };
				mDeclareEnumUtils( ConversionType );

private:

    const SurveyInfo&		survinfo_;

    uiGenInput*			manfld_;
    uiGenInput*			inputypfld_;
    uiGenInput*			outputtypfld_;
    uiGenInput*			leftinpfld_;
    uiGenInput*			rightinpfld_;
    uiGenInput*			leftoutfld_;
    uiGenInput*			rightoutfld_;
    Coords::uiCoordSystemSel*	inpcrdsysselfld_;
    Coords::uiCoordSystemSel*	outcrdsysselfld_;
    uiPushButton*		convertbut_;
    uiFileSel*			inpfilefld_;
    uiFileSel*			outfilefld_;

    TypeSet<int>		outidxs_;
    od_ostream*			ostream_;
    float			firstinp_;
    float			secondinp_;
    BufferString		linebuf_;

    void			selChg(CallBacker*);
    void			getCoord(CallBacker*);
    void			getBinID(CallBacker*);
    void			convertCB(CallBacker*);
    void			inputTypChg(CallBacker*);
    void			outputTypChg(CallBacker*);

    void			convFile();
    void			convManually();

    void			convFromIC(bool);
    void			convFromXY(bool);
    void			convFromLL(bool);

    void			launchSelConv(bool,int);

    ConversionType		getConversionType();
    void			errMsgNEmpFlds();
};

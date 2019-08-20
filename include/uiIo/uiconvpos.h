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
#include "uidlggroup.h"
#include "tableascio.h"

class SurveyInfo;
class uiCheckBox;
class uiComboBox;
class uiFileSel;
class uiGenInput;
class uiFileConvGroup;
class uiLabel;
class uiListBox;
class uiManualConvGroup;
class uiPushButton;
class uiTableImpDataSel;
class uiTabStack;

namespace Table { class Desc; }
namespace Coords 
{ 
    class uiCoordSystemSel;
    class CoordSystem;
}

mExpClass(uiIo) uiConvertPos : public uiDialog
{ mODTextTranslationClass(uiConvertPos);

public:
				uiConvertPos(uiParent*, const SurveyInfo&,
							    bool modal=true);
				~uiConvertPos();
    enum DataType		{ LL, IC, XY };
				mDeclareEnumUtils( DataType );
    enum LatLongType		{ Dec, DMS };
				mDeclareEnumUtils( LatLongType );
    static const uiString	sLLStr() { return tr("Latitude/Longitude"); }
    static const uiString	sICStr() { return tr("Inline/Crossline"); }
    static const uiString	sXYStr() { return tr("Cartesian Coordinate"); }

private:

    uiManualConvGroup*		mangrp_;
    uiFileConvGroup*		filegrp_;
    uiTabStack*			tabstack_=0;
};

typedef uiConvertPos::LatLongType   LLType;
typedef uiConvertPos::DataType	    DataSelType;

mExpClass(uiIo) uiConvPosAscIO : public Table::AscIO
{mODTextTranslationClass(uiConvPosAscIO)
public:
				    uiConvPosAscIO(const Table::FormatDesc&,
								od_istream&);

    static Table::FormatDesc*	    getDesc();
    bool			    getData( Coord& );
    float			    udfval_;
    od_istream&			    strm_;
    bool			    finishedreadingheader_;
    DataSelType			    getConvFromTyp();
    LLType			    getLatLongType();

protected:
    bool			    isXY() const;
    bool			    isLL() const;
    bool			    isIC() const;



};


mExpClass(uiIo) uiManualConvGroup : public uiDlgGroup
{mODTextTranslationClass(uiManualConvGroup)

public:
				uiManualConvGroup(uiParent*,const SurveyInfo&);
				~uiManualConvGroup();

protected:
    uiGenInput*			inptypfld_;
    uiGenInput*			lltypfld_;
    uiGenInput*			leftinpfld_;
    uiGenInput*			rightinpfld_;
    uiGenInput*			leftoutfld1_;
    uiGenInput*			rightoutfld1_;
    uiGenInput*			leftoutfld2_;
    uiGenInput*			rightoutfld2_;
    uiGenInput*			leftoutfld3_;
    uiGenInput*			rightoutfld3_;
    uiPushButton*		convertbut_;
    uiLabel*			lltypelbl_;
    Coords::uiCoordSystemSel*	inpcrdsysselfld_;
    Coords::uiCoordSystemSel*	outcrdsysselfld_;
    uiCheckBox*			towgs84fld_;

    Coords::CoordSystem*	inpcoordsys_;
    Coords::CoordSystem*	outcoordsys_;

    const SurveyInfo&		survinfo_;

    void			inputTypChg(CallBacker*);
    void			llFormatTypChg(CallBacker*);
    void			convButPushCB(CallBacker*);

    void			convFromLL();
    void			convFromIC();
    void			convFromXY();
};


mExpClass(uiIo) uiFileConvGroup : public uiDlgGroup
{ mODTextTranslationClass(uiFileConvGroup)

public:
				uiFileConvGroup(uiParent*,const SurveyInfo&);
				~uiFileConvGroup();

protected:
    Table::FormatDesc*		fd_;
    uiGenInput*			inptypfld_;
    uiCheckBox*			towgs84fld_;
    //uiGenInput*		outmodefld_;
    uiGenInput*			insertpos_;
    uiGenInput*			lltypfld_;
    uiListBox*			outtypfld_;
    uiTableImpDataSel*		dataselfld_;
    uiFileSel*			inpfilefld_;
    uiFileSel*			outfilefld_;
    uiPushButton*		convertbut_;
    Coords::uiCoordSystemSel*	inpcrdsysselfld_;
    Coords::uiCoordSystemSel*	outcrdsysselfld_;

    const SurveyInfo&		survinfo_;
    od_ostream*			ostream_;

    bool			convtoxy_;
    bool			convtoll_;
    bool			convtoic_;

    void			llFormatTypChg(CallBacker*);
    //void			outModeChg( CallBacker* );
    void			outTypChg(CallBacker*);
    void			convButPushCB(CallBacker*);
    void			inpFileSpecChg(CallBacker*);

    TypeSet<int>		outdisptypidxs_;
};

#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiiomod.h"

#include "enums.h"

#include "uidialog.h"
#include "uidlggroup.h"
#include "tableascio.h"

class SurveyInfo;
class uiCheckBox;
class uiComboBox;
class uiFileInput;
class uiGenInput;
class uiFileConvGroup;
class uiLatLongInp;
class uiListBox;
class uiManualConvGroup;
class uiPushButton;
class uiTableImpDataSel;
class uiTabStack;

namespace Table { class Desc; }
namespace Coords
{
    class CoordSystem;
    class uiCoordSystemSel;
    class uiLatLongSystemSel;
}

mExpClass(uiIo) uiConvertPos : public uiDialog
{ mODTextTranslationClass(uiConvertPos);

public:
				uiConvertPos(uiParent*, const SurveyInfo&,
							    bool modal = true);
				~uiConvertPos();
    enum DataType		{ XY, IC, LL };
				mDeclareEnumUtils(DataType);
    static const uiString	sLLStr() { return tr("Latitude/Longitude"); }
    static const uiString	sICStr() { return tr("Inline/Crossline"); }
    static const uiString	sXYStr() { return tr("X/Y coordinate"); }

private:

    uiManualConvGroup*		mangrp_;
    uiFileConvGroup*		filegrp_;
    uiTabStack*			tabstack_	= nullptr;

};


mExpClass(uiIo) uiConvPosAscIO : public Table::AscIO
{ mODTextTranslationClass(uiConvPosAscIO)
public:
				uiConvPosAscIO(const Table::FormatDesc&,
						od_istream&);

    static Table::FormatDesc*	getDesc(const SurveyInfo&);
    bool			getData(Coord&,
					ConstRefMan<Coords::CoordSystem>);
    float			udfval_;
    od_istream&			strm_;
    bool			finishedreadingheader_;
    uiConvertPos::DataType	getConvFromTyp();

protected:
    bool			isXY() const;
    bool			isLL() const;
    bool			isIC() const;
};


mExpClass(uiIo) uiManualConvGroup : public uiDlgGroup
{ mODTextTranslationClass(uiManualConvGroup)

public:
				uiManualConvGroup(uiParent*,const SurveyInfo&);
				~uiManualConvGroup();

protected:
    // Input
    uiGenInput*			inptypfld_;
    Coords::uiCoordSystemSel*	inpcrdsysselfld_	= nullptr;
    Coords::uiLatLongSystemSel* inpllsysselfld_		= nullptr;
    uiGenInput*			xyinfld_;
    uiGenInput*			inlcrlinfld_;
    uiLatLongInp*		llinfld_;

    // Output
    Coords::uiCoordSystemSel*	outcrdsysselfld_	= nullptr;
    Coords::uiLatLongSystemSel* outllsysselfld_		= nullptr;
    uiGenInput*			xyoutfld_;
    uiGenInput*			inlcrloutfld_;
    uiLatLongInp*		lloutfld_		= nullptr;
    uiPushButton*		convertbut_;

    const SurveyInfo&		survinfo_;

    EnumDef&			datatypedef_;

    void			inputTypChg(CallBacker*);
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

    uiGenInput*			insertpos_;
    uiGenInput*			lltypfld_;
    uiListBox*			outtypfld_;
    uiTableImpDataSel*		dataselfld_;
    uiFileInput*		inpfilefld_;
    uiFileInput*		outfilefld_;
    uiPushButton*		convertbut_;
    Coords::uiCoordSystemSel*	outcrdsysselfld_	= nullptr;
    Coords::uiLatLongSystemSel* outllsysselfld_		= nullptr;

    Table::FormatDesc*		fd_;
    const SurveyInfo&		survinfo_;
    od_ostream*			ostream_		= nullptr;

    EnumDef&			datatypedef_;

    void			outTypChg(CallBacker*);
    void			convButPushCB(CallBacker*);
    void			inpFileSpecChg(CallBacker*);

    TypeSet<int>		outdisptypidxs_;
};

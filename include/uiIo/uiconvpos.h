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
#include "tableascio.h"

class uiGenInput;
class uiFileSel;
class SurveyInfo;
class uiPushButton;
class uiComboBox;
class uiTableImpDataSel;
namespace Table { class Desc; }
namespace Coords { class uiCoordSystemSel; }


mExpClass(uiIo) uiConvertPos : public uiDialog
{ mODTextTranslationClass(uiConvertPos);

public:
				uiConvertPos(uiParent*, const SurveyInfo&,
							    bool modal=true);
				~uiConvertPos();
    enum DataType		{ LL, IC, XY };
				mDeclareEnumUtils( DataType );

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
    Table::FormatDesc*		fd_;
    uiTableImpDataSel*		dataselfld_;

    TypeSet<int>		outidxs_;
    od_ostream*			ostream_;
    float			firstinp_;
    float			secondinp_;
    BufferString		linebuf_;
    DataType			datatyp_;

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
    void			errMsgNEmpFlds();
    DataType			getConversionType();
};


mExpClass( uiIo ) uiConvPosAscIO : public Table::AscIO
{
public:
				uiConvPosAscIO( const Table::FormatDesc& fd,
							od_istream& strm )
				    : Table::AscIO( fd )
				    , finishedreadingheader_( false )
				    , strm_( strm ) {}
    static Table::FormatDesc*	    getDesc();
    bool			    getData( Coord& );
    float			    udfval_;
    od_istream&			    strm_;
    bool			    finishedreadingheader_;
    uiConvertPos::DataType	    getConvFromTyp();

protected:
    bool			    isXY() const;
    bool			    isLL() const;
    bool			    isIC() const;

};

#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          July 2015
 RCS:           $Id:$
________________________________________________________________________

-*/

#include "uisegycommon.h"
#include "uisegyimptype.h"

class RowCol;
class uiTable;
class uiSpinBox;
class uiLineEdit;
class uiComboBox;
class uiFileInput;
class uiSEGYByteNr;
class uiSEGYImpType;
class uiHistogramDisplay;
namespace SEGY
{
    class ImpType; class ScanInfoSet; class LoadDef; class HdrEntryKeyData;
}


/*!\brief Displays and edits info for the read start process. */

mExpClass(uiSEGYTools) uiSEGYReadStartInfo : public uiGroup
{ mODTextTranslationClass(uiSEGYReadStartInfo);
public:

			uiSEGYReadStartInfo(uiParent*,SEGY::LoadDef&,
					const SEGY::ImpType* fixedimptyp=0);
			~uiSEGYReadStartInfo();

    void		setImpTypIdx(int,bool updnow=true);
    void		setScanInfo(const SEGY::ScanInfoSet&);
    void		showNrSamplesSetting(bool);
    void		showZSamplingSetting(bool);
    void		setRev1Values();

    void		setLoadDefCache(const SEGY::LoadDef&);
    void		useLoadDef(); //!< when you have changed the loaddef
    void		fillLoadDef();

    Notifier<uiSEGYReadStartInfo> loaddefChanged;
    Notifier<uiSEGYReadStartInfo> revChanged; //!< implies loaddefChanged

    void		clearInfo();

protected:

    uiTable*		tbl_;
    uiComboBox*		revfld_;
    uiComboBox*		fmtfld_;
    uiSpinBox*		nsfld_;
    uiLineEdit*		zstartfld_;
    uiLineEdit*		srfld_;
    uiSEGYByteNr*	xcoordbytefld_;
    uiSEGYByteNr*	ycoordbytefld_;
    uiSEGYByteNr*	inlbytefld_;
    uiSEGYByteNr*	crlbytefld_;
    uiSEGYByteNr*	trcnrbytefld_;
    uiSEGYByteNr*	refnrbytefld_;
    uiSEGYByteNr*	offsetbytefld_;
    uiSEGYByteNr*	azimuthbytefld_;

    uiComboBox*		nrsampsrcfld_;
    uiComboBox*		zsampsrcfld_;
    uiComboBox*		trcnrsrcfld_;
    uiGroup*		trcnrgengrp_;
    uiLineEdit*		trcnrgenstartfld_;
    uiLineEdit*		trcnrgenstepfld_;
    uiComboBox*		psoffsrcfld_;
    uiGroup*		offsgengrp_;
    uiLineEdit*		offsgenstartfld_;
    uiLineEdit*		offsgenstepfld_;

    SEGY::LoadDef&	loaddef_;
    SEGY::ImpType	imptype_;
    const bool		inptypfixed_;
    int			nrrows_;
    bool		parsbeingset_;
    short		nrunswappedfmts_;
    const uiString	sBytePos;
    uiString		xinfotxt_;
    uiString		yinfotxt_;
    uiString		inlinfotxt_;
    uiString		crlinfotxt_;
    uiString		trcnrinfotxt_;
    uiString		refnrinfotxt_;
    uiString		offsetinfotxt_;
    uiString		azimuthinfotxt_;

    void		mkBasicInfoFlds();
    void		manNonBasicRows();
    void		manCoordDefFlds();
    void		man2DDefFlds();
    void		man3DDefFlds();
    void		manPSDefFlds();
    void		remove2DDefFlds();
    void		remove3DDefFlds();
    void		setScanInfoTexts(const SEGY::ScanInfoSet&);
    void		updateCellTexts();
    void		setByteFldContents(const SEGY::HdrEntryKeyData&);
    bool		hasRev1Value(const uiSEGYByteNr*) const;
    void		parChanged(bool);

    void		clearTable();
    void		removeFromTable(uiObject*);
    void		removeFromTable(uiGroup*);

    bool		isVSP() const		{ return imptype_.isVSP(); }
    void		setCellTxt(int col,int row,const uiString&);

    void		revChg(CallBacker*);
    void		parChg(CallBacker*);

};

#ifndef uisegyreadstartinfo_h
#define uisegyreadstartinfo_h
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

class DataClipSampler;
namespace SEGY { class ImpType; class ScanInfo; class LoadDef; }
class uiTable;
class uiSpinBox;
class uiLineEdit;
class uiComboBox;
class uiFileInput;
class uiSEGYByteNr;
class uiSEGYImpType;
class uiHistogramDisplay;


/*!\brief Displays and edits info for the read start process. */

mExpClass(uiSEGY) uiSEGYReadStartInfo : public uiGroup
{ mODTextTranslationClass(uiSEGYReadStartInfo);
public:

			uiSEGYReadStartInfo(uiParent*,SEGY::LoadDef&);

    void		setImpTypIdx(int);
    void		setScanInfo(const SEGY::ScanInfo&);

    void		useLoadDef(); //!< when you have changed the loaddef
    void		fillLoadDef();

    Notifier<uiSEGYReadStartInfo> loaddefChanged; //!< when I have changed it

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
    uiSEGYByteNr*	key1bytefld_;
    uiSEGYByteNr*	key2bytefld_;
    uiSEGYByteNr*	offsetbytefld_;

    SEGY::LoadDef&	loaddef_;
    SEGY::ImpType	imptype_;
    bool		parsbeingset_;
    BufferString	xinfotxt_;
    BufferString	yinfotxt_;
    BufferString	inlinfotxt_;
    BufferString	crlinfotxt_;
    BufferString	trcnrinfotxt_;
    BufferString	refnrinfotxt_;
    BufferString	offsetinfotxt_;

    void		revChg(CallBacker*);
    void		parChg(CallBacker*);

    void		mkCommonLoadDefFields();
    void		manSpecificLoadDefFields();
    void		showRelevantInfo();
    void		setCellTxt(int col,int row,const char*);

};


#endif

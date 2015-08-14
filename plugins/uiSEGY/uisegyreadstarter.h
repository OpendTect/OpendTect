#ifndef uisegyreadstarter_h
#define uisegyreadstarter_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          July 2015
 RCS:           $Id: $
________________________________________________________________________

-*/

#include "uisegycommon.h"
#include "uidialog.h"
#include "segyfiledef.h"
#include "segyuiscandata.h"

class Timer;
class DataClipSampler;
class uiButton;
class uiSpinBox;
class uiFileInput;
class uiHistogramDisplay;
class uiSEGYImpType;
class uiSEGYReadStartInfo;
namespace SEGY { class ImpType; }


/*!\brief Starts reading process of 'any SEG-Y file'. */

mExpClass(uiSEGY) uiSEGYReadStarter : public uiDialog
{ mODTextTranslationClass(uiSEGYReadStarter);
public:

    typedef SEGY::FileSpec FileSpec;
    typedef SEGY::FilePars FilePars;
    typedef SEGY::FileReadOpts FileReadOpts;

			uiSEGYReadStarter(uiParent*,const FileSpec* fs=0);
			~uiSEGYReadStarter();

    bool		isMulti() const		{ return filespec_.isMulti(); }
    const char*		fileName( int nr=0 ) const
			{ return filespec_.fileName(nr); }
    FullSpec		fullSpec() const;

    const SEGY::ImpType& impType() const;
    void		setImpTypIdx(int);

protected:

    FileSpec		filespec_;
    FilePars		filepars_;
    FileReadOpts*	filereadopts_;

    uiSEGYImpType*	typfld_;
    uiFileInput*	inpfld_;
    uiSEGYReadStartInfo* infofld_;
    uiHistogramDisplay*	ampldisp_;
    uiButton*		examinebut_;
    uiSpinBox*		examinenrtrcsfld_;
    uiSpinBox*		clipfld_;
    Timer*		filenamepopuptimer_;

    BufferString	userfilename_;
    SEGY::LoadDef	loaddef_;
    ObjectSet<SEGY::ScanInfo> scaninfo_;
    DataClipSampler&	clipsampler_;
    bool		infeet_;
    bool		veryfirstscan_;

    bool		getExistingFileName(BufferString& fnm,bool werr=true);
    bool		getFileSpec();
    void		execNewScan(bool);
    void		scanInput();
    bool		scanFile(const char*,bool);
    bool		obtainScanInfo(SEGY::ScanInfo&,od_istream&,bool);
    bool		completeLoadDef(od_istream&);

    void		clearDisplay();
    void		setExamineStatus();
    void		displayScanResults();

    void		initWin(CallBacker*);
    void		typChg(CallBacker*);
    void		inpChg(CallBacker*);
    void		defChg( CallBacker* )		{ execNewScan(true); }
    void		examineCB(CallBacker*);
    void		updateAmplDisplay(CallBacker*);
    bool		acceptOK(CallBacker*);

    bool		commit();

};


#endif

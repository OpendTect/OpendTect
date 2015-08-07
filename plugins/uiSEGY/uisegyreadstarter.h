#ifndef uisegyreadstarter_h
#define uisegyreadstarter_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          July 2015
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uisegycommon.h"
#include "uidialog.h"
#include "segyfiledef.h"
#include "segyuiscandata.h"

class DataClipSampler;
class uiButton;
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

    const SEGY::ImpType& impType() const;
    bool		isMulti() const		{ return filespec_.isMulti(); }
    const char*		fileName( int nr=0 ) const
			{ return filespec_.fileName(nr); }

    FileSpec&		fileSpec()		{ return filespec_; }
    FilePars&		filePars()		{ return filepars_; }
    FileReadOpts&	fileReadOpts()		{ return *filereadopts_; }

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

    BufferString	userfilename_;
    SEGY::uiScanDef	scandef_;
    ObjectSet<SEGY::uiScanData> scandata_;
    DataClipSampler&	clipsampler_;
    bool		infeet_;
    bool		veryfirstscan_;

    bool		getExistingFileName(BufferString& fnm,bool werr=true);
    bool		getFileSpec();
    void		execNewScan(bool);
    void		scanInput();
    bool		scanFile(const char*,bool);
    bool		obtainScanData(SEGY::uiScanData&,od_istream&,bool);
    bool		completeScanDef(od_istream&);

    void		clearDisplay();
    void		setExamineStatus();
    void		displayScanResults();

    void		initWin(CallBacker*);
    void		typChg(CallBacker*);
    void		inpChg(CallBacker*);
    void		defChg( CallBacker* )		{ execNewScan(true); }
    void		examineCB(CallBacker*);
    bool		acceptOK(CallBacker*);

    bool		commit();

};


#endif

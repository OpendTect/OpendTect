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

class uiComboBox;
class uiFileInput;
class uiTable;


/*!\brief Starts reading process of 'any SEG-Y file'. */

mExpClass(uiSEGY) uiSEGYReadStarter : public uiDialog
{ mODTextTranslationClass(uiSEGYReadStarter);
public:

    typedef SEGY::FileSpec FileSpec;
    typedef SEGY::FilePars FilePars;
    typedef SEGY::FileReadOpts FileReadOpts;

			uiSEGYReadStarter(uiParent*,const FileSpec* fs=0);
			~uiSEGYReadStarter();

    Seis::GeomType	geomType() const	{ return geomtype_; }
    bool		isVSP() const		{ return isvsp_; }
    bool		isMulti() const		{ return filespec_.isMulti(); }
    const char*		fileName( int nr=0 ) const
			{ return filespec_.fileName(nr); }

    FileSpec&		fileSpec()		{ return filespec_; }
    FilePars&		filePars()		{ return filepars_; }
    FileReadOpts&	fileReadOpts()		{ return *filereadopts_; }

protected:

    Seis::GeomType	geomtype_;
    bool		isvsp_;
    FileSpec		filespec_;
    FilePars		filepars_;
    FileReadOpts*	filereadopts_;

    uiComboBox*		typfld_;
    uiFileInput*	inpfld_;
    uiTable*		infotbl_;

    BufferString	curusrfname_;
    TypeSet<int>	inptyps_; // Seis::GeomType, or -1 for VSP
    SEGY::uiScanDef	scandef_;
    ObjectSet<SEGY::uiScanData> scandata_;

    void		addTyp(int);
    void		setCellTxt(int col,int row,const char*);
    bool		getExistingFileName(BufferString& fnm,bool werr=true);
    bool		getFileSpec();
    void		scanInput();
    bool		scanFile(const char*);
    bool		obtainScanData(SEGY::uiScanData&,od_istream&,bool);
    bool		guessScanDef(od_istream&);
    static bool		getHeaderBufData(od_istream&,char*);
    void		displayScanResults();

    void		initWin(CallBacker*);
    void		inpChg(CallBacker*);
    bool		acceptOK(CallBacker*);

};


#endif

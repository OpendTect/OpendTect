#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uisegycommon.h"
#include "uidialog.h"
#include "segyfiledef.h"
#include "segyuiscandata.h"
#include "uisegyimptype.h"
#include "uicoordsystem.h"

class Timer;
class TaskRunner;
class SurveyInfo;
class DataClipSampler;
class TrcKeyZSampling;
class uiLabel;
class uiButton;
class uiSpinBox;
class uiLineEdit;
class uiCheckBox;
class uiFileInput;
class uiPushButton;
class uiSurveyMap;
class uiRadioButton;
class uiHistogramDisplay;
class uiSEGYRead;
class uiSEGYImpType;
class uiSEGYReadStartInfo;
namespace SEGY { class ScanInfoCollectors; }


/*!\brief Starts reading process of 'any SEG-Y file'. */

mExpClass(uiSEGYTools) uiSEGYReadStarter : public uiDialog
{ mODTextTranslationClass(uiSEGYReadStarter);
public:

			uiSEGYReadStarter(uiParent*,bool forsurvsetup,
					  const SEGY::ImpType* fixedtype=0);
			~uiSEGYReadStarter();

    bool		isMulti() const		{ return filespec_.isMulti(); }
    const char*		fileName( int nr=0 ) const
			{ return filespec_.fileName(nr); }

    FullSpec		fullSpec() const;
    const char*		userFileName() const	{ return userfilename_; }

    const SurveyInfo*	survInfo() const
			{ return survinfook_ ? survinfo_ : 0; }
    bool		getInfo4SI(TrcKeyZSampling&,Coord crd[3]) const;
    bool		zInFeet() const
			{ return scaninfos_ && scaninfos_->inFeet(); }

    const SEGY::ImpType& impType() const;
    void		setImpTypIdx(int);

    MultiID		getOutputKey() const	{ return outputid_; }

    void		usePar(const IOPar&);
    void		fillPar(IOPar&) const;

protected:

    bool		forsurvsetup_;
    SEGY::FileSpec	filespec_;
    FilePars		filepars_;
    FileReadOpts*	filereadopts_		= nullptr;
    BufferStringSet	linenames_;
    int			wcidx_			= -1;

    uiGroup*		topgrp_;
    uiGroup*		midgrp_;
    uiGroup*		botgrp_;
    uiSEGYImpType*	typfld_			= nullptr;
    uiFileInput*	inpfld_;
    uiPushButton*	multilinebut_		= nullptr;
    uiSEGYReadStartInfo* infofld_;
    uiHistogramDisplay* ampldisp_		= nullptr;
    uiSurveyMap*	survmap_		= nullptr;
    uiButton*		examinebut_;
    uiButton*		fullscanbut_;
    uiButton*		editbut_;
    uiButton*		hdrentrysettsbut_;
    uiRadioButton*	useicbut_		= nullptr;
    uiRadioButton*	usexybut_		= nullptr;
    uiLineEdit*		coordscalefld_		= nullptr;
    uiSpinBox*		examinenrtrcsfld_;
    uiSpinBox*		clipfld_;
    uiCheckBox*		inc0sbox_;
    uiLabel*		nrfileslbl_;
    Timer*		timer_			= nullptr;
    Coords::uiCoordSystemSel* coordsysselfld_;

    BufferString	userfilename_;
    BufferString	lastparname_;
    SEGY::LoadDef	loaddef_;
    bool		detectrev0flds_		= true;
    SEGY::ScanInfoSet*	scaninfos_		= nullptr;
    DataClipSampler&	clipsampler_;
    SEGY::ImpType	fixedimptype_;
    SurveyInfo*		survinfo_		= nullptr;
    bool		survinfook_		= false;
    bool		lastscanwasfull_	= false;
    bool		nrsampmsgshown_		= false;
    uiSEGYRead*		classicrdr_		= nullptr;
    MultiID		outputid_;

    enum LoadDefChgType	{ KeepAll, KeepBasic, KeepNone };

    bool		imptypeFixed() const	{ return !typfld_; }
    bool		getExistingFileName(BufferString& fnm,bool werr=true);
    bool		getFileSpec();
    void		execNewScan(LoadDefChgType,bool full=false);
    void		scanInput();
    bool		scanFile(const char*,LoadDefChgType,TaskRunner*);
    bool		completeFileInfo(od_istream&,SEGY::BasicFileInfo&,bool);
    void		completeLoadDef();
    void		handleNewInputSpec(LoadDefChgType ct=KeepAll,
					   bool full=false);
    void		runClassic(bool);
    void		forceRescan(LoadDefChgType ct=KeepAll,bool full=false);
    bool		needICvsXY() const;
    int			examineNrTraces() const;
    float		ratioClip() const;
    bool		incZeros() const;

    void		createTools();
    uiGroup*		createAmplDisp();
    void		clearDisplay();
    void		setToolStates();
    void		displayScanResults();
    void		updateSurvMap();
    void		updateICvsXYButtons();
    void		updateCoordScale();
    bool		reviewAndEditLineNames();

    void		initWin(CallBacker*);
    void		firstSel(CallBacker*);
    void		typChg(CallBacker*);
    void		inpChg(CallBacker*);
    void		editFile(CallBacker*);
    void		fullScanReq(CallBacker*);
    void		editHdrEntrySettings(CallBacker*);
    void		runClassicImp(CallBacker*)	{ runClassic( true ); }
    void		runClassicLink(CallBacker*)	{ runClassic( false ); }
    void		defChg( CallBacker* )		{ forceRescan(); }
    void		revChg(CallBacker*);
    void		examineCB(CallBacker*);
    void		readParsCB(CallBacker*);
    void		writeParsCB(CallBacker*);
    void		icxyCB(CallBacker*);
    void		coordscaleChg(CallBacker*);
    void		updateAmplDisplay(CallBacker*);
    void		initClassic(CallBacker*);
    bool		acceptOK(CallBacker*);
    void		coordSysChangedCB(CallBacker*);
    void		multiLineSelCB(CallBacker*);

    bool		commit(bool permissive=false);

};

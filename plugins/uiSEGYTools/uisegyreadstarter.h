#pragma once
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
class uiComboBox;
class uiFileSel;
class uiSurveyMap;
class uiRadioButton;
class uiHistogramDisplay;
class uiSEGYRead;
class uiSEGYImpType;
class uiSEGYReadStartInfo;
class uiSEGYClassicSurvInfoProvider;
namespace SEGY { class ScanInfoCollectors; class ImpType; }
namespace SEGY { namespace Vintage { class Info; }}


/*!\brief Starts reading process of 'any SEG-Y file'. */

mExpClass(uiSEGYTools) uiSEGYReadStarter : public uiDialog
{ mODTextTranslationClass(uiSEGYReadStarter);
public:

    mExpClass(uiSEGYTools) Setup
    {
    public:
		Setup(bool for_survsetup, const SEGY::ImpType* type=0)
		    : forsurvsetup_(for_survsetup)
		    , imptype_(type)
		    , fixedfnm_(false)
		    , vintagecheckmode_(false)
		    , vntinfos_(0)
		{}

		mDefSetupMemb(bool, forsurvsetup);
		mDefSetupMemb(const SEGY::ImpType*, imptype);
		mDefSetupMemb(BufferString, filenm);
		mDefSetupMemb(BufferString, vintagenm);
		mDefSetupMemb(bool, fixedfnm);
		mDefSetupMemb(bool, vintagecheckmode);
		mDefSetupMemb(const ObjectSet<SEGY::Vintage::Info>*,vntinfos);
    };

			uiSEGYReadStarter(uiParent*, const Setup&);
			~uiSEGYReadStarter();

    bool		isMulti() const		{ return filespec_.isMulti(); }
    const char*		fileName( int nr=0 ) const
			{ return filespec_.fileName(nr); }

    FullSpec		fullSpec() const;
    const char*		userFileName() const	{ return userfilename_; }
    void		getDefaultPar( IOPar& par ) const
			{ par = defaultpar_; }
    const char*		getCurrentParName() const
			{ return lastparname_; }

    const SurveyInfo*	survInfo() const
			{ return survinfook_ ? survinfo_ : 0; }
    bool		getInfo4SI(TrcKeyZSampling&,Coord crd[3]) const;
    void		getSIPInfo(bool&,int&,bool&,IOPar&,BufferString&) const;
    bool		fileIsInTime() const;
    bool		zInFeet() const
			{ return scaninfos_ && scaninfos_->inFeet(); }

    const SEGY::ImpType& impType() const;
    void		setImpTypIdx(int);
    void		setZIsTime(bool);

    void		usePar(const IOPar&);
    void		fillPar(IOPar&) const;

protected:

    SEGY::FileSpec	filespec_;
    FilePars		filepars_;
    FileReadOpts*	filereadopts_;

    uiGroup*		topgrp_;
    uiGroup*		midgrp_;
    uiGroup*		botgrp_;
    uiSEGYImpType*	typfld_;
    uiFileSel*		inpfld_;
    uiSEGYReadStartInfo* infofld_;
    uiHistogramDisplay*	ampldisp_;
    uiSurveyMap*	survmap_;
    uiButton*		examinebut_;
    uiButton*		fullscanbut_;
    uiButton*		editbut_;
    uiButton*		hdrentrysettsbut_;
    uiRadioButton*	useicbut_;
    uiRadioButton*	usexybut_;
    uiLineEdit*		coordscalefld_;
    uiSpinBox*		examinenrtrcsfld_;
    uiSpinBox*		clipfld_;
    uiCheckBox*		zdombox_;
    uiCheckBox*		inc0sbox_;
    uiCheckBox*		keepzsampbox_;
    Timer*		timer_;
    uiComboBox*		vintagefld_;
    IOPar		defaultpar_;
    Coords::uiCoordSystemSel* coordsysselfld_;

    BufferString	userfilename_;
    BufferString	lastparname_;
    SEGY::LoadDef	loaddef_;
    bool		detectrev0flds_;
    SEGY::ScanInfoSet*	scaninfos_;
    DataClipSampler&	clipsampler_;
    SEGY::ImpType	fixedimptype_;
    SurveyInfo*		survinfo_;
    bool		survinfook_;
    bool		lastscanwasfull_;
    uiSEGYRead*		classicrdr_;
    uiSEGYClassicSurvInfoProvider* classicsip_;
    const Setup		setup_;

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

    void		initWin(CallBacker*);
    void		firstSel(CallBacker*);
    void		typChg(CallBacker*);
    void		zDomChgCB(CallBacker*);
    void		inpChg(CallBacker*);
    void		editFile(CallBacker*);
    void		fullScanReq(CallBacker*);
    void		editHdrEntrySettings(CallBacker*);
    void		runClassicImp(CallBacker*)	{ runClassic( true ); }
    void		runClassicLink(CallBacker*)	{ runClassic( false ); }
    void		defChg( CallBacker* )	{ forceRescan(); }
    void		revChg(CallBacker*);
    void		examineCB(CallBacker*);
    void		readParsCB(CallBacker*);
    void		writeParsCB(CallBacker*);
    bool		writePars();
    void		keepZChg(CallBacker*);
    void		icxyCB(CallBacker*);
    void		coordscaleChg(CallBacker*);
    void		updateAmplDisplay(CallBacker*);
    void		initClassic(CallBacker*);
    void		classicSurvSetupEnd(CallBacker*);
    bool		acceptOK();
    void		vntRefreshCB(CallBacker*);
    void		coordSysChangedCB(CallBacker*);
    bool		getVintageParameters();

    bool		commit(bool permissive=false);

    bool		isExampleVntSelected(const BufferString& inpnm);
};

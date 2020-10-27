#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Aug 2015
 RCS:           $Id: $
________________________________________________________________________

-*/

#include "uisegycommon.h"
#include "uidialog.h"
#include "dbkey.h"

class SeisImporter;
class SeisStdImporterReader;
class uiIOObjSel;
class uiSeisSel;
class uiComboBox;
class uiCheckBox;
class uiGenInput;
class uiFileSel;
class uiTable;
class uiBatchJobDispatcherSel;
class uiSeisTransfer;
class uiSEGYImportResult;
class uiSeis2DLineNameSel;
namespace Seis { class Storer; }
namespace SEGY { class FileIndexer; }
namespace SEGY { namespace Vintage {class Info; class Importer; }}


/*!\brief Finishes reading process of 'any SEG-Y file'. */

mExpClass(uiSEGYTools) uiSEGYReadFinisher : public uiDialog
{ mODTextTranslationClass(uiSEGYReadFinisher);
public:

			uiSEGYReadFinisher(uiParent*,const FullSpec&,
					   const char* usrspec,bool istime,
					   bool singlevintage=true,
			    const ObjectSet<SEGY::Vintage::Info>* vintinfo=0);
			//TODOSegyImpl Implement Setup
			~uiSEGYReadFinisher();

    FullSpec&		fullSpec()		{ return fs_; }

    void		setAsDefaultObj(); //!< call only after successful go()
    uiSEGYImportResult* getImportResult(int);
    const BufferString& getCurrentProcessingVntnm()
			{ return processingvntnm_; }
    const SEGY::Vintage::Info*	    getVintageInfo(const BufferString& Vntnm);
    Notifier<uiSEGYReadFinisher> updateStatus;

protected:

    FullSpec		fs_;
    BufferString	objname_;
    DBKey		outputid_;

    uiIOObjSel*		outwllfld_;
    uiComboBox*		lognmfld_;
    uiGenInput*		inpdomfld_;
    uiCheckBox*		isfeetfld_;

    uiSeisSel*		outimpfld_;
    uiSeisSel*		outscanfld_;
    uiSeisTransfer*	transffld_;
    uiGenInput*		docopyfld_;
    uiSeis2DLineNameSel* lnmfld_;
    uiComboBox*		coordsfromfld_;
    uiGenInput*		coordfileextfld_;
    uiGenInput*		coordsstartfld_;
    uiGenInput*		coordsstepfld_;
    uiFileSel*		coordfilefld_;
    uiBatchJobDispatcherSel* batchfld_;
    uiString		errmsg_;
    ObjectSet<uiSEGYImportResult> reports_;

    bool		singlevintage_;
    bool		trcsskipped_;
    BufferString	processingvntnm_;
    const ObjectSet<SEGY::Vintage::Info>* vntinfos_;

    void		crVSPFields(bool);
    void		crSeisFields(bool);
    void		cr2DCoordSrcFields(uiGroup*&,bool);

    bool		doVSP();
    bool		do3D(const IOObj&,const IOObj&,bool);
    bool		do2D(const IOObj&,const IOObj&,bool,const char*);
    bool		doBatch(bool doimp);
    bool		doBatch2D(bool doimp,const char* lnm);
    bool		getGeomID(const char* lnm,bool isnw,Pos::GeomID&) const;
    bool		doMultiVintage(const char* attr2dnm=0);
    void		updateResultDlg(const SEGY::Vintage::Importer&,
					uiSEGYImportResult*);
    void		updateInIOObjPars(IOObj&,const IOObj& outioobj,
					  const char* lnm=nullptr);
    SeisStdImporterReader* getImpReader(const IOObj&,Seis::Storer&,
					Pos::GeomID);
    bool		exec2Dimp(const IOObj&,const IOObj&,bool,const char*,
				  const char*,Pos::GeomID);
    bool		handleExistingGeometry(const char*,bool,bool&,bool&,
					       bool&);
    bool		handleWarnings(bool,SEGY::FileIndexer*,SeisImporter*);
    BufferString	getWildcardSubstLineName(int) const;
    bool		putCoordChoiceInSpec();

    void		initWin(CallBacker*);
    void		wllSel(CallBacker*);
    void		inpDomChg(CallBacker*);
    void		coordsFromChg(CallBacker*);
    void		doScanChg(CallBacker*);
    bool		acceptOK();

    static uiString	getWinTile(const FullSpec&,bool issingle);
    static uiString	getDlgTitle(const char*);

    uiSeisSel*		outFld( bool imp )
			{ return imp ? outimpfld_ : outscanfld_; }
};


mExpClass(uiSEGYTools) uiSEGYImportResult : public uiDialog
{mODTextTranslationClass(uiSEGYImportResult)
public:
			uiSEGYImportResult(uiParent*);
    uiTable*		table_;
    BufferString	vintagenm_;
    uiString		status_;
};

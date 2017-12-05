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

class uiIOObjSel;
class uiSeisSel;
class uiComboBox;
class uiCheckBox;
class uiGenInput;
class uiFileSel;
class uiBatchJobDispatcherSel;
class uiSeisTransfer;
class SeisImporter;
class SeisTrcWriter;
class SeisStdImporterReader;
class uiSEGYVintageInfo;
namespace SEGY { class FileIndexer; }


/*!\brief Finishes reading process of 'any SEG-Y file'. */

mExpClass(uiSEGY) uiSEGYReadFinisher : public uiDialog
{ mODTextTranslationClass(uiSEGYReadFinisher);
public:

			uiSEGYReadFinisher(uiParent*,const FullSpec&,
					   const char* usrspec,bool istime,
					   bool singlevintage=true,
				const ObjectSet<uiSEGYVintageInfo>* vintinfo=0);
			//TODO Implement Setup
			~uiSEGYReadFinisher();

    FullSpec&		fullSpec()		{ return fs_; }

    void		setAsDefaultObj(); //!< call only after successful go()

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
    uiComboBox*		coordsfromfld_;
    uiGenInput*		coordfileextfld_;
    uiGenInput*		coordsstartfld_;
    uiGenInput*		coordsstepfld_;
    uiFileSel*		coordfilefld_;
    uiBatchJobDispatcherSel* batchfld_;

    bool		singlevintage_;
    const ObjectSet<uiSEGYVintageInfo>* vntinfos_;

    void		crVSPFields(bool);
    void		crSeisFields(bool);
    void		cr2DCoordSrcFields(uiGroup*&,bool);

    bool		doVSP();
    bool		do3D(const IOObj&,const IOObj&,bool);
    bool		do2D(const IOObj&,const IOObj&,bool,const char*);
    bool		doBatch(bool);
    bool		doMultiVintage();

    void		updateInIOObjPars(IOObj&,const IOObj& outioobj);
    SeisStdImporterReader* getImpReader(const IOObj&,SeisTrcWriter&,
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

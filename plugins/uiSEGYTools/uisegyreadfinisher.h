#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uisegycommon.h"

#include "uidialog.h"
#include "multiid.h"

class uiBatchJobDispatcherSel;
class uiCheckBox;
class uiComboBox;
class uiGenInput;
class uiFileInput;
class uiIOObjSel;
class uiProgressBar;
class uiSeisSel;
class uiSeisTransfer;
class uiSeis2DLineNameSel;
class SeisImporter;
class SeisStdImporterReader;
class SeisTrcWriter;
namespace Coords { class CoordSystem; }
namespace SEGY { class FileIndexer; }


/*!\brief Finishes reading process of 'any SEG-Y file'. */

mExpClass(uiSEGYTools) uiSEGYReadFinisher : public uiDialog
{ mODTextTranslationClass(uiSEGYReadFinisher);
public:

			uiSEGYReadFinisher(uiParent*,const FullSpec&,
					   const char* usrspec,
					   const ZDomain::Info* =nullptr);
			~uiSEGYReadFinisher();

    FullSpec&		fullSpec()		{ return fs_; }

    void		setAsDefaultObj(); //!< call only after successful go()
    void		setCoordSystem(Coords::CoordSystem*);

    MultiID		getOutputKey() const	{ return outputid_;}

private:

    FullSpec		fs_;
    BufferString	objname_;
    MultiID		outputid_;
    Coords::CoordSystem* coordsys_		= nullptr;
    ZDomain::Info*	zdomain_;

    uiIOObjSel*		outwllfld_		= nullptr;
    uiComboBox*		lognmfld_		= nullptr;
    uiGenInput*		inpdomfld_		= nullptr;
    uiCheckBox*		isfeetfld_		= nullptr;

    uiSeisSel*		outimpfld_		= nullptr;
    uiSeisSel*		outscanfld_		= nullptr;
    uiSeisTransfer*	transffld_		= nullptr;
    uiGenInput*		remnullfld_		= nullptr;
    uiSeis2DLineNameSel* lnmfld_		= nullptr;
    uiGenInput*		docopyfld_		= nullptr;
    uiComboBox*		coordsfromfld_		= nullptr;
    uiGenInput*		coordfileextfld_	= nullptr;
    uiGenInput*		coordsstartfld_		= nullptr;
    uiGenInput*		coordsstepfld_		= nullptr;
    uiFileInput*	coordfilefld_		= nullptr;
    uiBatchJobDispatcherSel* batchfld_		= nullptr;
    uiProgressBar*	progressfld_		= nullptr;

    void		crVSPFields();
    void		crSeisFields();
    void		cr2DCoordSrcFields(uiGroup*&,bool);

    bool		doVSP();
    bool		do3D(const IOObj&,const IOObj&,bool doimp);
    bool		do2D(const IOObj&,const IOObj&,bool doimp,const char*);
    bool		doBatch(bool doimp);
    bool		doBatch2D(bool doimp,const char* lnm);
    bool		getGeomID(const char* lnm,bool isnew,
				  Pos::GeomID&) const;

    void		updateInIOObjPars(IOObj&,const IOObj& outioobj);
    SeisStdImporterReader* getImpReader(const IOObj&,SeisTrcWriter&,
					Pos::GeomID);
    bool		exec2Dimp(const IOObj&,const IOObj&,bool,const char*,
				  const char*,Pos::GeomID);
    bool		handleExistingGeometry(const char*,bool,bool&,bool&,
					       bool&);
    bool		handleWarnings(bool,SEGY::FileIndexer*,SeisImporter*);
    bool		putCoordChoiceInSpec();

    void		batchChgCB(CallBacker*);
    void		initDlgCB(CallBacker*);
    void		wllSel(CallBacker*);
    void		inpDomChg(CallBacker*);
    void		coordsFromChg(CallBacker*);
    void		doScanChg(CallBacker*);
    bool		acceptOK(CallBacker*) override;

    static uiString	getWinTile(const FullSpec&);
    static uiString	getDlgTitle(const char*);

    uiSeisSel*		outFld( bool imp )
			{ return imp ? outimpfld_ : outscanfld_; }

};

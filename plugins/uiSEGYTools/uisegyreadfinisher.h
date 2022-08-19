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
					   const char* usrspec);
			~uiSEGYReadFinisher();

    FullSpec&		fullSpec()		{ return fs_; }

    void		setAsDefaultObj(); //!< call only after successful go()
    void		setCoordSystem(Coords::CoordSystem*);

    MultiID		getOutputKey() const	{ return outputid_;}

protected:

    FullSpec		fs_;
    BufferString	objname_;
    MultiID		outputid_;
    Coords::CoordSystem* coordsys_ 	= nullptr;

    uiIOObjSel*		outwllfld_;
    uiComboBox*		lognmfld_;
    uiGenInput*		inpdomfld_;
    uiCheckBox*		isfeetfld_;

    uiSeisSel*		outimpfld_;
    uiSeisSel*		outscanfld_;
    uiSeisTransfer*	transffld_;
    uiGenInput*		remnullfld_;
    uiSeis2DLineNameSel* lnmfld_;
    uiGenInput*		docopyfld_;
    uiComboBox*		coordsfromfld_;
    uiGenInput*		coordfileextfld_;
    uiGenInput*		coordsstartfld_;
    uiGenInput*		coordsstepfld_;
    uiFileInput*	coordfilefld_;
    uiBatchJobDispatcherSel* batchfld_;

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

    void		initWin(CallBacker*);
    void		wllSel(CallBacker*);
    void		inpDomChg(CallBacker*);
    void		coordsFromChg(CallBacker*);
    void		doScanChg(CallBacker*);
    bool		acceptOK(CallBacker*);

    static uiString	getWinTile(const FullSpec&);
    static uiString	getDlgTitle(const char*);

    uiSeisSel*		outFld( bool imp )
			{ return imp ? outimpfld_ : outscanfld_; }

};

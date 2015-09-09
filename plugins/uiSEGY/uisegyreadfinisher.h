#ifndef uisegyreadfinisher_h
#define uisegyreadfinisher_h
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

class uiIOObjSel;
class uiSeisSel;
class uiComboBox;
class uiCheckBox;
class uiGenInput;
class uiBatchJobDispatcherSel;
class uiSeisTransfer;
class SeisImporter;
class SeisTrcWriter;
class SeisStdImporterReader;
namespace SEGY { class FileIndexer; }


/*!\brief Finishes reading process of 'any SEG-Y file'. */

mExpClass(uiSEGY) uiSEGYReadFinisher : public uiDialog
{ mODTextTranslationClass(uiSEGYReadFinisher);
public:

			uiSEGYReadFinisher(uiParent*,const FullSpec&,
					   const char* usrspec);
			~uiSEGYReadFinisher();

    FullSpec&		fullSpec()		{ return fs_; }

protected:

    FullSpec		fs_;
    BufferString	objname_;

    uiIOObjSel*		outwllfld_;
    uiComboBox*		lognmfld_;
    uiGenInput*		inpdomfld_;
    uiCheckBox*		isfeetfld_;

    uiGenInput*		docopyfld_;
    uiSeisSel*		outimpfld_;
    uiSeisSel*		outscanfld_;
    uiSeisTransfer*	transffld_;
    uiBatchJobDispatcherSel* batchfld_;

    void		crVSPFields();
    void		crSeisFields();
    bool		doVSP();
    bool		do3D(const IOObj&,const IOObj&,bool);
    bool		do2D(const IOObj&,const IOObj&,bool,const char*);
    bool		doBatch(bool);

    SeisStdImporterReader* getImpReader(const IOObj&,SeisTrcWriter&,
					Pos::GeomID);
    bool		exec2Dimp(const IOObj&,const IOObj&,bool,const char*,
				  const char*,Pos::GeomID);
    bool		handleExistingGeometry(const char*,bool,bool&,bool&);
    bool		handleWarnings(bool,SEGY::FileIndexer*,SeisImporter*);
    BufferString	getWildcardSubstLineName(int) const;

    void		initWin(CallBacker*);
    void		wllSel(CallBacker*);
    void		inpDomChg(CallBacker*);
    void		doScanChg(CallBacker*);
    bool		acceptOK(CallBacker*);

    static uiString	getWinTile(const FullSpec&);
    static uiString	getDlgTitle(const char*);

    uiSeisSel*		outFld( bool imp )
			{ return imp ? outimpfld_ : outscanfld_; }

};


#endif

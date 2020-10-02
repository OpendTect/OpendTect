#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nageswara
 Date:		July 2017
 RCS:		$Id: $
________________________________________________________________________

-*/

#include "uisegycommon.h"
#include "uidialog.h"

class uiTable;
class uiToolButton;
class uiSEGYFileSelector;
class uiSEGYImpType;;
class uiSEGYReadStarter;
class uiSEGYReadFinisher;
namespace SEGY{ namespace Vintage {class Info; }}

mExpClass(uiSEGYTools) uiSEGYMultiVintageImporter : public uiDialog
{ mODTextTranslationClass(uiSEGYMultiVintageImporter)
public:
			uiSEGYMultiVintageImporter(uiParent*);
			~uiSEGYMultiVintageImporter();

    int			nrSelFiles() { return selfilenms_.size(); }

protected:
    bool		selectVintage();
    void		addCB(CallBacker*);
    void		removeCB(CallBacker*);
    void		selectFilesCB(CallBacker*);
    void		editVntCB(CallBacker*);
    void		fillRow( int rowid);
    bool		acceptOK();
    void		saveIfNewVintage(const BufferString&);
    void		updateStatus(CallBacker*);
    void		displayReportCB(CallBacker*);
    void		setButtonSensitivity(bool);

    BufferStringSet	selfilenms_;
    uiTable*		table_;
    uiSEGYImpType*	imptypefld_;
    BufferString	vintagenm_;
    uiSEGYReadStarter*	rsdlg_;
    uiSEGYFileSelector* fsdlg_;
    uiSEGYReadFinisher* rfdlg_;
    ObjectSet<SEGY::Vintage::Info> vntinfos_;

    uiToolButton*	addbut_;
    uiToolButton*	removebut_;
    uiToolButton*	editbut_;
};

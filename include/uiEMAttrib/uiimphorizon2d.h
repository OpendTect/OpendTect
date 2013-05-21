#ifndef uiimphorizon2d_h
#define uiimphorizon2d_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman Singh
 Date:          May 2008
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiemattribmod.h"
#include "uidialog.h"
#include "emposid.h"
#include "multiid.h"

class BufferStringSet;
class Horizon2DScanner;
class SurfaceInfo;

class uiComboBox;
class uiFileInput;
class uiGenInput;
class uiListBox;
class uiPushButton;
class uiTableImpDataSel;
namespace Table { class FormatDesc; }

/*! \brief Dialog for Horizon Import */

mExpClass(uiEMAttrib) uiImportHorizon2D : public uiDialog
{
public:
			uiImportHorizon2D(uiParent*);
			~uiImportHorizon2D();

    void		getEMObjIDs(TypeSet<EM::ObjectID>&) const;
    Notifier<uiImportHorizon2D>	readyForDisplay;

protected:

    uiFileInput*	inpfld_;
    uiComboBox*		linesetfld_;
    uiPushButton*       scanbut_;
    uiListBox*		horselfld_;
    uiTableImpDataSel*  dataselfld_;
    uiGenInput*		udftreatfld_;

    virtual bool	acceptOK(CallBacker*);
    void                descChg(CallBacker*);
    void		setSel(CallBacker*);
    void		addHor(CallBacker*);
    void		formatSel(CallBacker*);
    void		scanPush(CallBacker*);

    bool		getFileNames(BufferStringSet&) const;
    bool		checkInpFlds();
    bool		doImport();

    Table::FormatDesc&  fd_;
    Horizon2DScanner*	scanner_;
    BufferStringSet&	linesetnms_;
    TypeSet<MultiID>	setids_;
    TypeSet<EM::ObjectID> emobjids_;

    ObjectSet<SurfaceInfo>	horinfos_;
};


#endif


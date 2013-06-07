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

#include "uidialog.h"

class BufferStringSet;
class Horizon2DScanner;
class MultiID;
class SurfaceInfo;
class uiCheckBox;
class uiComboBox;
class uiFileInput;
class uiListBox;
class uiPushButton;
class uiTableImpDataSel;
namespace Table { class FormatDesc; }

/*! \brief Dialog for Horizon Import */

mClass uiImportHorizon2D : public uiDialog
{
public:
			uiImportHorizon2D(uiParent*);
			~uiImportHorizon2D();

    bool                doDisplay() const;

protected:

    uiFileInput*	inpfld_;
    uiComboBox*		linesetfld_;
    uiPushButton*       scanbut_;
    uiListBox*		horselfld_;
    uiTableImpDataSel*  dataselfld_;
    uiCheckBox*         displayfld_;

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

    ObjectSet<SurfaceInfo>	horinfos_;
};


#endif

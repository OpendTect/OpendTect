#ifndef uiattr2dsel_h
#define uiattr2dsel_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        R. K. Singh
 Date:          Nov 2007
 RCS:           $Id: uiattr2dsel.h,v 1.2 2007-12-07 12:11:35 cvsraman Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "attribdescid.h"
#include "bufstring.h"

namespace Attrib { class Desc; class DescSet; class SelInfo; class SelSpec; };

class MultiID;
class uiButtonGroup;
class uiCheckBox;
class uiComboBox;
class uiGenInput;
class uiListBox;
class uiRadioButton;


class uiAttr2DSelDlg : public uiDialog
{
public:

			uiAttr2DSelDlg(uiParent*,const Attrib::DescSet*,
				       const MultiID&,const char* curnm=0);
			~uiAttr2DSelDlg();

    int			getSelType()		{ return seltype_; }
    const char*		getStoredAttrName()	{ return storednm_.buf(); }
    Attrib::DescID	getSelDescID()		{ return descid_; }
    bool		needToSetColTab()	{ return usecoltab_; }
    const char*		getColTabName()		{ return coltabnm_.buf(); }

protected:

    Attrib::SelInfo*	attrinf_;
    const MultiID&	setid_;
    Attrib::DescID	descid_;
    int			seltype_;
    bool		usecoltab_;
    BufferString	storednm_;
    BufferString	curnm_;
    BufferString	coltabnm_;

    uiButtonGroup*	selgrp_;
    uiRadioButton*	storfld_;
    uiRadioButton*	attrfld_;

    uiListBox*		storoutfld_;
    uiListBox*		attroutfld_;
    uiCheckBox*		coltabfld_;
    uiComboBox*		coltabsel_;

    void		createSelectionButtons();
    void		createSelectionFields();
    void		createColorFields();

    void		doFinalise( CallBacker* );
    void		selDone(CallBacker*);
    void		colTabSel(CallBacker*);
    virtual bool	acceptOK(CallBacker*);
    int			selType() const;
};


#endif

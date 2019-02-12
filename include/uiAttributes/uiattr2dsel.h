#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        R. K. Singh
 Date:          Nov 2007
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uidialog.h"
#include "attribsel.h"
#include "bufstring.h"

namespace Attrib { class Desc; class DescSet; class SelInfo; }

class uiButtonGroup;
class uiListBox;
class uiRadioButton;
class NLAModel;

/*!\brief selection dialog for 2D attributes.  */

mExpClass(uiAttributes) uiAttr2DSelDlg : public uiDialog
{ mODTextTranslationClass(uiAttr2DSelDlg)
public:

    typedef Attrib::DescID	DescID;
    typedef Attrib::DescSet	DescSet;

			uiAttr2DSelDlg(uiParent*,const DescSet*,
				       const GeomIDSet&,const NLAModel*,
				       const char* curnm=0);
			~uiAttr2DSelDlg();

    int			getSelType() const		{ return seltype_; }
    const char*		getStoredAttrName() const	{ return storednm_; }
    DescID		getSelDescID() const		{ return descid_; }
    int			getComponent() const		{ return compnr_; }
    int			getOutputNr() const		{ return outputnr_; }

protected:

    Attrib::SelInfo*	attrinf_;
    GeomIDSet		geomids_;
    DescID		descid_;
    const NLAModel*	nla_;
    int			seltype_;
    BufferString	storednm_;
    BufferString	curnm_;
    int			compnr_;
    int			outputnr_;

    uiButtonGroup*	selgrp_;
    uiRadioButton*	storfld_;
    uiRadioButton*	steerfld_;
    uiRadioButton*	attrfld_;
    uiRadioButton*	nlafld_;

    uiListBox*		storoutfld_;
    uiListBox*		steeroutfld_;
    uiListBox*		attroutfld_;
    uiListBox*		nlaoutfld_;

    void		createSelectionButtons();
    void		createSelectionFields();

    void		doFinalise(CallBacker*);
    void		selDone(CallBacker*);
    virtual bool	acceptOK();
    int			selType() const;
};

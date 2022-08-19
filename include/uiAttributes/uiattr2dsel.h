#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uidialog.h"
#include "attribsel.h"
#include "bufstring.h"

namespace Attrib { class Desc; class DescSet; class SelInfo; }

class uiButtonGroup;
class uiGenInput;
class uiListBox;
class uiListBoxFilter;
class uiRadioButton;
class NLAModel;

/*!
\brief Selection dialog for 2D attributes.
*/

mExpClass(uiAttributes) uiAttr2DSelDlg : public uiDialog
{ mODTextTranslationClass(uiAttr2DSelDlg)
public:

			uiAttr2DSelDlg(uiParent*,const Attrib::DescSet*,
					const TypeSet<Pos::GeomID>&,
					const NLAModel*,const char* curnm=0);
			uiAttr2DSelDlg(uiParent*,const Attrib::DescSet*,
					const TypeSet<Pos::GeomID>&,
					const NLAModel*,ZDomain::Info&,
					const char* curnm=0);
			~uiAttr2DSelDlg();

    int			getSelType() const		{ return seltype_; }
    const char*		getStoredAttrName() const	{ return storednm_; }
    Attrib::DescID	getSelDescID() const		{ return descid_; }
    int			getComponent() const		{ return compnr_; }
    int			getOutputNr() const		{ return outputnr_; }

protected:

    Attrib::SelInfo*	attrinf_;
    TypeSet<Pos::GeomID> geomids_;
    Attrib::DescID	descid_;
    const NLAModel*	nla_;
    int			seltype_	= 0;
    BufferString	storednm_;
    BufferString	curnm_;
    int			compnr_		= -1;
    int			outputnr_	= -1;

    uiButtonGroup*	selgrp_;
    uiRadioButton*	storfld_;
    uiRadioButton*	steerfld_	= nullptr;
    uiRadioButton*	attrfld_;
    uiRadioButton*	nlafld_		= nullptr;

    uiListBox*		storoutfld_;
    uiListBox*		steeroutfld_	= nullptr;
    uiListBox*		attroutfld_	= nullptr;
    uiListBox*		nlaoutfld_	= nullptr;
    uiGenInput*		storsteerfilter_;
    uiListBoxFilter*	attrfilter_		= nullptr;

    void		doFinalize( CallBacker* );
    void		initFields(const Attrib::DescSet&);
    void		selDone(CallBacker*);
    void		filtChg(CallBacker*);
    bool		acceptOK(CallBacker*) override;
    int			selType() const;

    void		createSelectionButtons(ZDomain::Info&);
    void		createSelectionFields(ZDomain::Info&);

    void		createSelectionButtons();	// ZDomain::SI()
    void		createSelectionFields();	// ZDomain::SI()
};

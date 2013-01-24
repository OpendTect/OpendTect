#ifndef uiattr2dsel_h
#define uiattr2dsel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        R. K. Singh
 Date:          Nov 2007
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uidialog.h"
#include "attribsel.h"
#include "bufstring.h"

namespace Attrib { class Desc; class DescSet; class SelInfo; class SelSpec; };

class MultiID;
class uiButtonGroup;
class uiGenInput;
class uiListBox;
class uiRadioButton;
class NLAModel;

/*!
\brief Selection dialog for 2D attributes.
*/

mExpClass(uiAttributes) uiAttr2DSelDlg : public uiDialog
{
public:

			uiAttr2DSelDlg(uiParent*,const Attrib::DescSet*,
				       const MultiID&,const NLAModel* nla,
				       const char* curnm=0);
			~uiAttr2DSelDlg();

    int			getSelType()		{ return seltype_; }
    const char*		getStoredAttrName()	{ return storednm_.buf(); }
    Attrib::DescID	getSelDescID()		{ return descid_; }

protected:

    Attrib::SelInfo*	attrinf_;
    const MultiID&	setid_;
    Attrib::DescID	descid_;
    const NLAModel*	nla_;
    int			seltype_;
    BufferString	storednm_;
    BufferString	curnm_;

    uiButtonGroup*	selgrp_;
    uiRadioButton*	storfld_;
    uiRadioButton*	attrfld_;
    uiRadioButton*	nlafld_;

    uiListBox*		storoutfld_;
    uiListBox*		attroutfld_;
    uiListBox*		nlaoutfld_;

    void		createSelectionButtons();
    void		createSelectionFields();

    void		doFinalise( CallBacker* );
    void		selDone(CallBacker*);
    virtual bool	acceptOK(CallBacker*);
    int			selType() const;
};


#endif


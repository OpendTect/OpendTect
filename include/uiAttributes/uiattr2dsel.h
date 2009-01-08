#ifndef uiattr2dsel_h
#define uiattr2dsel_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        R. K. Singh
 Date:          Nov 2007
 RCS:           $Id: uiattr2dsel.h,v 1.4 2009-01-08 08:50:11 cvsranojay Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "attribdescid.h"
#include "bufstring.h"

namespace Attrib { class Desc; class DescSet; class SelInfo; class SelSpec; };

class MultiID;
class uiButtonGroup;
class uiGenInput;
class uiListBox;
class uiRadioButton;


mClass uiAttr2DSelDlg : public uiDialog
{
public:

			uiAttr2DSelDlg(uiParent*,const Attrib::DescSet*,
				       const MultiID&,const char* curnm=0);
			~uiAttr2DSelDlg();

    int			getSelType()		{ return seltype_; }
    const char*		getStoredAttrName()	{ return storednm_.buf(); }
    Attrib::DescID	getSelDescID()		{ return descid_; }

protected:

    Attrib::SelInfo*	attrinf_;
    const MultiID&	setid_;
    Attrib::DescID	descid_;
    int			seltype_;
    BufferString	storednm_;
    BufferString	curnm_;

    uiButtonGroup*	selgrp_;
    uiRadioButton*	storfld_;
    uiRadioButton*	attrfld_;

    uiListBox*		storoutfld_;
    uiListBox*		attroutfld_;

    void		createSelectionButtons();
    void		createSelectionFields();

    void		doFinalise( CallBacker* );
    void		selDone(CallBacker*);
    virtual bool	acceptOK(CallBacker*);
    int			selType() const;
};


#endif

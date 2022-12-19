#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uidialog.h"
#include "uigroup.h"
#include "attribdescid.h"
#include "attribsel.h"

class BufferStringSet;
class uiCheckBox;
class uiListBox;
namespace Attrib { class Desc; class DescSet; }

/*! \brief Dialog used to determine which outputs of an attribute will be
computed at once and stored as multiple
components when creating a volume output */

mExpClass(uiAttributes) uiMultOutSel : public uiDialog
{ mODTextTranslationClass(uiMultOutSel)
public:
			uiMultOutSel(uiParent*,const Attrib::Desc&,
							    bool isisnglesel);
			~uiMultOutSel();

    void		getSelectedOutputs(TypeSet<int>&) const;
    void		getSelectedOutNames(BufferStringSet&) const;
    bool		doDisp() const;

    static void		fillInAvailOutNames(const Attrib::Desc&,
	    				    BufferStringSet&);
    static bool		handleMultiCompChain(Attrib::DescID& selid,
					 const Attrib::DescID& multicompinpid,
					 bool,const Attrib::SelInfo&,
					 Attrib::DescSet*,uiParent*,
					 TypeSet<Attrib::SelSpec>&);

protected:

    void		createMultOutDlg(const BufferStringSet&,
							    bool isisnglesel);
    void		allSel(CallBacker*);

    uiListBox*		outlistfld_ = nullptr;
    uiCheckBox*		outallfld_  = nullptr;

    TypeSet<int>	outputids_;

};



mExpClass(uiAttributes) uiMultiAttribSel : public uiGroup
{ mODTextTranslationClass(uiMultiAttribSel)
public:
			uiMultiAttribSel(uiParent*,const Attrib::DescSet*);
			~uiMultiAttribSel();

    void		getSelIds(TypeSet<Attrib::DescID>&) const;
    bool		is2D() const;
    void		setDescSet(const Attrib::DescSet*);

protected:

    void		fillAttribFld();
    void		updateSelFld();

    void		doAdd(CallBacker*);
    void		doRemove(CallBacker*);
    void		moveUp(CallBacker*);
    void		moveDown(CallBacker*);

    const Attrib::DescSet*  descset_;
    TypeSet<Attrib::DescID> allids_;
    TypeSet<Attrib::DescID> selids_;

    uiListBox*		attribfld_;
    uiListBox*		selfld_;
};

#ifndef uimultoutsel_h
#define uimultoutsel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        H. Huck
 Date:          Jan 2009
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uidialog.h"
#include "uigroup.h"
#include "attribdescid.h"

class BufferStringSet;
class uiCheckBox;
class uiListBox;
namespace Attrib { class Desc; class DescSet; };

/*! \brief Dialog used to determine which outputs of an attribute will be
computed at once and stored as multiple components when creating a volume output */

mExpClass(uiAttributes) uiMultOutSel : public uiDialog
{
public:
			uiMultOutSel(uiParent*,const Attrib::Desc&);

    void		getSelectedOutputs(TypeSet<int>&) const;
    void		getSelectedOutNames(BufferStringSet&) const;
    bool		doDisp() const;

protected:

    void		fillInAvailOutNames(Attrib::Desc*,
	                                    BufferStringSet&) const;

    void		createMultOutDlg(const BufferStringSet&);
    void		allSel(CallBacker*);

    uiListBox*		outlistfld_;
    uiCheckBox*		outallfld_;

};



mExpClass(uiAttributes) uiMultiAttribSel : public uiGroup
{
public:
			uiMultiAttribSel(uiParent*,const Attrib::DescSet&);
			~uiMultiAttribSel();

    void		getSelIds(TypeSet<Attrib::DescID>&) const;
    bool		is2D() const;

protected:

    void		fillAttribFld();
    void		updateSelFld();

    void		doAdd(CallBacker*);
    void		doRemove(CallBacker*);
    void		moveUp(CallBacker*);
    void		moveDown(CallBacker*);

    const Attrib::DescSet&  descset_;
    TypeSet<Attrib::DescID> allids_;
    TypeSet<Attrib::DescID> selids_;

    uiListBox*		attribfld_;
    uiListBox*		selfld_;
};

#endif


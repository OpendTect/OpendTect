#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uidlggroup.h"
#include "uivarwizarddlg.h"

class PropertyRef;
class PropertyRefSet;
class PropertyRefSelection;
class uiBuildPROPS;
class uiGenInput;
class uiListBox;


/*!\brief Manages the PROPS() */

mExpClass(uiIo) uiManPROPS : public uiDialog
{ mODTextTranslationClass(uiManPROPS);
public:
			uiManPROPS(uiParent*);
			~uiManPROPS();

    bool		haveUserChange() const;

protected:

    uiBuildPROPS*	buildfld_;
    uiGenInput*		srcfld_;

    bool		rejectOK(CallBacker*) override;

};


/*!\brief Allow user to select PropertyRefs.

  Beware. Even on cancel, the user may have removed defined refs from PROPS().
  Therefore, even on cancel the PropertyRefSelection can be changed. Code
  should look something like:

  uiSelectPropRefs dlg( this, prs );
  if ( dlg.go() || dlg.structureChanged() )
      handleRefChanges();

 */

mExpClass(uiIo) uiSelectPropRefsGrp : public uiDlgGroup
{ mODTextTranslationClass(uiSelectPropRefsGrp);
public:

			uiSelectPropRefsGrp(uiParent*,PropertyRefSelection&,
				const uiString& lbltxt=uiString::empty());
			~uiSelectPropRefsGrp();

    bool		structureChanged() const	{ return structchg_; }
    bool		acceptOK() override;

protected:

    uiListBox*		propfld_;
    PropertyRefSelection& prsel_;
    bool		structchg_;

    void		fillList();
    void		manPROPS(CallBacker*);

    const PropertyRefSet& props_; // PROPS()
    const PropertyRef*	thref_; // &PropertyRef::thickness()
};


mExpClass(uiIo) uiSelectPropRefs : public uiDialog
{ mODTextTranslationClass(uiSelectPropRefs);
public:
				uiSelectPropRefs(uiParent*,
				    PropertyRefSelection&,
				    const uiString& lbltxt=uiString::empty());
				~uiSelectPropRefs();

    bool		structureChanged() const
			{ return proprefgrp_->structureChanged(); }

protected:
    uiSelectPropRefsGrp*	proprefgrp_;
    
    bool			acceptOK(CallBacker*) override;
};


mExpClass(uiIo) uiSelectPropRefsVWDlg : public uiVarWizardDlg
{ mODTextTranslationClass(uiSelectPropRefsVWDlg);
public:
			uiSelectPropRefsVWDlg(uiParent*,PropertyRefSelection&,
			  IOPar&,int,const uiString& lbltxt=uiString::empty());
			~uiSelectPropRefsVWDlg();

protected:
    uiSelectPropRefsGrp*	proprefgrp_;
    
    bool			acceptOK(CallBacker*) override;
};

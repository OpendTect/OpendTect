#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Jun 2011
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
					    const char* lbltxt=nullptr);

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
						 const char* lbltxt=0);
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
					      IOPar&,int,const char* lbltxt=0);
protected:
    uiSelectPropRefsGrp*	proprefgrp_;
    
    bool			acceptOK(CallBacker*) override;
};


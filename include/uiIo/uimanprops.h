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
class Mnemonic;
class Property;
class PropertyRef;
class PropertySet;
class PropertySelection;
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

    bool		rejectOK(CallBacker*);

};


/*!\brief Allow user to select Propertys.

  Beware. Even on cancel, the user may have removed defined refs from PROPS().
  Therefore, even on cancel the PropertySelection can be changed. Code
  should look something like:

  uiSelectProps dlg( this, prs );
  if ( dlg.go() || dlg.structureChanged() )
      handleRefChanges();

 */

mExpClass(uiIo) uiSelectPropsGrp : public uiDlgGroup
{ mODTextTranslationClass(uiSelectPropsGrp);
public:

			uiSelectPropsGrp(uiParent*,PropertySelection&,
					    const char* lbltxt=0);

    bool		structureChanged() const	{ return structchg_; }
    bool		acceptOK();

protected:

    uiListBox*		propfld_;
    PropertySelection& prsel_;
    bool		structchg_;

    void		fillList();
    void		manPROPS(CallBacker*);

    const PropertySet& props_; // PROPS()
    const Property*	thref_; // &PropertyRef::thickness()
};


mExpClass(uiIo) uiSelectProps : public uiDialog
{ mODTextTranslationClass(uiSelectProps);
public:
				uiSelectProps(uiParent*,
						 PropertySelection&,
						 const char* lbltxt=0);
    bool		structureChanged() const
			{ return proprefgrp_->structureChanged(); }
protected:
    uiSelectPropsGrp*	proprefgrp_;
    
    bool			acceptOK(CallBacker*);
};


mExpClass(uiIo) uiSelectPropsVWDlg : public uiVarWizardDlg
{ mODTextTranslationClass(uiSelectPropsVWDlg);
public:
			uiSelectPropsVWDlg(uiParent*,PropertySelection&,
					      IOPar&,int,const char* lbltxt=0);
protected:
    uiSelectPropsGrp*	proprefgrp_;
    
    bool			acceptOK(CallBacker*);
};


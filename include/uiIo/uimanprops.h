#ifndef uimanprops_h
#define uimanprops_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jun 2011
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uidialog.h"
#include "uidialog.h"
class PropertyRef;
class PropertyRefSet;
class PropertyRefSelection;
class uiBuildPROPS;
class uiGenInput;
class uiListBox;


/*!\brief Manages the PROPS(). */

mClass uiManPROPS : public uiDialog
{
public:

    			uiManPROPS(uiParent*);

    bool		haveUserChange() const;

protected:

    uiBuildPROPS*	buildfld_;
    uiGenInput*		srcfld_;

    bool		rejectOK(CallBacker*);

};


/*!\brief Allow user to select PropertyRefs.

  Beware. Even on cancel, the user may have removed defined refs from PROPS().
  Therefore, even on cancel the PropertyRefSelection can be changed. Code
  should look something like:

  uiSelectPropRefs dlg( this, prs );
  if ( dlg.go() || dlg.structureChanged() )
      handleRefChanges();
 
 */

mClass uiSelectPropRefs : public uiDialog
{
public:

    			uiSelectPropRefs(uiParent*,PropertyRefSelection&,
					 const char* lbltxt=0);

    bool		structureChanged() const	{ return structchg_; }

protected:

    uiListBox*		propfld_;
    PropertyRefSelection& prsel_;
    bool		structchg_;

    void		fillList();
    void		manPROPS(CallBacker*);
    bool		acceptOK(CallBacker*);

    const PropertyRefSet& props_; // PROPS()
    const PropertyRef*	thref_; // &PropertyRef::thickness()

};


#endif

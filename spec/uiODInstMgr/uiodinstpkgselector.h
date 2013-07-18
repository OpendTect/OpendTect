#ifndef uiodinstpkgselector_h
#define uiodinstpkgselector_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Aneesh
 Date:          Aug 2012
 RCS:           $Id: uiodinstpkgselector.h 8009 2013-06-20 06:11:26Z kristofer.tingdahl@dgbes.com $
________________________________________________________________________

-*/
#include "uiodinstmgrmod.h"
#include "uitable.h"
#include "commondefs.h"
#include "uibutton.h"
#include "bufstringset.h"
#include "uitextedit.h"


namespace ODInst
{ class PkgSelMgr; class PkgProps; class HtmlComposer; 
  class PkgGroupSet; class PkgSelection; }

mDefClass(uiODInstMgr) uiODInstPackageSelector : public uiTable
{
public:
			uiODInstPackageSelector(uiParent*, ODInst::PkgSelMgr&, 
					       const ODInst::PkgGroupSet& pgs);
			~uiODInstPackageSelector();
    void		createTable();
    void		fillTable();
    void		setAction();
    void		updateActionCB(CallBacker*); 
    void		updateSelection();
    void		linkClickCB(CallBacker*);
    void		creatorClickCB(CallBacker*);

    CNotifier<uiODInstPackageSelector, ODInst::PkgSelection*> notifier;

protected:
    
    uiTextBrowser*	edittxt_;
    uiTextBrowser*	creatortxt_;
    uiCheckBox*		checkboxlist_;
    ODInst::PkgSelMgr&	pkgselmgr_;  
    ObjectSet<ODInst::PkgSelection> pkgsel_;
    ODInst::HtmlComposer& htmlcomp_;
};

#endif



#ifndef uistratutildlgs_h
#define uistratutildlgs_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck
 Date:          August 2007
 RCS:           $Id: uistratutildlgs.h,v 1.16 2010-07-14 10:05:13 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "ranges.h"
#include "stratunitref.h"

class BufferStringSet;
class uiColorInput;
class uiGenInput;
class uiListBox;
class uiCheckBox;
class uiStratMgr;
class uiSpinBox;
class uiTable;
namespace Strat { class Lithology; }

/*!\brief Displays a dialog to create new stratigraphic unit */

mClass uiStratUnitDlg : public uiDialog
{
public:

    mClass Setup
    {
	public :
	    		Setup(uiStratMgr* mgr)
			    : uistratmgr_(mgr)
			    , timerg_(0,4500)
			    , entrancename_("")	     
			    {}
			
	mDefSetupMemb(uiStratMgr*,uistratmgr)
	mDefSetupMemb(Interval<float>,timerg)
	mDefSetupMemb(BufferString,entrancename)	     
    };

			uiStratUnitDlg(uiParent*,Setup&);

    void		setUnitProps(const Strat::UnitRef::Props&);
    void		getUnitProps(Strat::UnitRef::Props&) const;	

protected:

    uiGenInput*		unitnmfld_;
    uiGenInput*		unitdescfld_;
    uiGenInput*		unitlithfld_;
    uiSpinBox*		agestartfld_;
    uiSpinBox*		agestopfld_;
    uiColorInput*	colfld_;
    
    uiStratMgr*		uistratmgr_;
    BufferString&	entrancename_;

    void		selLithCB(CallBacker*);
    bool		acceptOK(CallBacker*);

};


/*!\brief Displays a Table to create new units from an existing one */

mClass uiStratUnitDivideDlg : public uiDialog
{
public:
				uiStratUnitDivideDlg(uiParent*,
						const uiStratMgr& mgr,
						const Strat::UnitRef::Props&);

    void			gatherProps(ObjectSet<Strat::UnitRef::Props>&); 

protected :

    uiTable*                    table_;
    const Strat::UnitRef::Props& parentprop_;
    const uiStratMgr&		uistratmgr_;

    void			setUnit(int,const Strat::UnitRef::Props&);
    bool			areTimesOK(
				    ObjectSet<Strat::UnitRef::Props>&) const;

    void			mouseClick(CallBacker*);
    bool			acceptOK(CallBacker*);
    void			resetUnits(CallBacker*);
};



/*!\brief Displays a dialog to create new lithology */

mClass uiStratLithoDlg : public uiDialog
{
public:

			uiStratLithoDlg(uiParent*, uiStratMgr*);
    const char*		getLithName() const;
    void		setSelectedLith(const char*);

protected:

    uiListBox*		selfld_;
    uiGenInput*		nmfld_;
    uiCheckBox*		isporbox_;

    Strat::Lithology*	prevlith_;
    uiStratMgr*		uistratmgr_;

    void		fillLiths();
    void		newLith(CallBacker*);
    void		selChg(CallBacker*);
    void		rmSel(CallBacker*);
    void		renameCB(CallBacker*);

    bool		acceptOK(CallBacker*);
};


mClass uiStratLevelDlg : public uiDialog
{
public:

    uiStratLevelDlg(uiParent*,uiStratMgr*);

    void                setLvlInfo(const char*);

protected:

    BufferString        oldlvlnm_;
    uiStratMgr*         uistratmgr_;

    uiGenInput*         lvlnmfld_;
    uiColorInput*       lvlcolfld_;

    bool                acceptOK(CallBacker*);
};


mClass uiStratLinkLvlUnitDlg : public uiDialog
{
public:

    			uiStratLinkLvlUnitDlg(uiParent*,int,uiStratMgr&);

    int 		lvlid_;

protected:

    uiGenInput*         lvllistfld_;
    TypeSet<int>	ids_;

    bool		acceptOK(CallBacker*);
};


#endif

#ifndef uistratlaymodtools_h
#define uistratlaymodtools_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2012
 RCS:           $Id: uistratlaymodtools.h,v 1.6 2012-08-03 13:01:10 cvskris Exp $
________________________________________________________________________

-*/

#include "uistratmod.h"
#include "uigroup.h"
class uiSpinBox;
class uiGenInput;
class uiComboBox;
class uiToolButton;
namespace Strat { class Level; }


mClass(uiStrat) uiStratGenDescTools : public uiGroup
{
public:

		uiStratGenDescTools(uiParent*);

    int		nrModels() const;
    void	enableSave(bool);

    Notifier<uiStratGenDescTools>	openReq;
    Notifier<uiStratGenDescTools>	saveReq;
    Notifier<uiStratGenDescTools>	propEdReq;
    Notifier<uiStratGenDescTools>	genReq;

protected:

    uiGenInput*	nrmodlsfld_;
    uiToolButton* savetb_;

    void	openCB(CallBacker*)	{ openReq.trigger(); }
    void	saveCB(CallBacker*)	{ saveReq.trigger(); }
    void	propEdCB(CallBacker*)	{ propEdReq.trigger(); }
    void	genCB(CallBacker*)	{ genReq.trigger(); }

};


mClass(uiStrat) uiStratLayModEditTools : public uiGroup
{
public:

		uiStratLayModEditTools(uiParent*);

    void 	setProps(const BufferStringSet&);
    void 	setLevelNames(const BufferStringSet&);
    void 	setContentNames(const BufferStringSet&);

    const char*	selProp() const;		//!< May return null
    const char*	selLevel() const; 		//!< May return null
    const char*	selContent() const; 		//!< May return null
    int		dispEach() const;
    bool	dispZoomed() const;
    bool	dispLith() const;

    void	setSelProp(const char*);
    void	setSelLevel(const char*);
    void	setSelContent(const char*);
    void	setDispEach(int);
    void	setDispZoomed(bool);
    void	setDispLith(bool);

    Notifier<uiStratLayModEditTools>	selPropChg;
    Notifier<uiStratLayModEditTools>	selLevelChg;
    Notifier<uiStratLayModEditTools>	selContentChg;
    Notifier<uiStratLayModEditTools>	dispEachChg;
    Notifier<uiStratLayModEditTools>	dispZoomedChg;
    Notifier<uiStratLayModEditTools>	dispLithChg;

    int		selPropIdx() const;		//!< May return -1
    int		selLevelIdx() const;		//!< May return -1
    const Strat::Level*	selStratLevel() const; 	//!< May return null
    Color	selLevelColor() const;		//!< May return NoColor

    uiToolButton* lithButton() 		{ return lithtb_; }

protected:

    uiComboBox*	propfld_;
    uiComboBox*	lvlfld_;
    uiComboBox*	contfld_;
    uiSpinBox*	eachfld_;
    uiToolButton* zoomtb_;
    uiToolButton* lithtb_;

    void	selPropCB( CallBacker* )	{ selPropChg.trigger(); }
    void	selLevelCB( CallBacker* )	{ selLevelChg.trigger(); }
    void	selContentCB( CallBacker* )	{ selContentChg.trigger(); }
    void	dispEachCB( CallBacker* )	{ dispEachChg.trigger(); }
    void	dispZoomedCB( CallBacker* )	{ dispZoomedChg.trigger(); }
    void	dispLithCB( CallBacker* )	{ dispLithChg.trigger(); }

};


#endif


#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Nov 2010
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uibutton.h"
#include "uistrings.h"

class uiToolButtonBody;
class uiToolButtonSetup;


mExpClass(uiBase) uiToolButton : public uiButton
{
public:

    enum ArrowType		{ NoArrow,
				  UpArrow, DownArrow, LeftArrow, RightArrow };
    enum PopupMode		{ DelayedPopup, MenuButtonPopup, InstantPopup };

				uiToolButton(uiParent*,
					     const uiToolButtonSetup&);
				uiToolButton(uiParent*,const char* filenm,
					     const uiString& tooltip,
					     const CallBack&);
				uiToolButton(uiParent*,ArrowType,
					     const uiString& tooltip,
					     const CallBack&);
				~uiToolButton();

    static uiToolButton*	getStd(uiParent*,OD::StdActionType,
					const CallBack&,const uiString& ttip);

    bool			isOn() const;
    void			setOn(bool yn=true);

    void			setToggleButton(bool yn=true);
    bool			isToggleButton() const;

    void			setArrowType(ArrowType);

    void			setShortcut(const char*);
    void			setMenu(uiMenu*,PopupMode=MenuButtonPopup);
				//!<Menu becomes mine

    const uiMenu*		menu() const		{ return uimenu_; }

    void			setID( int i )		{ id_ = i; }
    int				id() const		{ return id_; }

    void			click() override;

private:

    uiToolButtonBody*		tbbody_;
    uiToolButtonBody&		mkbody(uiParent*,const char* icon_identifier,
				       const uiString&);

    int				id_; // Used by toolbar

    uiMenu*			uimenu_;

};


#define mDefuiTBSUMemb(typ,memb) mDefSetupClssMemb(uiToolButtonSetup,typ,memb)

mExpClass(uiBase) uiToolButtonSetup
{
public:
			uiToolButtonSetup( const char* ic, const uiString& tt,
					   const CallBack& c , const uiString&
					   nm = uiStrings::sEmptyString() )
			    : icid_(ic)
			    , cb_(c)
			    , tooltip_(tt)
			    , istoggle_(false)
			    , ison_(false)
			    , isimmediate_(false)
			    , arrowtype_(uiToolButton::NoArrow)
			    , name_(!nm.isEmpty() ? nm : tt)
			{}

    mDefuiTBSUMemb(BufferString,icid);
    mDefuiTBSUMemb(uiString,tooltip);
    mDefuiTBSUMemb(CallBack,cb);

    mDefuiTBSUMemb(bool,istoggle);
    mDefuiTBSUMemb(bool,ison);
    mDefuiTBSUMemb(bool,isimmediate);
    mDefuiTBSUMemb(uiToolButton::ArrowType,arrowtype);
    mDefuiTBSUMemb(BufferString,shortcut);
    mDefuiTBSUMemb(uiString,name);

    uiButton*		getButton(uiParent*,bool forcetoolbutton=false) const;
			//!< pushbutton if name_ != tooltip_ and !istoggle_
    uiToolButton*	getToolButton(uiParent*) const;
    uiPushButton*	getPushButton(uiParent*,bool withicon=true) const;

};


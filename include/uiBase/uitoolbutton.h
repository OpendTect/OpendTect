#ifndef uitoolbutton_h
#define uitoolbutton_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Nov 2010
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uibutton.h"
#include "uiicons.h"

class uiToolButtonBody;
class uiToolButtonSetup;


mExpClass(uiBase) uiToolButton : public uiButton
{
public:

    enum ArrowType		{ NoArrow,
				  UpArrow, DownArrow, LeftArrow, RightArrow };

				uiToolButton(uiParent*,
					     const uiToolButtonSetup&);
				uiToolButton(uiParent*,const char* filenm,
					     const uiString& tooltip,
					     const CallBack&);
				uiToolButton(uiParent*,ArrowType,
					     const uiString& tooltip,
					     const CallBack&);
				~uiToolButton();

    bool			isOn() const;
    void			setOn(bool yn=true);

    void			setToggleButton(bool yn=true);
    bool			isToggleButton() const;

    void			setArrowType(ArrowType);

    void			setShortcut(const char*);
    void			setMenu(uiMenu*); //!<Menu becomes mine

    const uiMenu*		menu() const		{ return uimenu_; }

    void			setID( int i )		{ id_ = i; }
    int				id() const		{ return id_; }

    void			click();

private:

    uiToolButtonBody*		tbbody_;
    uiToolButtonBody&		mkbody(uiParent*,const ioPixmap&,
				       const uiString&);

    int				id_; // Used by toolbar

    uiMenu*			uimenu_;
    mQtclass(QMenu*)		qmenu_;
};


#define mDefuiTBSUMemb(typ,memb) mDefSetupClssMemb(uiToolButtonSetup,typ,memb)

mExpClass(uiBase) uiToolButtonSetup
{
public:
			uiToolButtonSetup( const char* fnm, const uiString& tt,
					   const CallBack& c, const char* nm=0 )
			    : filename_(fnm)
			    , cb_(c)
			    , tooltip_(tt)
			    , istoggle_(false)
			    , ison_(false)
			    , isimmediate_(false)
			    , arrowtype_(uiToolButton::NoArrow)
			    , name_(nm && *nm ? nm : tt.getFullString().buf()){}

    mDefuiTBSUMemb(BufferString,filename);
    mDefuiTBSUMemb(uiString,tooltip);
    mDefuiTBSUMemb(CallBack,cb);

    mDefuiTBSUMemb(bool,istoggle);
    mDefuiTBSUMemb(bool,ison);
    mDefuiTBSUMemb(bool,isimmediate);
    mDefuiTBSUMemb(uiToolButton::ArrowType,arrowtype);
    mDefuiTBSUMemb(BufferString,shortcut);
    mDefuiTBSUMemb(BufferString,name);

    uiButton*		getButton(uiParent*,bool forcetoolbutton=false) const;
			//!< pushbutton if name_ != tooltip_ and !istoggle_

};


#endif


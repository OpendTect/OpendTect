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

#include "uibutton.h"
#include "uiicons.h"
class uiToolButtonBody;
class uiToolButtonSetup;


mClass uiToolButton : public uiButton
{
public:

    enum ArrowType		{ NoArrow,
				  UpArrow, DownArrow, LeftArrow, RightArrow };

				uiToolButton(uiParent*,
					     const uiToolButtonSetup&);
				uiToolButton(uiParent*,const char* filenm,
					     const char* tooltip,
					     const CallBack&);
				uiToolButton(uiParent*,ArrowType,
					     const char* tooltip,
					     const CallBack&);
				~uiToolButton();

    bool			isOn() const;
    void			setOn(bool yn=true);

    void			setToggleButton(bool yn=true);
    bool			isToggleButton() const;

    void			setPixmap(const char*);
    void			setPixmap(const ioPixmap&);
    void			setArrowType(ArrowType);

    void			setShortcut(const char*);
    void			setMenu(uiPopupMenu*); //!<Menu becomes mine

    const uiPopupMenu*		menu() const		{ return uimenu_; }

    void			setID( int i )		{ id_ = i; }
    int				id() const		{ return id_; }
    
    void			click();

private:

    uiToolButtonBody*		body_;
    uiToolButtonBody&		mkbody(uiParent*,const ioPixmap&, const char*); 

    int				id_; // Used by toolbar

    uiPopupMenu*		uimenu_;
    QMenu*			qmenu_;

};


#define mDefuiTBSUMemb(typ,memb) mDefSetupClssMemb(uiToolButtonSetup,typ,memb)

mClass uiToolButtonSetup
{
public:
		    uiToolButtonSetup( const char* fnm, const char* tt,
					const CallBack& c, const char* nm=0 )
			: filename_(fnm)
			, cb_(c)
			, tooltip_(tt)
			, istoggle_(false)
			, ison_(false)
			, arrowtype_(uiToolButton::NoArrow)
			, name_(nm && *nm ? nm : tt)	{}
			
    mDefuiTBSUMemb(BufferString,filename);
    mDefuiTBSUMemb(BufferString,tooltip);
    mDefuiTBSUMemb(CallBack,cb);

    mDefuiTBSUMemb(bool,istoggle);
    mDefuiTBSUMemb(bool,ison);
    mDefuiTBSUMemb(uiToolButton::ArrowType,arrowtype);
    mDefuiTBSUMemb(BufferString,shortcut);
    mDefuiTBSUMemb(BufferString,name);

};


#endif

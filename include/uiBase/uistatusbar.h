#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          30/05/2000
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uibaseobject.h"
#include "draw.h"
#include "uistringset.h"

mFDQtclass(QStatusBar)
class uiStatusBarBody;
class uiMainWin;
class uiObject;


mExpClass(uiBase) uiStatusBar : public uiBaseObject
{

    friend class	uiMainWinBody;

public:
			~uiStatusBar();

    int			addMsgFld(const uiString& lbltxt=uiString::empty(),
				const uiString& tooltip =uiString::empty(),
#ifdef __win__
				OD::Alignment al=mAlignment(Left,VCenter),
#else
				OD::Alignment al=mAlignment(Left,Bottom),
#endif
				int stretch=1);

    int			addMsgFld(const uiString& tooltip,
				  OD::Alignment al=mAlignment(Left,Bottom),
				  int stretch=1);
    bool		addObject(uiObject*);

    void		setToolTip(int,const uiString&);
    void		setTxtAlign(int,OD::Alignment);
    void		setLabelTxt(int,const uiString&);

    int			nrFields() const;
    void		message(const uiString&,int fldidx=0, int msecs=-1);
    void		message(const uiStringSet&,int msecs=-1);
    void		setPartiallyEmpty(int startat);
    void		setBGColor(int fldidx,const Color&);
    Color		getBGColor(int fldidx) const;
    bool		isEmpty() const	{ return nrFields() < 1; }
    void		setEmpty()	{ setPartiallyEmpty(0); }
    void		getMessages( uiStringSet& msgs ) const
			{ msgs = messages_; }

    int			getNrWidgets() const		{ return 1; }
    mQtclass(QWidget)*	getWidget(int);

protected:

                        uiStatusBar(uiMainWin*,const char*,
				    mQtclass(QStatusBar&));
private:

    uiStatusBarBody*	body_;
    uiStatusBarBody&	mkbody(uiMainWin*, const char*,
			       mQtclass(QStatusBar&));
    uiStringSet		messages_;
};

#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uigroup.h"
#include "uiobj.h"

class BufferStringSet;
class uiMdiArea;
class uiMdiAreaBody;

mFDQtclass(QMdiArea)
mFDQtclass(QMdiSubWindow)


mExpClass(uiBase) uiMdiAreaWindow : public uiGroup
{
public:
				uiMdiAreaWindow(uiMdiArea&,
						const uiString& title=
						       uiString::emptyString());
				~uiMdiAreaWindow();
				mOD_DisableCopy(uiMdiAreaWindow)

    void			setTitle(const uiString&);
    const uiString&		getTitle() const { return title_; }

    void			setIcon(const char* img[]);
    void			setIcon(const char* icnnm);

    void			show();
    void			close();
    void			showMinimized();
    void			showMaximized();
    bool			isMinimized() const;
    bool			isMaximized() const;

    uiMdiArea&			getMdiArea()		{ return mdiarea_; }

    NotifierAccess&		closed();
    Notifier<uiMdiAreaWindow>	changed;
    NotifierAccess&		windowShown();
    NotifierAccess&		windowHidden();


    mQtclass(QMdiSubWindow*)		qWidget();
    const mQtclass(QMdiSubWindow*)	qWidget() const;


private:
    uiString				title_;
    uiMdiArea&				mdiarea_;
    mQtclass(QMdiSubWindow*)		qmdisubwindow_;

};


mExpClass(uiBase) uiMdiArea : public uiObject
{
friend class		uiMdiAreaBody;
friend class		i_MdiAreaMessenger;
public:
			uiMdiArea(uiParent*,const char* nm="uiMdiArea");
			~uiMdiArea();
			mOD_DisableCopy(uiMdiArea)

    void		tileHorizontal();
    void		tileVertical();
    void		tile();
    void		cascade();
    void		closeAll();

    void		addWindow(uiMdiAreaWindow*);
    uiMdiAreaWindow*	getWindow(const char*);
    const uiMdiAreaWindow* getWindow(const char*) const;

    void		setActiveWin(const char*);
    void		setActiveWin(uiMdiAreaWindow*);
    const char*		getActiveWin() const;
    void		getWindowNames(uiStringSet&) const;

    Notifier<uiMdiArea> windowActivated;

protected:

    uiMdiAreaBody&	mkbody(uiParent*,const char*);
    uiMdiAreaBody*	body_;

    ObjectSet<uiMdiAreaWindow> grps_;

    void		grpClosed(CallBacker*);
    void		grpChanged(CallBacker*);

public:
			// Temporarily prevent some Qt feedback loop to change
			// active window (solves problems when disabling data
			// tree or when Scenes-menu overlaps workspace)
    bool		paralyse(bool yn);
};

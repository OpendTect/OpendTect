#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A. Huck
 Date:          December 2018
________________________________________________________________________

-*/

#include "uibasemod.h"

#include "uiobj.h"

class uiWebEngineViewBody;
mFDQtclass(QWebEngineView)

mExpClass(uiBase) uiWebEngineBase : public uiObject
{

protected:
			uiWebEngineBase(uiParent*,const char*,uiObjectBody&);

private:

    virtual mQtclass(QWebEngineView&) qte()		= 0;
    const mQtclass(QWebEngineView&)   qte() const
			{ return const_cast<uiWebEngineBase*>(this)->qte(); }

};



mExpClass(uiBase) uiWebEngine : public uiWebEngineBase
{
public:
                        uiWebEngine(uiParent*,const char* nm="Web Browser");

	void				setUrl(const char*);

	void				back();
	void				forward();
	void				reload();
	void				stop();

private:

    virtual mQtclass(QWebEngineView&)	qte();

    uiWebEngineViewBody*	body_;
    uiWebEngineViewBody&	mkbody(uiParent*,const char*);
};


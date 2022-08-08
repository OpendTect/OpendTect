#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2006
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uiobjfileman.h"

class uiToolButton;

mExpClass(uiSeis) uiSeisPreStackMan : public uiObjFileMan
{  mODTextTranslationClass(uiSeisPreStackMan);
public:
			uiSeisPreStackMan(uiParent*,bool for2d);
			~uiSeisPreStackMan();

    bool		is2D() const		{ return is2d_; }

    mDeclInstanceCreatedNotifierAccess(uiSeisPreStackMan);

    mExpClass(uiSeis) BrowserDef
    {
    public:

			BrowserDef( const char* nm )
			    : name_(nm)
			    , for2d_(false)	{}

	BufferString	name_;		// translator name for 3D
	uiString	tooltip_;	// %1 will be filled by object name
	CallBack	cb_;		// this will be passed
	bool		for2d_;		// only 3D supported yet

    };

    static int		addBrowser(BrowserDef*);

protected:
    static uiString	createCaption(bool for2d);

    bool		is2d_;

    void		ownSelChg() override;
    void		mkFileInfo() override;
    const BrowserDef*	getBrowserDef() const;

    void		copyPush(CallBacker*);
    void                mergePush(CallBacker*);
    void                mkMultiPush(CallBacker*);
    void                editPush(CallBacker*); // For SEGYDirect

    uiToolButton*	copybut_;
    uiToolButton*	mergebut_;
    uiToolButton*	editbut_;
};



#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uiobjfileman.h"
class uiToolButton;


mExpClass(uiSeis) uiSeisFileMan : public uiObjFileMan
{ mODTextTranslationClass(uiSeisFileMan);
public:
			uiSeisFileMan(uiParent*,bool);
			~uiSeisFileMan();

    bool		is2D() const		{ return is2d_; }

    mExpClass(uiSeis) BrowserDef
    {
    public:

			BrowserDef( const char* nm )
			    : name_(nm)
			    , for2d_(false)	{}
			~BrowserDef()		{}

	BufferString	name_;		// translator name for 3D
	uiString	tooltip_;	// %1 will be filled by object name
	CallBack	cb_;		// this will be passed
	bool		for2d_;		// only 3D supported yet

    };
    static int		addBrowser(BrowserDef*);
    static uiString	sShowAttributeSet() { return tr("Show AttributeSet"); }
    mDeclInstanceCreatedNotifierAccess(uiSeisFileMan);

protected:

    bool		is2d_;
    uiToolButton*	browsebut_	    = nullptr;
    uiToolButton*	attribbut_;
    uiToolButton*	segyhdrbut_;
    uiToolButton*	copybut_;
    uiToolButton*	man2dlinesbut_	    = nullptr;
    uiToolButton*	mergecubesbut_	    = nullptr;

    void		mergePush(CallBacker*);
    void		passSelToMergeDlgCB(CallBacker*);
    void		browsePush(CallBacker*);
    void		copyPush(CallBacker*);
    void		man2DPush(CallBacker*);
    void		manPS(CallBacker*);
    void		showAttribSet(CallBacker*);
    void		showSEGYHeader(CallBacker*);
    void		extrFrom3D(CallBacker*);

    void		mkFileInfo() override;
    void		ownSelChg() override;
    od_int64		getFileSize(const char*,int&) const override;
    void		setToolButtonProperties();
    const BrowserDef*	getBrowserDef() const;

};

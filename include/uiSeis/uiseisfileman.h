#ifndef uiseisfileman_h
#define uiseisfileman_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id$
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
    uiToolButton*	browsebut_;
    uiToolButton*	attribbut_;
    uiToolButton*	segyhdrbut_;
    uiToolButton*	copybut_;
    uiToolButton*	man2dlinesbut_;
    uiToolButton*	mergecubesbut_;

    void		mergePush(CallBacker*);
    void		browsePush(CallBacker*);
    void		copyPush(CallBacker*);
    void		man2DPush(CallBacker*);
    void		manPS(CallBacker*);
    void		showAttribSet(CallBacker*);
    void		showSEGYHeader(CallBacker*);

    virtual void	mkFileInfo();
    virtual void	ownSelChg();
    od_int64		getFileSize(const char*,int&) const;
    void		setToolButtonProperties();
    const BrowserDef*	getBrowserDef() const;

};

#endif

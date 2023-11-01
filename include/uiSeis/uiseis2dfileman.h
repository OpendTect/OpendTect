#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uidialog.h"
class IOObj;
class uiListBox;
class uiTextEdit;
class uiToolButton;
class uiManipButGrp;
class Seis2DDataSet;
class uiSeisIOObjInfo;


mExpClass(uiSeis) uiSeis2DFileMan : public uiDialog
{ mODTextTranslationClass(uiSeis2DFileMan);
public:

			uiSeis2DFileMan(uiParent*,const IOObj&);
			~uiSeis2DFileMan();

    uiListBox*		getListBox( bool attrs )
			{ return linefld_; }
    uiManipButGrp*	getButGroup( bool attrs )
			{ return linegrp_; }

    Seis2DDataSet*	dataset_;
    uiSeisIOObjInfo*	objinfo_;

    mDeclInstanceCreatedNotifierAccess(uiSeis2DFileMan);

protected:

    void		fillLineBox();
    void		redoAllLists();
    void		lineSel(CallBacker*);
    void		removeLine(CallBacker*);
    void		mergeLines(CallBacker*);
    void		browsePush(CallBacker*);

    uiListBox*		linefld_;
    uiTextEdit*		infofld_;
    uiManipButGrp*	linegrp_;

    const bool		issidomain; //deprecated
    const bool		zistm;	//deprecated

};

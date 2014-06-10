#ifndef uiseis2dfileman_h
#define uiseis2dfileman_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne/Bert
 Date:          April 2002/Nov 2009
 RCS:           $Id$
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
{
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
    bool		rename(const char*,BufferString&);
    void		lineSel(CallBacker*);
    void		renameLine(CallBacker*);
    void		removeLine(CallBacker*);
    void		mergeLines(CallBacker*);
    void		browsePush(CallBacker*);
    void		extrFrom3D(CallBacker*);

    uiListBox*		linefld_;
    uiTextEdit*		infofld_;
    uiToolButton*	browsebut_;
    uiToolButton*	mkdefbut_;
    uiManipButGrp*	linegrp_;

    const bool		issidomain;
    const bool		zistm;

};


#endif


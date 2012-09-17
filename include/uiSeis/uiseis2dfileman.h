#ifndef uiseis2dfileman_h
#define uiseis2dfileman_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne/Bert
 Date:          April 2002/Nov 2009
 RCS:           $Id: uiseis2dfileman.h,v 1.7 2011/09/16 10:01:23 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
class IOObj;
class uiListBox;
class uiTextEdit;
class uiToolButton;
class uiManipButGrp;
class Seis2DLineSet;
class uiSeisIOObjInfo;


mClass uiSeis2DFileMan : public uiDialog
{
public:

			uiSeis2DFileMan(uiParent*,const IOObj&);
			~uiSeis2DFileMan();

    uiListBox*		getListBox( bool attrs )
			{ return attrs ? attrfld_ : linefld_; }
    uiManipButGrp*	getButGroup( bool attrs )
			{ return attrs ? attrgrp_ : linegrp_; }

    Seis2DLineSet*	lineset_;
    uiSeisIOObjInfo*	objinfo_;

    mDeclInstanceCreatedNotifierAccess(uiSeis2DFileMan);

protected:

    void		fillLineBox();
    void		redoAllLists();
    bool		rename(const char*,BufferString&);
    void		lineSel(CallBacker*);
    void		attribSel(CallBacker*);
    void		browsePush(CallBacker*);
    void		renameLine(CallBacker*);
    void		mergeLines(CallBacker*);
    void		removeAttrib(CallBacker*);
    void		renameAttrib(CallBacker*);
    void		extrFrom3D(CallBacker*);

    uiListBox*		linefld_;
    uiListBox*		attrfld_;
    uiTextEdit*		infofld_;
    uiToolButton*	browsebut_;
    uiManipButGrp*	linegrp_;
    uiManipButGrp*	attrgrp_;

    const bool		issidomain;
    const bool		zistm;

};


#endif

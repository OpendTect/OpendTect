#ifndef uivolprocbatchsetup_h
#define uivolprocbatchsetup_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2001
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uivolumeprocessingmod.h"
#include "uidialog.h"
#include "mmbatchjobdispatch.h"

class IOObj;

class uiBatchJobDispatcherSel;
class uiIOObjSel;
class uiPosSubSel;
class uiPushButton;
class uiVelocityDesc;


namespace VolProc 
{

class Chain;

mExpClass(uiVolumeProcessing) uiBatchSetup : public uiDialog
{ mODTextTranslationClass(uiBatchSetup);

public:
                        uiBatchSetup(uiParent*,const IOObj* setupsel=0);
                        ~uiBatchSetup();

protected:

    bool		prepareProcessing();
    bool		fillPar();
    bool		retrieveChain();

    uiIOObjSel*		setupsel_;
    uiPushButton*	editsetup_;
    uiPosSubSel*	possubsel_;
    uiIOObjSel*		outputsel_;
    Chain*		chain_;
    uiBatchJobDispatcherSel* batchfld_;

    void		setupSelCB(CallBacker*);
    void		editPushCB(CallBacker*);
    bool		acceptOK(CallBacker*);
};

} // namespace VolProc


namespace Batch
{

class VolMMProgDef : public MMProgDef
{
public:
			VolMMProgDef() : MMProgDef( "od_SeisMMBatch" )	{}
    virtual bool	isSuitedFor(const char*) const;
    virtual bool	canHandle(const JobSpec&) const;
    static const char*	sKeyNeedsFullVolYN()	{ return "NeedsFullVol"; }
};


} // namespace Batch

#endif

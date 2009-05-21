#ifndef uiclusterjobprov_h
#define uiclusterjobprov_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Raman K Singh
 Date:          May 2009
 RCS:           $Id: uiclusterjobprov.h,v 1.1 2009-05-21 07:12:37 cvsraman Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "multiid.h"

class InlineSplitJobDescProv;
class IOPar;
class uiGenInput;
class uiFileInput;
class uiLabel;


mClass uiClusterJobProv : public uiDialog
{
public:
    			uiClusterJobProv(uiParent* p,const IOPar& iop,
					 const char* prog,const char* parfnm);
			~uiClusterJobProv();

    static const char*	sKeySeisOutIDKey();
    static const char*	sKeyOutputID();

protected:

    InlineSplitJobDescProv*	jobprov_;
    IOPar&		iopar_;
    const char*		prognm_;
    BufferString	tempstordir_;

    uiGenInput*		nrinlfld_;
    uiLabel*		nrjobsfld_;
    uiFileInput*	parfilefld_;
    uiFileInput*	tmpstordirfld_;
    uiFileInput*	scriptdirfld_;
    uiFileInput*	masterscriptfld_;
    uiGenInput*		cmdfld_;

    void		nrJobsCB(CallBacker*);
    bool		acceptOK(CallBacker*);

    bool		createJobScripts(const char*);
    const char*		getOutPutIDKey() const;
    MultiID		getTmpID(const char*) const;
    bool		writeScriptFile(const char* scrfnm,const char* parfnm,
					const char* logfnm) const;
    bool		createMasterScript(const char*,const char*) const;
};

#endif

#ifndef uibatchlaunch_h
#define uibatchlaunch_h
/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          January 2002
 RCS:           $Id: uibatchlaunch.h,v 1.2 2002-04-25 14:51:35 bert Exp $
________________________________________________________________________

-*/


#include "uidialog.h"
#include "uidset.h"
class IOParList;
class uiFileInput;
class uiGenInput;
class uiLabeledComboBox;
class uiPushButton;



class uiGenBatchLaunch : public uiDialog
{
public:

			uiGenBatchLaunch(uiParent*,const UserIDSet&);

    const char*		getProg();
    IOParList*		getParList()		{ return parlist; }

protected:

    uiLabeledComboBox*	progfld;
    uiFileInput*	parfld;
    IOParList*		parlist;
    UserIDSet           prognms;

    bool		acceptOK(CallBacker*);
};



class uiBatchLaunch : public uiDialog
{
public:
			uiBatchLaunch(uiParent*,const IOParList&,
				      BufferString& hostnm,const char* prognm,
				      bool with_print_pars=false);
			~uiBatchLaunch();

protected:

    UserIDSet		opts;
    const IOParList&	iopl;
    BufferString	hostname;
    BufferString	progname;

    uiFileInput*	filefld;
    uiLabeledComboBox*	optfld;
    uiGenInput*		remfld;
    uiGenInput*		remhostfld;;

    bool		acceptOK(CallBacker*);
    bool		execRemote() const;
    void		optSel(CallBacker*);
    void		remSel(CallBacker*);
    int			selected();

};


class uiFullBatchDialog : public uiDialog
{
protected:

    			uiFullBatchDialog(uiParent*,const char* procprogname,
					  const char* wintxt,const char* mpn);

    BufferString	procprognm;
    BufferString	multiprognm;
    uiGroup*		uppgrp;

    virtual bool	prepareProcessing()	= 0;
    virtual bool	fillPar(IOPar&)		= 0;

    void		doButPush(CallBacker*);
    void		preFinalise(CallBacker*);

    bool		singLaunch(const IOParList&);
    bool		multiLaunch(const char*);
    bool		distrLaunch(CallBacker*,const char*);

    uiPushButton*	singmachbut;
    uiPushButton*	multimachbut;

};

#endif

#ifndef uibatchlaunch_h
#define uibatchlaunch_h
/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          January 2002
 RCS:           $Id: uibatchlaunch.h,v 1.3 2003-01-21 08:17:46 bert Exp $
________________________________________________________________________

-*/


#include "uidialog.h"
#include "uidset.h"
class IOParList;
class uiGenInput;
class uiFileInput;
class uiPushButton;
class uiLabeledComboBox;



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
				      const char* hostnm,const char* prognm,
				      bool with_print_pars=false);
			~uiBatchLaunch();

    void		setParFileName( const char* fnm ) { parfname = fnm; }

protected:

    UserIDSet		opts;
    const IOParList&	iopl;
    BufferString	hostname;
    BufferString	progname;
    BufferString	parfname;

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

    const BufferString	procprognm;
    const BufferString	multiprognm;
    BufferString	singparfname;
    BufferString	multiparfname;
    uiGroup*		uppgrp;

    virtual bool	prepareProcessing()	= 0;
    virtual bool	fillPar(IOPar&)		= 0;
    void		addStdFields();
    			//!< Needs to be called at end of constructor

    void		doButPush(CallBacker*);
    void		singTogg(CallBacker*);

    bool		singLaunch(const IOParList&,const char*);
    bool		multiLaunch(const char*);
    bool		distrLaunch(CallBacker*,const char*);
    bool		acceptOK(CallBacker*);

    uiGenInput*		singmachfld;
    uiFileInput*	parfnamefld;

    bool		redo_; //!< set to true only for re-start

};

#endif

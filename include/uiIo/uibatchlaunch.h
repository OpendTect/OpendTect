#ifndef uibatchlaunch_h
#define uibatchlaunch_h
/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          Jan 2002
 RCS:           $Id: uibatchlaunch.h,v 1.9 2003-11-06 11:51:40 arend Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "bufstringset.h"

#ifndef __win__
# define MULTI_MACHINE
# define HAVE_OUTPUT_OPTIONS
#endif

class IOPar;
class IOParList;
class uiGenInput;
class uiFileInput;
class uiPushButton;
class uiLabeledComboBox;

#ifdef HAVE_OUTPUT_OPTIONS
class uiBatchLaunch : public uiDialog
{
public:
			uiBatchLaunch(uiParent*,const IOParList&,
				      const char* hostnm,const char* prognm,
				      bool with_print_pars=false);

    void		setParFileName( const char* fnm ) { parfname = fnm; }

protected:

    BufferStringSet	opts;
    const IOParList&	iopl;
    BufferString	hostname;
    BufferString	progname;
    BufferString	parfname;
    BufferString	rshcomm;
    int			nicelvl;

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
#endif

class uiFullBatchDialog : public uiDialog
{
protected:

    			uiFullBatchDialog(uiParent*,const char* wintxt,
					  const char* procprognm=0,
					  const char* multiprocprognm=0);

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

#ifdef MULTI_MACHINE
    void		singTogg(CallBacker*);
#endif

    bool		singLaunch(const IOParList&,const char*);
    bool		multiLaunch(const char*);
    bool		distrLaunch(CallBacker*,const char*);
    bool		acceptOK(CallBacker*);

#ifdef MULTI_MACHINE
    uiGenInput*		singmachfld;
#endif
    uiFileInput*	parfnamefld;

    bool		redo_; //!< set to true only for re-start

};


class uiRestartBatchDialog : public uiFullBatchDialog
{
public:

    			uiRestartBatchDialog(uiParent*,const char* ppn=0,
					     const char* mpn=0);

protected:

    virtual bool	prepareProcessing()	{ return true; }
    virtual bool	fillPar(IOPar&)		{ return true; }

};

#endif

#ifndef uibatchlaunch_h
#define uibatchlaunch_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          Jan 2002
 RCS:           $Id: uibatchlaunch.h,v 1.19 2007-08-10 09:52:07 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "bufstringset.h"

#ifndef __win__
# define HAVE_OUTPUT_OPTIONS
#endif

class IOPar;
class uiGenInput;
class uiFileInput;
class uiPushButton;
class uiLabeledComboBox;
class uiLabeledSpinBox;

#ifdef HAVE_OUTPUT_OPTIONS
class uiBatchLaunch : public uiDialog
{
public:
			uiBatchLaunch(uiParent*,const IOPar&,
				      const char* hostnm,const char* prognm,
				      bool with_print_pars=false);
			~uiBatchLaunch();

    void		setParFileName(const char*);

protected:

    BufferStringSet	opts;
    IOPar&		iop;
    BufferString	hostname;
    BufferString	progname;
    BufferString	parfname;
    BufferString	rshcomm;
    int			nicelvl;

    uiFileInput*	filefld;
    uiLabeledComboBox*	optfld;
    uiGenInput*		remfld;
    uiGenInput*		remhostfld;;
    uiLabeledSpinBox*	nicefld;

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

    class Setup
    {
    public:
			Setup(const char* txt)
			    : wintxt_(txt)
			    , procprognm_("")
			    , multiprocprognm_("")
			    , modal_(true)
			{}

	mDefSetupMemb(BufferString,wintxt)
	mDefSetupMemb(BufferString,procprognm)
	mDefSetupMemb(BufferString,multiprocprognm)
	mDefSetupMemb(bool,modal)
    };

    			uiFullBatchDialog(uiParent*,const Setup&);

    const BufferString	procprognm;
    const BufferString	multiprognm;
    BufferString	singparfname;
    BufferString	multiparfname;
    uiGroup*		uppgrp;

    virtual bool	prepareProcessing()	= 0;
    virtual bool	fillPar(IOPar&)		= 0;
    void		addStdFields(bool forread=false);
    			//!< Needs to be called at end of constructor
    void		setParFileNmDef(const char*);

    void		doButPush(CallBacker*);

    void		singTogg(CallBacker*);

    bool		singLaunch(const IOPar&,const char*);
    bool		multiLaunch(const char*);
    bool		distrLaunch(CallBacker*,const char*);
    bool		acceptOK(CallBacker*);

    uiGenInput*		singmachfld;
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
    virtual bool	parBaseName() const	{ return 0; }

};

#endif

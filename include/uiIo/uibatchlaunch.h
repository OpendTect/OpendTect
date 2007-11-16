#ifndef uibatchlaunch_h
#define uibatchlaunch_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          Jan 2002
 RCS:           $Id: uibatchlaunch.h,v 1.20 2007-11-16 21:25:45 cvskris Exp $
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

    BufferStringSet	opts_;
    IOPar&		iop_;
    BufferString	hostname_;
    BufferString	progname_;
    BufferString	parfname_;
    BufferString	rshcomm_;
    int			nicelvl_;

    uiFileInput*	filefld_;
    uiLabeledComboBox*	optfld_;
    uiGenInput*		remfld_;
    uiGenInput*		remhostfld_;;
    uiLabeledSpinBox*	nicefld_;

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

    const BufferString	procprognm_;
    const BufferString	multiprognm_;
    BufferString	singparfname_;
    BufferString	multiparfname_;
    uiGroup*		uppgrp_;

    virtual bool	prepareProcessing()	= 0;
    virtual bool	fillPar(IOPar&)		= 0;
    void		addStdFields(bool forread=false,bool onlysinglemachine=false);
    			//!< Needs to be called at end of constructor
    void		setParFileNmDef(const char*);

    void		doButPush(CallBacker*);

    void		singTogg(CallBacker*);

    bool		singLaunch(const IOPar&,const char*);
    bool		multiLaunch(const char*);
    bool		distrLaunch(CallBacker*,const char*);
    bool		acceptOK(CallBacker*);

    uiGenInput*		singmachfld_;
    uiFileInput*	parfnamefld_;

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

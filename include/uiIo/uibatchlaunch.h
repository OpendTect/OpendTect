#ifndef uibatchlaunch_h
#define uibatchlaunch_h
/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          Jan 2002
 RCS:           $Id: uibatchlaunch.h,v 1.6 2003-04-25 14:03:47 bert Exp $
________________________________________________________________________

-*/


#include "uidialog.h"
#include "uidset.h"
class IOParList;
class uiGenInput;
class uiFileInput;
class uiPushButton;
class uiLabeledComboBox;


class uiBatchLaunch : public uiDialog
{
public:
			uiBatchLaunch(uiParent*,const IOParList&,
				      const char* hostnm,const char* prognm,
				      bool with_print_pars=false);
			~uiBatchLaunch();

    void		setParFileName( const char* fnm ) { parfname = fnm; }
    void		setLicFeat( int lf )		  { licfeat = lf; }
    			//!< Fill with a Licenser::Feat if necessary
    			//!< to get a certificate into IOPar

protected:

    UserIDSet		opts;
    const IOParList&	iopl;
    BufferString	hostname;
    BufferString	progname;
    BufferString	parfname;
    BufferString	rshcomm;
    int			licfeat;
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


class uiFullBatchDialog : public uiDialog
{
public:

    void		setLicFeat( int lf )		  { licfeat = lf; }
    			//!< See uiBatchLaunch::setLicFeat comment

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
    void		singTogg(CallBacker*);

    bool		singLaunch(const IOParList&,const char*);
    bool		multiLaunch(const char*);
    bool		distrLaunch(CallBacker*,const char*);
    bool		acceptOK(CallBacker*);

    uiGenInput*		singmachfld;
    uiFileInput*	parfnamefld;

    bool		redo_; //!< set to true only for re-start
    int			licfeat;

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

#ifndef uibatchlaunch_h
#define uibatchlaunch_h
/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          January 2002
 RCS:           $Id: uibatchlaunch.h,v 1.1 2002-01-03 17:17:59 nanne Exp $
________________________________________________________________________

-*/


#include "uidialog.h"
#include "uidset.h"

class IOParList;
class uiFileInput;
class uiGenInput;
class uiLabeledComboBox;



class uiGenBatchLaunch : public uiDialog
{
public:

			uiGenBatchLaunch(uiParent*,UserIDSet);

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


#endif

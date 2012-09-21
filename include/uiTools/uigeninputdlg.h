#ifndef uigeninputdlg_h
#define uigeninputdlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          May 2002
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uidialog.h"
#include "uigroup.h"
#include "datainpspec.h"
class uiGenInput;

/*!\brief specifies how to get input from user - for uiGenInputDlg */

mClass(uiTools) uiGenInputDlgEntry
{
public:
    			uiGenInputDlgEntry( const char* t,
					    DataInpSpec* s )
			    //!< DataInpSpec becomes mine
			: txt(t), spec(s?s:new StringInpSpec)
			, allowundef(false)			{}
			~uiGenInputDlgEntry()			{ delete spec; }

    BufferString	txt;
    DataInpSpec*	spec;
    bool		allowundef;

};


mClass(uiTools) uiGenInputGrp : public uiGroup
{
public:
			uiGenInputGrp(uiParent*,const char* dlgtitle,
					const char* fldtxt,DataInpSpec* s=0);
			    //!< DataInpSpec becomes mine
			uiGenInputGrp(uiParent*,const char* dlgtitle,
				      ObjectSet<uiGenInputDlgEntry>*);
			    //!< the ObjectSet becomes mine.
			~uiGenInputGrp()	{ deepErase(*entries); }

    const char*		text(int i=0);
    int			getIntValue(int i=0);
    float		getfValue(int i=0);
    double		getdValue(int i=0);
    bool		getBoolValue(int i=0);

    int			nrFlds() const		{ return flds.size(); }
    uiGenInput*		getFld( int idx=0 )	{ return flds[idx]; }

    bool		acceptOK(CallBacker*);
    NotifierAccess*	enterClose();
    			/*!<\returns notifier when a simple text field is
			             displayed. An eventual notifier will
				     trigger if the user presses enter is
				     pressed. */

protected:

    ObjectSet<uiGenInput>		flds;
    ObjectSet<uiGenInputDlgEntry>*	entries;

private:

    void		build();

};



/*!\brief dialog with only uiGenInputs */

mClass(uiTools) uiGenInputDlg : public uiDialog
{ 	
public:
			uiGenInputDlg(uiParent*,const char* dlgtitle,
					const char* fldtxt,DataInpSpec* s=0);
			    //!< DataInpSpec becomes mine
			uiGenInputDlg(uiParent*,const char* dlgtitle,
				      ObjectSet<uiGenInputDlgEntry>*);
			    //!< the ObjectSet becomes mine.
			~uiGenInputDlg()	{ delete group; }

    const char*		text(int i=0);
    int			getIntValue(int i=0);
    float		getfValue(int i=0);
    double		getdValue(int i=0);
    bool		getBoolValue(int i=0);

    int			nrFlds() const;
    uiGenInput*		getFld( int idx=0 );

protected:
    void		setEnterClose(CallBacker*);
    bool		acceptOK(CallBacker*);
    uiGenInputGrp*	group;
};


#endif


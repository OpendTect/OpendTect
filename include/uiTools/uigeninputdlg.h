#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          May 2002
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uidialog.h"
#include "uigroup.h"
#include "datainpspec.h"
#include "uistring.h"
class uiGenInput;

/*!\brief specifies how to get input from user - for uiGenInputDlg */

mExpClass(uiTools) uiGenInputDlgEntry
{ mODTextTranslationClass(uiGenInputDlgEntry);
public:
			uiGenInputDlgEntry( const uiString& t,
					    DataInpSpec* s )
			    : txt(t), spec(s?s:new StringInpSpec)
			    , allowundef(false)			{}
			    // DataInpSpec becomes mine
			~uiGenInputDlgEntry()			{ delete spec; }

    uiString		txt;
    DataInpSpec*	spec;
    bool		allowundef;

};


mExpClass(uiTools) uiGenInputGrp : public uiGroup
{ mODTextTranslationClass(uiGenInputGrp)
public:
			uiGenInputGrp(uiParent*,const char* grpname,
					uiString fldtxt,DataInpSpec* s=0);
			    //!< DataInpSpec becomes mine
			uiGenInputGrp(uiParent*,const char* grpname,
				      ObjectSet<uiGenInputDlgEntry>*);
			    //!< the ObjectSet becomes mine.
			~uiGenInputGrp()	{ deepErase(*entries); }

    const char*		text(int i=0);
    int			getIntValue(int i=0);
    float		getFValue(int i=0);
    double		getDValue(int i=0);
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

public:
    mDeprecated ("Use getFValue")
    float	getfValue(int i=0)	{ return getFValue(i); }
    mDeprecated ("Use getDValue")
    double	getdValue(int i=0)	{ return getDValue(i); }
};



/*!\brief dialog with only uiGenInputs */

mExpClass(uiTools) uiGenInputDlg : public uiDialog
{ mODTextTranslationClass(uiGenInputDlg)
public:
			uiGenInputDlg(uiParent*,const uiString& dlgtitle,
					uiString fldtxt,DataInpSpec* s=0);
			    //!< DataInpSpec becomes mine
			uiGenInputDlg(uiParent*,const uiString& dlgtitle,
				      ObjectSet<uiGenInputDlgEntry>*);
			    //!< the ObjectSet becomes mine.
			~uiGenInputDlg()	{}

    const char*		text(int i=0);
    int			getIntValue(int i=0);
    float		getFValue(int i=0);
    double		getDValue(int i=0);
    bool		getBoolValue(int i=0);

    int			nrFlds() const;
    uiGenInput*		getFld( int idx=0 );

protected:
    void		setEnterClose(CallBacker*);
    bool		acceptOK(CallBacker*) override;
    uiGenInputGrp*	group;

    mDeprecated		("Use getFValue")
    float		getfValue(int i=0)	{ return getFValue(i); }
    mDeprecated		("Use getDValue")
    double		getdValue(int i=0)	{ return getDValue(i); }

};


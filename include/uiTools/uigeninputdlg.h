#ifndef uigeninputdlg_h
#define uigeninputdlg_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          May 2002
 RCS:           $Id: uigeninputdlg.h,v 1.1 2002-05-28 08:40:58 bert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "datainpspec.h"
class uiGenInput;

/*!\brief specifies how to get input from user - for uiGenInputDlg */

class uiGenInputDlgEntry
{
public:
    			uiGenInputDlgEntry( const char* t,
					    DataInpSpec* s )
			    //!< DataInpSpec becomes mine
			: txt(t), spec(s?s:new StringInpSpec)
			, allowundef(false), allowinvalid(false) {}
			~uiGenInputDlgEntry()			{ delete spec; }

    BufferString	txt;
    DataInpSpec*	spec;
    bool		allowundef;
    bool		allowinvalid;

};


/*!\brief dialog with only uiGenInputs */

class uiGenInputDlg : public uiDialog
{ 	
public:
			uiGenInputDlg(uiParent*,const char* dlgtitle,
					const char* fldtxt,DataInpSpec* s=0);
			    //!< DataInpSpec becomes mine
			uiGenInputDlg(uiParent*,const char* dlgtitle,
				      ObjectSet<uiGenInputDlgEntry>*);
			    //!< the ObjectSet becomes mine.
			~uiGenInputDlg()	{ deepErase(*entries); }

    const char*		text(int i=0);
    int			getIntValue(int i=0);
    float		getfValue(int i=0);
    double		getValue(int i=0);
    bool		getBoolValue(int i=0);

    int			nrFlds() const		{ return flds.size(); }
    uiGenInput*		getFld( int idx=0 )	{ return flds[idx]; }

protected:

    ObjectSet<uiGenInput>		flds;
    ObjectSet<uiGenInputDlgEntry>*	entries;

    bool		acceptOK(CallBacker*);

private:

    void		build();

};


#endif

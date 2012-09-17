#ifndef uisimplemultiwell_h
#define uisimplemultiwell_h
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Jun 2010
 * ID       : $Id: uisimplemultiwell.h,v 1.2 2010/09/13 10:39:37 cvsranojay Exp $
-*/

#include "uidialog.h"
#include "bufstringset.h"
#include "multiid.h"
class IOObj;
class uiTable;
class uiGenInput;
class uiSMWCData;


mClass uiSimpleMultiWellCreate : public uiDialog
{
public:

			uiSimpleMultiWellCreate(uiParent*);

    bool		wantDisplay() const;
    const BufferStringSet& createdWellIDs() const	{ return crwellids_; }

protected:

    uiTable*		tbl_;
    uiGenInput*		velfld_;
    const bool		zinft_;
    int			overwritepol_;
    float		vel_;
    Interval<float>	defzrg_;
    BufferStringSet	crwellids_;

    bool		acceptOK(CallBacker*);

    void		rdFilePush(CallBacker*);
    bool		getWellCreateData(int,const char*,uiSMWCData&);
    bool		createWell(const uiSMWCData&,const IOObj&);
    IOObj*		getIOObj(const char*);
    void		addRow(const uiSMWCData&,int&);

    friend class	uiSimpleMultiWellCreateReadData;

};


#endif

#ifndef uisimplemultiwell_h
#define uisimplemultiwell_h
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Jun 2010
 * ID       : $Id: uisimplemultiwell.h,v 1.6 2012-08-03 13:01:20 cvskris Exp $
-*/

#include "uiwellmod.h"
#include "uidialog.h"
#include "bufstringset.h"

class IOObj;
class UnitOfMeasure;
class uiGenInput;
class uiSMWCData;
class uiTable;


mClass(uiWell) uiSimpleMultiWellCreate : public uiDialog
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
    const UnitOfMeasure* zun_;

    bool		acceptOK(CallBacker*);

    void		rdFilePush(CallBacker*);
    bool		getWellCreateData(int,const char*,uiSMWCData&);
    bool		createWell(const uiSMWCData&,const IOObj&);
    IOObj*		getIOObj(const char*);
    void		addRow(const uiSMWCData&,int&);

    friend class	uiSimpleMultiWellCreateReadData;

};

#endif


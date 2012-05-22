#ifndef uisimplemultiwell_h
#define uisimplemultiwell_h
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Jun 2010
 * ID       : $Id: uisimplemultiwell.h,v 1.3 2012-05-22 16:41:57 cvsnanne Exp $
-*/

#include "uidialog.h"
#include "bufstringset.h"
#include "multiid.h"

class IOObj;
class uiFileInput;
class uiGenInput;
class uiSMWCData;
class uiTable;
class uiTableImpDataSel;

namespace Table { class FormatDesc; }
namespace Well { class Data; }


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


mClass uiBulkWellImport : public uiDialog
{
public:
			uiBulkWellImport(uiParent*);
			~uiBulkWellImport();

protected:

    bool		acceptOK(CallBacker*);

    uiFileInput*	inpfld_;
    uiTableImpDataSel*	dataselfld_;

    ObjectSet<Well::Data>   wells_;
    Table::FormatDesc*	    fd_;
};


mClass uiBulkLogImport : public uiDialog
{
public:
			uiBulkLogImport(uiParent*);
			~uiBulkLogImport();

protected:

    bool		acceptOK(CallBacker*);

    uiFileInput*	inpfld_;
};


mClass uiBulkMarkerImport : public uiDialog
{
public:
			uiBulkMarkerImport(uiParent*);
			~uiBulkMarkerImport();
protected:

    bool		acceptOK(CallBacker*);

    uiFileInput*	inpfld_;
};

#endif

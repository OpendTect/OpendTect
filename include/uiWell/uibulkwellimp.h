#ifndef uibulkwellimp_h
#define uibulkwellimp_h
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Nanne Hemstra
 * DATE     : May 2012
 * ID       : $Id$
-*/

#include "uiwellmod.h"
#include "uidialog.h"

class uiFileInput;
class uiGenInput;
class uiTableImpDataSel;
class BufferString;
class BufferStringSet;

namespace Table { class FormatDesc; }
namespace Well { class Data; class MarkerSet; }


mExpClass(uiWell) uiBulkTrackImport : public uiDialog
{
public:
			uiBulkTrackImport(uiParent*);
			~uiBulkTrackImport();

protected:

    void		readFile(std::istream&);
    void		addD2T(BufferString&);
    void		write(BufferStringSet&);
    bool		acceptOK(CallBacker*);

    uiFileInput*	inpfld_;
    uiTableImpDataSel*	dataselfld_;
    uiGenInput*		velocityfld_;

    ObjectSet<Well::Data>   wells_;
    Table::FormatDesc*	    fd_;
};


mExpClass(uiWell) uiBulkLogImport : public uiDialog
{
public:
			uiBulkLogImport(uiParent*);
			~uiBulkLogImport();

protected:

    bool		acceptOK(CallBacker*);

    uiFileInput*	inpfld_;
};


mExpClass(uiWell) uiBulkMarkerImport : public uiDialog
{
public:
			uiBulkMarkerImport(uiParent*);
			~uiBulkMarkerImport();
protected:

    bool		acceptOK(CallBacker*);
    void		readFile(std::istream&,BufferStringSet&,
	    			 BufferStringSet&,ObjectSet<Well::MarkerSet>&);

    uiFileInput*	inpfld_;
    uiTableImpDataSel*	dataselfld_;

    Table::FormatDesc*	    fd_;
};

#endif


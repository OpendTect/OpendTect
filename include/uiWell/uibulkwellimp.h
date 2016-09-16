#pragma once
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Nanne Hemstra
 * DATE     : May 2012
-*/

#include "uiwellmod.h"
#include "uidialog.h"
#include "uistring.h"

class uiFileInput;
class uiGenInput;
class uiTableImpDataSel;
class BufferStringSet;

namespace Table { class FormatDesc; }
namespace Well  { class Data; class D2TModel; class MarkerSet; }
class D2TModelData;


mExpClass(uiWell) uiBulkTrackImport : public uiDialog
{ mODTextTranslationClass(uiBulkTrackImport);
public:
			uiBulkTrackImport(uiParent*);
			~uiBulkTrackImport();

protected:

    void		readFile(od_istream&);
    void		addD2T(uiString&);
    void		write(uiStringSet&);
    bool		acceptOK();

    uiFileInput*	inpfld_;
    uiTableImpDataSel*	dataselfld_;
    uiGenInput*		velocityfld_;

    ObjectSet<Well::Data> wells_;
    Table::FormatDesc*	fd_;
};


mExpClass(uiWell) uiBulkLogImport : public uiDialog
{ mODTextTranslationClass(uiBulkLogImport);
public:
			uiBulkLogImport(uiParent*);
			~uiBulkLogImport();

protected:

    bool		acceptOK();

    uiFileInput*	inpfld_;
    uiGenInput*		istvdfld_;
    uiGenInput*		udffld_;
};


mExpClass(uiWell) uiBulkMarkerImport : public uiDialog
{ mODTextTranslationClass(uiBulkMarkerImport);
public:
			uiBulkMarkerImport(uiParent*);
			~uiBulkMarkerImport();
protected:

    bool		acceptOK();
    void		readFile(od_istream&,BufferStringSet&,
				 ObjectSet<Well::MarkerSet>&);

    uiFileInput*	inpfld_;
    uiTableImpDataSel*	dataselfld_;

    Table::FormatDesc*	fd_;
};


mExpClass(uiWell) uiBulkD2TModelImport : public uiDialog
{ mODTextTranslationClass(uiBulkD2TModelImport);
public:
			uiBulkD2TModelImport(uiParent*);
			~uiBulkD2TModelImport();
protected:

    bool		acceptOK();
    void		readFile(od_istream&,
				 ObjectSet<D2TModelData>&);

    uiFileInput*	inpfld_;
    uiTableImpDataSel*	dataselfld_;

    Table::FormatDesc*	fd_;
};

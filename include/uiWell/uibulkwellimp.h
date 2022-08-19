#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwellmod.h"
#include "uidialog.h"
#include "uistring.h"
#include "welldata.h"

class uiFileInput;
class uiGenInput;
class uiProgressBar;
class uiTable;
class uiTableImpDataSel;
class BufferStringSet;

namespace Table { class FormatDesc; }
namespace Well	{ class D2TModel; class MarkerSet; class Track; }
class D2TModelData;
class DirData;


mExpClass(uiWell) uiBulkTrackImport : public uiDialog
{ mODTextTranslationClass(uiBulkTrackImport);
public:
			uiBulkTrackImport(uiParent*);
			~uiBulkTrackImport();

protected:

    void		readFile(od_istream&);
    void		write(uiStringSet&);
    bool		acceptOK(CallBacker*) override;

    uiFileInput*	inpfld_;
    uiTableImpDataSel*	dataselfld_;
    uiGenInput*		velocityfld_ = nullptr;

    RefObjectSet<Well::Data> wells_;
    ObjectSet<Well::Track> origtracks_;
    TypeSet<Interval<float> > mdrgs_;
    Table::FormatDesc*	fd_;
};


mExpClass(uiWell) uiBulkLogImport : public uiDialog
{ mODTextTranslationClass(uiBulkLogImport);
public:
			uiBulkLogImport(uiParent*);
			uiBulkLogImport(uiParent*,const BufferStringSet&);
			~uiBulkLogImport();

private:

    bool		acceptOK(CallBacker*) override;
    void		lasSel(CallBacker*);
    void		nameSelChg(CallBacker*);

    uiFileInput*	inpfld_;
    uiGenInput*		ismdfld_;
    uiGenInput*		udffld_;
    uiGenInput*		lognmfld_;
    uiGenInput*		welluwinmfld_;
    uiTable*		wellstable_;
};


mExpClass(uiWell) uiBulkMarkerImport : public uiDialog
{ mODTextTranslationClass(uiBulkMarkerImport);
public:
			uiBulkMarkerImport(uiParent*);
			~uiBulkMarkerImport();
protected:

    bool		acceptOK(CallBacker*) override;
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

    bool		acceptOK(CallBacker*) override;
    void		readFile(od_istream&,
				 ObjectSet<D2TModelData>&);

    uiFileInput*	inpfld_;
    uiTableImpDataSel*	dataselfld_;

    Table::FormatDesc*	fd_;
};


mExpClass(uiWell) uiBulkDirectionalImport : public uiDialog
{ mODTextTranslationClass(uiBulkDirectionalImport);
public:
			uiBulkDirectionalImport(uiParent*);
			~uiBulkDirectionalImport();
protected:

    void		finalizeCB(CallBacker*);
    void		reset();
    void		fileCB(CallBacker*);
    void		applyCB(CallBacker*);
    bool		acceptOK(CallBacker*) override;
    bool		readFile();
    void		fillTable();

    uiFileInput*	inpfld_;
    uiTableImpDataSel*	dataselfld_;
    uiTable*		wellstable_;
    uiProgressBar*	progressbar_;

    Table::FormatDesc*	fd_;
    ObjectSet<DirData>	dirdatas_;
    bool		fromuwi_ = false;
};

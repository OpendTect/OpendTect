#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2023 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uidialog.h"

#include "datapointset.h"

class uiASCIIFileInput;
class uiIOObjSel;
class uiIOObjSelGrp;


mExpClass(uiIo) uiDataPointSetSave : public uiDialog
{
mODTextTranslationClass(uiDataPointSetSave)
public:
    virtual			~uiDataPointSetSave();

protected:
				uiDataPointSetSave(uiParent*,const uiString&,
						   HelpKey,
						   const DataPointSet*);
    bool			save(const char* fnm,bool ascii);

    ConstRefMan<DataPointSet>	dps_;
};


mExpClass(uiIo) uiExportDataPointSet : public uiDataPointSetSave
{
mODTextTranslationClass(uiExportDataPointSet)
public:
				uiExportDataPointSet(uiParent*,
						const DataPointSet* =nullptr);
				~uiExportDataPointSet();

private:
    void			inpSelCB(CallBacker*);
    void			setOutputName(const char*);
    bool			acceptOK(CallBacker*) override;

    uiIOObjSel*			infld_				= nullptr;
    uiASCIIFileInput*		outfld_;
};



mExpClass(uiIo) uiSaveCrossplotData : public uiDataPointSetSave
{
mODTextTranslationClass(uiSaveCrossplotData)
public:
				uiSaveCrossplotData(uiParent*,
						    const DataPointSet&,
						    const char* type);
				~uiSaveCrossplotData();

    MultiID			getOutputKey() const;

private:
    bool			acceptOK(CallBacker*) override;

    BufferString		type_;
    uiIOObjSelGrp*		outfld_;
};

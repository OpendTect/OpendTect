#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigroup.h"
#include "iopar.h"
#include "uistring.h"

class HelpKey;
class uiGenInput;
class uiTableFormatDescFldsEd;
class uiTableFmtDescFldsParSel;
namespace Table { class FormatDesc; }

/*!\brief Table-based data import selection

  This class is meant to accept data structures describing table import/export
  as defined in General/tabledef.h. Resulting FormatDesc's selections can be
  used by a descendent of Table::AscIO.

  For example, the Wavelet import dialog creates a Table::FormatDesc object
  using WaveletAscIO::getDesc(), this class lets the user fill the FormatDesc's
  selection_, after which WaveletAscIO creates the imported object.

*/

mExpClass(uiTools) uiTableImpDataSel : public uiGroup
{ mODTextTranslationClass(uiTableImpDataSel);
public:
				uiTableImpDataSel(uiParent*,Table::FormatDesc&,
						  const HelpKey&);

    Table::FormatDesc&		desc()		{ return fd_; }
    const OD::String&		fmtName()	{ return fmtname_; }
				//!< May not be correct: it's the last selected

    void			updateSummary();
    bool			commit();
    int				nrHdrLines() const; //!< '-1' = variable

    Notifier<uiTableImpDataSel>	descChanged;

protected:

    Table::FormatDesc&		fd_;
    IOPar			storediop_;
    BufferString		fmtname_;

    uiGenInput*			hdrtypefld_;
    uiGenInput*			hdrlinesfld_;
    uiGenInput*			hdrtokfld_;
    uiTableFmtDescFldsParSel*	fmtdeffld_;
    friend class		uiTableFmtDescFldsParSel;
    friend class		uiTableFormatDescFldsEd;

    bool			commitHdr(bool witherror);
    void			typChg(CallBacker*);
    void			hdrChg(CallBacker*);
    void			descChg(CallBacker*);
    void			openFmt(CallBacker*);
};

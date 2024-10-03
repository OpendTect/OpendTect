#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseismod.h"

#include "bufstringset.h"
#include "uidialog.h"

class uiGenInput;
class uiGISExpStdFld;
class uiSeis2DLineChoose;
class uiSelLineStyle;


mExpClass(uiSeis) uiGISExport2DLines : public uiDialog
{
mODTextTranslationClass(uiGISExport2DLines);
public:
			uiGISExport2DLines(uiParent*,
					  const TypeSet<Pos::GeomID>* =nullptr);
			~uiGISExport2DLines();

    void		setSelected(const Pos::GeomID&);
    void		setSelected(const TypeSet<Pos::GeomID>&);

private:

    void		getCoordsForLine(const Pos::GeomID&,
					 BufferString& linenm,
					 TypeSet<Coord>&);
    bool		acceptOK(CallBacker*) override;

    uiSeis2DLineChoose* linesselfld_;
    uiSelLineStyle*	lsfld_;
    uiGISExpStdFld*	expfld_;

};

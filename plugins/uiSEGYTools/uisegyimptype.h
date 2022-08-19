#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uisegycommon.h"
#include "typeset.h"
#include "uigroup.h"

class uiComboBox;


namespace SEGY
{

mExpClass(uiSEGYTools) ImpType
{ mODTextTranslationClass(ImpType)
public:

			ImpType(bool isvsp=false);
			ImpType(Seis::GeomType);

    Seis::GeomType	geomType() const
			{ return (Seis::GeomType)(isVSP() ? types_[0]
							  : types_[tidx_]); }
    bool		isVSP() const
			{ return types_[tidx_] > Seis::LinePS; }
    bool		is2D() const	{ return Seis::is2D(geomType()); }
    bool		isPS() const	{ return Seis::isPS(geomType()); }

    void		setGeomType(Seis::GeomType);
    void		setIsVSP()
			{ tidx_ = types_.size()-1; }

    int			tidx_;
    TypeSet<int>	types_;

    uiString		dispText() const;
    void		fillPar(IOPar&) const;

private:

    void		init();

};

} // namespace SEGY


mExpClass(uiSEGYTools) uiSEGYImpType : public uiGroup
{ mODTextTranslationClass(uiSEGYImpType)
public:

			uiSEGYImpType(uiParent*,bool withvsp,
				      const uiString* lbltxt=0);

    const SEGY::ImpType& impType() const;
    void		setTypIdx(int);

    void		usePar(const IOPar&);

    Notifier<uiSEGYImpType> typeChanged;

protected:

    uiComboBox*		fld_;
    mutable SEGY::ImpType typ_;

    void		typChg( CallBacker* )	{ typeChanged.trigger(); }

};

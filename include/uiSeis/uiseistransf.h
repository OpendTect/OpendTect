#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseismod.h"

#include "uigroup.h"
#include "coordsystem.h"
#include "seisioobjinfo.h"
#include "seisselection.h"

class IOObj;
class Executor;
class Scaler;
class SeisResampler;
class uiGenInput;
class uiMultiZSeisSubSel;
class uiScaler;
class uiSeisSubSel;
class uiSeis2DSubSel;
class uiSeis3DSubSel;
namespace Seis		{ class SelData; }


mExpClass(uiSeis) uiSeisTransfer : public uiGroup
{ mODTextTranslationClass(uiSeisTransfer);
public:

    mExpClass(uiSeis) Setup : public Seis::SelSetup
    {
    public:
			Setup(Seis::GeomType);
			Setup(bool is2d,bool isps);
			Setup(const Seis::SelSetup&);
			~Setup();

	mDefSetupMemb(bool,withnullfill)	// false
	mDefSetupMemb(bool,withmultiz)		// false
    };

			uiSeisTransfer(uiParent*,const Setup&);
			~uiSeisTransfer();

    void		updateFrom(const IOObj&);

    Executor*		getTrcProc(const IOObj& from,const IOObj& to,
				   const char* executor_txt,
				   const uiString& work_txt,
				   int compnr=-1) const;

    void		setOutputHeader(const char*);
			//!< Only for the write translators which supports it
    void		setCoordSystem(const Coords::CoordSystem&,bool inp);
			//!< Only for the translators which supports it

    void		setSteering(bool);
    void		setInput(const IOObj&);
    void		setInput(const TrcKeyZSampling&);
    void		setSelectedLine(const char* lnm);
    void		setSelFldSensitive(bool yn);

    bool		is2D() const;
    bool		isSingleLine() const;
    BufferString	selectedLine() const;
    Seis::SelData*	getSelData() const;
    SeisResampler*	getResampler() const; //!< may return null
    Scaler*		getScaler() const;
    bool		removeNull() const;
    bool		extendTrcsToSI() const;
    bool		fillNull() const;
    int			nullTrcPolicy() const
			{ return removeNull() ? 0 : (fillNull() ? 2 : 1); }
    od_int64		expectedNrTraces() const;
    SeisIOObjInfo::SpaceInfo spaceInfo(int bps=4) const;
    const ZDomain::Info* zDomain() const;

    void		fillPar(IOPar&) const;
    void		fillSelPar(IOPar&) const;

    Notifier<uiSeisTransfer> selChange;

    static const char*	sKeyNullTrcPol()	{ return "Null trace policy"; }

protected:

    const uiSeisSubSel* selFld() const;
    const uiSeis2DSubSel* selFld2D() const;
    const uiSeis3DSubSel* selFld3D() const;
    uiSeisSubSel*	selFld();
    uiSeis2DSubSel*	selFld2D();
    uiSeis3DSubSel*	selFld3D();

private:

    void		initGrpCB(CallBacker*);
    void		selChangeCB(CallBacker*);
    void		updSteerCB(CallBacker*);
    Pos::GeomID		curGeomID() const;

    uiSeisSubSel*	selfld_ = nullptr;
    uiMultiZSeisSubSel* multizselfld_ = nullptr;
    uiScaler*		scalefld_;
    uiGenInput*		remnullfld;
    uiGenInput*		trcgrowfld_ = nullptr;

    Setup		setup_;
    bool		issteer_ = false;
    BufferString	outheader_;
    ConstRefMan<Coords::CoordSystem> inpcrs_;
    ConstRefMan<Coords::CoordSystem> outpcrs_;

public:
    void		showSubselFld(bool yn);

    mDeprecated("Use SeisTrcTranslator::setGeomID on 'to'")
    Executor*		getTrcProc(const IOObj& from,const IOObj& to,
				   const char* executor_txt,
				   const uiString& work_txt,
				   const char* linenm2d_overrule) const;


};

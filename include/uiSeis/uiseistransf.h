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
class Scaler;
class Executor;
class uiScaler;
class uiGenInput;
class uiSeisSubSel;
class SeisResampler;
class uiSeis2DSubSel;
class uiSeis3DSubSel;
namespace Seis		{ class SelData; }


mExpClass(uiSeis) uiSeisTransfer : public uiGroup
{ mODTextTranslationClass(uiSeisTransfer);
public:

    mExpClass(uiSeis) Setup : public Seis::SelSetup
    {
    public:
			Setup( Seis::GeomType gt )
			    : Seis::SelSetup(gt)
			    , withnullfill_(false)	{}
				    //!< 'add null traces' (3D)
			Setup( const Seis::SelSetup& sss )
			    : Seis::SelSetup(sss)
			    , withnullfill_(false)	{}
			Setup( bool _is2d, bool _isps )
			    : Seis::SelSetup(_is2d,_isps)
			    , withnullfill_(false)	{}

	mDefSetupMemb(bool,withnullfill)
    };

			uiSeisTransfer(uiParent*,const Setup&);
			~uiSeisTransfer();

    void		updateFrom(const IOObj&);

    Executor*		getTrcProc(const IOObj& from,const IOObj& to,
				   const char* executor_txt,
				   const uiString& work_txt,
				   int compnr=-1) const;

    uiSeisSubSel*	selfld;
    uiScaler*		scalefld_;
    uiGenInput*		remnullfld;
    uiGenInput*		trcgrowfld_ = nullptr;

    uiSeis2DSubSel*	selFld2D(); //!< null when not 2D
    uiSeis3DSubSel*	selFld3D(); //!< null when not 3D

    void		setOutputHeader(const char*);
			//!< Only for the write translators which supports it
    void		setCoordSystem(const Coords::CoordSystem&,bool inp);
			//!< Only for the translators which supports it

    void		setSteering(bool);
    void		setInput(const IOObj&);
    Seis::SelData*	getSelData() const;
    SeisResampler*	getResampler() const; //!< may return null

    Scaler*		getScaler() const;
    bool		removeNull() const;
    bool		extendTrcsToSI() const;
    bool		fillNull() const;
    int			nullTrcPolicy() const
			{ return removeNull() ? 0 : (fillNull() ? 2 : 1); }

    void		fillPar(IOPar&) const;
    static const char*	sKeyNullTrcPol()	{ return "Null trace policy"; }

    SeisIOObjInfo::SpaceInfo spaceInfo(int bps=4) const;

protected:

    Setup		setup_;
    bool		issteer_ = false;
    BufferString	outheader_;
    ConstRefMan<Coords::CoordSystem> inpcrs_;
    ConstRefMan<Coords::CoordSystem> outpcrs_;

    void		updSteer(CallBacker*);
    Pos::GeomID		curGeomID() const;

public:
    void		showSubselFld(bool showselfld);

    mDeprecated("Use SeisTrcTranslator::setGeomID on 'to'")
    Executor*		getTrcProc(const IOObj& from,const IOObj& to,
				   const char* executor_txt,
				   const uiString& work_txt,
				   const char* linenm2d_overrule) const;


};

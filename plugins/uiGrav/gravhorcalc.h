#ifndef gravhorcalc_h
#define gravhorcalc_h
/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Mar 2010
 RCS:		$Id$
________________________________________________________________________

*/

#include "uigravmod.h"
#include "executor.h"
#include "dbkey.h"
#include "grav.h"
class ZAxisTransform;
namespace EM { class Horizon3D; }


namespace Grav
{


mClass(uiGrav) HorCalc : public ::Executor
{ mODTextTranslationClass(HorCalc);
public:

    mExpClass(uiGrav) Setup
    { mODTextTranslationClass(Setup);
    public:
			Setup( const DBKey& calcmid )
			    : calcid_(calcmid)		{}

	mDefSetupMemb(DBKey,calcid)
	mDefSetupMemb(DBKey,topid)
	mDefSetupMemb(DBKey,botid)
	mDefSetupMemb(BufferString,denattr)
    };

			HorCalc(const DBKey&,const DBKey* top=0,
				const DBKey* bot=0,float ang=1);
			~HorCalc();

    void		setCutOffAngle( float a )	{ cutoffangle_ = a; }
    void		setVelModel( const DBKey& m )	{ velmid_ = m; }

    uiString		message() const		{ return msg_; }
    uiString		nrDoneText() const;
    od_int64		nrDone() const			{ return nrdone_; }
    od_int64		totalNr() const			{ return totnr_; }
    int			nextStep();

protected:

    float		cutoffangle_;
    DBKey		velmid_;

    EM::Horizon3D*	calchor_;
    EM::Horizon3D*	tophor_;
    EM::Horizon3D*	bothor_;
    ZAxisTransform*	ztransf_;

    uiString		msg_;
    od_int64		totnr_;
    od_int64		nrdone_;

    int			doLoadStep();

};

} // namespace Grav


#endif


#ifndef posimpexppars_h
#define posimpexppars_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Aug 2009
 RCS:		$Id: posimpexppars.h,v 1.1 2009-08-25 10:09:29 cvsbert Exp $
________________________________________________________________________

-*/

#include "position.h"
#include "callback.h"
class IOPar;


/* Parameters to apply on import (add or multiply), and on export (subtract
   or divide).

   On import, the 'scale' is applied before the 'offset', hence offset must be
   in units of the scaled values. On export, the offset subtracted first (of
   course).
 
 */

class PosImpExpPars : public CallBacker
{
    static const PosImpExpPars& SVY();
    void		refresh()		{ getSVY().getFromSI(); }

    int			binidScale() const	{ return binidscale_; }
    BinID		binidOffset() const	{ return binidoffs_; }
    int			trcnrScale() const	{ return trcnrscale_; }
    int			trcnrOffset() const	{ return trcnroffs_; }
    double		coordScale() const	{ return coordscale_; }
    Coord		coordOffset() const	{ return coordoffs_; }
    float		zScale() const		{ return zscale_; }
    float		zOffset() const		{ return zoffs_; }
    float		offsScale() const	{ return offsscale_; }
    float		offsOffset() const	{ return offsoffs_; }

    void		adjustBinID(BinID&) const;
    void		adjustTrcNr(int&) const;
    void		adjustCoord(Coord&) const;
    void		adjustZ(float&) const;
    void		adjustOffset(float&) const;

protected:

    int			binidscale_;
    BinID		binidoffs_;
    int			trcnrscale_;
    int			trcnroffs_;
    double		coordscale_;
    Coord		coordoffs_;
    float		zscale_;
    float		zoffs_;
    float		offsscale_;
    float		offsoffs_;

public:

    			PosImpExpPars()			{ clear(); }
    void		clear()
			{
			    binidscale_ = 1;	binidoffs_ = BinID(0,0);
			    trcnrscale_ = 1;	trcnroffs_ = 0;
			    coordscale_ = 1;	coordoffs_ = Coord(0,0);
			    zscale_ = 1;	zoffs_ = 0;
			    offsscale_ = 1;	offsoffs_ = 0;
			}

    static PosImpExpPars& getSVY();
    void		usePar(const IOPar&);
    void		getFromSI();
    void		survChg( CallBacker* )	{ refresh(); }

    static const char*	sKeyBase()		{ return "Seis.ImpExp"; }
    static const char*	sKeyOffset();		// sKey::Offset
    static const char*	sKeyScale();		// sKey::Scale
    static const char*	sKeyBinID()		{ return "BinID"; }
    static const char*	sKeyTrcNr();		//!< sKey::TraceNr
    static const char*	sKeyCoord()		{ return "Coord"; }
    static const char*	sKeyZ()			{ return "Z"; }

    static const char*	fullKey(const char*,bool true_is_scale_else_offs);

};


#endif

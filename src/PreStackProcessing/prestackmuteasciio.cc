/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		June 2008
 RCS:		$Id: prestackmuteasciio.cc,v 1.1 2008-06-23 06:49:31 cvsumesh Exp $
________________________________________________________________________

-*/

#include "prestackmuteasciio.h"
#include "tabledef.h"
#include "prestackmutedef.h"
#include "unitofmeasure.h"

using namespace PreStack;

static const char* extrapoltypes[] = {"Linear","Poly","Snap",0};

Table::FormatDesc* MuteAscIO::getDesc()
{
    Table::FormatDesc* fd = new Table::FormatDesc("Mute");
    const char* unitstr = SI().zIsTime() ? "Milliseconds"
	    			           : SI().zInFeet() ? "Feet" : "Meter";

    Table::TargetInfo* posinfo = new Table::TargetInfo("X/Y", FloatInpSpec(),
	    					       Table::Required );
    posinfo->form(0).add( FloatInpSpec());
    posinfo->add(posinfo->form(0).duplicate("Inl/Crl"));

    fd->bodyinfos_ += posinfo;
    fd->bodyinfos_ += new Table::TargetInfo("Offset", FloatInpSpec(),
	    				    Table::Required );
    Table::TargetInfo* bti = new Table::TargetInfo("Z Values", FloatInpSpec(),
	    				    Table::Required,
					    PropertyRef::surveyZType());
    bti->selection_.unit_ = UoMR().get( unitstr );
    fd->bodyinfos_ += bti;

    return fd;
}

float MuteAscIO::getUdfVal()
{
    if( !getHdrVals(strm_) )
	return mUdf(float);

    return getfValue( 0 );
}

bool MuteAscIO::isXY() const
{
    const Table::TargetInfo* xinfo = fd_.bodyinfos_[0];
    if(!xinfo) return false;

    const int sel = xinfo->selection_.form_;
    return !sel;
}

int MuteAscIO::getMuteDef( MuteDef& mutedef, bool extrapol, 
			   PointBasedMathFunction::InterpolType iptype)
{
    bool isxy = isXY();
    int ret = getNextBodyVals( strm_ );
    
    PointBasedMathFunction* pointbasedmathfunc = 0;
    
    while( ret > 0 )
    {
	BinID binid;
	if(isxy)
	{
	    Coord coord(getfValue(0),getfValue(1));
	    binid = SI().transform(coord);
	}
	else // its inl/crl
	{
   	    binid.inl = getIntValue(0);
   	    binid.crl = getIntValue(1);
	}	 
	int binidpos = mutedef.indexOf(binid);
	if(binidpos < 0)
	{
	    pointbasedmathfunc = new PointBasedMathFunction(iptype);
	    mutedef.add( pointbasedmathfunc, binid );
	}
	else
	{
	    pointbasedmathfunc = &mutedef.getFn(binidpos);
	}
   
	pointbasedmathfunc->add(getfValue(2),getfValue(3));
   
	ret = getNextBodyVals( strm_);
    }

    return ret;
}

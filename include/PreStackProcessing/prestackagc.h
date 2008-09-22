#ifndef prestackagc_h
#define prestackagc_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		April 2005
 RCS:		$Id: prestackagc.h,v 1.5 2008-09-22 13:10:42 cvskris Exp $
________________________________________________________________________


-*/

#include "ranges.h"
#include "prestackprocessor.h"

namespace PreStack
{


class AGC : public Processor
{
public:
    static void			initClass();
    static Processor*		createFunc();
				AGC();

    bool			prepareWork();

    void			setWindow(const Interval<float>&);
    const Interval<float>&	getWindow() const;
    void			getWindowUnit(BufferString&,
	    				      bool withparens) const;

    void			setLowEnergyMute(float fraction);
    float			getLowEnergyMute() const;

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);

    static const char*		sName()			{ return "AGC"; }
    static const char*		sKeyWindow()		{ return "Window"; }
    static const char*		sKeyMuteFraction()	{return "Mutefraction";}

protected:
    bool			doWork(od_int64,od_int64,int);

    Interval<float>		window_;
    Interval<int>		samplewindow_;
    float			mutefraction_;
};


}; //namespace

#endif

#ifndef prestackeventsapimgr_h
#define prestackeventsapimgr_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		March 2007
 RCS:		$Id$
________________________________________________________________________


-*/

#include "sets.h"

class BinID;

namespace PreStack { class EventManager; }
namespace Vel { class Picks; }

namespace PreStack
{

/*!
\brief API Manager for PreStack Events.
*/

mClass(PreStackProcessing) EventsAPIMgr
{
public:
    		EventsAPIMgr();
		~EventsAPIMgr();

    int		setSurvey( const char* dataroot, const char* survey );
    float	inlDistance() const;
    float	crlDistance() const;

    int		openReader(const char* reference);
    void	closeReader(int handle);

    int		getRanges(int handle,int& firstinl,int& lastinl,int& inlstep,
			  int& firstcrl,int& lastcrl,int& crlstep) const;
    int		getNextCDP(int handle, int previnl, int prevcrl,
			   int& nextinl, int& nextcrl );
    int		moveReaderTo(int handle,int inl,int crl);

    int		getNrEvents(int handle) const;

    int		getEventSize(int handle,int eventindex) const;

    void	getEvent(int handle,int eventindex,float* offsets,float* angles,
		         float* depths, float* quality ) const;

    void	getDip(int handle,int eventindex,
	    	       float& inldip,float& crldip);
    void	getQuality(int handle, int eventindex,float& weight) const;
    int		getHorizonID(int handle,int eventindex,int& horid) const;

    static EventsAPIMgr& getMgr();

protected:

    ObjectSet<Vel::Picks>		velpicks_;
    ObjectSet<PreStack::EventManager>	events_;
    ObjectSet<BinIDValueSet>		locations_;
    TypeSet<int>			ids_;
    TypeSet<BinID>			curpos_;
};



}; //namespace


#endif

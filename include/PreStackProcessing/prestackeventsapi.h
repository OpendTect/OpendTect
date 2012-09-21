#ifndef prestackeventsapi_h
#define prestackeventsapi_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		March 2007
 RCS:		$Id$
________________________________________________________________________


-*/
#include "prestackprocessingmod.h"
#include "commondefs.h"

extern "C" {

int mGlobal(PreStackProcessing) dGBPreStackEventsSetSurvey( const char* dataroot, const char* survey );
//!<\returns -1 on failure

float mGlobal(PreStackProcessing) dGBPreStackEventsGetInlDistance();
//!<returns the distance on a crossline between two inlines numbers.

float mGlobal(PreStackProcessing) dGBPreStackEventsGetCrlDistance();
//!<returns the distance on a inline between two crossline numbers.


int mGlobal(PreStackProcessing) dGBPreStackEventsOpenReader( const char* reference );
//!<\returns handle, or -1 if failure

void mGlobal(PreStackProcessing) dGBPreStackEventsCloseReader( int handle );


int mGlobal(PreStackProcessing) dGBPreStackEventsGetRanges(int handle, int& firstinl, int& lastinl,int& inlstep,
	 	          int& firstcrl, int& lastcrl, int& crlstep);
//!<\note There is not guarantee that all cdps in the sampling
//!<	  are present, but if you traverse all these cdps, you are
//!<	  guaranteed to get all available data.

int mGlobal(PreStackProcessing) dGBPreStackEventsGetNextCDP( int handle, int previnl, int prevcrl,
			    int& nextinl, int& nextxrl );
//!<Enables traversal of all data. At start with previnl=-1, prevcrl=-1,
//!<and the first cdp will be return. Next time, put in the current cdp, and
//!<the next cdp will be returned.
//!<|return -1 on error, 0 on no more cdps and 1 on success.


int mGlobal(PreStackProcessing) dGBPreStackEventsMoveReaderTo( int handle, int inl, int crl );
//!<\returns -1 on failure, 0 on no data present, 1 on success


int mGlobal(PreStackProcessing) dGBPreStackEventsGetNrEvents( int handle );
//!<\returns number of picks available at current position

int mGlobal(PreStackProcessing) dGBPreStackEventsGetEventSize( int handle, int eventindex );
//!<\param eventindex goes from 0 to getNrEvents() -1
//!<\returns number length of the pick

void mGlobal(PreStackProcessing) dGBPreStackEventsGetEvent( int handle, int eventindex, float* offsets,
			   float* angles, float* depths,
       			   float* weights );
//!<\param eventindex goes from 0 to getNrEvents() -1
//!<\param offsets is either zero or a pointer to an array with at
//!<               least getEventSize number of floats
//!<\param angles is either zero or a pointer to an array with at
//!<               least getEventSize number of floats
//!<\param depths is either zero or a pointer to an array with at
//!<               least getEventSize number of floats

void mGlobal(PreStackProcessing) dGBPreStackEventsGetDip( int handle, int eventindex,
			 float& inldip, float& crldip );
//!<\param inldip dip when going in the direction of increasing
//!<		  inline numbers
//!<\param crldip dip when going in the direction of increasing
//!<		  crossline numbers

void mGlobal(PreStackProcessing) dGBPreStackEventsGetEventWeight( int handle, int eventindex, float& weight );

int mGlobal(PreStackProcessing) dGBPreStackEventsGetHorizonID( int handle, int eventindex, int& horid );
//!<\returns 1 if the pick has an identifier for the (post stack) horizon thats
//!<	     was used to track the pick into offset domain. Otherwise,
//!<	     it returns zero. The eventual horizon id is set in the horid
//!<	     parameter.

};


#endif


/**
 * @file		leap.h
 * @brief	Minimal Leap Motion API wrapper. Tested with Ultraleap 5.17.1
 *
 * @author	Achille Peternier (C) SUPSI [achille.peternier@supsi.ch]
 */
#pragma once



//////////////
// #INCLUDE //
//////////////

   // Leap Motion SDK:
   #include <LeapC.h>



////////////////
// CLASS Leap //
////////////////

/**
 * @brief Leap Motion wrapper. 
 */
class ENG_API Leap
{	
//////////
public: //
//////////
     	
	// Const/dest:	 
   Leap();	 
	~Leap();	 	      

   // Init/free:
   bool init();
   bool free();

   // Polling:
   bool update();
   const LEAP_TRACKING_EVENT *getCurFrame() const;
   
   		 
///////////	 
private:	//
///////////			

   // Leap Motion:
   LEAP_CONNECTION connection;
   LEAP_DEVICE_REF leapDevice;
   LEAP_TRACKING_EVENT curFrame;
   signed long long lastFrameId;
};

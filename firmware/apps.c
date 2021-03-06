/*! \file apps.c
  \brief Application manager.

  This module manages the different applications, and the switching
  between them.
 */

#include <msp430.h>
#include <stdio.h>

#include "api.h"

#include "applist.h"

//! We begin on the clock.
static int appindex=DEFAULTAPP, idlecount=0;

//! Every 3 minutes we return to the clock unless this is called.
void app_cleartimer(){
  idlecount=0;
}

//! Renders the current app to the screen.
void app_draw(){
  static int lastmin=0;;
  
  void (*tocall)(void)=apps[appindex].draw;

  //If we go three minutes without action, return to main screen.
  if(lastmin!=RTCMIN){
    lastmin=RTCMIN;
    idlecount++;
  }
  if(idlecount>3){
    app_cleartimer();
    app_forcehome();
  }
  
  //Call the cap if it exists, or switch to the clock if we're at the
  //end of the list.
  if(tocall)
    tocall();
  else
    app_forcehome();
  return;
}

//! Force return to the home app.
void app_forcehome(){
  //First we try to exit politely.
  if(apps[appindex].exit)
    apps[appindex].exit();

  //And force it if that doesn't work.
  appindex=0;
  apps[appindex].init();
}

//! Initializes the set of applications.
void app_init(){
  void (*tocall)(void)=apps[appindex].init;
  if(tocall)
    tocall();
  else
    appindex=0;

  return;
}

//! Move to the next application if the current allows it.
void app_next(){
  void (*tocall)(void)=apps[appindex].draw;
  
  //Clear the 3-minute timer when we switch apps.  This is also
  //cleared by keypresses.
  app_cleartimer();

  /* First we ask the current app if it will allow the transaction by
     calling its exit() routine.  Zero or a null function pointer allow
     for the transition, but non-zero indicates that the transition has
     been cancelled.  For example, this is done by the RPN calculator
     when the item on the stack is not zero.
  */
  //Return if there is an exit function and it returns non-zero.
  if(apps[appindex].exit && apps[appindex].exit())
    return;

  tocall=apps[++appindex].draw;
  if(!tocall)
    appindex=0;

  //Initialize the new application.
  apps[appindex].init();
  return;
}

//! Provide an incoming packet.
void app_packetrx(uint8_t *packet, int len){
  /* In monitor mode, we forward the packet to the monitor, rather
     than to the application.
   */
  if(uartactive){
    monitor_packetrx(packet,len);
    return;
  }

  /* Otherwise, we send it to the active application, but only if that
     application has a handler.
   */
  if(!apps[appindex].packetrx){
    printf("No packet RX handler for %s.",
	   apps[appindex].name);
    return;
  }

  apps[appindex].packetrx(packet,len);
}

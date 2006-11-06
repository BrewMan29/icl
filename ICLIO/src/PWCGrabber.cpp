#include "PWCGrabber.h"


#ifdef __APPLE__

#include "ICLcc.h"
#include "Img.h"

#else

#include "ICLcc.h"

#include <stdio.h> 
#include <errno.h>
#include <sys/types.h>
#include <linux/videodev.h> 
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>
#include <string.h> 

#endif

#ifndef __APPLE__

#define PWC_DEBUG(X,S) if(X) {printf("pwc-debug:[%s]\n",S); return false;}
#define PWC_DEBUG_CALL(X,S) if(X<0){printf("pwc-debug-call:[%s]\n",S); return false; }

namespace icl {

// {{{ PWC-IOCTL

#ifndef PWC_IOCTL_H
#define PWC_IOCTL_H

/* (C) 2001 Nemosoft Unv.    webcam@smcc.demon.nl
   
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/*
  Changes
  2001/08/03  Alvarado   Added ioctl constants to access methods for 
  changing white balance and red/blue gains
 */

/* These are private ioctl() commands, specific for the Philips webcams.
  They contain functions not found in other webcams, and settings not
  specified in the Video4Linux API. 
   
   The #define names are built up like follows:
  VIDIOC   VIDeo IOCtl prefix
  PWC    Philps WebCam
  G           optional: Get
  S           optional: Set
  ...  the function
 */




/* The frame rate is encoded in the video_window.flags parameter using
  the upper 16 bits, since some flags are defined nowadays. The following
  defines provide a mask and shift to filter out this value.
   
  In 'Snapshot' mode the camera freezes its automatic exposure and colour 
  balance controls.
 */
#define PWC_FPS_SHIFT   16
#define PWC_FPS_MASK    0x00FF0000
#define PWC_FPS_FRMASK    0x003F0000
#define PWC_FPS_SNAPSHOT  0x00400000


  /* pwc_whitebalance.mode values */
#define PWC_WB_INDOOR   0
#define PWC_WB_OUTDOOR    1
#define PWC_WB_FL   2
#define PWC_WB_MANUAL   3
#define PWC_WB_AUTO   4

/**
   * Used with VIDIOCPWC[SG]AWB (Auto White Balance). 
   * Set mode to one of the PWC_WB_* values above.
   * *red and *blue are the respective gains of these colour components inside 
   * the camera; range 0..65535
   * When mode == PWC_WB_MANUAL, manual_red and manual_blue are set or read; 
   * otherwise undefined.
   *  read_red and read_blue are read-only.
 */   
  struct pwc_whitebalance
  {
  ///mode 
    int mode; 
  /// R/W
    int manual_red, manual_blue;
  /// R/O
    int read_red, read_blue;
  };



///Used with VIDIOCPWC[SG]LED 
  struct pwc_leds
  {
    /// Led on-time; range = 0..25000 */
    int led_on;   

    /// Led off-time; range = 0..25000  */
    int led_off;      
  };



  /* Restore user settings */
#define VIDIOCPWCRUSER    _IO('v', 192)
  /* Save user settings */
#define VIDIOCPWCSUSER    _IO('v', 193)
  /* Restore factory settings */
#define VIDIOCPWCFACTORY  _IO('v', 194)

 /* You can manipulate the compression factor. A compression preference of 0
  means use uncompressed modes when available; 1 is low compression, 2 is
  medium and 3 is high compression preferred. Of course, the higher the
  compression, the lower the bandwidth used but more chance of artefacts
  in the image. The driver automatically chooses a higher compression when
  the preferred mode is not available.
 */
  /* Set preferred compression quality (0 = uncompressed, 3 = highest compression) */
#define VIDIOCPWCSCQUAL   _IOW('v', 195, int)
  /* Get preferred compression quality */
#define VIDIOCPWCGCQUAL   _IOR('v', 195, int)

  /* Set AGC (Automatic Gain Control); int < 0 = auto, 0..65535 = fixed */
#define VIDIOCPWCSAGC   _IOW('v', 200, int)
  /* Get AGC; int < 0 = auto; >= 0 = fixed, range 0..65535 */
#define VIDIOCPWCGAGC   _IOR('v', 200, int)
  /* Set shutter speed; int < 0 = auto; >= 0 = fixed, range 0..65535 */
#define VIDIOCPWCSSHUTTER _IOW('v', 201, int)

  /* Color compensation (Auto White Balance) */
#define VIDIOCPWCSAWB           _IOW('v', 202, struct pwc_whitebalance)
#define VIDIOCPWCGAWB           _IOR('v', 202, struct pwc_whitebalance)

  /* Turn LED on/off ; int range 0..65535 */
#define VIDIOCPWCSLED           _IOW('v', 205, struct pwc_leds)
  /* Get state of LED; int range 0..65535 */
#define VIDIOCPWCGLED           _IOR('v', 205, struct pwc_leds)

#endif
  // }}}
// {{{ Global variables
int                     usbvflg_verbosity=0; //if verbosity>0 more messages will be printed

int                     usbvflg_fd[]={-1,-1,-1,-1}; /* one filedescriptor for each device */
int                     usbvflg_opencount[]={0,0,0,0}; /* number of objects using device */
int                     usbvflg_maxwidth[4]; /* maximal width (obtained from hardware) */
struct video_mbuf       usbvflg_vmbuf[4];
struct video_mmap       usbvflg_params[4][2];
struct video_channel    usbvflg_vchan[4];
icl8u           *usbvflg_buf[]={NULL,NULL,NULL,NULL};
int                     usbvflg_useframe[]={-1,-1,-1,-1};
double                  usbvflg_starttime; 
double                  usbvflg_diff_time=0;

pthread_t               usb_grabber_thread[4];
pthread_mutex_t         usb_frame_mutex[4];
pthread_mutex_t         usb_semph_mutex[4];
sem_t                   usb_new_pictures[4];  

double                  usb_last_time[4];
int                     usb_image_widths[4];
int                     usb_image_heights[4];
// }}}
// {{{ usb_grabber_funct

bool usb_grabber_funct(void *data ){
  int device=*(int*)data;
  int use_frame=0;
  int semval;
  
  long  usec;
  struct timeval zeit;
  struct timespec delay;
  
  //init
   gettimeofday(&zeit,NULL);
   usec=zeit.tv_usec;
   delay.tv_sec = 0;
   delay.tv_nsec = 1500000; //entspricht 15msec  (Kamera liefert max. alle 40msec ein Bild)
 

   //blockieren
   pthread_mutex_lock(&usb_frame_mutex[device]);

   //grab a frame
   PWC_DEBUG_CALL(ioctl(usbvflg_fd[device],VIDIOCMCAPTURE,&(usbvflg_params[device][use_frame])),"error capturing image");
  
   // Frame wechseln
   use_frame=1-use_frame;

   while (1) {       
     //grab a frame
     PWC_DEBUG_CALL(ioctl(usbvflg_fd[device],VIDIOCMCAPTURE,&(usbvflg_params[device][use_frame])),"error capturing image");
       
     // Frame wechseln
     use_frame=1-use_frame;
     usbvflg_useframe[device]=use_frame;

     //Semaphore korrigieren
     sem_post(&usb_new_pictures[device]); 
     pthread_mutex_lock(&usb_semph_mutex[device]);
     sem_getvalue(&usb_new_pictures[device],&semval);
     
     //falls Bild nicht gelesen wurde:
     if (semval>1) {
       sem_trywait(&usb_new_pictures[device]); 
     }
     pthread_mutex_unlock(&usb_semph_mutex[device]);

     //auf grabber warten
     PWC_DEBUG_CALL(ioctl(usbvflg_fd[device], VIDIOCSYNC, &(usbvflg_params[device][use_frame].frame)),"error waiting for video-sync");
       
     //fertigen Frame freigeben
     pthread_mutex_unlock(&usb_frame_mutex[device]);

     //evtl. warten
     nanosleep(&delay, NULL);

     //check ob Schleife zu langsam
     gettimeofday(&zeit,NULL);
     usec=zeit.tv_usec;
     
     //blockieren
     pthread_mutex_lock(&usb_frame_mutex[device]);
   }
}

// }}}
// {{{ setparams(int device)

void setparams(int device){
   usbvflg_params[device][0].width= usb_image_widths[device];
   usbvflg_params[device][0].height= usb_image_heights[device];
   usbvflg_params[device][0].frame=0;
   memcpy(&(usbvflg_params[device][1]),&(usbvflg_params[device][0]),sizeof(struct video_mmap));
   usbvflg_params[device][1].frame=1;
}

// }}}
// {{{ save_setparams(int device)

void save_setparams(int device){

   /* Block buffers */
   pthread_mutex_lock(&usb_frame_mutex[device]);
  
   setparams(device);

   /* Unblock buffers */
   pthread_mutex_unlock(&usb_frame_mutex[device]);

   /* simulate two reading cycles 
      to make shure both grabbers are restarted */
   pthread_mutex_lock(&usb_semph_mutex[device]);
   sem_wait(&usb_new_pictures[device]); 
   pthread_mutex_unlock(&usb_semph_mutex[device]);

   pthread_mutex_lock(&usb_frame_mutex[device]);
   pthread_mutex_unlock(&usb_frame_mutex[device]);

   pthread_mutex_lock(&usb_semph_mutex[device]);
   sem_wait(&usb_new_pictures[device]); 
   pthread_mutex_unlock(&usb_semph_mutex[device]);
}
// }}}

PWCGrabber::PWCGrabber(void) : m_poRGB8Image(0) {
}

PWCGrabber::PWCGrabber(const Size &s, float fFps, int iDevice) : m_poRGB8Image(0) {
  if (!init(s, fFps, iDevice)) { exit(0); }
}
PWCGrabber::~PWCGrabber(void) {
  // {{{ open 
  if (m_poRGB8Image)
    delete m_poRGB8Image;
  //delete m_pucFlippedData;
  
  usbvflg_opencount[m_iDevice]--;
  if (usbvflg_verbosity>1)
    fprintf(stderr,"destructor leaving %d instances for /dev/video%d\n",
            usbvflg_opencount[m_iDevice],m_iDevice);
  
  // ---- deleting last instance of grabber ----
  if (usbvflg_opencount[m_iDevice] == 0) { 
    if (pthread_cancel(usb_grabber_thread[m_iDevice])<0) { // kill thread
      printf("Error: Cancel pthread: %s\n",strerror(errno));
    }
    
    if (pthread_join(usb_grabber_thread[m_iDevice],NULL)<0) {  // wait
      printf("Error: Cancel pthread: %s\n",strerror(errno));
    }
  
    if (pthread_mutex_destroy(&usb_frame_mutex[m_iDevice])<0) {
      printf("Error: Mutex destroy frame: %s\n",strerror(errno));
    }
    
    if (pthread_mutex_destroy(&usb_semph_mutex[m_iDevice])<0) {
      printf("Error: Mutex destroy semph: %s\n",strerror(errno));
    }
    
    if (sem_destroy(&usb_new_pictures[m_iDevice])<0) {
      printf("Error: Sem destroy: %s\n",strerror(errno));
    }
    
    if (usbvflg_verbosity) {
      printf("thread stuff destroyed\n");
    }

    if (usbvflg_fd[m_iDevice]>=0) {  // close
      close(usbvflg_fd[m_iDevice]);          
      if (usbvflg_verbosity) {
        printf("closing /dev/video%d\n",m_iDevice);
      }
     
      if (usbvflg_buf[m_iDevice]) { // munmap
        munmap(usbvflg_buf[m_iDevice],usbvflg_vmbuf[m_iDevice].size);
        
        if (usbvflg_verbosity)
          printf("unmapping memory for /dev/video%d\n",m_iDevice);
      }
    }
  }
}

// }}}

bool PWCGrabber::restoreUserSettings()
{
   PWC_DEBUG_CALL(ioctl(usbvflg_fd[m_iDevice], VIDIOCPWCRUSER), "cannot restore user settings");
   return true;
}

bool PWCGrabber::saveUserSettings()
{
  PWC_DEBUG_CALL(ioctl(usbvflg_fd[m_iDevice], VIDIOCPWCSUSER), "cannot save user settings");
  return true;
}

bool PWCGrabber::setGain(signed int iGainValue) // use a negative value to set to auto-gain or use a value from interval 0..65535 to set it to a fixed value
{
  if (iGainValue > 65535)
  {
    return false;
  }
  PWC_DEBUG_CALL(ioctl(usbvflg_fd[m_iDevice], VIDIOCPWCSAGC, &iGainValue), "cannot set gain");

  return true;
}

bool PWCGrabber::setWhiteBalance(int mode, int manual_red, int manual_blue)
{
/* mode can be:
  PWC_WB_AUTO - Automatic
  PWC_WB_MANUAL - Manually set by manual_red and manual_blue
  PWC_WB_INDOOR - Indoor lightbulb type lighting
  PWC_WB_OUTDOOR - Outdoor lighting
  PWC_WB_FL - Flourescent lighting (FL tubes)

  other values can be between 0..65535 if you set mode = PWC_WB_MANUAL
  read_xxx values are ignored
*/
  pwc_whitebalance wb;
  wb.mode = mode;
  wb.manual_red = manual_red;
  wb.manual_blue = manual_blue;
  wb.read_red = 0;
  wb.read_blue = 0;
  
  PWC_DEBUG_CALL(ioctl(usbvflg_fd[m_iDevice], VIDIOCPWCSAWB, &wb), "cannot set white balance");

  return true;
}

bool PWCGrabber::init(const Size &s,float fFps, int iDevice)
{
  m_iWidth = s.width;
  m_iHeight = s.height;
  m_iDevice = iDevice;
  m_fFps = fFps;
  m_poRGB8Image = new Img8u(s,formatRGB);
  
  // {{{ open

  usb_image_widths[m_iDevice]=m_iWidth;
  usb_image_heights[m_iDevice]=m_iHeight;

  static char *sollname="Philips 740 webcam";
  struct timeval zeit;
  static char *devname[] = { "/dev/video0", "/dev/video1",
                             "/dev/video2", "/dev/video3", 
                             NULL };
  
   struct video_capability vcap;
   struct video_picture picture;
   struct video_window vwin;
  
   gettimeofday(&zeit,NULL);
   usbvflg_starttime=zeit.tv_sec+float(zeit.tv_usec)/1000000;
   usb_last_time[m_iDevice]=zeit.tv_sec+double(zeit.tv_usec)/1000000-usbvflg_starttime;

   if (!usbvflg_opencount[m_iDevice]){  /* realy open */
     PWC_DEBUG_CALL((usbvflg_fd[m_iDevice]=open(devname[m_iDevice], O_RDWR)),"error opening device");
     
     PWC_DEBUG_CALL(ioctl(usbvflg_fd[m_iDevice], VIDIOCGCAP, &vcap),"error checking capturing device");

     usbvflg_maxwidth[m_iDevice]=vcap.maxwidth;

     PWC_DEBUG(strcmp(vcap.name,sollname),"Wrong hardware name (need something like \"/dev/video0\"");

     PWC_DEBUG_CALL(ioctl(usbvflg_fd[m_iDevice], VIDIOCGWIN, &vwin),"error getting video-window");

     vwin.flags &= ~PWC_FPS_FRMASK;
     int _fps_ = m_iWidth <=320 ? 30 : 15;

     /**ATTENTION 40 is max FPS value and will cause 25 fps in this system**/
     vwin.flags |= (_fps_ << PWC_FPS_SHIFT); // new framerate 15 fps
     
     vwin.width=m_iWidth; //these widht and height settings added later, 
     vwin.height=m_iHeight;//because via set_params it does't work always
     
     printf ("%i %i\n", m_iWidth, m_iHeight);
     PWC_DEBUG_CALL(ioctl(usbvflg_fd[m_iDevice], VIDIOCSWIN, &vwin),"error setting video-window");

     /* Set CHANNEL and PAL_MODE */
     usbvflg_vchan[m_iDevice].channel=0; /* 0: Tuner, 1: Comp1, 2: S-VHS */
      //    usbvflg_vchan[device].norm=VIDEO_MODE_PAL;

     PWC_DEBUG_CALL(ioctl(usbvflg_fd[m_iDevice], VIDIOCSCHAN, &(usbvflg_vchan[m_iDevice])),"error setting video-channel");

     PWC_DEBUG_CALL(ioctl(usbvflg_fd[m_iDevice], VIDIOCGPICT, &picture),"error getting video-picture");

     usbvflg_params[m_iDevice][0].format= picture.palette;
     usbvflg_params[m_iDevice][1].format= picture.palette;
    
     /* get infos about snapshot buffer memory size */
     PWC_DEBUG_CALL(ioctl(usbvflg_fd[m_iDevice], VIDIOCGMBUF, &usbvflg_vmbuf[m_iDevice]),"error getting video-membuffer" );

     /* Alloc memory for snapshot  */
     usbvflg_buf[m_iDevice]=(icl8u *)mmap(0,usbvflg_vmbuf[m_iDevice].size, 
                                               PROT_READ|PROT_WRITE,
                                               MAP_SHARED, usbvflg_fd[m_iDevice],0);
    
     /* Mutex stuff */
     pthread_mutex_init(&usb_frame_mutex[m_iDevice],NULL);
     pthread_mutex_init(&usb_semph_mutex[m_iDevice],NULL);
     /* Semaphore stuff */
     sem_init(&usb_new_pictures[m_iDevice],0,0);
    
     setparams(m_iDevice);
      
     /* Thread starten  */
     pthread_create(&usb_grabber_thread[m_iDevice], NULL, 
                    (void*(*)(void*))&usb_grabber_funct,
                    (void *)&(this->m_iDevice)); 
   }   
   else {  /* already using device and thread already running */
     printf("error device %d is already open \n",m_iDevice);
     return false;
   }
   //   if(ioctl(usbvflg_fd[device], VIDIOCGWIN, &vwin)<0){
   //throw CException("vfl:VIDIOCGWIN: %s\n",strerror(errno)); 
   //}
   //width=vwin.width; 
   //height=vwin.height;
   usbvflg_opencount[m_iDevice]++;
   
   return true;
}
   // }}}

ImgBase* PWCGrabber::grab(ImgBase *poOutput){
  // {{{ open 

  pthread_mutex_lock(&usb_semph_mutex[m_iDevice]);
  sem_wait(&usb_new_pictures[m_iDevice]); 
  pthread_mutex_unlock(&usb_semph_mutex[m_iDevice]);
  
  pthread_mutex_lock(&usb_frame_mutex[m_iDevice]);
  int use_frame=usbvflg_useframe[m_iDevice];

  icl8u *pucPwcData = usbvflg_buf[m_iDevice] + usbvflg_vmbuf[m_iDevice].offsets[use_frame];
  
  icl8u *pY = pucPwcData;
  icl8u *pU = pY+m_iWidth*m_iHeight;
  icl8u *pV = pY+(int)(1.25*m_iWidth*m_iHeight);

  if(poOutput) {
    if(poOutput->getFormat() == formatRGB &&
       poOutput->getDepth() == depth8u &&
       poOutput->getWidth() == m_iWidth &&
       poOutput->getHeight() == m_iHeight ){
      
      convertYUV420ToRGB8(poOutput->asImg<icl8u>(),pY,Size(m_iWidth,m_iHeight));
      
    }else if(poOutput->getFormat() == formatYUV){ // not yet tested
      
      Img8u oTmpSrc_Y(Size(m_iWidth,m_iHeight),    1,std::vector<icl8u*>(1, pY));
      Img8u oTmpSrc_U(Size(m_iWidth/2,m_iHeight/2),1,std::vector<icl8u*>(1, pU));
      Img8u oTmpSrc_V(Size(m_iWidth/2,m_iHeight/2),1,std::vector<icl8u*>(1, pV));
      
      if(poOutput->getDepth()==depth8u){
        icl8u *pucTmpY = poOutput->asImg<icl8u>()->getData(0);
        icl8u *pucTmpU = poOutput->asImg<icl8u>()->getData(1);
        icl8u *pucTmpV = poOutput->asImg<icl8u>()->getData(2);
        
        Img8u oTmpDst_Y(poOutput->getSize(),1,std::vector<icl8u*>(1, pucTmpY));
        Img8u oTmpDst_U(poOutput->getSize(),1,std::vector<icl8u*>(1, pucTmpU));
        Img8u oTmpDst_V(poOutput->getSize(),1,std::vector<icl8u*>(1, pucTmpV));
        
        oTmpSrc_Y.scaledCopy(&oTmpDst_Y);
        oTmpSrc_U.scaledCopy(&oTmpDst_U);
        oTmpSrc_V.scaledCopy(&oTmpDst_V);
        
      }else{
        icl32f *pfTmpY = poOutput->asImg<icl32f>()->getData(0);
        icl32f *pfTmpU = poOutput->asImg<icl32f>()->getData(1);
        icl32f *pfTmpV = poOutput->asImg<icl32f>()->getData(2);
        
        Img32f oTmpDst_Y(poOutput->getSize(),1,std::vector<icl32f*>(1, pfTmpY));
        Img32f oTmpDst_U(poOutput->getSize(),1,std::vector<icl32f*>(1, pfTmpU));
        Img32f oTmpDst_V(poOutput->getSize(),1,std::vector<icl32f*>(1, pfTmpV));
        
        m_oConverter.convert(&oTmpDst_Y,&oTmpSrc_Y);
        m_oConverterHalfSize.convert(&oTmpDst_U,&oTmpSrc_U);
        m_oConverterHalfSize.convert(&oTmpDst_V,&oTmpSrc_V);
      }
    }else{
      convertYUV420ToRGB8(m_poRGB8Image,pY,Size(m_iWidth,m_iHeight));
      m_oConverter.convert(poOutput,m_poRGB8Image);
    }

    pthread_mutex_unlock(&usb_frame_mutex[m_iDevice]);
  }
  else {
    convertYUV420ToRGB8(m_poRGB8Image,pY,Size(m_iWidth,m_iHeight));
    pthread_mutex_unlock(&usb_frame_mutex[m_iDevice]);
    return m_poRGB8Image;
  }
  return poOutput;
}

// }}}

} //namespace icl


#else // for the apple architecture

using namespace icl;
PWCGrabber::PWCGrabber(){
  printf("The PWC Grabber is not suppoted under MacOS-X \n");
  m_poRGB8Image = new Img8u();
}
PWCGrabber::PWCGrabber(const Size &s, float fFps, int iDevice){
  m_iWidth = s.width;
  m_iHeight = s.height;
  (void)fFps;
  (void)iDevice;
  m_poRGB8Image = new Img8u(s,1);
  printf("The PWC Grabber is not suppoted under MacOS-X \n");
}

PWCGrabber::~PWCGrabber(){}
bool PWCGrabber::init(const Size &s,float fFps, int iDevice){
  m_poRGB8Image->setSize(s);
  (void)fFps;
  (void)iDevice;
  return false;
}
ImgBase* PWCGrabber::grab(ImgBase *poDst){
  if(!poDst){
    return m_poRGB8Image;
  }else{
    return poDst;
  }  
}
bool PWCGrabber::restoreUserSettings(){
  return false;
}
bool PWCGrabber::saveUserSettings(){
  return false;
}
bool PWCGrabber::setGain(signed int iGainValue){
  (void)iGainValue;
  return false;
}
bool PWCGrabber::setWhiteBalance(int mode, int manual_red, int manual_blue){
  (void)mode;
  (void)manual_red;
  (void)manual_blue;
  return false;
}

#endif

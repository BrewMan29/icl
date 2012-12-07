/********************************************************************
**                Image Component Library (ICL)                    **
**                                                                 **
** Copyright (C) 2006-2012 CITEC, University of Bielefeld          **
**                         Neuroinformatics Group                  **
** Website: www.iclcv.org and                                      **
**          http://opensource.cit-ec.de/projects/icl               **
**                                                                 **
** File   : include/ICLIO/SharedMemoryGrabber.h                    **
** Module : ICLIO                                                  **
** Authors: Christof Elbrechter, Viktor Richter                    **
**                                                                 **
**                                                                 **
** Commercial License                                              **
** ICL can be used commercially, please refer to our website       **
** www.iclcv.org for more details.                                 **
**                                                                 **
** GNU General Public License Usage                                **
** Alternatively, this file may be used under the terms of the     **
** GNU General Public License version 3.0 as published by the      **
** Free Software Foundation and appearing in the file LICENSE.GPL  **
** included in the packaging of this file.  Please review the      **
** following information to ensure the GNU General Public License  **
** version 3.0 requirements will be met:                           **
** http://www.gnu.org/copyleft/gpl.html.                           **
**                                                                 **
** The development of this software was supported by the           **
** Excellence Cluster EXC 277 Cognitive Interaction Technology.    **
** The Excellence Cluster EXC 277 is a grant of the Deutsche       **
** Forschungsgemeinschaft (DFG) in the context of the German       **
** Excellence Initiative.                                          **
**                                                                 **
*********************************************************************/

#pragma once

#include <ICLIO/GrabberHandle.h>

namespace icl{
  namespace io{
    
    /// Grabber class that grabs images from QSharedMemory instances
    /** Images that are published using the SharedMemoryPublisher can
        be grabbed with this grabber type. Please don't use this
        Grabber class directly, but instantiate GenericGrabber with
        Devide type 'sm'.
    */
    class SharedMemoryGrabberImpl : public Grabber {
        /// Internal Data storage class
        struct Data;

        /// Hidden Data container
        Data *m_data;


        /// Creates a new SharedMemoryGrabber instance (please use the GenericGrabber instead)
        SharedMemoryGrabberImpl(const std::string &sharedMemorySegmentID="") throw(utils::ICLException);

        /// Connects an unconnected grabber to given shared memory segment
        void init(const std::string &sharedMemorySegmentID) throw (utils::ICLException);

      public:

        /// Only the 'real' graber can instantiate the -Impl
        friend class SharedMemoryGrabber;

        /// Destructor
        ~SharedMemoryGrabberImpl();

        /// returns a list of all available shared-memory image-streams
        /** evaluates the special memory segment named
          'icl-shared-mem-grabbers' in order to find
          out which devices are available
      */
        static const std::vector<GrabberDeviceDescription> &getDeviceList(bool rescan);

        /// grabbing function
        /** \copydoc icl::io::Grabber::grab(core::ImgBase**)  **/
        virtual const core::ImgBase* acquireImage();

        /// callback for changed configurable properties
        void processPropertyChange(const utils::Configurable::Property &prop);
    };


    /// Grabber class that grabs images from QSharedMemory instances
    /** for more details: @see SharedMemoryGrabberImpl */
    class SharedMemoryGrabber : public GrabberHandle<SharedMemoryGrabberImpl>{
      public:

        /// returns a list of available pwc devices
        /** @see SharedMemoryGrabberImpl for more details*/
        static inline const std::vector<GrabberDeviceDescription> &getDeviceList(bool rescan){
          return SharedMemoryGrabberImpl::getDeviceList(rescan);
        }

        /// creates a new PWCGrabber instance
        /** @see SharedMemoryGrabberImpl for more details */
        inline SharedMemoryGrabber(const std::string &memorySegmentName){
          if(isNew(memorySegmentName)){
            initialize(new SharedMemoryGrabberImpl(memorySegmentName),memorySegmentName);
          }else{
            initialize(memorySegmentName);
          }
        }
        /// empty constructor (initialize late using init())
        /** @see SharedMemoryGrabberImpl for more details */
        inline SharedMemoryGrabber(){}

        /// for deferred connection to (other) shared memory segment
        /** @see SharedMemoryGrabberImpl for more details */
        inline void init(const std::string &memorySegmentName) throw (utils::ICLException){
          if(isNew(memorySegmentName)){
            initialize(new SharedMemoryGrabberImpl(memorySegmentName),memorySegmentName);
          }else{
            initialize(memorySegmentName);
          }
        }

        /// resets the internal list of 'shared-grabbers'
        static void resetBus();
    };


    
  } // namespace io
}


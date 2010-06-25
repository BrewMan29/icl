/********************************************************************
 **                Image Component Library (ICL)                    **
 **                                                                 **
 ** Copyright (C) 2006-2010 CITEC, University of Bielefeld          **
 **                         Neuroinformatics Group                  **
 ** Website: www.iclcv.org and                                      **
 **          http://opensource.cit-ec.de/projects/icl               **
 **                                                                 **
 ** File   : include/ICLIO/OpenCVVideoWriter.h                      **
 ** Module : ICLIO                                                  **
 ** Authors: Christian Groszewski                                   **
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

#ifndef ICL_OPENCVVIDEOWRITER_H
#define ICL_OPENCVVIDEOWRITER_H

#include <string>
#include <cxtypes.h>
#include <highgui.h>
#include <ICLCore/ImgBase.h>

namespace icl{

class OpenCVVideoWriter {
private:
	///OpenCV VideoWriter struct
	CvVideoWriter *writer;
public:
	///Possible codecs(test if available on your system)
	//on win32 the value can also be -1
	enum FOURCC{
		MPEG_1 = CV_FOURCC('P','I','M','1'),
		MOTION_JPEG = CV_FOURCC('M','J','P','G'),
		MPEG_4_2 = CV_FOURCC('M', 'P', '4', '2'),
		MPEG_4_3 = CV_FOURCC('D', 'I', 'V', '3'),
		MPEG_4 = CV_FOURCC('D', 'I', 'V', 'X'),
		H263 = CV_FOURCC('U', '2', '6', '3'),
		H263I = CV_FOURCC('I', '2', '6', '3'),
		FLV1 = CV_FOURCC('F', 'L', 'V', '1')
#ifdef SYSTEM_LINUX
		,IYUV = CV_FOURCC('I', 'Y', 'U', 'V')
#endif
#ifdef SYSTEM_WINDOWS
		,OPEN_DIALOG= -1
#endif
	};

	/// Creates a new videowriter with given filename
	/** @param filename the filename to write to
	    @param fourcc determinates which videocodec to use
	    @param fps frames per second
	    @param frame_size size of the frames to be written out
	    @param frame_color currently only supported on windows 0 for greyscale else color
	 **/
	OpenCVVideoWriter(const std::string &filename, FOURCC fourcc,
	          double fps, Size frame_size, int frame_color=1) throw (ICLException);

	/// Destructor
	~OpenCVVideoWriter();

	/// writes the next image
	void write(const ImgBase *image);

	/// as write but in stream manner
	OpenCVVideoWriter &operator<<(const ImgBase *image);

};

}

#endif
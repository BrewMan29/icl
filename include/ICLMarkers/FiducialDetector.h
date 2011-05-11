/********************************************************************
**                Image Component Library (ICL)                    **
**                                                                 **
** Copyright (C) 2006-2010 CITEC, University of Bielefeld          **
**                         Neuroinformatics Group                  **
** Website: www.iclcv.org and                                      **
**          http://opensource.cit-ec.de/projects/icl               **
**                                                                 **
** File   : include/ICLMarkers/FiducialDetector.h                  **
** Module : ICLBlob                                                **
** Authors: Christof Elbrechter                                    **
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

#ifndef ICL_FIDUCIAL_DETECTOR_H
#define ICL_FIDUCIAL_DETECTOR_H

#include <ICLUtils/Configurable.h>
#include <ICLUtils/Uncopyable.h>
#include <ICLUtils/Any.h>
#include <ICLUtils/ParamList.h>

#include <ICLGeom/Camera.h>

#include <ICLMarkers/Fiducial.h>


namespace icl{
  
  /// Main Fiducial Detector class
  class FiducialDetector : public Uncopyable, public Configurable{
    
    /// hidden data class
    class Data;
    
    /// hidden data pointer
    Data *data;
    
    public:
    
    /// create a FiducialDetector instance with speical plugin type
    /** The given plugin type can be:
        
        - "art"
          for ARToolkit markers (visit the 
          <a href="http://www.hitl.washington.edu/artoolkit/">ARToolkit Webpage</a>
          for more details)
          ART Fiducials can provide the full set of information including the 3D pose
          from a single marker
        - "bch" 
          for ARToolkit+/Studierstube-Tracker like bch-code based binary markers
          (visit the <a href="http://studierstube.icg.tugraz.at/handheld_ar/stbtracker.php">
          Studierstube Tracker Homepage</a> for more details)
          BCH Fiducials can provide the full set of information including the 3D pose
          from a single marker. Usually, the detection rate is faster and more accurate
          in comparison to artoolkit markers. 
        - "amoeba"
          for The amoeba style hierarchical markers provided by the
          <a href="http://reactivision.sourceforge.net/">reactivision software</a>
          Amoeba fiducials cannot be detected in 3D. They do only provide 2D center and
          rotation and the 2D boundary. 
        - "icl1" 
          for ICL's former 'paper' markers <b>not yet supported</a>
        
        <b>other parameters</b>\n
        For extraction of 3D marker information, usually a camera
        is needed. The camera can also set later using setCamera
        Even though the camera is passed per pointer, it is
        compied deeply into the FiducialDetector instance. The
        camera can be updated using setCamera() lateron\n
        The also optionally given markersToLoad and params
        are directly passed to the loadMarkers method
        
        <b>benchmarks</b>
        TODO
    */
    FiducialDetector(const std::string &plugin="bch", 
                     const Any &markersToLoad=Any(),
                     const ParamList &params=ParamList(),
                     const Camera *camera=0) throw (ICLException);

    /// Destructor
    ~FiducialDetector();
    
    /// returns the current plugin type
    const std::string &getPluginType() const;
    
    /// sets the current camera
    /** The camera is usually needed for extraction of 3D marker 
        information. After setting a camera, some already existent
        Fiducials might become out of date. Therefore, detect
        must be called again when the camera was changed. */
    void setCamera(const Camera &camera);
    
    /// returns the current camera (or throws an exception if no camera is available)
    const Camera &getCamera() const throw (ICLException);
    
    /// loads markers according to the current plugin type
    /** - "bch":\n
          params is a string that is either a range "[a,b]" of markers
          that are used or a list "{a,b,c,...}" of allowed marker IDs or a single marker ID.
          The used bch codes are able to encode marker IDs in range [0,4095].
          Please note that angular braces define a range, while curly
          braces define a list here.\n
          Artoolkit markers need extra param in the param list which is the real marker size (in mm)
          An example param list is <tt>ParamList("size",Size(50,50));</tt>
        - "art":\n
          here, an image file-name or file-pattern is required. This image is automatically
          converted into an appropriate matching representation
        - "amoeba"\n
          Each amoeba marker is represented by a special two-level hierarchical
          code representation that is wrapped by the TwoLevelRegionStructure class.
          You can pass either a file, that contains a list of these codes (one code per line)
          or a single code or a comma- or space separated list of codes. Please note
          that the "amoeba" markers are only free to use in combination with the
          <a href="http://reactivision.sourceforge.net">reacTIVision</a> software.
    */
    void loadMarkers(const Any &which, const ParamList &params=ParamList()) throw (ICLException);
    
    /// unloads all already defined markers
    /** usually, markers are unloaded with the same pattern that was
        also used for loading the markers before */
    void unloadMarkers(const Any &which);
 
    /// detects markers in the given and returns all found markers
    /** Please note, the preferred input format is Img8u. Other formats
        are converted to depth8u internally. Usually, your 
        plugin will use gray images as input. You can query this information
        by calling getPropertyValue("preferred image type") */
    const std::vector<Fiducial> &detect(const ImgBase *image) throw (ICLException);
    
    /// returns the list of supported features
    Fiducial::FeatureSet getFeatures() const;
    
    /// returns a list (comma separated) of all available intermediate image results
    /** The returned images can be used for debugging or for further processing.
        the Fiducial Detector does always add the two tokes "input" and "pp", which
        refers to the last input image and the last preprocessing result image
    */
    std::string getIntermediateImageNames() const;
    
    /// returns the intermediate result image associated with given name
    /** If no image is associated with this name, an exception is thrown. Note,
        the image might be associated, but still be null, in particular if detect
        was not called one before */
    const ImgBase *getIntermediateImage(const std::string &name) const throw (ICLException);
    
    
  };
  
}

#endif

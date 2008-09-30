#include "iclFileWriter.h"
#include "iclFile.h"
#include "iclIOUtils.h"
#include "iclStringUtils.h"

// plugins
#include "iclFileWriterPluginPNM.h" 
#include "iclFileWriterPluginCSV.h" 
#ifdef HAVE_LIBJPEG
#include "iclFileWriterPluginJPEG.h" 
#endif
#ifdef HAVE_IMAGEMAGICK
#include "iclFileWriterPluginImageMagick.h"
#endif
using namespace std;
using namespace icl::ioutils;

namespace icl{

  map<string,FileWriterPlugin*> FileWriter::s_mapPlugins;
  
  struct FileWriterPluginMapInitializer{
    // {{{ open

    FileWriterPluginMapInitializer(){
      FileWriter::s_mapPlugins[".ppm"] = new FileWriterPluginPNM;
      FileWriter::s_mapPlugins[".pgm"] = new FileWriterPluginPNM;
      FileWriter::s_mapPlugins[".pnm"] = new FileWriterPluginPNM;
      FileWriter::s_mapPlugins[".icl"] = new FileWriterPluginPNM;
      FileWriter::s_mapPlugins[".csv"] = new FileWriterPluginCSV;

#ifdef HAVE_LIBJPEG
      FileWriter::s_mapPlugins[".jpeg"] = new FileWriterPluginJPEG;
      FileWriter::s_mapPlugins[".jpg"] = new FileWriterPluginJPEG;
#elif HAVE_IMAGEMAGICK
      FileWriter::s_mapPlugins[".jpeg"] = new FileWriterPluginMagick;
      FileWriter::s_mapPlugins[".jpg"] = new FileWriterPluginJMagick;
#endif

#ifdef HAVE_LIBZ
      FileWriter::s_mapPlugins[".ppm.gz"] = new FileWriterPluginPNM;
      FileWriter::s_mapPlugins[".pgm.gz"] = new FileWriterPluginPNM;
      FileWriter::s_mapPlugins[".pnm.gz"] = new FileWriterPluginPNM;
      FileWriter::s_mapPlugins[".icl.gz"] = new FileWriterPluginPNM;
      FileWriter::s_mapPlugins[".csv.gz"] = new FileWriterPluginCSV;
#endif
      
#ifdef HAVE_IMAGEMAGICK
      
      static const char *imageMagickFormats[] = {
        "png","gif","pdf","ps","avs","bmp","cgm","cin","cur","cut","dcx",
        "dib","dng","dot","dpx","emf","epdf","epi","eps","eps2","eps3",
        "epsf","epsi","ept","fax","gplt","gray","hpgl","html","ico","info",
        "jbig","jng","jp2","jpc","man","mat","miff","mono","mng","mpeg","m2v",
        "mpc","msl","mtv","mvg","palm","pbm","pcd","pcds","pcl","pcx","pdb",
        "pfa","pfb","picon","pict","pix","ps","ps2","ps3","psd","ptif","pwp",
        "rad","rgb","pgba","rla","rle","sct","sfw","sgi","shtml","sun","svg",
        "tga","tiff","tim","ttf","txt","uil","uyuv","vicar","viff","wbmp",
        "wmf","wpg","xbm","xcf","xpm","xwd","ydbcr","ycbcra","yuv",0
      };
      for(const char **pc=imageMagickFormats;*pc;++pc){
        FileWriter::s_mapPlugins[std::string(".")+*pc] = new FileWriterPluginImageMagick;
      }


#endif
      // add plugins
    }
    ~FileWriterPluginMapInitializer(){
      for(std::map<string,FileWriterPlugin*>::iterator it = FileWriter::s_mapPlugins.begin();
          it != FileWriter::s_mapPlugins.end(); ++it){
        delete it->second;
      }
    }
  };

  // }}}
  
  static FileWriterPluginMapInitializer __static_filewriter_plugin_initializer__;

  
  FileWriter::FileWriter(){
    // {{{ open

  }

  // }}}

  FileWriter::FileWriter(const std::string &filepattern):
    // {{{ open

    m_oGen(filepattern){}

  // }}}

  FileWriter::FileWriter(const FilenameGenerator &gen):
    // {{{ open

    m_oGen(gen){}

  // }}}

  FileWriter::~FileWriter(){
    // {{{ open

  }

  // }}}

  const FilenameGenerator &FileWriter::getFilenameGenerator() const{
    // {{{ open

    return m_oGen;
  }

  // }}}
  
  void FileWriter::write(const ImgBase *image){
    // {{{ open
    ICLASSERT_RETURN(image);
    ICLASSERT_RETURN(image->getDim());
    ICLASSERT_RETURN(image->getChannels());
    ICLASSERT_RETURN(!m_oGen.isNull());
    ICLASSERT_RETURN(m_oGen.filesLeft());
    
    File file(m_oGen.next());
    
    std::map<string,FileWriterPlugin*>::iterator it = FileWriter::s_mapPlugins.find(toLower(file.getSuffix()));
    if(it == s_mapPlugins.end()){
      ERROR_LOG("No Plugin to write files with suffix " << file.getSuffix() << " available");
      return;
    }
    
    it->second->write(file,image);
  }

  // }}}

  FileWriter &FileWriter::operator<<(const ImgBase *image){
    // {{{ open

    write(image);
    return *this;
  }

  // }}}

  void FileWriter::setOption(const std::string &option, const std::string &value){
    if(option == "csv:extend-file-name"){
      if(toLower(value) == "true"){
        FileWriterPluginCSV::setExtendFileName(true);
      }else if(toLower(value) == "false"){
        FileWriterPluginCSV::setExtendFileName(false);
      }else{
        ERROR_LOG("Undefined value \"" << value <<"\" for option \"" << option << "\"");
      }
    }else if(option == "jpg:quality"){
#ifdef WITH_JPEG_SUPPORT
      FileWriterPluginJPEG::setQuality(parse<int>(value));
#else
      ERROR_LOG("Unable to set option \"jpg:quality\" (JPEG support is currently disables!)");
#endif      
    }else{
      ERROR_LOG("Unsupported Option \"" << option << "\" (value: \"" << value << "\")");
    }
  
  }

  
}



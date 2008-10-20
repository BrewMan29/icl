#include <iclProximityOp.h>
#include <iclFileGrabber.h>
#include <iclFileWriter.h>

using namespace icl;

int main(){  
#ifdef WITH_IPP_OPTIMIZATION
  FileGrabber r("demoImages/macaw2.ppm");
  Img8u *image = r.grab()->asImg<icl8u>()->deepCopy();

  image->setROI(Rect(150,100,200,200));
  Img8u *imageTemplate = image->deepCopyROI();
  image->setFullROI();
  
  ProximityOp pop(ProximityOp::crossCorrCoeff, ProximityOp::valid);
  
  ImgBase *output = 0;
  
  pop.apply(image,imageTemplate,&output);
  output->normalizeAllChannels(Range<icl64f>(0,255));
  
  FileWriter("./template.ppm").write(imageTemplate);
  FileWriter("./proximity_result.ppm").write(output);
  printf("ProximityDemo applied successfully \n");
  
#endif
  return 0;
}
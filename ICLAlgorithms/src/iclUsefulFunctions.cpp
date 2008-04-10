#include "iclUsefulFunctions.h"
#include <iclProximityOp.h>
#include <iclBinaryLogicalOp.h>
#ifdef WITH_IPP_OPTIMIZATION
#include <ippi.h>
#endif 

#include <iclQuick.h>
namespace icl{

  namespace{
    template<int N,typename BinaryCompare>
    inline void apply_inplace_threshold(Img8u &image,int dim, icl8u thresh, BinaryCompare cmp){
      icl8u *pc[N];
      for(int j=0;j<N;++j){
        pc[j] = image.getData(j);
      }
      for(int i=0;i<dim;i++){
        bool result = true;
        for(int j=0;j<N;++j){
          result &= cmp(pc[j][i],thresh);
        }
        pc[0][i] = result * 255;
      }          
    }

    template<int N,typename BinaryCompare>
    inline void apply_inplace_threshold_2(Img8u &image,int dim, icl8u thresh, BinaryCompare cmp){
      int threshN = N*thresh;
      icl8u *pc[N];
      for(int j=0;j<N;++j){
        pc[j] = image.getData(j);
      }
      for(int i=0;i<dim;i++){
        int sumVal = 0;
        for(int j=0;j<N;++j){
          sumVal += pc[j][i];
        }
        pc[0][i] = 255 * cmp(sumVal,threshN);
      }          
    }
  }
  
  std::vector<Rect> iclMatchTemplate(const Img8u &src, 
                                     const Img8u &templ, 
                                     float significance,
                                     Img8u *bufferGiven,
                                     RegionDetector *rdGiven,
                                     bool useCrossCorrCoeffInsteadOfSqrDistance){    

    //DEBUG_LOG("src:" << src << "\ntempl:" << templ);
    Size bufSize = src.getROISize()-templ.getROISize()+Size(1,1);
    ICLASSERT_RETURN_VAL(bufSize.width > 0 && bufSize.height > 0, std::vector<Rect>());

    ImgBase *useBuffer = bufferGiven ? bufferGiven : new Img8u;
    RegionDetector *useRD = rdGiven ? rdGiven : new RegionDetector;


    useBuffer->setChannels(src.getChannels());
    useBuffer->setSize(bufSize);
    for(int i=0;i<src.getChannels();i++){
#ifdef WITH_IPP_OPTIMIZATION
      if(useCrossCorrCoeffInsteadOfSqrDistance){
        ippiCrossCorrValid_Norm_8u_C1RSfs(src.getROIData(i),src.getLineStep(),
                                          src.getROISize(), templ.getROIData(i),
                                          templ.getLineStep(),templ.getROISize(),
                                          useBuffer->asImg<icl8u>()->getData(i),
                                          useBuffer->getLineStep(),-8);
      }else{
        ippiSqrDistanceValid_Norm_8u_C1RSfs(src.getROIData(i),src.getLineStep(),
                                            src.getROISize(), templ.getROIData(i),
                                            templ.getLineStep(),templ.getROISize(),
                                            useBuffer->asImg<icl8u>()->getData(i),
                                            useBuffer->getLineStep(),-8);
      }
#else


  ERROR_LOG("not supported without IPP");
#endif
    }    

    Img8u &m = *useBuffer->asImg<icl8u>();
    
    //    show(cvt(m));

    icl8u t = useCrossCorrCoeffInsteadOfSqrDistance ? (icl8u)(float(255)*significance) : 255 - (icl8u)(float(255)*significance);
    
    if(!useCrossCorrCoeffInsteadOfSqrDistance){
      switch(m.getChannels()){
        case 1: apply_inplace_threshold_2<1,std::less<int> >(m,m.getDim(),t,std::less<int>());  break;
        case 2: apply_inplace_threshold_2<2,std::less<int> >(m,m.getDim(),t,std::less<int>());  break;
        case 3: apply_inplace_threshold_2<3,std::less<int> >(m,m.getDim(),t,std::less<int>());  break;
        default: 
          ERROR_LOG("this function is only supported for 1,2 and 3 channel images (channel count was " << m.getChannels() << ")");
          return std::vector<Rect>();
      }
    }else{
      switch(m.getChannels()){
        case 1: apply_inplace_threshold_2<1,std::greater<int> >(m,m.getDim(),t,std::greater<int>());  break;
        case 2: apply_inplace_threshold_2<2,std::greater<int> >(m,m.getDim(),t,std::greater<int>());  break;
        case 3: apply_inplace_threshold_2<3,std::greater<int> >(m,m.getDim(),t,std::greater<int>());  break;
        default: 
          ERROR_LOG("this function is only supported for 1,2 and 3 channel images (channel count was " << m.getChannels() << ")");
          return std::vector<Rect>();
      }
    }

    
    const ImgBase *c0 = useBuffer->shallowCopy(useBuffer->getImageRect(),
                                               std::vector<int>(1,0),
                                               formatMatrix);

    //    show(norm(cvt(c0)));
    useRD->setRestrictions(0,2<<20,1,255);
    const std::vector<Region> &blobData = useRD->detect(c0);
    std::vector<Rect> resultVec(blobData.size());
    
    Point halfTemplROI(templ.getROISize().width/2,-templ.getROISize().height/2);
    Point offs = src.getROIOffset()+halfTemplROI+Point(1,1);
    Rect templRect(halfTemplROI * (-1),templ.getROISize());
    templRect += offs;
    for(unsigned int i=0;i<blobData.size();i++){
      resultVec[i] =  (templRect + blobData[i].getCOG()) & src.getImageRect();
      
      // const Img8u *tmp = src.shallowCopy(resultVec[i]);
      // show(copyroi(cvt(tmp)));
      // delete tmp;
    }
    
    
    delete c0;
    if(!rdGiven) delete useRD;
    if(!bufferGiven) delete useBuffer;
    return resultVec;
  }
                


  std::vector<Rect> iclMatchTemplate(const Img8u &src, 
                                     const Img8u *srcMask,
                                     const Img8u &templ, 
                                     const Img8u *templMask,
                                     float significance,
                                     Img8u *srcBuffer,
                                     Img8u *templBuffer,
                                     Img8u *buffer,
                                     bool clipBuffersToROI,
                                     RegionDetector *rd,
                                     bool useCrossCorrCoeffInsteadOfSqrDistance){
    Img8u *useSrc = 0;
    Img8u *useTempl = 0;
    if(srcMask){
      useSrc = srcBuffer ? srcBuffer : new Img8u;
      Img8u srcMaskRepChannel(ImgParams(srcMask->getSize(),0,srcMask->getROI()));
      for(int i=0;i<src.getChannels()/srcMask->getChannels();i++){
        srcMaskRepChannel.append(const_cast<Img8u*>(srcMask));
      }
      BinaryLogicalOp binop(BinaryLogicalOp::andOp);
      binop.setClipToROI(clipBuffersToROI);
      binop.apply(&src,&srcMaskRepChannel,bpp(useSrc));
    }else{
      useSrc = const_cast<Img8u*>(&src);
    }
    if(templMask){
      useTempl = templBuffer ? templBuffer : new Img8u;

      Img8u templMaskRepChannel(ImgParams(templMask->getSize(),0,templMask->getROI()));
      for(int i=0;i<templ.getChannels()/templMask->getChannels();i++){
        templMaskRepChannel.append(const_cast<Img8u*>(templMask));
      }
      
      BinaryLogicalOp binop(BinaryLogicalOp::andOp);
      binop.setClipToROI(clipBuffersToROI);
      binop.apply(&templ,&templMaskRepChannel,bpp(useTempl));
    }else{
      useTempl = const_cast<Img8u*>(&templ);
    }
    
    std::vector<Rect> results = iclMatchTemplate(*useSrc,*useTempl,significance,buffer,rd,useCrossCorrCoeffInsteadOfSqrDistance);
    
    if(clipBuffersToROI && srcMask){
      DEBUG_LOG("");
      for(unsigned int i=0;i<results.size();++i){
        results[i]+=src.getROIOffset();
      }
    }
    
    if(!srcBuffer && srcMask) delete useSrc;
    if(!templBuffer && templMask) delete useTempl;
    
    return results;
  }
                     
  
  
}


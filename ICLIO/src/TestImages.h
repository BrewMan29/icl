#ifndef TESTIMAGES_H
#define TESTIMAGES_H

#include "Img.h"
#include <string>

namespace icl{
  class TestImages{
    public:
    /// creates a new testimage instance
    /** possible values for name are
        - women
        - house
        - tree
    */
    static ImgI* create(string name, const Size &size=Size(320,240),format f=formatRGB, depth d=depth8u);

    /// writes the image to the disc an shows it using xv.
    static void xv(ImgI *image, string tmpName="./tmp_image.ppm",long msec_to_rm_call=1000);
  };
}

#endif

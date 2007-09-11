#ifdef HAVE_XCF
#include <vector>
#include <iclXCFGrabber.h>
#include <log4cxx/PropertyConfigurator.h>
using namespace icl;
#endif

int main() {
#ifdef HAVE_XCF
	log4cxx::PropertyConfigurator::configure(std::string("log4cxx.props"));
	XCFGrabber *grabber = new XCFGrabber("XCFSonyFwImageServer");
	grabber->setRequest("<IMAGEREQUEST>"
                    	"<GRAB stereo=\"true\" timestamp=\"\"/>"
                    	"</IMAGEREQUEST>");

	std::vector<ImgBase*> imgs;
	imgs.resize(2);
	imgs[0] = 0;
	imgs[1] = 0;

	Time before = Time::now();
	for (int i=0; i<200; i++) {
		grabber->grab(imgs);
	}
	Time dur = before.age();
	std::cout << "Grabbed 200 images in " << dur.toSeconds() << "seconds!" << std::endl;
	std::cout << dur.toMicroSeconds() / (200 * 1000) << " hz" << std::endl;

#ifdef WIN32
	Sleep(1000);
#endif

	delete imgs[0];
	delete imgs[1];
	delete grabber;
#endif
	return 0;
}

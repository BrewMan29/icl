#ifndef CAM_CFG_WIDGET_H
#define CAM_CFG_WIDGET_H

#include <string>
#include <vector>
#include <QString>
#include <QWidget>
#include <iclUnicapDevice.h>

/** \cond **/  
class QHBoxLayout;
class QComboBox;
class QVBoxLayout;
class QLabel;
class QScrollArea;
class QPushButton;
class QTimer;
class QGroupBox;
class QTabWidget;
/** \endcond **/  

namespace icl{
  /** \cond **/
  class ICLWidget;
  class UnicapGrabber;
  class PWCGrabber;
  class DoubleSlider;
  class BorderBox;
  /** \endcond **/  


  class CamCfgWidget : public QWidget{
    Q_OBJECT
    public:
    CamCfgWidget();
    
    public slots:
    void deviceChanged(const QString &text);
    void deviceChanged(int index);
    void formatChanged(const QString &text);
    void sizeChanged(const QString &text);

    void propertySliderChanged(const QString &id, double value);
    void propertyComboBoxChanged(const QString &text);

    void startStopCapture(bool on);
    void updateImage();
    
    private:
    void updateSizeCombo();
    void updateFormatCombo();
    
    void fillLayout(QLayout *l, UnicapDevice &dev);
    
    QHBoxLayout *m_poTopLevelLayout;
    ICLWidget *m_poICLWidget;
    QWidget *m_poCenterPanel, *m_poRightPanel;
    QVBoxLayout *m_poCenterPanelLayout, *m_poRightPanelLayout;
    QScrollArea *m_poPropertyScrollArea;
    QPushButton *m_poCaptureButton;
    QTabWidget *m_poTabWidget;
    
    QComboBox *m_poDeviceCombo;
    QComboBox *m_poFormatCombo;
    QComboBox *m_poSizeCombo;

    QTimer *m_poTimer;
    UnicapGrabber *m_poGrabber;
    UnicapDevice m_oUnicapDevice;
    std::vector<UnicapDevice> m_vecDeviceList;
    
  
    
    bool m_bDisableSlots;
    bool m_bCapturing;
  
  
  };
}

#endif
  

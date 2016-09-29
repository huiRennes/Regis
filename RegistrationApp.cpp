#include <fstream>
#include <iostream>
#include <cstdlib>
#include <algorithm>
#include <string>
#include <time.h>
#include <stdio.h>
using namespace std;

#include <itkImageFileReader.h>
#include <itkImage.h>
#include <itkThresholdImageFilter.h>
#include "itkImageFileWriter.h"
#include <itkImageToVTKImageFilter.h>
#include <itkMetaImageIO.h>

#include <vtkSmartPointer.h>
#include <vtkObjectFactory.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkActor.h>
// headers needed for this example
#include <vtkImageViewer2.h>
#include <vtkDICOMImageReader.h>
#include <vtkInteractorStyleImage.h>
#include <vtkActor2D.h>
#include <vtkTextProperty.h>
#include <vtkTextMapper.h>
// needed to easily convert int to std::string
#include <sstream>



#define PI 3.14159265358979323846264338327950288419716939937510

// #include <boost/iterator/iterator_concepts.hpp>
#include <boost/program_options.hpp>

using namespace boost;

namespace po = boost::program_options; // BOOST program options ( command line parser )

//Types 
const unsigned int                          Dimension = 3;
typedef  float                              PixelType;
typedef itk::Image< PixelType, Dimension >  ImageType;
typedef itk::ImageFileReader< ImageType > ImageReaderType;
typedef itk::ThresholdImageFilter <ImageType> ThresholdImageFilterType;
typedef itk::ImageFileWriter< ImageType > WriterType;
typedef itk::ImageToVTKImageFilter<ImageType>       ConnectorType;
typedef itk::MetaImageIO    MetaImageIOType;
MetaImageIOType::Pointer metaIO=MetaImageIOType::New(); 


ImageType::Pointer readItkImage( string filename)
{
  ImageReaderType::Pointer reader = ImageReaderType::New();   
  reader->SetImageIO(metaIO);
  reader->SetFileName( filename );
  
  try
  {
    reader->Update();      
  }
  catch( itk::ExceptionObject & e )
  {
    std::cout  << "ExceptionObject caught opening gray level image!" << std::endl; 
    std::cout  << e << std::endl; 
    //~ return EXIT_FAILURE;
  }
  return reader->GetOutput();
}


// helper class to format slice status message
class StatusMessage {
public:
   static std::string Format(int slice, int maxSlice) {
      std::stringstream tmp;
      tmp << "Slice Number  " << slice + 1 << "/" << maxSlice + 1;
      return tmp.str();
   }
};

// Define own interaction style
class myVtkInteractorStyleImage : public vtkInteractorStyleImage
{
public:
   static myVtkInteractorStyleImage* New();
   vtkTypeMacro(myVtkInteractorStyleImage, vtkInteractorStyleImage);
 
protected:
   vtkImageViewer2* _ImageViewer;
   vtkTextMapper* _StatusMapper;
   int _Slice;
   int _MinSlice;
   int _MaxSlice;
 
public:
   void SetImageViewer(vtkImageViewer2* imageViewer) {
      _ImageViewer = imageViewer;
      _MinSlice = imageViewer->GetSliceMin();
      _MaxSlice = imageViewer->GetSliceMax();
      _Slice = _MinSlice;
      cout << "Slicer: Min = " << _MinSlice << ", Max = " << _MaxSlice << std::endl;
   }
 
   void SetStatusMapper(vtkTextMapper* statusMapper) {
      _StatusMapper = statusMapper;
   }
 
 
protected:
   void MoveSliceForward() {
      if(_Slice < _MaxSlice) {
         _Slice += 1;
         cout << "MoveSliceForward::Slice = " << _Slice << std::endl;
         _ImageViewer->SetSlice(_Slice);
         std::string msg = StatusMessage::Format(_Slice, _MaxSlice);
         _StatusMapper->SetInput(msg.c_str());
         _ImageViewer->Render();
      }
   }
 
   void MoveSliceBackward() {
      if(_Slice > _MinSlice) {
         _Slice -= 1;
         cout << "MoveSliceBackward::Slice = " << _Slice << std::endl;
         _ImageViewer->SetSlice(_Slice);
         std::string msg = StatusMessage::Format(_Slice, _MaxSlice);
         _StatusMapper->SetInput(msg.c_str());
         _ImageViewer->Render();
      }
   }
 
 
   virtual void OnKeyDown() {
      std::string key = this->GetInteractor()->GetKeySym();
      if(key.compare("Up") == 0) {
         //cout << "Up arrow key was pressed." << endl;
         MoveSliceForward();
      }
      else if(key.compare("Down") == 0) {
         //cout << "Down arrow key was pressed." << endl;
         MoveSliceBackward();
      }
      // forward event
      vtkInteractorStyleImage::OnKeyDown();
   }
 
 
   virtual void OnMouseWheelForward() {
      //std::cout << "Scrolled mouse wheel forward." << std::endl;
      MoveSliceForward();
      // don't forward events, otherwise the image will be zoomed 
      // in case another interactorstyle is used (e.g. trackballstyle, ...)
      // vtkInteractorStyleImage::OnMouseWheelForward();
   }
 
 
   virtual void OnMouseWheelBackward() {
      //std::cout << "Scrolled mouse wheel backward." << std::endl;
      if(_Slice > _MinSlice) {
         MoveSliceBackward();
      }
      // don't forward events, otherwise the image will be zoomed 
      // in case another interactorstyle is used (e.g. trackballstyle, ...)
      // vtkInteractorStyleImage::OnMouseWheelBackward();
   }
};
 
vtkStandardNewMacro(myVtkInteractorStyleImage);


int main( int argc, char *argv[] )
{
  string initFilename;
  string outputFilename;
  float thresholdBelow;
  float thresholdUpper;

  po::options_description desc("Allowed options");
  desc.add_options()
      ("help", "produce help message")   
      ("initIm", po::value< string >(&initFilename),  "Initial image") 
      ("thresholdMin",po::value< float >(&thresholdBelow),  "threshold below") 
      ("thresholdMax",po::value< float >(&thresholdUpper),  "threshold upper") 
      ("outPutFile", po::value< string >(&outputFilename), "outPut file name for save registration score");

  po::variables_map vm;
  po::store(po::command_line_parser(argc, argv).options(desc).style( po::command_line_style::unix_style ^ po::command_line_style::allow_short  ).run(), vm);
  po::notify(vm);    
  
  if ( (argc <= 2) || vm.count("help") )
  {    
     cout << desc << "\n \n";
     return EXIT_FAILURE;
  } 
  
  //checking that REQUIRED parameters have been provided. 
  if (!vm.count("initIm"))
  {
    cout << "inital image must be provided" << endl;
  }

  if (!vm.count("outPutFile"))
  {
    cout << "output file path for score must be provided" << endl; 
    return EXIT_FAILURE;
  }

  if(!vm.count("thresholdMin")){
    cout << "Threshold value must be provided"<<endl;
    return EXIT_FAILURE;
  }
  if(!vm.count("thresholdMin") && !vm.count("thresholdMax")){

    cout << "Must provid one threshold value"<<endl;
    return EXIT_FAILURE;
  }


  // to do for registration evaluation
  
  ImageType::Pointer initImage = ImageType::New();

  if ( vm.count("initIm") ){

      initImage = readItkImage( initFilename );

      // if no threshold max provided, we initialize it at 3000
      if(!vm.count("thresholdMax")){
	thresholdUpper = 3000;
	}
      ThresholdImageFilterType::Pointer thresholdFilter = ThresholdImageFilterType::New();
      thresholdFilter->SetInput(initImage);
      thresholdFilter->SetOutsideValue( 0 );
      thresholdFilter->ThresholdOutside(thresholdBelow,thresholdUpper);
      thresholdFilter->Update();

      // itk to vtk
      /*ConnectorType::Pointer connector = ConnectorType::New();
      connector->SetInput(thresholdFilter->GetOutput());
      // show segmented image
      vtkSmartPointer<vtkImageViewer2> imageViewer = vtkSmartPointer<vtkImageViewer2>::New();
      imageViewer->SetInputData(connector->GetOutput());*/

      // slice status message
      /*vtkSmartPointer<vtkTextProperty> sliceTextProp = vtkSmartPointer<vtkTextProperty>::New();
      sliceTextProp->SetFontFamilyToCourier();
      sliceTextProp->SetFontSize(20);
      sliceTextProp->SetVerticalJustificationToBottom();
      sliceTextProp->SetJustificationToLeft();
 
      vtkSmartPointer<vtkTextMapper> sliceTextMapper = vtkSmartPointer<vtkTextMapper>::New();
      std::string msg = StatusMessage::Format(imageViewer->GetSliceMin(), imageViewer->GetSliceMax());
      sliceTextMapper->SetInput(msg.c_str());
      sliceTextMapper->SetTextProperty(sliceTextProp);
 
      vtkSmartPointer<vtkActor2D> sliceTextActor = vtkSmartPointer<vtkActor2D>::New();
      sliceTextActor->SetMapper(sliceTextMapper);
      sliceTextActor->SetPosition(15, 10);
 
      // usage hint message
      vtkSmartPointer<vtkTextProperty> usageTextProp = vtkSmartPointer<vtkTextProperty>::New();
      usageTextProp->SetFontFamilyToCourier();
      usageTextProp->SetFontSize(14);
      usageTextProp->SetVerticalJustificationToTop();
      usageTextProp->SetJustificationToLeft();
 
      vtkSmartPointer<vtkTextMapper> usageTextMapper = vtkSmartPointer<vtkTextMapper>::New();
      usageTextMapper->SetInput("- Slice with mouse wheel\n  or Up/Down-Key\n- Zoom with pressed right\n  mouse button while dragging");
      usageTextMapper->SetTextProperty(usageTextProp);
 
      vtkSmartPointer<vtkActor2D> usageTextActor = vtkSmartPointer<vtkActor2D>::New();
      usageTextActor->SetMapper(usageTextMapper);
      usageTextActor->GetPositionCoordinate()->SetCoordinateSystemToNormalizedDisplay();
      usageTextActor->GetPositionCoordinate()->SetValue( 0.05, 0.95);*/

      // create an interactor with our own style (inherit from vtkInteractorStyleImage)
      // in order to catch mousewheel and key events
      /*vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
 
      vtkSmartPointer<myVtkInteractorStyleImage> myInteractorStyle = vtkSmartPointer<myVtkInteractorStyleImage>::New();
 
      // make imageviewer2 and sliceTextMapper visible to our interactorstyle
      // to enable slice status message updates when scrolling through the slices
      myInteractorStyle->SetImageViewer(imageViewer);
      myInteractorStyle->SetStatusMapper(sliceTextMapper);
 
      imageViewer->SetupInteractor(renderWindowInteractor);
      // make the interactor use our own interactorstyle
      // cause SetupInteractor() is defining it's own default interatorstyle 
      // this must be called after SetupInteractor()
      renderWindowInteractor->SetInteractorStyle(myInteractorStyle);
      // add slice status message and usage hint message to the renderer
      imageViewer->GetRenderer()->AddActor2D(sliceTextActor);
      imageViewer->GetRenderer()->AddActor2D(usageTextActor);*/
 
   // initialize rendering and interaction
   //imageViewer->GetRenderWindow()->SetSize(400, 300);
   //imageViewer->GetRenderer()->SetBackground(0.2, 0.3, 0.4);
      /*imageViewer->Render();
      imageViewer->GetRenderer()->ResetCamera();
      imageViewer->Render();
      renderWindowInteractor->Start();*/

	/*
      // show image segmented
      ConnectorType::Pointer connector = ConnectorType::New();
      connector->SetInput(thresholdFilter->GetOutput());
      vtkSmartPointer<vtkImageActor> actor = vtkSmartPointer<vtkImageActor>::New();
      #if VTK_MAJOR_VERSION <= 5
        actor->SetInput(connector->GetOutput());
      #else
  	connector->Update();
  	actor->GetMapper()->SetInputData(connector->GetOutput());
      #endif
	*/
      /*vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
      renderer->AddActor(actor);
      renderer->ResetCamera();
      vtkSmartPointer<vtkRenderWindow> renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
      renderWindow->AddRenderer(renderer);
      vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
      vtkSmartPointer<vtkInteractorStyleImage> style = vtkSmartPointer<vtkInteractorStyleImage>::New();
 
      renderWindowInteractor->SetInteractorStyle(style);
 
      renderWindowInteractor->SetRenderWindow(renderWindow);
      renderWindowInteractor->Initialize();
 
      renderWindowInteractor->Start();*/
      
      // save image
      WriterType::Pointer writer = WriterType::New();
      writer->SetImageIO(metaIO);
      writer->SetFileName(outputFilename);
      writer->SetInput(thresholdFilter->GetOutput());
      writer->Update();
      
  }

  return EXIT_SUCCESS;
}

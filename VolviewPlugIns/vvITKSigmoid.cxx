/* perform a pixel-wise intensity transformation using a Sigmoid function */

#include "vvITKFilterModule.h"

#include "itkSigmoidImageFilter.h"


template <class InputPixelType>
class SigmoidRunner
  {
  public:
      typedef  InputPixelType                       PixelType;
      typedef  itk::Image< PixelType, 3 >           ImageType; 
      typedef  itk::SigmoidImageFilter< ImageType,  ImageType >   FilterType;
      typedef  VolView::PlugIn::FilterModule< FilterType >        ModuleType;

  public:
    SigmoidRunner() {}
    void Execute( vtkVVPluginInfo *info, vtkVVProcessDataStruct *pds )
    {
      const float alpha   = atof( info->GetGUIProperty(info, 0, VVP_GUI_VALUE ));
      const float beta    = atof( info->GetGUIProperty(info, 1, VVP_GUI_VALUE ));
      const float minimum = atof( info->GetGUIProperty(info, 2, VVP_GUI_VALUE ));
      const float maximum = atof( info->GetGUIProperty(info, 3, VVP_GUI_VALUE ));

      ModuleType  module;
      module.SetPluginInfo( info );
      module.SetUpdateMessage("Transforming intensities with a Sigmoid function...");
      // Set the parameters on it
      module.GetFilter()->SetAlpha( alpha );         
      module.GetFilter()->SetBeta(  beta  );
      module.GetFilter()->SetOutputMinimum( static_cast<PixelType>( minimum ) ); 
      module.GetFilter()->SetOutputMaximum( static_cast<PixelType>( maximum ) );
      // Execute the filter
      module.ProcessData( pds  );
    }
  };



static int ProcessData(void *inf, vtkVVProcessDataStruct *pds)
{

  vtkVVPluginInfo *info = (vtkVVPluginInfo *)inf;


  try 
  {
  switch( info->InputVolumeScalarType )
    {
    case VTK_CHAR:
      {
      SigmoidRunner<signed char> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_UNSIGNED_CHAR:
      {
      SigmoidRunner<unsigned char> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_SHORT:
      {
      SigmoidRunner<signed short> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_UNSIGNED_SHORT:
      {
      SigmoidRunner<unsigned short> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_INT:
      {
      SigmoidRunner<signed int> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_UNSIGNED_INT:
      {
      SigmoidRunner<unsigned int> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_LONG:
      {
      SigmoidRunner<signed long> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_UNSIGNED_LONG:
      {
      SigmoidRunner<unsigned long> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_FLOAT:
      {
      SigmoidRunner<float> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_DOUBLE:
      {
      SigmoidRunner<double> runner;
      runner.Execute( info, pds );
      break; 
      }
    }
  }
  catch( itk::ExceptionObject & except )
  {
    info->SetProperty( info, VVP_ERROR, except.what() ); 
    return -1;
  }
  return 0;
}




static int UpdateGUI(void *inf)
{
  char tmp[1024];
  vtkVVPluginInfo *info = (vtkVVPluginInfo *)inf;

  info->SetGUIProperty(info, 0, VVP_GUI_LABEL, "Alpha");
  info->SetGUIProperty(info, 0, VVP_GUI_TYPE, VVP_GUI_SCALE);
  info->SetGUIProperty(info, 0, VVP_GUI_DEFAULT, "5");
  info->SetGUIProperty(info, 0, VVP_GUI_HELP, "Factor that defines the width of the Sigmoid in the range scale. Setting a small alpha results in a step transion on the Sigmoid function. A large alpha value produces a very smooth and low slanted Sigmoid.");
  info->SetGUIProperty(info, 0, VVP_GUI_HINTS , "-100 100 0.1");

  info->SetGUIProperty(info, 1, VVP_GUI_LABEL, "Beta");
  info->SetGUIProperty(info, 1, VVP_GUI_TYPE, VVP_GUI_SCALE);
  const float meanValue = (info->InputVolumeScalarTypeRange[1] - 
                           info->InputVolumeScalarTypeRange[0]) / 2.0;
  sprintf(tmp,"%f",meanValue);
  info->SetGUIProperty(info, 1, VVP_GUI_DEFAULT, tmp );
  info->SetGUIProperty(info, 1, VVP_GUI_HELP, "Origin of the Sigmoid function in the range scale. It cooresponds to the intensity of the imput image that will be mapped almost linearly to the output image. Intensities far from this value will be transformed non-linearly.");
  info->SetGUIProperty(info, 1, VVP_GUI_HINTS , VolView::PlugIn::FilterModuleBase::GetInputVolumeScalarTypeRange( info ) );

  // The output image type is equal to the input image type. 
  // We can use then the ranges returned by the GetInput... methods.
  //
  info->SetGUIProperty(info, 2, VVP_GUI_LABEL, "Output Minimum");
  info->SetGUIProperty(info, 2, VVP_GUI_TYPE, VVP_GUI_SCALE);
  info->SetGUIProperty(info, 2, VVP_GUI_DEFAULT, VolView::PlugIn::FilterModuleBase::GetInputVolumeScalarTypeMinimum( info ) );
  info->SetGUIProperty(info, 2, VVP_GUI_HELP, "Desired value for the minimum intensity of the output image.");
  info->SetGUIProperty(info, 2, VVP_GUI_HINTS , VolView::PlugIn::FilterModuleBase::GetInputVolumeScalarTypeRange( info ) );

  info->SetGUIProperty(info, 3, VVP_GUI_LABEL, "Output Maximum");
  info->SetGUIProperty(info, 3, VVP_GUI_TYPE, VVP_GUI_SCALE);
  info->SetGUIProperty(info, 3, VVP_GUI_DEFAULT, VolView::PlugIn::FilterModuleBase::GetInputVolumeScalarTypeMaximum( info ) );
  info->SetGUIProperty(info, 3, VVP_GUI_HELP, "Desired value for the maximum intensity of the output image.");
  info->SetGUIProperty(info, 3, VVP_GUI_HINTS , VolView::PlugIn::FilterModuleBase::GetInputVolumeScalarTypeRange( info ) );

  info->SetProperty(info, VVP_REQUIRED_Z_OVERLAP, "0");
  
  info->OutputVolumeScalarType = info->InputVolumeScalarType;
  info->OutputVolumeNumberOfComponents = 
    info->InputVolumeNumberOfComponents;
  memcpy(info->OutputVolumeDimensions,info->InputVolumeDimensions,
         3*sizeof(int));
  memcpy(info->OutputVolumeSpacing,info->InputVolumeSpacing,
         3*sizeof(float));
  memcpy(info->OutputVolumeOrigin,info->InputVolumeOrigin,
         3*sizeof(float));

  return 1;
}


extern "C" {
  
void VV_PLUGIN_EXPORT vvITKSigmoidInit(vtkVVPluginInfo *info)
{
  vvPluginVersionCheck();

  // setup information that never changes
  info->ProcessData = ProcessData;
  info->UpdateGUI   = UpdateGUI;
  info->SetProperty(info, VVP_NAME, "Sigmoid (ITK)");
  info->SetProperty(info, VVP_GROUP, "Intensity Transformation");
  info->SetProperty(info, VVP_TERSE_DOCUMENTATION,
                            "Simoid Intensity Transform");
  info->SetProperty(info, VVP_FULL_DOCUMENTATION,
    "This filters applies a pixel-wise intensity transform by using a Sigmoid function");
  info->SetProperty(info, VVP_SUPPORTS_IN_PLACE_PROCESSING, "0");
  info->SetProperty(info, VVP_SUPPORTS_PROCESSING_PIECES,   "1");
  info->SetProperty(info, VVP_NUMBER_OF_GUI_ITEMS,          "4");
  info->SetProperty(info, VVP_REQUIRED_Z_OVERLAP,           "0");
  info->SetProperty(info, VVP_PER_VOXEL_MEMORY_REQUIRED,    "1"); // actually depends on pixel size
}

}

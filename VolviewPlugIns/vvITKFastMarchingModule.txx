/** Generic interface for protocol communication between an ITK filter
    and the VolView Plugin Interface */

#ifndef _itkVVFastMarchingModule_txx
#define _itkVVFastMarchingModule_txx

#include "vvITKFastMarchingModule.h"

namespace VolView 
{

namespace PlugIn
{

/*
 *    Constructor
 */
template <class TInputPixelType >
FastMarchingModule<TInputPixelType>
::FastMarchingModule()
{
    m_ImportFilter               = ImportFilterType::New();
    m_GradientMagnitudeFilter    = GradientMagnitudeFilterType::New();
    m_SigmoidFilter              = SigmoidFilterType::New();
    m_FastMarchingFilter         = FastMarchingFilterType::New();
    m_IntensityWindowingFilter   = IntensityWindowingFilterType::New();

    m_NodeContainer           = NodeContainerType::New();

    m_InitialSeedValue        = 0.0;

    m_PerformPostprocessing   = true;

    m_CommandObserver         = CommandType::New();
    m_Info                    = 0;

    m_NodeContainer->Initialize();
    m_FastMarchingFilter->SetTrialPoints( m_NodeContainer );
    m_FastMarchingFilter->SetNormalizationFactor( 255.0 );

    m_SigmoidFilter->SetOutputMinimum(   0 );
    m_SigmoidFilter->SetOutputMaximum( 255 );

    m_CurrentNumberOfSeeds    = 0;

    // This transfer function will invert the map
    m_IntensityWindowingFilter->SetWindowMinimum( m_InitialSeedValue );
    m_IntensityWindowingFilter->SetWindowMaximum( m_FastMarchingFilter->GetStoppingValue() );
    m_IntensityWindowingFilter->SetOutputMinimum( m_FastMarchingFilter->GetStoppingValue() );
    m_IntensityWindowingFilter->SetOutputMaximum( m_InitialSeedValue );

    // Set up the pipeline
    m_GradientMagnitudeFilter->SetInput(  m_ImportFilter->GetOutput()             );
    m_SigmoidFilter->SetInput(            m_GradientMagnitudeFilter->GetOutput()  );
    m_FastMarchingFilter->SetInput(       m_SigmoidFilter->GetOutput()            );
    m_IntensityWindowingFilter->SetInput( m_FastMarchingFilter->GetOutput()       );

    // Allow progressive release of memory as the pipeline is executed
    m_GradientMagnitudeFilter->ReleaseDataFlagOn();
    m_SigmoidFilter->ReleaseDataFlagOn();
    m_FastMarchingFilter->ReleaseDataFlagOn();
    m_IntensityWindowingFilter->ReleaseDataFlagOn();
}


/*
 *    Destructor
 */
template <class TInputPixelType >
FastMarchingModule<TInputPixelType>
::~FastMarchingModule()
{
 
}



/*
 *    Add a seed point to the node container.
 *    There is a node per seed.
 */
template <class TInputPixelType >
void 
FastMarchingModule<TInputPixelType>
::AddSeed( float x, float y, float z )
{
  NodeType node;
  typename NodeType::IndexType seedPosition;
  seedPosition[0] = x;
  seedPosition[1] = y;
  seedPosition[2] = z;
  node.SetValue( m_InitialSeedValue );
  node.SetIndex( seedPosition );
  m_NodeContainer->InsertElement( m_CurrentNumberOfSeeds, node );
  m_CurrentNumberOfSeeds++;
}




/*
 *    Method provided as a callback for the progress update.
 */
template <class TInputPixelType >
void 
FastMarchingModule<TInputPixelType>
::ProgressUpdate( itk::Object * caller, const itk::EventObject & event )
{

  if( typeid( itk::ProgressEvent ) != typeid( event ) )
    {
    return;
    }

  itk::ProcessObject::Pointer process =
            dynamic_cast< itk::ProcessObject *>( caller );

  const float progress = process->GetProgress();

  m_Info->UpdateProgress( m_Info, progress, m_UpdateMessage.c_str() ); 

}


/*
 *  Set the initial value of the seed.
 *  This can be used to generate zero sets at
 *  a certain distance of the set of seeds.
 */
template <class TInputPixelType >
void 
FastMarchingModule<TInputPixelType>
::SetInitialSeedValue( float value )
{
  m_InitialSeedValue = value;
}




/*
 *  Set the Plugin Info structure 
 */
template <class TInputPixelType >
void 
FastMarchingModule<TInputPixelType>
::SetPluginInfo( vtkVVPluginInfo * info )
{
  m_Info = info;
}



/*
 *  Set the Plugin Info structure 
 */
template <class TInputPixelType >
void 
FastMarchingModule<TInputPixelType>
::SetStoppingValue( float value )
{
  m_FastMarchingFilter->SetStoppingValue( value );
  m_IntensityWindowingFilter->SetOutputMinimum( value );
  m_IntensityWindowingFilter->SetWindowMaximum( value );
}



/*
 *  Set the Sigma value for the Gradient Magnitude filter
 */
template <class TInputPixelType >
void 
FastMarchingModule<TInputPixelType>
::SetSigma( float value )
{
  m_GradientMagnitudeFilter->SetSigma( value );
}




/*
 *  Set the lowest value of the basin to be segmented
 */
template <class TInputPixelType >
void 
FastMarchingModule<TInputPixelType>
::SetLowestBasinValue( float value )
{
  m_LowestBasinValue = value;
}




/*
 *  Set the lowest value of the basin border to be segmented
 */
template <class TInputPixelType >
void 
FastMarchingModule<TInputPixelType>
::SetLowestBorderValue( float value )
{
  m_LowestBorderValue = value;
}




/*
 *  Set the boolean flag controlling whether 
 *  post-processing will be performed or not.
 */
template <class TInputPixelType >
void 
FastMarchingModule<TInputPixelType>
::SetPerformPostProcessing( bool value )
{
  m_PerformPostprocessing = value;
}



/*
 *  Set the Plugin Info structure 
 */
template <class TInputPixelType >
const FastMarchingModule<TInputPixelType>::RealImageType *
FastMarchingModule<TInputPixelType>
::GetLevelSet()
{
   return m_FastMarchingFilter->GetOutput();
}





/*
 *  Performs the actual filtering on the data 
 */
template <class TInputPixelType >
void 
FastMarchingModule<TInputPixelType>
::ProcessData( const vtkVVProcessDataStruct * pds )
{

  SizeType   size;
  IndexType  start;

  double     origin[3];
  double     spacing[3];

  size[0]     =  m_Info->InputVolumeDimensions[0];
  size[1]     =  m_Info->InputVolumeDimensions[1];
  size[2]     =  m_Info->InputVolumeDimensions[2];

  m_FastMarchingFilter->SetOutputSize( size );

  m_SigmoidFilter->SetBeta(   (m_LowestBorderValue + m_LowestBasinValue ) / 2.0 );
  m_SigmoidFilter->SetAlpha( -(m_LowestBorderValue - m_LowestBasinValue ) / 3.0 );

  for(unsigned int i=0; i<3; i++)
    {
    origin[i]   =  m_Info->InputVolumeOrigin[i];
    spacing[i]  =  m_Info->InputVolumeSpacing[i];
    start[i]    =  0;
    }

  RegionType region;

  region.SetIndex( start );
  region.SetSize(  size  );
 
  m_ImportFilter->SetSpacing( spacing );
  m_ImportFilter->SetOrigin(  origin  );
  m_ImportFilter->SetRegion(  region  );

  const unsigned int totalNumberOfPixels = region.GetNumberOfPixels();

  const bool         importFilterWillDeleteTheInputBuffer = false;

  const unsigned int numberOfPixelsPerSlice = size[0] * size[1];

  InputPixelType *   dataBlockStart = 
                        static_cast< InputPixelType * >( pds->inData )  
                      + numberOfPixelsPerSlice * pds->StartSlice;

  m_ImportFilter->SetImportPointer( dataBlockStart, 
                                    totalNumberOfPixels,
                                    importFilterWillDeleteTheInputBuffer );

  // Set the Observer for updating progress in the GUI
  m_CommandObserver->SetCallbackFunction( this, &FastMarchingModule::ProgressUpdate );
  m_GradientMagnitudeFilter->AddObserver( itk::ProgressEvent(), m_CommandObserver );
  m_SigmoidFilter->AddObserver( itk::ProgressEvent(), m_CommandObserver );
  m_FastMarchingFilter->AddObserver( itk::ProgressEvent(), m_CommandObserver );

  // Execute the filters and progressively remove temporary memory
  m_GradientMagnitudeFilter->Update();
  m_SigmoidFilter->Update();
  m_FastMarchingFilter->Update();

  if( m_PerformPostprocessing )
    {
    this->PostProcessData( pds );
    }

} // end of ProcessData



/*
 *  Performs post-processing of data. 
 *  This involves an intensity window operation and
 *  data copying into the volview provided buffer.
 */
template <class TInputPixelType >
void 
FastMarchingModule<TInputPixelType>
::PostProcessData( const vtkVVProcessDataStruct * pds )
{
  m_IntensityWindowingFilter->Update();

  // Copy the data (with casting) to the output buffer provided by the Plug In API
  typename OutputImageType::ConstPointer outputImage =
                               m_IntensityWindowingFilter->GetOutput();

  typedef itk::ImageRegionConstIterator< OutputImageType >  OutputIteratorType;

  OutputIteratorType ot( outputImage, outputImage->GetBufferedRegion() );

  InputPixelType * outData = (InputPixelType *)(pds->outData);

  ot.GoToBegin(); 
  while( !ot.IsAtEnd() )
    {
    *outData = static_cast< InputPixelType >( ot.Get() );
    ++ot;
    ++outData;
    }

} // end of PostProcessData


} // end of namespace PlugIn

} // end of namespace Volview

#endif

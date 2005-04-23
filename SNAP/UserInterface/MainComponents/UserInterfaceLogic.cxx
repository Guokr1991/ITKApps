/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    UserInterfaceLogic.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
// Borland compiler is very lazy so we need to instantiate the template
//  by hand 
#if defined(__BORLANDC__)
#include "SNAPBorlandDummyTypes.h"
#endif

#include "UserInterfaceLogic.h"

#include "GlobalState.h"
#include "GreyImageWrapper.h"
#include "EdgePreprocessingImageFilter.h"
#include "IntensityCurveUILogic.h"
#include "IRISApplication.h"
#include "IRISImageData.h"
#include "IRISVectorTypesToITKConversion.h"
#include "LabelEditorUILogic.h"
#include "SNAPImageData.h"
#include "SNAPLevelSetDriver.h"
#include "SNAPRegistryIO.h"
#include "SmoothBinaryThresholdImageFilter.h"
#include "SystemInterface.h"

// Additional UI component inludes
#include "AppearanceDialogUILogic.h"
#include "HelpViewerLogic.h"
#include "PreprocessingUILogic.h"
#include "SnakeParametersUILogic.h"
#include "GreyImageIOWizardLogic.h"
#include "PreprocessingImageIOWizardLogic.h"
#include "ResizeRegionDialogLogic.h"
#include "RestoreSettingsDialogLogic.h"
#include "SegmentationImageIOWizardLogic.h"
#include "SimpleFileDialogLogic.h"
#include "SliceWindowCoordinator.h"
#include "SNAPAppearanceSettings.h"
#include "FLTKWidgetActivationManager.h"

#include "FL/Fl_File_Chooser.H"

#include <itksys/SystemTools.hxx>
#include <strstream>
#include <iomanip>
#include <string>
#include <vector>
#include "UserInterfaceLogic.h"

// Disable some utterly annoying windows messages
#if defined(_MSC_VER)
#pragma warning ( disable : 4786 )
#pragma warning ( disable : 4503 )
#endif

using namespace itk;
using namespace std;

/**
 * \class GreyImageInfoCallback
 * \brief Adapter for the interface ImageInfoCallbackInterface, used to 
 * pass on registry information to the IO Wizard.
 */
class GreyImageInfoCallback : public ImageInfoCallbackInterface
{
public:
  GreyImageInfoCallback(SystemInterface *system)
    { m_SystemInterface = system; }

  bool FindRegistryAssociatedWithImage(const char *file, Registry &registry)
    { 
    if(!m_SystemInterface->FindRegistryAssociatedWithFile(file, m_Registry))
      return false;
    registry = m_Registry.Folder("Files.Grey");
    return true;
    }
private:
  SystemInterface *m_SystemInterface;
  Registry m_Registry;
};

void UserInterfaceLogic
::InitializeActivationFlags()
{
  // ---------------------------------------------------------
  //    Intialize activation object
  // ---------------------------------------------------------
  m_Activation = new FLTKWidgetActivationManager<UIStateFlags>;
  
  // ---------------------------------------------------------
  //    Configure Flag Relationships
  // ---------------------------------------------------------
  
  // Set the parent-child relationships between flags
  m_Activation->SetFlagImplies(UIF_SNAP_SPEED_AVAILABLE, UIF_SNAP_ACTIVE);
  m_Activation->SetFlagImplies(UIF_SNAP_MESH_CONTINUOUS_UPDATE, UIF_SNAP_SNAKE_INITIALIZED);
  m_Activation->SetFlagImplies(UIF_IRIS_WITH_GRAY_LOADED, UIF_GRAY_LOADED);
  m_Activation->SetFlagImplies(UIF_IRIS_WITH_GRAY_LOADED, UIF_IRIS_ACTIVE);
  m_Activation->SetFlagImplies(UIF_IRIS_MESH_DIRTY, UIF_IRIS_WITH_GRAY_LOADED);
  m_Activation->SetFlagImplies(UIF_IRIS_MESH_ACTION_PENDING, UIF_IRIS_WITH_GRAY_LOADED);
  m_Activation->SetFlagImplies(UIF_IRIS_ROI_VALID, UIF_GRAY_LOADED);
  m_Activation->SetFlagImplies(UIF_LINKED_ZOOM, UIF_GRAY_LOADED);
  m_Activation->SetFlagImplies(UIF_SNAP_PREPROCESSING_ACTIVE, UIF_SNAP_ACTIVE);

  m_Activation->SetFlagImplies(UIF_SNAP_PAGE_PREPROCESSING, UIF_SNAP_ACTIVE);
  m_Activation->SetFlagImplies(UIF_SNAP_PAGE_BUBBLES, UIF_SNAP_ACTIVE);
  m_Activation->SetFlagImplies(UIF_SNAP_PAGE_SEGMENTATION, UIF_SNAP_ACTIVE);

  m_Activation->SetFlagImplies(UIF_SNAP_SNAKE_RUNNING, UIF_SNAP_SNAKE_INITIALIZED);
  m_Activation->SetFlagImplies(UIF_SNAP_SNAKE_EDITABLE, UIF_SNAP_SNAKE_INITIALIZED);
  m_Activation->SetFlagImplies(UIF_SNAP_SNAKE_EDITABLE, UIF_SNAP_SNAKE_RUNNING, true, false);

  // Set up the exclusive relationships between flags
  m_Activation->SetFlagImplies( UIF_SNAP_ACTIVE, UIF_IRIS_ACTIVE, true, false );
  m_Activation->SetFlagImplies(UIF_SNAP_PREPROCESSING_DONE, 
    UIF_SNAP_PREPROCESSING_ACTIVE, true, false );  
  m_Activation->SetFlagsMutuallyExclusive( 
    UIF_SNAP_PAGE_SEGMENTATION, 
    UIF_SNAP_PAGE_BUBBLES, 
    UIF_SNAP_PAGE_PREPROCESSING );
  
  // ---------------------------------------------------------
  //    Relate flags to widgets
  // ---------------------------------------------------------
  
  // Link widget activation to flags
  m_Activation->AddWidget(m_BtnAcceptPreprocessing, UIF_SNAP_PREPROCESSING_DONE);
  m_Activation->AddWidget(m_GrpImageOptions, UIF_SNAP_SPEED_AVAILABLE);
  m_Activation->AddWidget(m_BtnSNAPMeshUpdate, UIF_SNAP_SNAKE_INITIALIZED);
  m_Activation->AddWidget(m_BtnMeshUpdate, UIF_IRIS_MESH_DIRTY);
  m_Activation->AddWidget(m_BtnStartSnake, UIF_IRIS_ROI_VALID);
  m_Activation->AddWidget(m_InZoomPercentage, UIF_LINKED_ZOOM);
  m_Activation->AddWidget(m_BtnAccept3D, UIF_IRIS_MESH_ACTION_PENDING);

  m_Activation->AddWidget(m_BtnAcceptSegmentation, UIF_SNAP_SNAKE_INITIALIZED);
  m_Activation->AddWidget(m_BtnRestartInitialization, UIF_SNAP_SNAKE_INITIALIZED);
  m_Activation->AddWidget(m_BtnSnakePlay, UIF_SNAP_SNAKE_EDITABLE);
  m_Activation->AddWidget(m_BtnSnakeRewind, UIF_SNAP_SNAKE_INITIALIZED);
  m_Activation->AddWidget(m_BtnSnakeStop, UIF_SNAP_SNAKE_INITIALIZED);
  m_Activation->AddWidget(m_BtnSnakeStep, UIF_SNAP_SNAKE_INITIALIZED);
  
  // Add more complex relationships
  m_Activation->AddCheckBox(m_ChkContinuousView3DUpdate, 
    UIF_SNAP_SNAKE_INITIALIZED, false, false);

  // Activate widgets indexed by dimension
  for(unsigned int i = 0; i < 3; i++)
    {
    m_Activation->AddWidget(m_InSNAPSliceSlider[i], UIF_SNAP_ACTIVE);
    m_Activation->AddWidget(m_OutSNAPSliceIndex[i], UIF_SNAP_ACTIVE);
    m_Activation->AddWidget(m_InIRISSliceSlider[i], UIF_IRIS_WITH_GRAY_LOADED);
    m_Activation->AddWidget(m_OutIRISSliceIndex[i], UIF_IRIS_WITH_GRAY_LOADED);
    m_Activation->AddWidget(m_BtnIRISPanelResetView[i], UIF_IRIS_WITH_GRAY_LOADED);
    m_Activation->AddWidget(m_BtnIRISPanelZoom[i], UIF_IRIS_WITH_GRAY_LOADED);
    }

  // The 3D controls that are indexed by i
  m_Activation->AddWidget(m_BtnIRISPanelResetView[3], UIF_IRIS_WITH_GRAY_LOADED);
  m_Activation->AddWidget(m_BtnIRISPanelZoom[3], UIF_IRIS_WITH_GRAY_LOADED);

  // Link menu items to flags
  m_Activation->AddMenuItem(m_MenuLoadGrey, UIF_IRIS_ACTIVE);
  m_Activation->AddMenuItem(m_MenuLoadSegmentation, UIF_IRIS_WITH_GRAY_LOADED);
  m_Activation->AddMenuItem(m_MenuSaveGreyROI, UIF_SNAP_ACTIVE);
  m_Activation->AddMenuItem(m_MenuSaveSegmentation, UIF_IRIS_WITH_GRAY_LOADED);
  m_Activation->AddMenuItem(m_MenuSaveLabels, UIF_IRIS_WITH_GRAY_LOADED);
  m_Activation->AddMenuItem(m_MenuLoadLabels, UIF_IRIS_WITH_GRAY_LOADED);
  m_Activation->AddMenuItem(m_MenuSaveVoxelCounts, UIF_IRIS_WITH_GRAY_LOADED);
  m_Activation->AddMenuItem(m_MenuIntensityCurve, UIF_GRAY_LOADED);
  m_Activation->AddMenuItem(m_MenuExportSlice, UIF_GRAY_LOADED);
  m_Activation->AddMenuItem(m_MenuSavePreprocessed, UIF_SNAP_SPEED_AVAILABLE);
  m_Activation->AddMenuItem(m_MenuLoadPreprocessed, UIF_SNAP_PAGE_PREPROCESSING);
  m_Activation->AddMenuItem(m_MenuLoadAdvection, UIF_SNAP_PAGE_PREPROCESSING);
  m_Activation->AddMenuItem(m_MenuImageInfo, UIF_IRIS_WITH_GRAY_LOADED);
}

UserInterfaceLogic
::UserInterfaceLogic(IRISApplication *iris, SystemInterface *system)
: UserInterface()
{
  // Store the pointers to application and system high level objects
  m_Driver = iris;
  m_SystemInterface = system;

  // This is just done for shorthand
  m_GlobalState = iris->GetGlobalState();

  // Load the appearance settings from the system interface
  m_AppearanceSettings = new SNAPAppearanceSettings();
  Registry &regAppearance = system->Folder("UserInterface.AppearanceSettings");
  m_AppearanceSettings->LoadFromRegistry(regAppearance);

  // Instantiate the IO wizards
  m_WizGreyIO = new GreyImageIOWizardLogic; 
  m_WizSegmentationIO = new SegmentationImageIOWizardLogic;
  m_WizPreprocessingIO = new PreprocessingImageIOWizardLogic;
  
  // Initialize the Image IO wizards
  m_WizGreyIO->MakeWindow();
  m_WizSegmentationIO->MakeWindow();
  m_WizPreprocessingIO->MakeWindow();

  // Provide the registry callback for the greyscale image wizard
  m_GreyCallbackInterface = new GreyImageInfoCallback(system);
  m_WizGreyIO->SetImageInfoCallback(m_GreyCallbackInterface);
  
  // Instantiate other windows
  m_IntensityCurveUI = new IntensityCurveUILogic;
  m_IntensityCurveUI->MakeWindow();

  // Create the label editor window
  m_LabelEditorUI = new LabelEditorUILogic();
  m_LabelEditorUI->MakeWindow();
  m_LabelEditorUI->Register(this);
  
  // Add a listener for the events generated by the curve editor
  SimpleCommandType::TMemberFunctionPointer ptrMember = 
    & UserInterfaceLogic::OnIntensityCurveUpdate;

  SimpleCommandType::Pointer cmd = SimpleCommandType::New();
  cmd->SetCallbackFunction(this,ptrMember);
  m_IntensityCurveUI->GetEventSystem()->AddObserver(
    IntensityCurveUILogic::CurveUpdateEvent(),cmd);
  
  // Initialize the progress command
  m_ProgressCommand = ProgressCommandType::New();
  m_ProgressCommand->SetCallbackFunction(
    this,&UserInterfaceLogic::OnITKProgressEvent);

  // Initialize the preprocessing windows
  m_PreprocessingUI = new PreprocessingUILogic;
  m_PreprocessingUI->MakeWindow();
  m_PreprocessingUI->Register(this);

  // Initialize the snake parameter window
  m_SnakeParametersUI = new SnakeParametersUILogic;
  m_SnakeParametersUI->MakeWindow();
  m_SnakeParametersUI->Register(this);

  // Initialize the restore settings dialog
  m_DlgRestoreSettings = new RestoreSettingsDialogLogic;
  m_DlgRestoreSettings->MakeWindow();

  // Initialize the resample dialog
  m_DlgResampleRegion = new ResizeRegionDialogLogic;
  m_DlgResampleRegion->MakeWindow();

  // Initialize the appearance dialog
  m_DlgAppearance = new AppearanceDialogUILogic;
  m_DlgAppearance->MakeWindow();
  m_DlgAppearance->Register(this);

  // Initialize the slice window coordinator object
  m_SliceCoordinator = new SliceWindowCoordinator();
  
  // Group the three windows inside the window coordinator
  m_SliceCoordinator->RegisterWindows(
    reinterpret_cast<GenericSliceWindow **>(m_IRISWindow2D));    

  // Create a callback command for the snake loop
  m_PostSnakeCommand = SimpleCommandType::New();

  // Initialize the Help UI
  m_HelpUI = new HelpViewerLogic;
  m_HelpUI->MakeWindow();
  m_HelpUI->SetContentsLink(
    m_SystemInterface->GetFileInRootDirectory("HTMLHelp/Tutorial.html").c_str());

  //  Initialize the label IO dialog
  m_DlgLabelsIO = new SimpleFileDialogLogic();
  m_DlgLabelsIO->MakeWindow();
  m_DlgLabelsIO->SetFileBoxTitle("Label description file:");
  m_DlgLabelsIO->SetPattern("All Label Files (*.{label,lbl,lab,txt})");
  m_DlgLabelsIO->SetLoadCallback(this,&UserInterfaceLogic::OnLoadLabelsAction);
  m_DlgLabelsIO->SetSaveCallback(this,&UserInterfaceLogic::OnSaveLabelsAction);

  /** Write voxels dialog */
  m_DlgVoxelCountsIO = new SimpleFileDialogLogic();
  m_DlgVoxelCountsIO->MakeWindow();
  m_DlgVoxelCountsIO->SetFileBoxTitle("Voxel count file:");
  m_DlgVoxelCountsIO->SetPattern("Text files (*.{txt})");
  m_DlgVoxelCountsIO->SetSaveCallback(
    this,&UserInterfaceLogic::OnWriteVoxelCountsAction);

  // Set the welcome page to display
  m_WizControlPane->value(m_GrpWelcomePage);

  InitializeActivationFlags();
  InitializeUI();

  // Enter the IRIS-ACTiVE state
  m_Activation->UpdateFlag(UIF_IRIS_ACTIVE, true);
}

UserInterfaceLogic
::~UserInterfaceLogic() 
{
  // Delete the IO wizards
  delete m_WizGreyIO;
  delete m_WizSegmentationIO;
  delete m_WizPreprocessingIO;

  // Delete the IO wizard registry adaptor
  delete m_GreyCallbackInterface;

  // Other IO dialogs
  delete m_DlgLabelsIO;
  delete m_DlgVoxelCountsIO;

  // Delete the UI's
  delete m_IntensityCurveUI;
  delete m_SnakeParametersUI;
  delete m_PreprocessingUI;
  delete m_DlgRestoreSettings;
  delete m_DlgResampleRegion;
  delete m_DlgAppearance;
  delete m_LabelEditorUI;

  // Delete the window coordinator
  delete m_SliceCoordinator;

  // Delete the appearance settings
  delete m_AppearanceSettings;

  // Delete the activation manager
  delete m_Activation;
}

void 
UserInterfaceLogic
::OnResetROIAction()
{
  if (!m_FileLoaded) return;

  // Get the grey image's region
  GlobalState::RegionType roi = 
    m_Driver->GetIRISImageData()->GetImageRegion();

  // The region can not be empty!
  assert(roi.GetNumberOfPixels() > 0);

  // Set the Region of interest
  m_GlobalState->SetSegmentationROI(roi);
  m_GlobalState->SetIsValidROI(true);
  
  // Update the UI
  this->RedrawWindows();
  m_OutMessage->value("Region of interest set to volume extents");
}

//--------------------------------------------
//
//
// SEGMENT 3D BUTTON CALLBACK
//
//
//--------------------------------------------

unsigned int 
UserInterfaceLogic
::GetImageAxisForDisplayWindow(unsigned int window)
{
  return m_Driver->GetCurrentImageData()->
    GetImageGeometry().GetDisplayToImageTransform(window).
    GetCoordinateIndexZeroBased(2);
}

void 
UserInterfaceLogic
::OnSnakeStartAction()
{
  uchar index = m_GlobalState->GetDrawingColorLabel();

  if (0 == index) 
    {
    fl_alert("Cannot start snake segmentation with clear color");
    return;
    }

  if (!m_Driver->GetColorLabelTable()->GetColorLabel(index).IsVisible()) 
    {
    fl_alert("Current label must be visible to start snake segmentation");
    return;
    }

  // Get the region of interest
  SNAPSegmentationROISettings roi = m_GlobalState->GetSegmentationROISettings();

  // The voxel size for the resampled region
  Vector3d voxelSizeSrc(
    m_Driver->GetCurrentImageData()->GetGrey()->GetImage()->GetSpacing().GetDataPointer());

  // Check if the user wants to resample the image
  if(m_ChkResampleRegion->value())
    {
    // Show the resampling dialog, updating the ROI object
    m_DlgResampleRegion->DisplayDialog(voxelSizeSrc.data_block(), roi);
    }
  else
    {
    roi.SetVoxelScale(Vector3d(1.0));
    roi.SetResampleFlag(false);
    }

  // Update the segmentation ROI
  m_GlobalState->SetSegmentationROISettings(roi);

  // The region can not be empty
  assert(roi.GetROI().GetNumberOfPixels() > 0);

  // Set the current application image mode to SNAP data
  m_Driver->InitializeSNAPImageData(roi,m_ProgressCommand);
  m_Driver->SetCurrentImageDataToSNAP();

  // Inform the global state that we're in sNAP
  m_GlobalState->SetSNAPActive(true);

  // Set bubble radius range according to volume dimensions (world dimensions)
  Vector3ui size = m_Driver->GetCurrentImageData()->GetVolumeExtents();
  Vector3d voxdims = m_Driver->GetSNAPImageData()->GetImageSpacing();
  double mindim = vector_multiply_mixed<double,unsigned int,3>(voxdims, size).min_value();

  m_InBubbleRadius->value(1);
  if ((int)(mindim/2) < 1)
    m_InBubbleRadius->range(1,1);
  else 
    {
    m_InBubbleRadius->range(1,(int)(mindim/2));
    m_InBubbleRadius->value((int)(mindim/8));
    }
  m_InBubbleRadius->redraw();

  // Use the current SnakeParameters to determine which type of snake to use
  const SnakeParameters &parameters = m_GlobalState->GetSnakeParameters();
  if(parameters.GetSnakeType() == SnakeParameters::EDGE_SNAKE)
    {
    m_RadSnakeEdge->set();
    m_RadSnakeInOut->clear();
    m_GlobalState->SetSnakeMode(EDGE_SNAKE);
    }
  else
    {
    m_RadSnakeInOut->set();
    m_RadSnakeEdge->clear();
    m_GlobalState->SetSnakeMode(IN_OUT_SNAKE);
    }

  // The edge preprocessing settings pass through unchanged
  // Get the current thresholding properties
  ThresholdSettings threshSettings = m_GlobalState->GetThresholdSettings();

  // We want to keep the current preprocessing settings, but we have to be
  // careful that they are in range of image intensity
  GreyType iMax =  m_Driver->GetCurrentImageData()->GetGrey()->GetImageMax();
  GreyType iMin =  m_Driver->GetCurrentImageData()->GetGrey()->GetImageMin();
  if(threshSettings.GetUpperThreshold() > iMax)
    threshSettings.SetUpperThreshold(iMax);
  if(threshSettings.GetLowerThreshold() < iMin)
    threshSettings.SetLowerThreshold(iMin);
  m_GlobalState->SetThresholdSettings(threshSettings);


  // This method basically sends the current button state from IRIS to SNAP
  SyncSnakeToIRIS();

  // Initialize GUI widgets 
  // TODO: WTF is this?
  m_RadioSNAPViewPreprocessed->value(0);
  m_RadioSNAPViewOriginal->value(1);
  m_BtnAcceptInitialization->show();
  m_BtnRestartInitialization->hide();

  m_GlobalState->SetShowSpeed(false);

  m_BrsActiveBubbles->clear();

  uchar rgb[3];
  m_Driver->GetColorLabelTable()->GetColorLabel(index).GetRGBVector(rgb);
  m_GrpSNAPCurrentColor->color(fl_rgb_color(rgb[0], rgb[1], rgb[2]));
  m_GrpSNAPCurrentColor->redraw();

  m_SnakeStepSize = 1;
  m_InStepSize->value(0);

  // reset Mesh in IRIS window
  m_IRISWindow3D->ClearScreen();

  // Hide the label editor since it's open
  m_LabelEditorUI->OnCloseAction();

  // show the snake window, hide the IRIS window
  ShowSNAP();

  // Image geometry has changed
  OnImageGeometryUpdate();

  m_OutMessage->value("Initalize snake");
}

//--------------------------------------------
//
//
// PREPROCESSING
//
//
//--------------------------------------------
void 
UserInterfaceLogic
::OnPreprocessAction()
{
  // Disable the 'Next' button on the preprocessing page
  m_Activation->UpdateFlag(UIF_SNAP_PREPROCESSING_ACTIVE, true);

  // Display the right window
  if(m_GlobalState->GetSnakeMode() == EDGE_SNAKE)
    {
    m_PreprocessingUI->DisplayEdgeWindow();
    }
  else
    {
    m_PreprocessingUI->DisplayInOutWindow();
    }
}

void 
UserInterfaceLogic
::OnPreprocessClose()
{
  // Check if the preprocessing image has been computed
  if(m_GlobalState->GetSpeedValid())
    m_Activation->UpdateFlag(UIF_SNAP_PREPROCESSING_DONE, true);
  else
    m_Activation->UpdateFlag(UIF_SNAP_PREPROCESSING_ACTIVE, false);    
}

void 
UserInterfaceLogic
::OnAcceptPreprocessingAction()
{  
  SetActiveSegmentationPipelinePage(1);
}

//--------------------------------------------
//
//
// SWITCH BETWEEN GREY/PREPROC DATA
//
//
//--------------------------------------------

void 
UserInterfaceLogic
::OnSNAPViewOriginalSelect()
{
  m_GlobalState->SetShowSpeed(false);
  RedrawWindows();
}

void 
UserInterfaceLogic
::OnViewPreprocessedSelect()
{
  if (!m_GlobalState->GetSpeedValid())
    return;
  m_GlobalState->SetShowSpeed(true);
  RedrawWindows();
}

//--------------------------------------------
//
//
// BUBBLES
//
//
//--------------------------------------------

void 
UserInterfaceLogic
::OnAddBubbleAction()
{
  m_OutMessage->value("");
  Bubble* bubble;
  bubble=new Bubble();
  bubble->center=to_int(m_GlobalState->GetCrosshairsPosition());
  bubble->radius=(int) m_InBubbleRadius->value();
  char msg[1024];
  sprintf(msg,"x,y,z=%d,%d,%d; R=%d ", bubble->center[0]+1,
    bubble->center[1]+1,bubble->center[2]+1,bubble->radius);
  m_BrsActiveBubbles->add(msg, bubble);
  m_BrsActiveBubbles->value(GetNumberOfBubbles()); 
  OnActiveBubblesChange();
  m_BrsActiveBubbles->redraw();
  RedrawWindows();
  cerr<<msg<<endl;
}

Bubble* 
UserInterfaceLogic
::GetBubbles()
{
  int n=GetNumberOfBubbles();
  Bubble* bubbles=new Bubble[n];
  for (int i=0;i<n;i++) 
    {
    bubbles[i]=*(Bubble*) m_BrsActiveBubbles->data(i+1);
    }
  return bubbles;
}

int 
UserInterfaceLogic
::GetNumberOfBubbles()
{
  return m_BrsActiveBubbles->size();
}

void 
UserInterfaceLogic
::OnRemoveBubbleAction()
{
  m_OutMessage->value("");
  if (m_HighlightedBubble!=0) 
    {
    m_BrsActiveBubbles->remove(m_HighlightedBubble);
    m_HighlightedBubble=0;
    m_OutMessage->value("Bubble removed");
    RedrawWindows();
    } 
  else
    m_OutMessage->value("Highlight a bubble in browser window");

}

void 
UserInterfaceLogic
::OnActiveBubblesChange()
{
  m_OutMessage->value("");
  m_HighlightedBubble=m_BrsActiveBubbles->value();

  //  if (!m_HighlightedBubble) cerr << "problem here" << endl;

  if (m_HighlightedBubble) 
    {
    m_InBubbleRadius->value(((Bubble*) m_BrsActiveBubbles->
        data(m_HighlightedBubble))->radius);
    }
}

void 
UserInterfaceLogic
::OnBubbleRadiusChange()
{
  m_OutMessage->value("");
  if (m_HighlightedBubble!=0) 
    {
    ((Bubble*) m_BrsActiveBubbles->
     data(m_HighlightedBubble))->radius=(int) m_InBubbleRadius->value();
    char msg[1024];
    sprintf(msg,"x,y,z=%d,%d,%d; R=%d ",
      ((Bubble*) m_BrsActiveBubbles->data(m_HighlightedBubble))->
      center[0]+1, ((Bubble*) m_BrsActiveBubbles->
        data(m_HighlightedBubble))->center[1]+1,
      ((Bubble*) m_BrsActiveBubbles->data(m_HighlightedBubble))->
      center[2]+1,((Bubble*) m_BrsActiveBubbles->
        data(m_HighlightedBubble))->radius);
    m_BrsActiveBubbles->text(m_HighlightedBubble, msg);
    RedrawWindows();
    }
  else
    m_OutMessage->value("Highlight a bubble in browser window");

}

//--------------------------------------------
//
//
// SNAKE TYPE RADIO BUTTONS
//
//
//--------------------------------------------

void 
UserInterfaceLogic
::OnInOutSnakeSelect()
{
  m_RadSnakeInOut->set();
  m_RadSnakeEdge->clear();
  m_GlobalState->SetSnakeMode(IN_OUT_SNAKE);

  //Nathan Moon
  if (m_GlobalState->GetSpeedValid()) 
    {
    //make sure they want to do this
    if (0 == fl_ask("Preprocessed data will be lost!  Continue?")) 
      {
      m_RadSnakeInOut->clear();
      m_RadSnakeEdge->set();
      m_GlobalState->SetSnakeMode(EDGE_SNAKE);
      return;
      }
    }

  // Set parameters to default values
  m_GlobalState->SetSnakeParameters(
    SnakeParameters::GetDefaultInOutParameters());

  m_Driver->GetSNAPImageData()->ClearSpeed();

  m_GlobalState->SetSpeedValid(false);

  // Update widget state
  m_Activation->UpdateFlag(UIF_SNAP_SPEED_AVAILABLE, false);

  m_PreprocessingUI->HidePreprocessingWindows();

  m_RadioSNAPViewOriginal->setonly();
  OnSNAPViewOriginalSelect();
}

void 
UserInterfaceLogic
::OnEdgeSnakeSelect()
{
  m_RadSnakeEdge->set();
  m_RadSnakeInOut->clear();
  m_GlobalState->SetSnakeMode(EDGE_SNAKE);

  //Nathan Moon
  if (m_GlobalState->GetSpeedValid()) 
    {
    //make sure they want to do this
    if (0 == fl_ask("Preprocessed data will be lost!  Continue?")) 
      {
      m_RadSnakeInOut->set();
      m_RadSnakeEdge->clear();
      m_GlobalState->SetSnakeMode(IN_OUT_SNAKE);
      return;
      }
    }

  // Set parameters to default values
  m_GlobalState->SetSnakeParameters(
    SnakeParameters::GetDefaultEdgeParameters());

  if (m_Driver->GetSNAPImageData()) m_Driver->GetSNAPImageData()->ClearSpeed();
  m_GlobalState->SetSpeedValid(false);
  
  // Update widget state
  m_Activation->UpdateFlag(UIF_SNAP_SPEED_AVAILABLE, false);
  
  m_PreprocessingUI->HidePreprocessingWindows();
  m_RadioSNAPViewOriginal->setonly();

  OnSNAPViewOriginalSelect();
}

/*
 * This method is called when the user has finished adding bubbles
 */
void 
UserInterfaceLogic
::OnAcceptInitializationAction()
{
  // Get bubbles, turn them into segmentation
  Bubble * bubbles = GetBubbles();
  int numbubbles = GetNumberOfBubbles();

  // Shorthand
  SNAPImageData *snapData = m_Driver->GetSNAPImageData();

  // Put on a wait cursor
  // TODO: Progress bar is needed here
  fl_cursor(FL_CURSOR_WAIT,FL_FOREGROUND_COLOR, FL_BACKGROUND_COLOR);

  // Merge bubbles with the segmentation image and initialize the snake
  bool rc = snapData->InitializeSegmentation(
    m_GlobalState->GetSnakeParameters(),bubbles, numbubbles,
    m_GlobalState->GetDrawingColorLabel());

  // Restore the cursor
  fl_cursor(FL_CURSOR_DEFAULT,FL_FOREGROUND_COLOR, FL_BACKGROUND_COLOR);
  
  // Check if we need to bail out
  if (!rc) 
    {    
    // There were no voxels selected in the end
    fl_alert("Can not proceed without an initialization\n"
             "Please place a bubble into the image!");
    
    // Clear bubble array
    if (bubbles)
      delete [] bubbles;

    return;        
    } 

  m_BtnAcceptInitialization->hide();
  m_BtnRestartInitialization->show();

  // Set the UI for the segmentation state
  m_Activation->UpdateFlag(UIF_SNAP_SNAKE_EDITABLE, true);
  
  m_SNAPWindow3D->ClearScreen(); // reset Mesh object in Window3D_s
  m_SNAPWindow3D->ResetView();   // reset cursor

  // Flip to the next page in the wizard
  SetActiveSegmentationPipelinePage(2);

  m_OutMessage->value("Snake initialized successfully");

  m_GlobalState->SetSnakeActive(true);

  OnSnakeUpdate();
}


void 
UserInterfaceLogic
::SetActiveSegmentationPipelinePage(unsigned int page)
{
  switch(page)
    {
    case 0 : 
      m_WizSegmentationPipeline->value(m_GrpSNAPStepPreprocess);
      m_Activation->UpdateFlag(UIF_SNAP_PAGE_PREPROCESSING, true);
      break;

    case 1 :
      m_WizSegmentationPipeline->value(m_GrpSNAPStepInitialize);
      m_Activation->UpdateFlag(UIF_SNAP_PAGE_BUBBLES, true);
      break;

    case 2 :
      m_WizSegmentationPipeline->value(m_GrpSNAPStepSegment);
      m_Activation->UpdateFlag(UIF_SNAP_PAGE_SEGMENTATION, true);
      break;
    }
}

void 
UserInterfaceLogic
::OnRestartInitializationAction()
{
  // Stop the segmentation if it's running
  OnSnakeStopAction();
  
  // If the segmentation pipeline is active, deactivate it
  if(m_Driver->GetSNAPImageData()->IsSegmentationActive())
    {
    // Tell the update loop to terminate
    m_Driver->GetSNAPImageData()->TerminateSegmentation();
    }

  // Tell the UI to activate related widgets
  m_Activation->UpdateFlag(UIF_SNAP_SNAKE_INITIALIZED, false);

  m_GlobalState->SetSnakeActive(false);
  m_BtnRestartInitialization->hide();
  m_BtnAcceptInitialization->show();

  m_SNAPWindow3D->ClearScreen(); // reset Mesh object in Window3D_s
  m_SNAPWindow3D->ResetView();   // reset cursor
  RedrawWindows();

  m_OutMessage->value("Snake initialization restarted");

  // Flip to the second page
  SetActiveSegmentationPipelinePage(1);
}

void
UserInterfaceLogic
::OnRestartPreprocessingAction()
{
  // Clear the bubble list
  m_HighlightedBubble = 0;

  // Flip to the first page
  SetActiveSegmentationPipelinePage(0);

  // Repaint the screen
  RedrawWindows();
}

void 
UserInterfaceLogic
::OnSnakeParametersAction()
{
  // Get the current parameters from the system
  SnakeParameters pGlobal = m_GlobalState->GetSnakeParameters();
  
  // Send current parameter to the snake parameter setting UI
  m_SnakeParametersUI->SetParameters(pGlobal);

  // Chech whether we need to warn user about changing the solver in the 
  // process of evolution
  m_SnakeParametersUI->SetWarnOnSolverUpdate(
    m_Driver->GetSNAPImageData()->GetElapsedSegmentationIterations() > 0);
  
  // Show the snake parameters window
  CenterChildWindowInParentWindow(m_SnakeParametersUI->GetWindow(),m_WinMain);
  m_SnakeParametersUI->DisplayWindow();
  
  // Wait until the window has been closed
  while(m_SnakeParametersUI->GetWindow()->visible())
    Fl::wait();
    
  // Have the parameters been accepted by the user?
  if(m_SnakeParametersUI->GetUserAccepted())
    {
    // Get the new parameters
    SnakeParameters pNew = m_SnakeParametersUI->GetParameters();
    
    // Have the parameters changed?
    if(!(pGlobal == pNew))
      {
      // Update the system's parameters with new values
      m_GlobalState->SetSnakeParameters(pNew);

      // Update the running snake
      if (m_Driver->GetSNAPImageData()->IsSegmentationActive()) 
        {
        m_Driver->GetSNAPImageData()->SetSegmentationParameters(pNew);
        }
      }
    }
}

void 
UserInterfaceLogic
::OnSnakeRewindAction()
{
  // Stop the snake if it's running
  OnSnakeStopAction();

  // Basically, we tell the level set driver that we want a restart
  m_Driver->GetSNAPImageData()->RestartSegmentation();

  // Update the display
  OnSnakeUpdate();
}

void fnSnakeIdleFunction(void *userData)
{
  // Get the instance of the calling class
  UserInterfaceLogic *uiLogic = (UserInterfaceLogic *) userData;

  // Request that the desired number of iterations be executed
  uiLogic->GetDriver()->GetSNAPImageData()->RunSegmentation(
    uiLogic->m_SnakeStepSize);
  
  // Update the display
  uiLogic->OnSnakeUpdate();
}

void 
UserInterfaceLogic
::OnSnakeStopAction()
{
  Fl::remove_idle(fnSnakeIdleFunction, this);
  m_Activation->UpdateFlag(UIF_SNAP_SNAKE_EDITABLE, true);
}

void 
UserInterfaceLogic
::OnSnakePlayAction()
{
  m_Activation->UpdateFlag(UIF_SNAP_SNAKE_RUNNING, true);
  Fl::add_idle(fnSnakeIdleFunction, this);
}

void 
UserInterfaceLogic
::OnSnakeUpdate()
{
  // Update the number of elapsed iterations
  std::ostringstream oss;
  oss << m_Driver->GetSNAPImageData()->GetElapsedSegmentationIterations();
  m_OutCurrentIteration->value(oss.str().c_str());

  // Update the mesh if necessary
  if(m_ChkContinuousView3DUpdate->value())
    m_SNAPWindow3D->UpdateMesh();
  else
    m_Activation->UpdateFlag(UIF_SNAP_MESH_DIRTY, true);

  // Redraw the windows
  RedrawWindows();
}

void 
UserInterfaceLogic
::OnSnakeStepAction()
{
  // Stop the snake if it's running
  OnSnakeStopAction();

  // Call the idle function directly
  fnSnakeIdleFunction(this);
}

void 
UserInterfaceLogic
::OnSnakeStepSizeChange()
{
  // Save the step size
  m_SnakeStepSize = atoi(m_InStepSize->text());
}


//--------------------------------------------
//
//
// SWITCHING BETWEEN WINDOWS STUFF
//
//
//--------------------------------------------

void 
UserInterfaceLogic
::SyncSnakeToIRIS()
{
  m_InSNAPLabelOpacity->value(m_InIRISLabelOpacity->value());
  // Contrast_slider_s->value(Contrast_slider->value());
  // Brightness_slider_s->value(Brightness_slider->value());

  switch (m_GlobalState->GetToolbarMode()) 
    {
  case(NAVIGATION_MODE):
    m_BtnSNAPNavigation->setonly();
    break;
  default:
    SetToolbarMode(CROSSHAIRS_MODE);
    m_BtnSNAPCrosshairs->setonly();
    break;
    }
}

void 
UserInterfaceLogic
::SyncIRISToSnake()
{
  m_InIRISLabelOpacity->value(m_InSNAPLabelOpacity->value());
  // Contrast_slider->value(Contrast_slider_s->value());
  // Brightness_slider->value(Brightness_slider_s->value());

  switch (m_GlobalState->GetToolbarMode()) 
    {
  case(NAVIGATION_MODE):
    m_BtnNavigationMode->setonly();
    break;
  default:
    SetToolbarMode(CROSSHAIRS_MODE);
    m_BtnCrosshairsMode->setonly();
    break;
    }
}

void 
UserInterfaceLogic
::CloseSegmentationCommon()
{
  // This makes no sense if there is no SNAP
  assert(m_Driver->GetSNAPImageData());

  // Clean up SNAP image data
  m_Driver->SetCurrentImageDataToIRIS();
  m_Driver->ReleaseSNAPImageData();

  // The segmentation has changed, so the mesh should be updatable
  m_Activation->UpdateFlag(UIF_IRIS_MESH_DIRTY, true);
  
  // Inform the global state that we're not in sNAP
  m_GlobalState->SetSNAPActive(false);

  // Speed image is no longer visible
  m_GlobalState->SetSpeedValid(false);
  m_GlobalState->SetShowSpeed(false);
  m_GlobalState->SetSnakeActive(false);

  // Updates some UI components (?)
  SyncIRISToSnake();

  // Reset the mesh display
  m_SNAPWindow3D->ClearScreen();
  m_SNAPWindow3D->ResetView();

  // Image geometry has changed
  OnImageGeometryUpdate();

  // Clear the list of bubbles
  m_BrsActiveBubbles->clear();
  m_HighlightedBubble = 0;  

  // Activate/deactivate menu items
  m_Activation->UpdateFlag(UIF_IRIS_ACTIVE, true);
  
  // Show IRIS window, Hide the snake window
  ShowIRIS();

  // Redraw the windows
  RedrawWindows();
}

void 
UserInterfaceLogic
::OnAcceptSegmentationAction()
{
  // Stop the segmentation if it's running
  OnSnakeStopAction();
  
  // Turn off segmentation if it's active
  if(m_Driver->GetSNAPImageData()->IsSegmentationActive())
    {
    // Tell the update loop to terminate
    m_Driver->GetSNAPImageData()->TerminateSegmentation();
    }

  // Get data from SNAP back into IRIS
  m_Driver->UpdateIRISWithSnapImageData(m_ProgressCommand);

  // Close up SNAP
  this->CloseSegmentationCommon();

  // Message to the user
  m_OutMessage->value("Accepted snake segmentation");
}

void 
UserInterfaceLogic
::OnITKProgressEvent(itk::Object *source, const EventObject &)
{
  // Get the elapsed progress
  itk::ProcessObject *po = reinterpret_cast<ProcessObject *>(source);
  float progress = po->GetProgress();

  // Update the progress bar and value
  m_OutProgressMeter->value(100 * progress);
  m_OutProgressCounter->value(100 * progress);

  // Show or hide progress bar if necessary
  if(progress < 1.0f && !m_WinProgress->visible())
    {
    this->CenterChildWindowInMainWindow(m_WinProgress);
    m_WinProgress->show();
    }
  else if (progress == 1.0f && m_WinProgress->visible())
    {
    m_WinProgress->hide();
    }

  // Update the screen
  Fl::check();
}

void 
UserInterfaceLogic
::OnCancelSegmentationAction()
{
  // Stop the segmentation if it's running
  OnSnakeStopAction();
  
  // This callback has double functionality, depending on whether the 
  // level set update loop is active or not
  if(m_Driver->GetSNAPImageData()->IsSegmentationActive())
    {
    // Tell the update loop to terminate
    m_Driver->GetSNAPImageData()->TerminateSegmentation();
    }

  // Clean up SNAP image data
  CloseSegmentationCommon();

  // Message to the user
  m_OutMessage->value("Snake segmentation cancelled");
}

void 
UserInterfaceLogic
::OnMenuQuit()
{
  this->OnMainWindowCloseAction();
}

void 
UserInterfaceLogic
::OnMenuImageInfo()
{
  m_WinImageInfo->show();
  CenterChildWindowInMainWindow(m_WinImageInfo);
}

void 
UserInterfaceLogic
::OnCloseImageInfoAction()
{
  m_WinImageInfo->hide();
}

void 
UserInterfaceLogic
::OnMainWindowCloseAction()
{
  // We don't want to just exit when users press escape
  if(Fl::event_key() == FL_Escape) return;

  // Associate the current state with the current image
  OnGreyImageUnload();

  // Create an array of open windows
  vector<Fl_Window *> openWindows;
  openWindows.push_back(Fl::first_window());

  // Add all the open windows to the list
  while(true)
    {
    Fl_Window *win = Fl::next_window(openWindows.back());
    if(win && win != openWindows.front())
      openWindows.push_back(win);
    else break;
    }

  // Close all the windows
  for(unsigned int i=0;i<openWindows.size();i++)
    openWindows[i]->hide();
}

void 
UserInterfaceLogic
::ShowIRIS()
{
  // Make sure the window is visible
  m_WinMain->show();

  // Show the right wizard page
  m_WizWindows->value(m_GrpIRISWindows);
  m_WizControlPane->value(
    m_Driver->GetCurrentImageData()->IsGreyLoaded() ? 
    m_GrpToolbarPage : m_GrpWelcomePage);

  // Show and hide the GL windows
  for(unsigned int i=0;i<3;i++)
    {
    m_IRISWindow2D[i]->show();
    m_SNAPWindow2D[i]->hide();
    }

  // Swap the 3D window visibility
  m_IRISWindow3D->show();
  m_SNAPWindow3D->hide();

  m_GrpCurrentColor->show();
  m_OutDrawOverColor->show();
  RedrawWindows();
}

void 
UserInterfaceLogic
::ShowSNAP()
{
  m_WizWindows->value(m_GrpSNAPWindows);
  m_WizControlPane->value(m_GrpSNAPPage);

  for(unsigned int i=0;i<3;i++)
    {
    m_SNAPWindow2D[i]->show();
    m_IRISWindow2D[i]->hide();
    }

  // Swap the visible windows
  m_SNAPWindow3D->show();
  m_IRISWindow3D->hide();

  // Reset the view in the snap window
  m_SNAPWindow3D->ResetView();

  // Go to the first page in the SNAP wizard
  SetActiveSegmentationPipelinePage( 0 );

  // Repaint everything
  RedrawWindows();
}

void 
UserInterfaceLogic
::InitializeUI()
{
  // Make the menu bar global
  m_MenubarMain->global();

  int i;
  // Register the GUI with its children
  for (i=0; i<3; i++) m_IRISWindow2D[i]->Register(i,this);
  m_IRISWindow3D->Register(3,this);

  for (i = 0; i < 3; i++) m_SNAPWindow2D[i]->Register(i,this);
  m_SNAPWindow3D->Register(3,this);

  // Set local variables
  m_FileLoaded = 0;
  m_SegmentationLoaded = 0;

  // Sync global state to GUI
  m_BtnCrosshairsMode->setonly();
  // SetToolbarMode(CROSSHAIRS_MODE);

  m_GlobalState->SetSegmentationAlpha(128);
  m_InIRISLabelOpacity->Fl_Valuator::value(128);

  // Initialize the color map for the first time
  UpdateColorLabelMenu();

  // Window title
  this->UpdateMainLabel();
  m_OutMessage->value("Welcome to SnAP; select File->Load->Grey Data to begin");

  m_GlobalState->SetShowSpeed(false);

  m_InSNAPLabelOpacity->Fl_Valuator::value(128);

  // Initialize the highlighted bubble
  m_HighlightedBubble = 0;

  //this should probably go into the .h, a #define or something
  m_InStepSize->add("1");
  m_InStepSize->add("2");
  m_InStepSize->add("5");
  m_InStepSize->add("10");

  // Callbacks
  if (m_GlobalState->GetSnakeMode() == IN_OUT_SNAKE) 
    {
    OnInOutSnakeSelect();
    }
  else 
    {
    OnEdgeSnakeSelect();
    }

  // Apply the special appearance settings that determine startup behavior
  if(m_AppearanceSettings->GetFlagLinkedZoomByDefault())
    {
    m_ChkLinkedZoom->value(1);
    m_InZoomPercentage->value(100.0);
    OnLinkedZoomChange();
    }
}

void 
UserInterfaceLogic
::UpdateColorLabelSelection()
{
  // Set the current drawing label
  LabelType iDrawing = m_GlobalState->GetDrawingColorLabel();
  LabelType iDrawOver = m_GlobalState->GetOverWriteColorLabel();

  // Select the drawing label
  for(size_t i = 0; i < m_InDrawingColor->size(); i++)
    if(iDrawing == (LabelType) m_InDrawingColor->menu()[i].user_data())
      m_InDrawingColor->value(i);

  // Set the draw over label
  if(m_GlobalState->GetCoverageMode() == PAINT_OVER_ALL)
    m_InDrawOverColor->value(0);
  else if (m_GlobalState->GetCoverageMode() == PAINT_OVER_COLORS)
    m_InDrawOverColor->value(1);
  else for(size_t j = 0; j < m_InDrawingColor->size(); j++)
    if(iDrawOver == (LabelType) m_InDrawingColor->menu()[j].user_data())
      m_InDrawOverColor->value(j + 2);
}

void 
UserInterfaceLogic
::UpdateColorLabelMenu() 
{  
  // Set up the over-paint label
  m_InDrawOverColor->clear();
  m_InDrawOverColor->add("All labels");
  m_InDrawOverColor->add("Visible labels");
  m_InDrawOverColor->add("Clear label", NULL, NULL, (void *) 0);
  
  // Set up the draw color
  m_InDrawingColor->clear();
  m_InDrawingColor->add("Clear label", NULL, NULL, (void *) 0);

  // Add each of the color labels to the drop down boxes
  if (m_FileLoaded)
    {
    for (size_t i = 1; i < MAX_COLOR_LABELS; i++) 
      {
      ColorLabel cl = m_Driver->GetColorLabelTable()->GetColorLabel(i);
      if (cl.IsValid()) 
        {
        // Add these entries
        m_InDrawOverColor->add(cl.GetLabel(), NULL, NULL, (void *) i);
        m_InDrawingColor->add(cl.GetLabel(), NULL, NULL, (void *) i);
        }
      }
    }
  
  // Update the color label that is currently selected
  UpdateColorLabelSelection();
  
  // Update the color chips
  UpdateColorChips();
}

void 
UserInterfaceLogic
::RedrawWindows() 
{
  if(m_WizWindows->value() == m_GrpSNAPWindows) 
    {
    m_SNAPWindow2D[0]->redraw();
    m_SNAPWindow2D[1]->redraw();
    m_SNAPWindow2D[2]->redraw();
    m_SNAPWindow3D->redraw();
    m_GrpSNAPCurrentColor->redraw();
    }
  else
    {
    m_IRISWindow2D[0]->redraw();
    m_IRISWindow2D[1]->redraw();
    m_IRISWindow2D[2]->redraw();
    m_IRISWindow3D->redraw();
    }
}

void 
UserInterfaceLogic
::ResetScrollbars() 
{
  // Get the cursor position in image coordinated
  Vector3d cursor = to_double(m_GlobalState->GetCrosshairsPosition());

  // Update the correct scroll bars
  for (unsigned int dim=0; dim<3; dim++)
    {
    // What image axis does dim correspond to?
    unsigned int imageAxis = GetImageAxisForDisplayWindow(dim);

    if (!m_GlobalState->GetSNAPActive())
      {
      // IRIS Scrollbars (notice the negation of the value!)
      m_InIRISSliceSlider[dim]->Fl_Valuator::value( -cursor[imageAxis] );      
      }
    else
      {
      // SNAP Scrollbars (notice the negation of the value!)
      m_InSNAPSliceSlider[dim]->Fl_Valuator::value( -cursor[imageAxis] );      
      }

    // Update the little display box at the bottom of the scroll bar
    UpdatePositionDisplay(dim);
    }
}

void 
UserInterfaceLogic
::OnCrosshairPositionUpdate()
{
  this->ResetScrollbars();
  this->UpdateImageProbe();
}

void 
UserInterfaceLogic
::UpdateColorChips()
{
  uchar index = m_GlobalState->GetDrawingColorLabel();
  uchar rgb[3];
  m_Driver->GetColorLabelTable()->GetColorLabel(index).GetRGBVector(rgb);
  m_GrpCurrentColor->color(fl_rgb_color(rgb[0], rgb[1], rgb[2]));
  m_GrpCurrentColor->redraw();

  if(m_GlobalState->GetCoverageMode() == PAINT_OVER_ONE)
    {
    index = m_GlobalState->GetOverWriteColorLabel();
    m_Driver->GetColorLabelTable()->GetColorLabel(index).GetRGBVector(rgb);
    m_OutDrawOverColor->color(fl_rgb_color(rgb[0], rgb[1], rgb[2]));
    m_OutDrawOverColor->redraw();
    }
  else
    {
    rgb[0]=rgb[1]=rgb[2]=192;
    m_OutDrawOverColor->color(fl_rgb_color(rgb[0],rgb[1],rgb[2]));
    m_OutDrawOverColor->redraw();
    }
}

void 
UserInterfaceLogic
::OnDrawingLabelUpdate()
{
  // Get the drawing label that was selected
  LabelType iLabel = (LabelType) m_InDrawingColor->mvalue()->user_data();
  
  // Set the global state
  m_GlobalState->SetDrawingColorLabel((LabelType) iLabel);
  
  // Update the color chips
  this->UpdateColorChips();
}

void 
UserInterfaceLogic
::OnDrawOverLabelUpdate()
{
  // See what the user selected
  if(m_InDrawOverColor->value() == 0)
    // The first menu item is 'paint over all'
    m_GlobalState->SetCoverageMode(PAINT_OVER_ALL);
  else if(m_InDrawOverColor->value() == 1)
    // The first menu item is 'paint over visible'
    m_GlobalState->SetCoverageMode(PAINT_OVER_COLORS);
  else
    {
    LabelType iLabel = (LabelType) m_InDrawOverColor->mvalue()->user_data();
    m_GlobalState->SetCoverageMode(PAINT_OVER_ONE);
    m_GlobalState->SetOverWriteColorLabel(iLabel);
    }

  // Update the color chips
  this->UpdateColorChips();
}

void 
UserInterfaceLogic
::UpdateImageProbe() 
{
  // Code common to SNAP and IRIS
  Vector3ui crosshairs = m_GlobalState->GetCrosshairsPosition();
    
  // String streams for different labels
  IRISOStringStream sGrey,sSegmentation,sSpeed;

  // Get the grey intensity
  sGrey << m_Driver->GetCurrentImageData()->GetGrey()->GetVoxel(crosshairs);

  // Get the segmentation lavel intensity
  int iSegmentation = 
    (int)m_Driver->GetCurrentImageData()->GetSegmentation()->GetVoxel(crosshairs);
  sSegmentation << iSegmentation;

  // Update the cursor position in the image info window
  Vector3d xPosition = m_Driver->GetCurrentImageData()->GetGrey()->
    TransformVoxelIndexToPosition(crosshairs);
  for(size_t d = 0; d < 3; d++)
    {
    m_OutImageInfoCursorIndex[d]->value(crosshairs[d]);
    m_OutImageInfoCursorPosition[d]->value(xPosition[d]);
    }
  
  // The rest depends on the current mode
  if(m_GlobalState->GetSNAPActive())
    {
    // Fill the grey and label outputs
    m_OutSNAPProbe->value(sGrey.str().c_str());
    m_OutSNAPLabelProbe->value(sSegmentation.str().c_str());

    // Get a pointer to the speed image wrapper
    SNAPImageData *snap = m_Driver->GetSNAPImageData();

    // Get the speed value (if possible)
    if(m_GlobalState->GetSpeedPreviewValid())
      {
      // Speed preview is being shown.  Get a preview pixel
      sSpeed << std::setprecision(4) << 
        snap->GetSpeed()->GetPreviewVoxel(crosshairs);
      }
    else if(m_GlobalState->GetSpeedValid())
      {
      // Speed image is valid, i.e., has been properly computed
      sSpeed << std::setprecision(4) << 
        snap->GetSpeed()->GetVoxel(crosshairs);
      }
    else 
      {
      sSpeed << "N/A";
      }

    // Display the speed string
    m_OutSNAPSpeedProbe->value(sSpeed.str().c_str());
    }
  else
    {
    m_OutGreyProbe->value(sGrey.str().c_str());
    m_OutLabelProbe->value(sSegmentation.str().c_str());

    // Get the label description
    ColorLabel cl = 
      m_Driver->GetColorLabelTable()->GetColorLabel(iSegmentation);
    m_OutLabelProbeText->value(cl.GetLabel());      
    }
}

void 
UserInterfaceLogic
::UpdateMainLabel() 
{
  // Will print to this label
  IRISOStringStream mainLabel;

  // Print version
  mainLabel << SNAPSoftVersion << ": ";

  // Get the grey and segmentation file names
  string fnGrey = m_GlobalState->GetGreyFileName();
  string fnSeg = m_GlobalState->GetSegmentationFileName();

  // Grey file name
  if (fnGrey.length()) 
    {
    // Strip the path of the file
    mainLabel << itksys::SystemTools::GetFilenameName(fnGrey.c_str());
    }
  else
    {
    mainLabel << "no Grey img";
    }

  mainLabel << " - ";

  // Segmentation file name
  if (fnSeg.length()) 
    {
    // Strip the path of the file
    mainLabel << itksys::SystemTools::GetFilenameName(fnSeg.c_str());
    }
  else
    {
    mainLabel << "no Seg img";
    }

  // Store the label
  m_MainWindowLabel = mainLabel.str();

  // Apply to the window
  m_WinMain->label(m_MainWindowLabel.c_str());

  return;
}

void 
UserInterfaceLogic
::OnEditLabelsAction()
{
  // Get the currently selected color index
  LabelType iLabel = (LabelType) m_InDrawingColor->mvalue()->user_data();
  m_LabelEditorUI->OnLabelListUpdate(iLabel);

  // Display the label editor
  m_LabelEditorUI->DisplayWindow();
}

void 
UserInterfaceLogic
::UpdateEditLabelWindow() 
{
  /** 
  // Get properties from VoxData
  int index = m_ColorMap[m_InDrawingColor->value()];

  // Get the color label
  ColorLabel cl = m_Driver->GetCurrentImageData()->GetColorLabel(index);
  assert(cl.IsValid());

  // Set widgets in EditLabel window
  m_InEditLabelName->value(cl.GetLabel());
  m_InEditLabelAlpha->value(cl.GetAlpha() / 255.0);
  m_InEditLabelVisibility->value(!cl.IsVisible());
  m_InEditLabelMesh->value(!cl.IsVisibleIn3D());
  m_BtnEditLabelChange->setonly();

  // convert from uchar [0,255] to double [0.0,1.0]
  m_GrpEditLabelColorChooser->rgb( cl.GetRGB(0)/255.0, cl.GetRGB(1)/255.0, cl.GetRGB(2)/255.0 );

  // If this is clear label, assume we're going to add a label
  if (index == 0) 
    {
    m_BtnEditLabelAdd->setonly();
    m_InEditLabelName->value("New Label");
    m_InEditLabelAlpha->value(1.0);
    m_InEditLabelVisibility->clear();
    m_InEditLabelMesh->clear();
    }

  char title[100];
  sprintf(title, "Label No. %d", index);
  m_WinEditLabel->label(title);
  */
  
  
}

/*
void 
UserInterfaceLogic
::ChangeLabelsCallback() 
{
  // Check the new label name
  const char* new_name = m_InEditLabelName->value();
  if (strlen(new_name) == 0) 
    {
    m_OutMessage->value("You must enter a non-null name");
    return;
    }

  // Update the label Choice widgets
  int offset;
  if (m_BtnEditLabelAdd->value() == 1) 
    {  // add a new label
    offset = m_InDrawingColor->add(new_name);
    m_InDrawOverColor->add(new_name);   
    }
  else 
    {  // Replace an old label
    offset = m_InDrawingColor->value();
    if (!offset) 
      {
      m_OutMessage->value("You cannot recolor the clear color");
      return;
      }
    m_InDrawingColor->replace(offset, new_name);
    m_InDrawOverColor->replace(offset+2, new_name);
    }
  m_InDrawingColor->value(offset);
  m_InDrawingColor->set_changed();

  // Search for a place to put this new label
  if (m_ColorMap[offset] <0) 
    {
    int i;
    for (i=1; i<256 && m_Driver->GetCurrentImageData()->GetColorLabel(i).IsValid(); i++) 
      {
      }

    m_ColorMap[offset] = i;
    }

  unsigned char rgb[3];
  rgb[0] = (uchar) ( 255*m_GrpEditLabelColorChooser->r() );
  rgb[1] = (uchar) ( 255*m_GrpEditLabelColorChooser->g() );
  rgb[2] = (uchar) ( 255*m_GrpEditLabelColorChooser->b() );

  // Send changes to the Voxel Data Structure
  ColorLabel cl = m_Driver->GetCurrentImageData()->GetColorLabel(m_ColorMap[offset]);
  cl.SetRGBVector(rgb);
  cl.SetAlpha(static_cast<unsigned char>(255 * m_InEditLabelAlpha->value()));
  cl.SetVisible(!m_InEditLabelVisibility->value());
  cl.SetVisibleIn3D(!m_InEditLabelMesh->value());
  cl.SetLabel(new_name);
  cl.SetValid(true);

  // Make sure that the display windows get the updated color labels
  m_Driver->GetCurrentImageData()->SetColorLabel(m_ColorMap[offset],cl);

  // Set the new current color in the GUI
  m_GrpCurrentColor->color(fl_rgb_color(rgb[0], rgb[1], rgb[2]));

  m_GlobalState->SetDrawingColorLabel((unsigned char) m_ColorMap[offset]);

  // The segmentation has become dirty
  m_Activation->UpdateFlag(UIF_IRIS_MESH_DIRTY, true);

  // Redraw the windows 
  this->RedrawWindows();
  m_WinMain->redraw();
}
*/

/** 
 * This method is executed when the labels have been changed by the label editor
 * or by loading them from file
 */
void
UserInterfaceLogic
::OnLabelListUpdate()
{
  // Update the little color chips that show the current label
  UpdateColorChips();

  // Update the drop down box of labels to reflect the current settings
  UpdateColorLabelMenu();

  // The label wrappers depend on color maps
  m_Driver->GetCurrentImageData()->GetSegmentation()->UpdateColorMappingCache();
  
  // Repaint the windows
  RedrawWindows();
}

void 
UserInterfaceLogic
::OnSliceSliderChange(int id) 
{
  // Get the new value depending on the current state
  unsigned int value = (unsigned int)
    (m_GlobalState->GetSNAPActive() ? 
     - m_InSNAPSliceSlider[id]->value() :
     - m_InIRISSliceSlider[id]->value());
  
  // Get the cursor position
  Vector3ui cursor = m_GlobalState->GetCrosshairsPosition();

  // Determine which image axis the display window 'id' corresponds to
  unsigned int imageAxis = GetImageAxisForDisplayWindow(id);

  // Update the cursor
  cursor[imageAxis] = value;

  // TODO: Unify this!
  m_Driver->GetCurrentImageData()->SetCrosshairs(cursor);
  m_GlobalState->SetCrosshairsPosition(cursor);

  // Update the little display box under the scroll bar
  UpdatePositionDisplay(id);

  // Update the probe values
  UpdateImageProbe();

  // Repaint the windows
  RedrawWindows();
}

void 
UserInterfaceLogic
::UpdatePositionDisplay(int id) 
{
  // Dump out the slice index of the given window
  IRISOStringStream sIndex;

  // Code depends on SNAP/IRIS mode
  if(m_GlobalState->GetSNAPActive())
    {
    sIndex << std::setw(4) << (1 - m_InSNAPSliceSlider[id]->value());
    sIndex << " of ";
    sIndex << (1 - m_InSNAPSliceSlider[id]->minimum());
    m_OutSNAPSliceIndex[id]->value(sIndex.str().c_str());
    }
  else
    {
    sIndex << std::setw(4) << (1 - m_InIRISSliceSlider[id]->value());
    sIndex << " of ";
    sIndex << (1 - m_InIRISSliceSlider[id]->minimum());
    m_OutIRISSliceIndex[id]->value(sIndex.str().c_str());
    }    
}

void 
UserInterfaceLogic
::OnAcceptPolygonAction(unsigned int window)
{
  if(m_IRISWindow2D[window]->AcceptPolygon())
    {
    // The polygon update was successful
    OnPolygonStateUpdate(window);

    // The segmentation has been dirtied
    m_Activation->UpdateFlag(UIF_IRIS_MESH_DIRTY, true);
    }
  else
    {
    // Probably, the draw-over-label is set incorrectly!
    fl_message(
      "The segmentation was not updated. Most likely, the 'Draw Over' label \n"
      "is set incorrectly. Change it to 'All Labels' or 'Clear Label'.");
    }
}

void 
UserInterfaceLogic
::OnInsertIntoPolygonSelectedAction(unsigned int window)
{
  m_IRISWindow2D[window]->InsertPolygonPoints();  
  OnPolygonStateUpdate(window);
}

void 
UserInterfaceLogic
::OnDeletePolygonSelectedAction(unsigned int window)
{
  m_IRISWindow2D[window]->DeleteSelectedPolygonPoints();
  OnPolygonStateUpdate(window);
}

void 
UserInterfaceLogic
::OnPastePolygonAction(unsigned int window)
{
  m_IRISWindow2D[window]->PastePolygon();
  OnPolygonStateUpdate(window);
}

void 
UserInterfaceLogic
::OnPolygonStateUpdate(unsigned int id)
{
  // Get the drawing object
  PolygonDrawing *draw = m_IRISWindow2D[id]->GetPolygonDrawing();

  if (draw->GetState() == PolygonDrawing::INACTIVE_STATE) 
    {
    // There is no active polygon
    m_BtnPolygonAccept[id]->deactivate();
    m_BtnPolygonDelete[id]->deactivate();
    m_BtnPolygonInsert[id]->deactivate();

    // Check if there is a cached polygon for pasting
    if(draw->GetCachedPolygon()) 
      m_BtnPolygonPaste[id]->activate();
    else
      m_BtnPolygonPaste[id]->deactivate();      
    }
  else if(draw->GetState() == PolygonDrawing::DRAWING_STATE)
    {
    // There is no active polygon
    m_BtnPolygonAccept[id]->deactivate();
    m_BtnPolygonDelete[id]->deactivate();
    m_BtnPolygonInsert[id]->deactivate();
    m_BtnPolygonPaste[id]->deactivate();
    }
  else
    {
    m_BtnPolygonAccept[id]->activate();
    m_BtnPolygonPaste[id]->deactivate();

    if(draw->GetSelectedVertices())
      {
      m_BtnPolygonDelete[id]->activate();
      m_BtnPolygonInsert[id]->activate();
      }
    else
      {
      m_BtnPolygonDelete[id]->deactivate();
      m_BtnPolygonInsert[id]->deactivate();
      }
    }

  // Redraw the windows
  RedrawWindows();
}

void 
UserInterfaceLogic
::OnMenuShowDisplayOptions()
{
  m_DlgAppearance->ShowDialog();
}


void
UserInterfaceLogic
::OnIRISWindowFocus(unsigned int i)
{
  Fl_Group *panels[] = 
    { m_GrpIRISSlicePanel[0], m_GrpIRISSlicePanel[1], m_GrpIRISSlicePanel[2], m_GrpIRISView3D };
  Fl_Gl_Window *boxes[] = 
    { m_IRISWindow2D[0], m_IRISWindow2D[1], m_IRISWindow2D[2], m_IRISWindow3D };

  // The dimensions of the parent window
  int x = m_GrpIRISWindows->x(), y = m_GrpIRISWindows->y();
  int w = m_GrpIRISWindows->w(), h = m_GrpIRISWindows->h();

  // Check if this is an expansion or a collapse operation
  if( panels[i]->w() == w )
    {
    // Restore all panels to original configuration
    panels[0]->resize(x, y, w >> 1, h >> 1);
    panels[1]->resize(x + (w >> 1), y, w - (w >> 1), h >> 1);
    panels[3]->resize(x, y + (h >> 1), w >> 1, h - (h >> 1));
    panels[2]->resize(x + (w >> 1), y + (h >> 1), w - (w >> 1), h - (h >> 1));
    
    // Remove the current panel
    m_GrpIRISWindows->remove(panels[i]);

    // Show everything
    for(unsigned int j = 0; j < 4; j++)
      {
      m_GrpIRISWindows->add(panels[j]);
      panels[j]->show();
      boxes[j]->show();
      panels[j]->redraw();
      }
    }
  else 
    {
    for(unsigned int j = 0; j < 4; j++)
      {
      if(i != j)
        {
        panels[j]->hide();
        boxes[j]->hide();
        m_GrpIRISWindows->remove(panels[j]);
        }
      }

    panels[i]->resize(
      m_GrpIRISWindows->x(),m_GrpIRISWindows->y(),
      m_GrpIRISWindows->w(),m_GrpIRISWindows->h());
    m_GrpIRISWindows->resizable(panels[i]);
    panels[i]->redraw();
    }
}

void
UserInterfaceLogic
::OnImageGeometryUpdate()
{
  if(!m_Driver->GetCurrentImageData()->IsGreyLoaded())
    return;

  // Set the crosshairs to the center of the image
  Vector3ui size = m_Driver->GetCurrentImageData()->GetVolumeExtents();
  Vector3ui xCross = size / ((unsigned int) 2);
  m_GlobalState->SetCrosshairsPosition(xCross);
  m_Driver->GetCurrentImageData()->SetCrosshairs(xCross);

  // Update the crosshairs display
  this->OnCrosshairPositionUpdate();

  // Update the source for slice windows as well as scroll bars
  if(m_GlobalState->GetSNAPActive())
    {
    for (unsigned int i=0; i<3; i++) 
      {
      // Connect slices to windows
      m_SNAPWindow2D[i]->InitializeSlice(m_Driver->GetCurrentImageData());
      
      // Get the image axis that corresponds to the display window i
      unsigned int imageAxis = GetImageAxisForDisplayWindow(i);

      // Notice the sliders have a negative range!  That's so that the 1 position is at the 
      // bottom.  We need to always negate the slider values
      m_InSNAPSliceSlider[i]->range( 1.0 - size[imageAxis], 0.0 );
      m_InSNAPSliceSlider[i]->slider_size( 1.0/ size[imageAxis] );
      m_InSNAPSliceSlider[i]->linesize(1);
      }
    }
  else
    {
    for (unsigned int i=0; i<3; i++) 
      {
      m_IRISWindow2D[i]->InitializeSlice(m_Driver->GetCurrentImageData());
      
      // Get the image axis that corresponds to the display window i
      unsigned int imageAxis = GetImageAxisForDisplayWindow(i);

      // As with the other sliders, the range is negative
      m_InIRISSliceSlider[i]->range( 1.0 - size[imageAxis], 0.0 );
      m_InIRISSliceSlider[i]->slider_size( 1.0/size[imageAxis] );
      m_InIRISSliceSlider[i]->linesize(1);
      }      
    }

  // Group the three windows inside the window coordinator
  m_SliceCoordinator->RegisterWindows(
    reinterpret_cast<GenericSliceWindow **>(m_IRISWindow2D));    

  // Reset the view in 2D windows to fit
  m_SliceCoordinator->ResetViewToFitInAllWindows();
}
  
void 
UserInterfaceLogic
::OnIRISMeshResetViewAction()
{
  m_IRISWindow3D->ResetView();
  m_IRISWindow3D->redraw();
}

void 
UserInterfaceLogic
::OnIRISMeshAcceptAction()
{
  m_IRISWindow3D->Accept();
  m_IRISWindow3D->redraw();
  
  m_Activation->UpdateFlag(UIF_IRIS_MESH_ACTION_PENDING, false);
  m_Activation->UpdateFlag(UIF_IRIS_MESH_DIRTY, true);
}

void 
UserInterfaceLogic
::OnIRISMeshUpdateAction()
{
  m_IRISWindow3D->UpdateMesh();
  m_IRISWindow3D->redraw();
  
  m_Activation->UpdateFlag(UIF_IRIS_MESH_DIRTY, false);
}

void
UserInterfaceLogic
::OnIRISMeshEditingAction()
{
  m_Activation->UpdateFlag(UIF_IRIS_MESH_ACTION_PENDING, true);
}

void
UserInterfaceLogic
::OnIRISMeshDisplaySettingsUpdate()
{
  m_Activation->UpdateFlag(UIF_IRIS_MESH_DIRTY, true);
}

void 
UserInterfaceLogic
::OnSNAPMeshResetViewAction()
{
  m_SNAPWindow3D->ResetView();
  m_SNAPWindow3D->redraw();
}

void
UserInterfaceLogic
::OnSNAPMeshUpdateAction()
{
  m_SNAPWindow3D->UpdateMesh();
  m_SNAPWindow3D->redraw();

  m_Activation->UpdateFlag(UIF_SNAP_MESH_DIRTY, false);
}

void 
UserInterfaceLogic
::OnSNAPMeshContinuousUpdateAction()
{
  if (m_ChkContinuousView3DUpdate->value()) 
    {
    m_SNAPWindow3D->UpdateMesh();
    m_Activation->UpdateFlag(UIF_SNAP_MESH_CONTINUOUS_UPDATE, true);
    }
  else 
    {
    m_Activation->UpdateFlag(UIF_SNAP_MESH_CONTINUOUS_UPDATE, false);
    }
}

void 
UserInterfaceLogic
::SetToolbarMode(ToolbarModeType mode)
{
  // There must be an active image before we do this stuff
  assert(m_Driver->GetCurrentImageData()->IsGreyLoaded());

  // Set the mode on the toolbar
  m_GlobalState->SetToolbarMode(mode);

  // Set the mode of each window
  for(unsigned int i=0;i<3;i++)
    {
    switch(mode) 
      {
    case CROSSHAIRS_MODE:
      m_IRISWindow2D[i]->EnterCrosshairsMode();
      m_SNAPWindow2D[i]->EnterCrosshairsMode();
      m_TabsToolOptions->value(m_GrpToolOptionCrosshairs);
      break;

    case NAVIGATION_MODE:
      m_IRISWindow2D[i]->EnterZoomPanMode();
      m_SNAPWindow2D[i]->EnterZoomPanMode();
      m_TabsToolOptions->value(m_GrpToolOptionZoomPan);
      break;

    case POLYGON_DRAWING_MODE:
      m_IRISWindow2D[i]->EnterPolygonMode();
      m_TabsToolOptions->value(m_GrpToolOptionPolygon);
      break;

    case ROI_MODE:
      m_IRISWindow2D[i]->EnterRegionMode();
      m_TabsToolOptions->value(m_GrpToolOptionSnAP);
      break;

    default:
      break;  
      }

    // Enable the polygon editing windows
    if(mode == POLYGON_DRAWING_MODE)
      m_GrpPolygonEdit[i]->show();
    else
      m_GrpPolygonEdit[i]->hide();

    }

  // Redraw the windows
  RedrawWindows();
}

void 
UserInterfaceLogic
::SetToolbarMode3D(ToolbarMode3DType mode)
{
  switch (mode)
    {
    case TRACKBALL_MODE:
      m_IRISWindow3D->EnterTrackballMode();
      m_SNAPWindow3D->EnterTrackballMode();
      break;

    case CROSSHAIRS_3D_MODE:
      m_IRISWindow3D->EnterCrosshairsMode();
      m_SNAPWindow3D->EnterCrosshairsMode();
      break;

    case SPRAYPAINT_MODE:
      m_IRISWindow3D->EnterSpraypaintMode();
      break;

    case SCALPEL_MODE:
      m_IRISWindow3D->EnterScalpelMode();
      break;

    default:
      break;  
    }

  // Redraw the 3D windows
  if(m_WizWindows->value() == m_GrpSNAPWindows) 
    {
    m_SNAPWindow3D->redraw();
    }
  else
    {
    m_IRISWindow3D->redraw();
    }
}


void 
UserInterfaceLogic
::OnMenuWriteVoxelCounts() 
{
  // Display the load labels dialog
  m_DlgVoxelCountsIO->SetTitle("Save Volume Statistics");
  m_DlgVoxelCountsIO->DisplaySaveDialog(
    m_SystemInterface->GetHistory("VolumeStatistics"));
}

void 
UserInterfaceLogic
::OnWriteVoxelCountsAction() 
{
  // Get the selected file name
  const char *file = m_DlgVoxelCountsIO->GetFileName();

  // Try writing
  try 
    {
    // Compute the statistics and write them to file
    m_Driver->ExportSegmentationStatistics(file);
    
    // Update the history
    m_SystemInterface->UpdateHistory("VolumeStatistics",file);
    }
  catch (itk::ExceptionObject &exc) 
    {
    // Alert the user to the failure
    fl_alert("Error writing volume statistics:\n%s",exc.GetDescription());

    // Rethrow the exception
    throw;
    }
}

void 
UserInterfaceLogic
::OnGreyImageUnload()
{
  // This method is called when a grey image gets unloaded.  It saves the 
  // current settings and associates them with the grey image file
  
  // Make sure there is actually an image
  string fnGrey = m_GlobalState->GetGreyFileName();
  if(fnGrey.length())
    m_SystemInterface->AssociateCurrentSettingsWithCurrentImageFile(
      fnGrey.c_str(),m_Driver);
}

void 
UserInterfaceLogic
::OnGreyImageUpdate()
{
  // Blank the screen - useful on a load of new grey data when there is 
  // already a segmentation file present
  m_IRISWindow3D->ClearScreen(); 

  // Flip over to the toolbar page
  m_WizControlPane->value(m_GrpToolbarPage);

  // Enable some menu items
  m_Activation->UpdateFlag(UIF_IRIS_WITH_GRAY_LOADED, true);

  // Image geometry has changed
  OnImageGeometryUpdate();

  // We now have valid grey data
  m_FileLoaded = 1;
  m_SegmentationLoaded = 0;

  m_InDrawingColor->set_changed();
  m_InDrawOverColor->set_changed();

  m_GlobalState->SetGreyExtension((char *)m_WizGreyIO->GetFileName());
  
  // Update the label of the UI
  UpdateMainLabel();

  // Get the largest image region
  GlobalState::RegionType roi = m_Driver->GetIRISImageData()->GetImageRegion();
  
  // Check if the region is real
  if (roi.GetNumberOfPixels() == 0) 
    {
    m_Activation->UpdateFlag( UIF_IRIS_ROI_VALID, false );
    m_GlobalState->SetIsValidROI(false);
    } 
  else 
    {
    m_Activation->UpdateFlag( UIF_IRIS_ROI_VALID, true );
    m_GlobalState->SetSegmentationROI(roi);
    m_GlobalState->SetIsValidROI(true);
    }   

  // Update the polygon buttons
  OnPolygonStateUpdate(0);
  OnPolygonStateUpdate(1);
  OnPolygonStateUpdate(2);

  // Update the image info window controls
  GreyImageWrapper *wrpGrey = m_Driver->GetCurrentImageData()->GetGrey();
  for(size_t d = 0; d < 3; d++)
    {
    m_OutImageInfoDimensions[d]->value(wrpGrey->GetSize()[d]);
    m_OutImageInfoOrigin[d]->value(wrpGrey->GetImage()->GetOrigin()[d]);
    m_OutImageInfoSpacing[d]->value(wrpGrey->GetImage()->GetSpacing()[d]);
    }
  m_OutImageInfoRange[0]->value(wrpGrey->GetImageMin());
  m_OutImageInfoRange[1]->value(wrpGrey->GetImageMax());

  // Now that we've loaded the image, check if there are any settings 
  // associated with it.  If there are, give the user an option to restore 
  // these settings
  Registry associated;
  if(m_SystemInterface->FindRegistryAssociatedWithFile(
    m_GlobalState->GetGreyFileName(),associated))
    {
    // Display the restore associated settings dialog
    m_DlgRestoreSettings->DisplayDialog(this,&associated);

    // Restore the settings if requested
    if(m_DlgRestoreSettings->GetRestoreSettings())
      {
      // Load the settings using RegistryIO
      SNAPRegistryIO rio;
      rio.ReadImageAssociatedSettings(associated,m_Driver,
                                      m_DlgRestoreSettings->GetRestoreLabels(),
                                      m_DlgRestoreSettings->GetRestorePreprocessing(),
                                      m_DlgRestoreSettings->GetRestoreParameters(),
                                      m_DlgRestoreSettings->GetRestoreDisplayOptions());

      // Update the label display if necessary
      if(m_DlgRestoreSettings->GetRestoreLabels())
        {
        // This will reset the color label table
        OnLabelListUpdate();

        // Update the opacity slider
        m_InIRISLabelOpacity->value(m_GlobalState->GetSegmentationAlpha());

        // Update the polygon inversion state
        m_ChkInvertPolygon->value(m_GlobalState->GetPolygonInvert() ? 1 : 0);
        }          

      // Update the mesh options display if necessary
      // if(m_DlgRestoreSettings->GetRestoreDisplayOptions())
      //  {
      //  FillRenderingOptions();
      //  }
      }
    
    // The associated settings may have been updated in the dialog.
    if(m_DlgRestoreSettings->GetAssociatedSettingsHaveChanged())
      {
      m_SystemInterface->AssociateRegistryWithFile(
        m_GlobalState->GetGreyFileName(),associated); 
      }
    }
    
  // Redraw the crosshairs in the 3D window  
  m_IRISWindow3D->ResetView(); 

  // Redraw the intensity mapping window and controls
  m_IntensityCurveUI->SetCurve(m_Driver->GetIntensityCurve());    
  m_IntensityCurveUI->SetImageWrapper(
    m_Driver->GetCurrentImageData()->GetGrey());
  m_IntensityCurveUI->OnCurveChange();

  // Redraw the user interface
  RedrawWindows();
  m_WinMain->redraw();
}

void 
UserInterfaceLogic
::OnSegmentationImageUpdate()
{
  m_SegmentationLoaded =1; 

  // The list of labels may have changed
  OnLabelListUpdate();

  // Re-Initialize the 2D windows
  // for (unsigned int i=0; i<3; i++) 
  //   m_IRISWindow2D[i]->InitializeSlice(m_Driver->GetCurrentImageData());

  m_IRISWindow3D->ResetView();
  m_Activation->UpdateFlag(UIF_IRIS_MESH_DIRTY, true);
  
  UpdateMainLabel();
  
  RedrawWindows();
  m_WinMain->redraw();
}

/* 
void 
UserInterfaceLogic
::OnSegmentationLabelsUpdate(bool resetCurrentAndDrawOverLabels)
{
  InitColorMap(resetCurrentAndDrawOverLabels);
  m_Activation->UpdateFlag(UIF_IRIS_MESH_DIRTY, true);
  
  m_WinMain->redraw();          
  m_OutMessage->value("Loading Label file successful");
}
*/

void 
UserInterfaceLogic
::OnSpeedImageUpdate()
{
  if(m_GlobalState->GetSpeedValid())
    {
    // Set UI state
    m_Activation->UpdateFlag(UIF_SNAP_SPEED_AVAILABLE, true);

    // Choose to view the preprocessed image
    m_RadioSNAPViewPreprocessed->setonly();

    // Run the callback associated with that change
    OnViewPreprocessedSelect();
    }
  else
    {
    // Set UI state
    m_Activation->UpdateFlag(UIF_SNAP_SPEED_AVAILABLE, false);

    // Choose to view the grey image
    m_RadioSNAPViewOriginal->setonly();

    // Run the callback associated with that change
    OnViewPreprocessedSelect();
    }
}

void 
UserInterfaceLogic
::OnMenuLoadGrey() 
{
  // Set the history for the input wizard
  m_WizGreyIO->SetHistory(m_SystemInterface->GetHistory("GreyImage"));
  
  // Show the input wizard with no file selected
  m_WizGreyIO->DisplayInputWizard("");

  // If the load operation was successful, populate the data and GUI with the
  // new image
  if(m_WizGreyIO->IsImageLoaded()) 
    {
    // Update the system's history list
    m_SystemInterface->UpdateHistory("GreyImage",m_WizGreyIO->GetFileName());

    // Perform the clean-up tasks before loading the image
    OnGreyImageUnload();

    // Send the image and RAI to the IRIS application driver
    m_Driver->UpdateIRISGreyImage(m_WizGreyIO->GetLoadedImage(),
                                  m_WizGreyIO->GetLoadedImageRAI());

    // Save the filename
    m_GlobalState->SetGreyFileName(m_WizGreyIO->GetFileName());

    // Update the user interface accordingly
    OnGreyImageUpdate();
    }
}

void 
UserInterfaceLogic
::OnLoadLabelsAction() 
{
  // Get the selected file name
  const char *file = m_DlgLabelsIO->GetFileName();

  // Try reading the file
  try 
    {
    // Read the labels from file
    m_Driver->GetColorLabelTable()->LoadFromFile(file);

    // Reset the current drawing and overlay labels
    m_GlobalState->SetDrawingColorLabel(
      m_Driver->GetColorLabelTable()->GetFirstValidLabel());
    m_GlobalState->SetOverWriteColorLabel(0);
    
    // Update the label editor window
    m_LabelEditorUI->OnLabelListUpdate(
      m_GlobalState->GetDrawingColorLabel());

    // Update the user interface in response
    OnLabelListUpdate();

    // Update the history
    m_SystemInterface->UpdateHistory("LabelDescriptions",file);
    }
  catch (itk::ExceptionObject &exc) 
    {
    // Alert the user to the failure
    fl_alert("Error loading labels:\n%s",exc.GetDescription());

    // Rethrow the exception
    throw;
    }
}

void 
UserInterfaceLogic
::OnSaveLabelsAction()
{
  // Get the selected file name
  const char *file = m_DlgLabelsIO->GetFileName();

  // Try writing
  try 
    {
    // Write labels to the file
    m_Driver->GetColorLabelTable()->SaveToFile(file);
    
    // Update the history
    m_SystemInterface->UpdateHistory("LabelDescriptions",file);
    }
  catch (itk::ExceptionObject &exc) 
    {
    // Alert the user to the failure
    fl_alert("Error writing labels:\n%s",exc.GetDescription());

    // Rethrow the exception
    throw;
    }
}

void 
UserInterfaceLogic
::OnMenuLoadLabels() 
{
  // Display the load labels dialog
  m_DlgLabelsIO->SetTitle("Load Labels");
  m_DlgLabelsIO->DisplayLoadDialog(
    m_SystemInterface->GetHistory("LabelDescriptions"));
}

void UserInterfaceLogic
::OnMenuSaveLabels() 
{
  // Display the save labels dialog
  m_DlgLabelsIO->SetTitle("Save Labels");
  m_DlgLabelsIO->DisplaySaveDialog(
    m_SystemInterface->GetHistory("LabelDescriptions"));
}

void UserInterfaceLogic
::OnMenuExportSlice(unsigned int iSlice)
{
  // We need to get a filename for the export
  char* fName = fl_file_chooser(
    "Export Slice As", "Image Files (*.{png,jpg,gif,tiff})", NULL);
  
  if(fName)
    m_Driver->ExportSlice(iSlice, fName);
}

void UserInterfaceLogic
::OnMenuLoadSegmentation() 
{
  // Grey image should be loaded
  assert(m_Driver->GetCurrentImageData()->IsGreyLoaded());

  // Should not be in SNAP mode
  assert(!m_GlobalState->GetSNAPActive());
  
  // Set up the input wizard with the grey image
  m_WizSegmentationIO->SetGreyImage(
    m_Driver->GetCurrentImageData()->GetGrey()->GetImage());

  // Set the history for the input wizard
  m_WizSegmentationIO->SetHistory(
    m_SystemInterface->GetHistory("SegmentationImage"));
  
  // Show the input wizard
  m_WizSegmentationIO->DisplayInputWizard(
    m_GlobalState->GetSegmentationFileName());

  // If the load operation was successful, populate the data and GUI with the
  // new image
  if(m_WizSegmentationIO->IsImageLoaded()) 
    {
    // Update the system's history list
    m_SystemInterface->UpdateHistory(
      "SegmentationImage",m_WizSegmentationIO->GetFileName());

    // Send the image and RAI to the IRIS application driver
    m_Driver->UpdateIRISSegmentationImage(m_WizSegmentationIO->GetLoadedImage());

    // Save the segmentation file name
    m_GlobalState->SetSegmentationFileName(m_WizSegmentationIO->GetFileName());

    // Update the user interface accordingly
    OnSegmentationImageUpdate();
    }

  // Disconnect the input wizard from the grey image
  m_WizSegmentationIO->SetGreyImage(NULL);
}

void UserInterfaceLogic
::OnMenuSaveGreyROI() 
{
  // Better be in snap
  assert(m_GlobalState->GetSNAPActive());

  // Set the history for the wizard
  m_WizGreyIO->SetHistory(m_SystemInterface->GetHistory("GreyImage"));

  // Save the segmentation
  if(m_WizGreyIO->DisplaySaveWizard(
    m_Driver->GetCurrentImageData()->GetGrey()->GetImage(),NULL))
    {
    // Update the history for the wizard
    m_SystemInterface->UpdateHistory(
      "GreyImage",m_WizGreyIO->GetSaveFileName());

    // Some silly feedback
    m_OutMessage->value("Saving segmentation file successful");    
    }
}

void UserInterfaceLogic
::OnMenuSaveSegmentation() 
{
  // Better have a segmentation image
  assert(m_Driver->GetIRISImageData()->IsSegmentationLoaded());

  // Set the history for the wizard
  m_WizSegmentationIO->SetHistory(
    m_SystemInterface->GetHistory("SegmentationImage"));

  // Save the segmentation
  if(m_WizSegmentationIO->DisplaySaveWizard(
    m_Driver->GetIRISImageData()->GetSegmentation()->GetImage(),
    m_GlobalState->GetSegmentationFileName()))
    {
    // Update the history for the wizard
    m_SystemInterface->UpdateHistory(
      "SegmentationImage",m_WizSegmentationIO->GetSaveFileName());

    // Store the new filename
    m_GlobalState->SetSegmentationFileName(
      m_WizSegmentationIO->GetSaveFileName());

    // Some silly feedback
    m_OutMessage->value("Saving segmentation file successful");    
    }
}

void 
UserInterfaceLogic
::OnMenuLoadPreprocessed() 
{
  this->OnLoadPreprocessedImageAction();
}

void
UserInterfaceLogic
::OnMenuLoadAdvection()
{
  typedef itk::Image<float, 3> AdvectionImage;
  AdvectionImage::Pointer imgAdvection[3];
  
  // Preprocessing image can only be loaded in SNAP mode
  assert(m_GlobalState->GetSNAPActive());
  
  // Set up the input wizard with the grey image
  m_WizPreprocessingIO->SetGreyImage(
    m_Driver->GetCurrentImageData()->GetGrey()->GetImage());

  // Load the three advection images as floating point
  bool flagLoadCompleted = true;
  for(unsigned int i=0;i<3;i++)
    {
    // Set the history for the input wizard
    m_WizPreprocessingIO->SetHistory(
      m_SystemInterface->GetHistory("AdvectionImage"));

    // Show the input wizard
    m_WizPreprocessingIO->DisplayInputWizard(
      m_GlobalState->GetAdvectionFileName(i));

    // If the load operation was successful, populate the data and GUI with the
    // new image
    if(m_WizPreprocessingIO->IsImageLoaded()) 
      {
      // Update the system's history list
      m_SystemInterface->UpdateHistory(
        "AdvectionImage",m_WizPreprocessingIO->GetFileName());

      // Update the application with the new speed image
      imgAdvection[i] = m_WizPreprocessingIO->GetLoadedImage();

      // Save the segmentation file name
      m_GlobalState->SetAdvectionFileName(i, m_WizPreprocessingIO->GetFileName());
      }
    else
      {
      flagLoadCompleted = false;
      break;
      }
    }
  
  // Disconnect the input wizard from the grey image
  m_WizPreprocessingIO->SetGreyImage(NULL);

  // Add the advection image to SNAP
  if(flagLoadCompleted)
    m_Driver->GetSNAPImageData()->SetExternalAdvectionField(
      imgAdvection[0],imgAdvection[1],imgAdvection[2]);
}

void 
UserInterfaceLogic
::OnLoadPreprocessedImageAction() 
{
  // Preprocessing image can only be loaded in SNAP mode
  assert(m_GlobalState->GetSNAPActive());
  
  // Set up the input wizard with the grey image
  m_WizPreprocessingIO->SetGreyImage(
    m_Driver->GetCurrentImageData()->GetGrey()->GetImage());

  // Set the history for the input wizard
  m_WizPreprocessingIO->SetHistory(
    m_SystemInterface->GetHistory("PreprocessingImage"));

  // Show the input wizard
  m_WizPreprocessingIO->DisplayInputWizard(
    m_GlobalState->GetPreprocessingFileName());

  // If the load operation was successful, populate the data and GUI with the
  // new image
  if(m_WizPreprocessingIO->IsImageLoaded()) 
    {
    // Update the system's history list
    m_SystemInterface->UpdateHistory(
      "PreprocessingImage",m_WizPreprocessingIO->GetFileName());

    // Update the application with the new speed image
    m_Driver->UpdateSNAPSpeedImage(
      m_WizPreprocessingIO->GetLoadedImage(),
      m_RadSnakeEdge->value() ? EDGE_SNAKE : IN_OUT_SNAKE);
    
    // Save the segmentation file name
    m_GlobalState->SetPreprocessingFileName(m_WizPreprocessingIO->GetFileName());

    // Update the user interface accordingly
    OnSpeedImageUpdate();
    }

  // Disconnect the input wizard from the grey image
  m_WizPreprocessingIO->SetGreyImage(NULL);
}

void 
UserInterfaceLogic
::OnMenuSavePreprocessed() 
{
  // Better have a speed image to save
  assert(m_GlobalState->GetSpeedValid());

  // Set the history for the wizard
  m_WizPreprocessingIO->SetHistory(
    m_SystemInterface->GetHistory("PreprocessingImage"));

  // Save the speed
  if(m_WizPreprocessingIO->DisplaySaveWizard(
    m_Driver->GetSNAPImageData()->GetSpeed()->GetImage(),
    m_GlobalState->GetPreprocessingFileName()))
    {
    // Update the history for the wizard
    m_SystemInterface->UpdateHistory(
      "PreprocessingImage",m_WizPreprocessingIO->GetSaveFileName());
    
    // Store the new filename
    m_GlobalState->SetPreprocessingFileName(
      m_WizPreprocessingIO->GetSaveFileName());

    // Some silly feedback
    m_OutMessage->value("Saving preprocessing file successful");    
    }
}


void UserInterfaceLogic
::OnMenuIntensityCurve() 
{
  // The image should be loaded before bringing up the curve
  assert(m_Driver->GetCurrentImageData()->IsGreyLoaded());

  // Update the curve in the UI dialog (is this necessary?)
  // m_IntensityCurveUI->SetCurve(m_Driver->GetIntensityCurve());    
  // m_IntensityCurveUI->SetImageWrapper(
  //  m_Driver->GetCurrentImageData()->GetGrey());

  // Show the window
  m_IntensityCurveUI->DisplayWindow();
}

void UserInterfaceLogic
::OnIntensityCurveUpdate() 
{
  // Update the image wrapper
  m_Driver->GetCurrentImageData()->GetGrey()->SetIntensityMapFunction(
    m_Driver->GetIntensityCurve());

  // Update the display
  RedrawWindows();
}

void
UserInterfaceLogic
::OnIRISLabelOpacityChange()
{
  m_GlobalState->SetSegmentationAlpha( (unsigned char) m_InIRISLabelOpacity->value());  
  this->RedrawWindows();
}

void
UserInterfaceLogic
::OnSNAPLabelOpacityChange()
{
  m_GlobalState->SetSegmentationAlpha( (unsigned char) m_InSNAPLabelOpacity->value());  
  this->RedrawWindows();
}

void
UserInterfaceLogic
::OnLaunchTutorialAction()
{
  // Find the tutorial file name
  ShowHTMLPage("Tutorial.html");
}

void
UserInterfaceLogic
::ShowHTMLPage(const char *link)
{
  // Get the path to the file name
  string completeLink = string("HTMLHelp/") +  link;
  string file = 
    m_SystemInterface->GetFileInRootDirectory(completeLink.c_str());

  // Show the help window
  m_HelpUI->ShowHelp(file.c_str());  
}

void
UserInterfaceLogic
::ShowSplashScreen()
{
  // Place the window in the center of display
  CenterChildWindowInMainWindow(m_WinSplash);

  // Show a wait cursor
  fl_cursor(FL_CURSOR_WAIT,FL_FOREGROUND_COLOR, FL_BACKGROUND_COLOR);

  // Show the splash screen
  m_WinSplash->show();

  // Make FL update the screen
  Fl::check();

  // Save the time of the splash screen
  m_SplashScreenStartTime = clock();
}

void
UserInterfaceLogic
::HideSplashScreen()
{
  // Wait a second with the splash screen
  while(clock() - m_SplashScreenStartTime < CLOCKS_PER_SEC) {}
  
  // Hide the screen 
  m_WinSplash->hide();

  // Clear the cursor
  fl_cursor(FL_CURSOR_DEFAULT,FL_FOREGROUND_COLOR, FL_BACKGROUND_COLOR);
}

void
UserInterfaceLogic
::UpdateSplashScreen(const char *message)
{
  m_OutSplashMessage->label(message);
  Fl::check();

  // Save the time of the splash screen
  m_SplashScreenStartTime = clock();
}

void
UserInterfaceLogic
::CenterChildWindowInParentWindow(Fl_Window *childWindow,
                                  Fl_Window *parentWindow)
{
  int px = parentWindow->x() + (parentWindow->w() - childWindow->w()) / 2;
  int py = parentWindow->y() + (parentWindow->h() - childWindow->h()) / 2;
  childWindow->resize(px,py,childWindow->w(),childWindow->h());
}

void 
UserInterfaceLogic
::CenterChildWindowInMainWindow(Fl_Window *childWindow)
{
  CenterChildWindowInParentWindow(childWindow,m_WinMain);
}

void
UserInterfaceLogic
::OnResetView2DAction(unsigned int window)
{
  m_SliceCoordinator->ResetViewToFitInOneWindow(window);
  
  // Update the zoom level display
  OnZoomUpdate();
}

void
UserInterfaceLogic
::OnResetAllViews2DAction()
{
  m_SliceCoordinator->ResetViewToFitInAllWindows();
  
  // Update the zoom level display
  OnZoomUpdate();
}

void
UserInterfaceLogic
::OnLinkedZoomChange()
{
  if(m_ChkLinkedZoom->value() > 0)
    {
    m_SliceCoordinator->SetLinkedZoom(true);
    m_Activation->UpdateFlag(UIF_LINKED_ZOOM, true);
    }
  else
    {
    m_SliceCoordinator->SetLinkedZoom(false);
    m_Activation->UpdateFlag(UIF_LINKED_ZOOM, false);
    }
  
  // Update the zoom level display
  OnZoomUpdate();
}

void
UserInterfaceLogic
::OnZoomPercentageChange()
{
  float zoom = m_InZoomPercentage->value();
  m_SliceCoordinator->SetZoomFactorAllWindows(zoom / 100.0f);  
}

void 
UserInterfaceLogic
::OnZoomUpdate()
{
  // This method should be called whenever the zoom changes
  if(m_SliceCoordinator->GetLinkedZoom())
    {
    // Get the zoom from the first window and display it on the screen
    m_InZoomPercentage->value(m_SliceCoordinator->GetCommonZoom() * 100);
    }

  else
    {
    // Otherwise, clear the display
    m_InZoomPercentage->value(0);
    }
}


// Get the window under mouse focus or -1 if none
int 
UserInterfaceLogic
::GetWindowUnderFocus(void)
{
  for(int i = 0; i < 3; i++)
    if(m_SliceCoordinator->GetWindow(i)->GetFocus())
      return i;
  return -1;
}

/*
 *Log: UserInterfaceLogic.cxx
 *Revision 1.35  2005/04/21 18:52:38  pauly
 *ENH: Furhter improvements to SNAP label editor
 *
 *Revision 1.34  2005/04/21 14:46:30  pauly
 *ENH: Improved management and editing of color labels in SNAP
 *
 *Revision 1.33  2005/04/15 19:04:19  pauly
 *ENH: Improved the Intensity Contrast features in SNAP
 *
 *Revision 1.32  2005/04/14 16:35:10  pauly
 *ENH: Added Image Info window to SNAP
 *
 *Revision 1.31  2005/03/08 03:12:51  pauly
 *BUG: Minor bugfixes in SNAP, mostly to the user interface
 *
 *Revision 1.30  2004/09/21 15:50:40  jjomier
 *FIX: vector_multiply_mixed requires template parameters otherwise MSVC cannot deduce them
 *
 *Revision 1.29  2004/09/14 14:11:10  pauly
 *ENH: Added an activation manager to main UI class, improved snake code, various UI fixes and additions
 *
 *Revision 1.28  2004/09/08 12:09:45  pauly
 *ENH: Adapting SNAP to work with stop-n-go function in finite diff. framewk
 *
 *Revision 1.27  2004/08/26 19:43:27  pauly
 *ENH: Moved the Borland code into Common folder
 *
 *Revision 1.26  2004/08/26 18:29:19  pauly
 *ENH: New user interface for configuring the UI options
 *
 *Revision 1.25  2004/08/03 23:26:32  ibanez
 *ENH: Modification for building in multple platforms. By Julien Jomier.
 *
 *Revision 1.24  2004/07/29 14:00:36  pauly
 *ENH: A new interface for changing the appearance of SNAP
 *
 *Revision 1.23  2004/07/24 19:00:06  pauly
 *ENH: Thumbnail UI for slice zooming
 *
 *Revision 1.22  2004/07/22 19:22:49  pauly
 *ENH: Large image support for SNAP. This includes being able to use more screen real estate to display a slice, a fix to the bug with manual segmentation of images larger than the window size, and a thumbnail used when zooming into the image.
 *
 *Revision 1.21  2004/07/21 18:17:45  pauly
 *ENH: Enhancements to the way that the slices are displayed
 *
 *Revision 1.20  2004/03/19 00:54:48  pauly
 *ENH: Added the ability to externally load the advection image
 *
 *Revision 1.19  2004/01/24 18:21:00  king
 *ERR: Merged warning fixes from ITK 1.6 branch.
 *
 *Revision 1.18.2.1  2004/01/24 18:16:50  king
 *ERR: Fixed warnings.
 *
 *Revision 1.18  2003/12/12 19:34:01  pauly
 *FIX: Trying to get everything to compile again after API changes
 *
 *Revision 1.17  2003/12/10 23:20:15  hjohnson
 *UPD: Code changes to allow compilation under linux.
 *
 *Revision 1.16  2003/12/10 02:21:14  lorensen
 *ENH: Spacing is now a FixedArray.
 *
 *Revision 1.15  2003/12/07 21:19:32  pauly
 *ENH: SNAP can now resample the segmentation ROI, facilitating
 *multires segmentation and segmentation of anisotropic images
 *
 *Revision 1.14  2003/12/07 19:48:41  pauly
 *ENH: Resampling, multiresolution
 *
 *Revision 1.13  2003/11/29 17:06:48  pauly
 *ENH: Minor Help issues
 *
 *Revision 1.12  2003/11/29 14:02:42  pauly
 *FIX: History list and file associations faili with spaces in filenames
 *
 *Revision 1.11  2003/11/10 00:27:26  pauly
 *FIX: Bug with linear interpolation in PDE solver
 *ENH: Help viewer and tutorial
 *
 *Revision 1.10  2003/10/09 22:45:14  pauly
 *EMH: Improvements in 3D functionality and snake parameter preview
 *
 *Revision 1.9  2003/10/06 12:30:00  pauly
 *ENH: Added history lists, remembering of settings, new snake parameter preview
 *
 *Revision 1.8  2003/10/02 20:57:46  pauly
 *FIX: Made sure that the previous check-in compiles on Linux
 *
 *Revision 1.7  2003/10/02 18:43:47  pauly
 *FIX: Fixed crashes with using vtkContourFilter
 *
 *Revision 1.6  2003/10/02 14:55:52  pauly
 *ENH: Development during the September code freeze
 *
 *Revision 1.5  2003/09/16 16:10:17  pauly
 *FIX: No more Internal compiler errors on VC
 *FIX: Intensity curve no longer crashes
 *ENH: Histogram display on intensity curve window
 *
 *Revision 1.4  2003/09/15 19:06:58  pauly
 *FIX: Trying to get last changes to compile
 *
 *Revision 1.3  2003/09/15 17:32:19  pauly
 *ENH: Removed ImageWrapperImplementation classes
 *
 *Revision 1.2  2003/09/13 15:18:01  pauly
 *FIX: Got SNAP to work properly with different image orientations
 *
 *Revision 1.1  2003/09/11 13:51:01  pauly
 *FIX: Enabled loading of images with different orientations
 *ENH: Implemented image save and load operations
 *
 *Revision 1.5  2003/08/28 22:58:30  pauly
 *FIX: Erratic scrollbar behavior
 *
 *Revision 1.4  2003/08/28 14:37:09  pauly
 *FIX: Clean 'unused parameter' and 'static keyword' warnings in gcc.
 *FIX: Label editor repaired
 *
 *Revision 1.3  2003/08/27 14:03:22  pauly
 *FIX: Made sure that -Wall option in gcc generates 0 warnings.
 *FIX: Removed 'comment within comment' problem in the cvs log.
 *
 *Revision 1.2  2003/08/27 04:57:46  pauly
 *FIX: A large number of bugs has been fixed for 1.4 release
 *
 *Revision 1.1  2003/07/12 04:46:50  pauly
 *Initial checkin of the SNAP application into the InsightApplications tree.
 *
 *Revision 1.1  2003/07/11 23:33:57  pauly
 **** empty log message ***
 *
 *Revision 1.20  2003/07/10 14:30:26  pauly
 *Integrated ITK into SNAP level set segmentation
 *
 *Revision 1.19  2003/07/01 16:53:59  pauly
 **** empty log message ***
 *
 *Revision 1.18  2003/06/23 23:59:32  pauly
 *Command line argument parsing
 *
 *Revision 1.17  2003/06/14 22:42:06  pauly
 *Several changes.  Started working on implementing the level set function
 *in ITK.
 *
 *Revision 1.16  2003/06/08 23:27:56  pauly
 *Changed variable names using combination of ctags, egrep, and perl.
 *
 *Revision 1.15  2003/06/08 16:11:42  pauly
 *User interface changes
 *Automatic mesh updating in SNAP mode
 *
 *Revision 1.14  2003/05/22 17:36:19  pauly
 *Edge preprocessing settings
 *
 *Revision 1.13  2003/05/17 21:39:30  pauly
 *Auto-update for in/out preprocessing
 *
 *Revision 1.12  2003/05/14 18:33:58  pauly
 *SNAP Component is working. Double thresholds have been enabled.  Many other changes.
 *
 *Revision 1.11  2003/05/12 02:51:08  pauly
 *Got code to compile on UNIX
 *
 *Revision 1.10  2003/05/08 21:59:05  pauly
 *SNAP is almost working
 *
 *Revision 1.9  2003/05/07 19:14:46  pauly
 *More progress on getting old segmentation working in the new SNAP.  Almost there, region of interest and bubbles are working.
 *
 *Revision 1.8  2003/05/05 12:30:18  pauly
 **** empty log message ***
 *
 *Revision 1.7  2003/04/29 14:01:42  pauly
 *Charlotte Trip
 *
 *Revision 1.6  2003/04/25 02:58:29  pauly
 *New window2d model with InteractionModes
 *
 *Revision 1.5  2003/04/23 20:36:23  pauly
 **** empty log message ***
 *
 *Revision 1.4  2003/04/23 06:05:18  pauly
 **** empty log message ***
 *
 *Revision 1.3  2003/04/18 17:32:18  pauly
 **** empty log message ***
 *
 *Revision 1.2  2003/04/18 00:25:37  pauly
 **** empty log message ***
 *
 *Revision 1.1  2003/04/16 05:04:17  pauly
 *Incorporated intensity modification into the snap pipeline
 *New IRISApplication
 *Random goodies
 *
 *Revision 1.2  2003/04/01 18:20:56  pauly
 **** empty log message ***
 *
 *Revision 1.1  2003/03/07 19:29:47  pauly
 *Initial checkin
 *
 *Revision 1.1.1.1  2002/12/10 01:35:36  pauly
 *Started the project repository
 *
 *
 *Revision 1.63  2002/05/08 17:32:57  moon
 *I made some changes Guido wanted in the GUI, including removing
 *Turello/Sapiro/Schlegel options (I only hid them, the code is all still there), changing a bunch of the ranges, etc. of the snake parameters.
 *
 *Revision 1.62  2002/04/28 17:29:43  scheuerm
 *Added some documentation
 *
 *Revision 1.61  2002/04/27 18:30:03  moon
 *Finished commenting
 *
 *Revision 1.60  2002/04/27 17:48:33  bobkov
 *Added comments
 *
 *Revision 1.59  2002/04/27 00:08:27  talbert
 *Final commenting run through . . . no functional changes.
 *
 *Revision 1.58  2002/04/26 17:37:12  moon
 *Fixed callback on save preproc dialog cancel button.
 *Fixed bubble browser output.  Position was zero-based, which didn't match the 2D
 *window slice numbers (1 based), so I changed the bubble positions to be cursor
 *position +1.
 *Disallowed starting snake window if current label in not visible.
 *Put in Apply+ button in threshold dialog, which changes seg overlay to be an
 *overlay of the positive voxels in the preproc data (a zero-level visualization).
 *Added more m_OutMessage and m_OutMessage messages.
 *
 *Revision 1.57  2002/04/25 14:13:13  moon
 *Enabled render options in snake window.
 *Changed snake params dialog slider (messed one up last time)
 *Hide r_ params in snake params dialog with in/out snake (they don't apply)
 *Calling segment3D with clear color is not allowed (error dialog)
 *
 *Revision 1.56  2002/04/24 19:50:22  moon
 *Pulled LoadGreyFileCallback out of GUI into UserInterfaceLogic, made modifications due
 *to change in ROI semantics.  Before, the ROI was from ul to lr-1, which is a bad
 *decision.  I changed everything to work with a ROI that is inclusive, meaning
 *that all voxels from ul through lr inclusive are part of the ROI. This involved
 *a lot of small changes to a lot of files.
 *
 *Revision 1.55  2002/04/24 17:10:33  bobkov
 *Added some changes to OnSnakeStartAction(),
 *OnAcceptInitializationAction() and
 *OnRestartInitializationAction()
 *so that ClearScreen() method is called on
 *m_IRISWindow3D and m_SNAPWindow3D to clear the glLists and
 *the previous segmentation is not shown in the 3D window
 *
 *Revision 1.54  2002/04/24 14:54:32  moon
 *Changed the ranges of some of the snake parameters after talking with Guido.
 *Put in a line to update mesh when the update continuously checkbox is checked.
 *
 *Revision 1.53  2002/04/24 14:13:26  moon
 *Implemented separate brightness/contrast settings for grey/preproc data
 *
 *Revision 1.52  2002/04/24 01:05:00  talbert
 *Changed IRIS2000 labels to SnAP.
 *
 *Revision 1.51  2002/04/23 21:56:50  moon
 *small bug fix with the snake params and the global state.  wasn't getting
 *the sapiro/turello/etc. option from the dialog to put into the global state.
 *
 *Revision 1.50  2002/04/23 03:19:40  talbert
 *Made some changes so that the load preproc menu option is unuseable once
 *the snake starts.
 *
 *Revision 1.49  2002/04/22 21:54:39  moon
 *Closed dialogs when accept/restart initialization is pressed, or snake type is
 *switched.
 *
 *Revision 1.48  2002/04/22 21:24:20  talbert
 *Small changes so that error messages for preprocessing loading appeared in
 *the correct status bar.
 *
 *Revision 1.47  2002/04/20 21:56:47  talbert
 *Made it impossible to save preprocessed data when it doesn't make sense
 *(if no preprocessing has been done since the last preproc load or since
 *the snake win opened).  Moved some checks for data type validity out of
 *the callbacks and into the Vox and SnakeVoxData classes where they belong.
 *
 *Revision 1.46  2002/04/19 23:03:59  moon
 *Changed more stuff to get the snake params state synched with the global state.
 *Changed the range of ground in snake params dialog.
 *Removed the use_del_g stuff, since it's really not necessary, I found out.
 *
 *Revision 1.45  2002/04/19 20:34:58  moon
 *Made preproc dialogs check global state and only preproc if parameters have changed.
 *So no if you hit apply, then ok, it doesn't re process on the ok.
 *
 *Revision 1.44  2002/04/18 21:36:51  scheuerm
 *Added documentation for my recent changes.
 *Fixed inverted display of edge preprocessing.
 *
 *Revision 1.43  2002/04/18 21:14:03  moon
 *I had changed the Cancel buttons to be Close on the Filter dialogs, and I changed
 *the names of the callbacks in GUI, but not in UserInterfaceLogic.  So I just hooked them
 *up so the dialogs get closed.
 *
 *Revision 1.42  2002/04/18 21:04:51  moon
 *Changed the IRIS window ROI stuff.  Now the ROI is always valid if an image is
 *loaded, but there is a toggle to show it or not.  This will work better with
 *Konstantin's addition of being able to drag the roi box.
 *
 *I also changed a bunch of areas where I was calling InitializeSlice for the 2D windows,
 *when this is not at all what I should have done.  Now those spots call
 *MakeSegTextureCurrent, or MakeGreyTextureCurrent.  This means that the view is not
 *reset every time the snake steps, the preproc/orig radio buttons are changed, etc.
 *
 *Revision 1.41  2002/04/16 18:54:32  moon
 *minor bug with not stopping snake when play is pushed, and then other
 *buttons are pushed.  Also added a function that can be called when the user
 *clicks the "X" on a window, but it's not what we want, I don't think.  The
 *problem is if the user clicks the "X" on the snake window when a "non modal"
 *dialog is up, all the windows close, but the program doesn't quit.  I think
 *it's a bug in FLTK, but I can't figure out how to solve it.
 *
 *Revision 1.40  2002/04/16 14:44:49  moon
 *Changed bubbles to be in world coordinates instead of image coordinates.
 *
 *Revision 1.39  2002/04/16 13:07:56  moon
 *Added tooltips to some widgets, made minor changes to enabling/disabling of
 *widgets, clearing 3D window when initialization is restarted in snake window,
 *changed kappa in edge preproc dialog to be [0..1] range instead of [0..3]
 *
 *Revision 1.38  2002/04/14 22:02:54  scheuerm
 *Changed loading dialog for preprocessed image data. Code restructuring
 *along the way: Most important is addition of
 *SnakeVoxDataClass::ReadRawPreprocData()
 *
 *Revision 1.37  2002/04/13 17:43:48  moon
 *Added some initslice calls to Win2Ds, so the redraw problem comming back
 *from snake to iris window (where the whole 2D window is yellow) would go away.
 *
 *Revision 1.36  2002/04/13 16:20:08  moon
 *Just put in a bunch of debug printouts.  They'll have to come out eventually.
 *
 *Revision 1.35  2002/04/10 21:20:16  moon
 *just added debug comments.
 *
 *Revision 1.34  2002/04/10 20:19:40  moon
 *got play and stop vcr buttons to work.
 *put in lots of comments.
 *
 *Revision 1.33  2002/04/10 14:45:03  scheuerm
 *Added documentation to the methods I worked on.
 *
 *Revision 1.32  2002/04/09 21:56:42  bobkov
 *
 *modified Step button callback to display snake in 3d window
 *
 *Revision 1.31  2002/04/09 19:32:22  talbert
 *Added comments to the save and load preprocessed functions.  Checked that
 *only float files entered as preprocessed.  Made some small cosmetic
 *changes:  loading a file switches to preproc view and sets snake mode.
 *
 *Revision 1.30  2002/04/09 18:59:33  moon
 *Put in dialog to change snake parameters.  Also implemented Rewind button, which
 *now restarts the snake.  It seems for now that changing snake parameters restarts
 *the snake.  I don't know if this is the way it has to be, or I just did something
 *wrong in snakewrapper.  I'll have to check with Sean.
 *
 *Revision 1.29  2002/04/09 17:59:37  talbert
 *Made changes to LoadPreprocessedCallback which allowed edge detection
 *preproc data to be loaded correctly.
 *
 *Revision 1.28  2002/04/09 03:48:51  talbert
 *Changed some functionality in the LoadPreprocessedCallback() so that it
 *would work with floating point data being loaded.  Most of the stuff
 *is uncommented, hackish, and limited in its testing beyond verification
 *that it displays on the screen with the right values.  These changes
 *will have to be cleaned up.
 *
 *Revision 1.27  2002/04/08 13:32:35  talbert
 *Added a preprocessed save dialog box as well as a save preprocessed menu
 *option in the snake window.  Added the code necessary to implement the
 *GUI side of saving.
 *
 *Revision 1.26  2002/04/07 02:22:49  scheuerm
 *Improved handling of OK and Apply buttons in preprocessing dialogs.
 *
 *Revision 1.23  2002/04/05 03:42:29  scheuerm
 *Thresholding sort of works. Steepness needs to be made configurable.
 *
 *Revision 1.21  2002/04/04 15:30:08  moon
 *Put in code to get StepSize choice box filled with values and working.
 *AcceptSegment button callback puts snake seg data into full_data (IRIS)
 *Fixed a couple more UI cosmetic things.
 *
 *Revision 1.20  2002/04/03 22:12:07  moon
 *Added color chip, image probe, seg probe to snake window, although seg probe
 *maybe shouldn't be there.  added update continuously checkbox to 3Dwindow.
 *changes accept/restart to be on top of each other, and one is shown at a time,
 *which I think is more intuitive.
 *changed snake iteration field to be text output.  added callback for step size
 *choice.
 *
 *Revision 1.19  2002/04/02 23:51:17  scheuerm
 *Gradient magnitude preprocessing is implemented. Stupid, stupid VTK.
 *Adjusted the range and resolution of the sigma slider. Apply button
 *still doesn't do anything but I think we don't need it.
 *
 *Revision 1.18  2002/04/02 15:12:43  moon
 *Put code in the step vcr button.  Now the snake can be "stepped"
 *
 *Revision 1.17  2002/04/01 22:29:54  moon
 *Modified OnAcceptInitializationAction, added functionality to
 *OnRestartInitializationAction
 *
 *Revision 1.16  2002/03/29 20:17:25  scheuerm
 *Implemented remapping of preprocessed data to (-1,1). The mapping can
 *be changed by altering the parameters to RemapPreprocData(...) in
 *LoadPreprocessedDataCallback() (UserInterfaceLogic.cpp).
 *
 *Revision 1.15  2002/03/29 03:33:29  scheuerm
 *Loaded preprocessed data is now converted to float. No remapping yet.
 *Stupid VTK. Added vtkImageDeepCopyFloat which copies the region of
 *interest out of a gray image and converts it to float as it goes.
 *
 *Revision 1.14  2002/03/27 17:59:40  moon
 *changed a couple things.  nothing big. a callback in .fl was bool return type
 *which didn't compile in windows. this is the version I think will work for a
 *demo for Kye
 *
 *Revision 1.13  2002/03/27 17:05:04  talbert
 *Made changes necessary to compile in Windows 2000 using Microsoft Visual C++ 6.0.
 *GUI.cpp - needed to return something from function LoadPreprocessedCallback()
 *UserInterfaceLogic.cpp - moved definitions of for loop control variables outside of loop for
 *scope reasons.
 *SnakeWrapper.cpp - changed outdt1->data to *outdt1 and outdt2->data to *outdt because
 *these variables are float, not structures.  Also changed a line using snprintf to
 *sprintf because snprintf is a GNU extension.
 *Added the files snake32.dsp and snake32.dsw for compiling in Windows 2000.
 *
 *Revision 1.12  2002/03/27 15:04:26  moon
 *Changed a bunch of stuff so that the state was basically reset when the snake
 *window is hidden (accept or cancel segmentation), and then opened again.  for
 *example, the bubbles browser needed to be emptied, the active/inactive groups
 *needed to be set to the defaults again, the radio button for the preproc
 *data needed to be turned off (so original data is shown), etc.
 *
 *Added code to the acceptinitialization button that converts bubble information
 *into binary snake initialization image, and previous segmentation info of the
 *same label should also be preserved (i.e. segmentation info that comes from
 *IRIS can be used for the initialization as well as bubbles). The snake is
 *initialized, and the controls are activated.
 *
 *Still need to code the resetinitialization so that the bubble stuff, etc. is re-
 *enabled.
 *
 *None of the vcr buttons do anything, still.
 *
 *Revision 1.11  2002/03/26 19:22:14  moon
 *I don't think I really changed anything, but I had updated, and it tried to "merge" versions with and without ^M endline characters
 *
 *Revision 1.10  2002/03/26 18:16:32  scheuerm
 *Added loading and display of preprocessed data:
 *- added vtkImageDeepCopy function
 *- added flags indicating which dataset to display in GlobalState
 *- added flag indicating whether to load gray or preprocessed data
 *  in the GUI class
 *
 *Revision 1.9  2002/03/25 02:15:57  scheuerm
 *Added loading of preprocessed data. It isn't being converted
 *to floats yet. It's not possible to actually display the data
 *right now.
 *
 *Revision 1.8  2002/03/24 19:27:46  talbert
 *Added callback the preprocess button to show dialog boxes for filtering.  Added callbacks for buttons in filtering dialog boxes.  Modified the AddBubbles callback so that the newest bubble is selected in the Bubble Browser.  m_OutAboutCompiled and ran to verify that new bubbles are selected and that the dialogs appear over the
 *3d window.  talbert s f
 *
 *Revision 1.7  2002/03/22 16:44:16  moon
 *added OpenGL display of bubbles in Window2D_s::draw
 *
 *Revision 1.6  2002/03/21 15:45:46  bobkov
 *implemented callbacks for buttons AddBubble and RemoveBubble, implemented callbacks for Radius slider and ActiveBubble browser, created methods getBubbles and getNumberOfBubbles   e
 *
 *Revision 1.5  2002/03/19 19:35:32  moon
 *added snakewrapper to makefile so it gets compiled. started putting in callback,
 *etc. for snake vcr buttons.  added snake object to IrisGlobals, instantiated in Main
 *
 *Revision 1.4  2002/03/19 17:47:10  moon
 *added some code to disable widgets, make the radio buttons work, etc. in the snake window.  fixed the quit callback from the snake window to work (crashed before)
 *changed the [accept/restart]bubble_button widgets to be acceptinitialization_button and added callbacks (empty).
 *
 *Revision 1.3  2002/03/08 14:06:29  moon
 *Added Header and Log tags to all files
 **/


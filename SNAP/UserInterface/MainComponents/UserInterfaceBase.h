/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    UserInterfaceBase.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#ifndef __UserInterfaceBase__h_
#define __UserInterfaceBase__h_

// This should all be gone
#include "SNAPCommonUI.h"
#include "GlobalState.h" 

// Borland compiler stuff. Note to whoever went through the code and added all 
// these borland things: you just can't add ITK headers to headers like this one
// that get included in lots of files. This makes compilation time insane!!!
#if defined(__BORLANDC__)
#include "SNAPBorlandDummyTypes.h"
#endif;

// Forward refences to some classes
class IRISApplication;
class SystemInterface;
class SNAPAppearanceSettings;
class SliceWindowCoordinator;
class Fl_Window;

/**
 * \class UserInterfaceBase
 * \brief Base class for the main user interface.
 */
class UserInterfaceBase {
public:
    virtual ~UserInterfaceBase(void) {} /* Needed to avoid compiler warning */

  // Methods for switching between SNAP and IRIS modes
  virtual void ShowIRIS() = 0;
  virtual void ShowSNAP() = 0;
  
  // Method for exiting IRIS
  virtual void OnMainWindowCloseAction() = 0;

  // Methods that set the IRIS Toolbar and 3D Toolbar modes
  virtual void SetToolbarMode(ToolbarModeType mode) = 0;
  virtual void SetToolbarMode3D(ToolbarMode3DType mode) = 0;
  
  // Menu item callbacks
  virtual void OnMenuLoadGrey() = 0;
  virtual void OnMenuSaveGreyROI() = 0;
  virtual void OnMenuLoadSegmentation() = 0;
  virtual void OnMenuSaveSegmentation() = 0;
  virtual void OnMenuLoadPreprocessed() = 0;  
  virtual void OnMenuSavePreprocessed() = 0; 
  virtual void OnMenuLoadLabels() = 0;
  virtual void OnMenuSaveLabels() = 0;
  virtual void OnMenuLoadAdvection() = 0;
  virtual void OnMenuWriteVoxelCounts() = 0;
  virtual void OnLoadRecentAction(unsigned int iRecent) = 0;  
  virtual void OnMenuIntensityCurve() = 0;  
  virtual void OnMenuShowDisplayOptions() = 0;
  virtual void OnMenuExportSlice(unsigned int iSlice) = 0;
  virtual void OnMenuImageInfo() = 0;
  virtual void OnMenuQuit() = 0;
  
  // IRIS: Slice selection actions  
  virtual void OnSliceSliderChange(int id) = 0;
  virtual void UpdatePositionDisplay(int id) = 0;

  // IRIS: Zoom/pan interaction callbacks
  virtual void OnResetView2DAction(unsigned int window) = 0;
  virtual void OnResetAllViews2DAction() = 0;
  virtual void OnLinkedZoomChange() = 0;
  virtual void OnZoomPercentageChange() = 0;
  
  // IRIS: Color label selection and editing callbacks
  virtual void UpdateColorChips() = 0;
  virtual void OnDrawingLabelUpdate() = 0;
  virtual void OnDrawOverLabelUpdate() = 0;
  virtual void UpdateEditLabelWindow() = 0;
  virtual void OnEditLabelsAction() = 0;
  
  // IRIS: Polygon buttons callbacks
  virtual void OnAcceptPolygonAction(unsigned int window) = 0;
  virtual void OnDeletePolygonSelectedAction(unsigned int window) = 0;
  virtual void OnInsertIntoPolygonSelectedAction(unsigned int window) = 0;
  virtual void OnPastePolygonAction(unsigned int window) = 0;

  // IRIS: 3D Window callbacks
  virtual void OnMeshResetViewAction() = 0;
  virtual void OnIRISMeshUpdateAction() = 0;
  virtual void OnIRISMeshAcceptAction() = 0;
  
  // IRIS: ROI manipulation callbacks
  virtual void OnResetROIAction() = 0;
  virtual void OnSnakeStartAction() = 0;

  // IRIS: Image Info Window callbacks
  virtual void OnCloseImageInfoAction() = 0;
  
  // SNAP Preprocessing page actions
  virtual void OnInOutSnakeSelect() = 0;
  virtual void OnEdgeSnakeSelect() = 0;
  virtual void OnPreprocessAction() = 0;
  virtual void OnPreprocessClose() = 0;
  virtual void OnLoadPreprocessedImageAction() = 0;
  virtual void OnAcceptPreprocessingAction() = 0;

  // SNAP Color map operations
  virtual void OnPreprocessedColorMapAction() = 0;
  virtual void OnColorMapCloseAction() = 0;
  virtual void OnColorMapSelectAction() = 0;

  // SNAP Initialization page actions
  virtual void OnAddBubbleAction() = 0;
  virtual void OnRemoveBubbleAction() = 0;
  virtual void OnActiveBubblesChange() = 0;
  virtual void OnBubbleRadiusChange() = 0;
  virtual void OnAcceptInitializationAction() = 0;
  
  // SNAP Segmentation page actions
  virtual void OnRestartInitializationAction() = 0;
  virtual void OnSnakeParametersAction() = 0;
  virtual void OnAcceptSegmentationAction() = 0;
  virtual void OnSnakeRewindAction() = 0;
  virtual void OnSnakeStopAction() = 0;
  virtual void OnSnakePlayAction() = 0;
  virtual void OnSnakeStepAction() = 0;
  virtual void OnSnakeStepSizeChange() = 0;
  virtual void OnRestartPreprocessingAction() = 0;
  virtual void OnCancelSegmentationAction() = 0;
  
  // SNAP: display interaction actions  
  virtual void OnSNAPViewOriginalSelect() = 0;
  virtual void OnViewPreprocessedSelect() = 0;
  
  virtual void UpdateMainLabel() = 0;

  // Opacity sliders  
  virtual void OnIRISLabelOpacityChange() = 0;
  virtual void OnSNAPLabelOpacityChange() = 0;

  // SNAP: 3D window related callbacks  
  virtual void OnSNAPMeshUpdateAction() = 0;
  virtual void OnSNAPMeshContinuousUpdateAction() = 0;

  // virtual void Activate3DAccept(bool on) = 0;

  // Help related callbacks
  virtual void OnLaunchTutorialAction() = 0;
  virtual void ShowHTMLPage(const char *link) = 0;

  // Window size manipulation calls
  virtual void OnWindowFocus(unsigned int i) = 0;

  // Save as PNG
  virtual void OnActiveWindowSaveSnapshot(unsigned int window) = 0;

  // The following methods are not referenced in the .fl file, but are defined here
  // so that other classes in the project can include UserInterfaceBase.h instead of
  // UserInterfaceLogic.h; this way there is not a huge build every time we change
  // a user interface element
  virtual IRISApplication *GetDriver() const = 0;
  virtual SystemInterface *GetSystemInterface() const = 0;
  virtual SNAPAppearanceSettings *GetAppearanceSettings() const = 0;
  virtual SliceWindowCoordinator *GetSliceCoordinator() const = 0;

  virtual void OnImageGeometryUpdate() = 0;
  virtual void RedrawWindows() = 0;
  virtual void OnIRISMeshDisplaySettingsUpdate() = 0;
  virtual void ResetScrollbars() = 0;
  virtual void UpdateImageProbe() = 0;
  virtual void OnLabelListUpdate() = 0;
  virtual void OnSegmentationImageUpdate() = 0;
  virtual void CenterChildWindowInMainWindow(Fl_Window *) = 0;
  virtual void OnPreprocessingPreviewStatusUpdate(bool) = 0;
  virtual void OnSpeedImageUpdate() = 0;
  virtual void OnCrosshairPositionUpdate() = 0;
  virtual void OnPolygonStateUpdate(unsigned int) = 0;
  virtual void OnZoomUpdate() = 0;
  virtual void OnIRISMeshEditingAction() = 0;

protected:
    GlobalState *m_GlobalState;
};

#endif // __UserInterfaceBase__h_

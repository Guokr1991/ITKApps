/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    CrosshairsInteractionMode.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#include "CrosshairsInteractionMode.h"
#include "IRISApplication.h"
#include "IRISImageData.h"
#include "UserInterfaceLogic.h"
#include "SNAPAppearanceSettings.h"

CrosshairsInteractionMode
::CrosshairsInteractionMode(GenericSliceWindow *parent) 
: GenericSliceWindow::EventHandler(parent)
{
  m_NeedToRepaintControls = false;
}

int
CrosshairsInteractionMode
::OnMousePress(const FLTKEvent &event)
{
  UpdateCrosshairs(event);
  m_Parent->m_GlobalState->SetUpdateSliceFlag(0);
  return 1;
}

int
CrosshairsInteractionMode
::OnMouseRelease(const FLTKEvent &event, 
                   const FLTKEvent &irisNotUsed(pressEvent))
{
  UpdateCrosshairs(event);
  return 1;
}

int
CrosshairsInteractionMode
::OnMouseDrag(const FLTKEvent &event, 
                const FLTKEvent &irisNotUsed(pressEvent))
{
  UpdateCrosshairs(event);
  m_Parent->m_GlobalState->SetUpdateSliceFlag(1);
  return 1;
}

int
CrosshairsInteractionMode
::OnMouseWheel(const FLTKEvent &irisNotUsed(event))
{
  // Must have the cursor inside the window to process key events
  if(!m_Parent->m_Focus) return 0;

  // Get the amount of the scroll
  float scroll = (float) Fl::event_dy();
  
  // Get the cross-hairs position in image space
  Vector3ui xCrossImage = m_GlobalState->GetCrosshairsPosition();

  // Map it into slice space
  Vector3f xCrossSlice = 
    m_Parent->MapImageToSlice(to_float(xCrossImage) + Vector3f(0.5f));

  // Advance by the scroll amount
  xCrossSlice[2] += scroll;

  // Map back into display space
  xCrossImage = to_unsigned_int(m_Parent->MapSliceToImage(xCrossSlice));

  // Clamp by the display size
  Vector3ui xSize = m_Driver->GetCurrentImageData()->GetVolumeExtents();
  Vector3ui xCrossClamped = xCrossImage.clamp(
    Vector3ui(0,0,0),xSize - Vector3ui(1,1,1));
  
  // Update the crosshairs position in the current image data
  m_Parent->m_ImageData->SetCrosshairs(xCrossClamped);
  
  // Set the crosshair
  m_GlobalState->SetCrosshairsPosition(xCrossClamped);

  // Cause a repaint
  m_NeedToRepaintControls = true;
  m_ParentUI->RedrawWindows();    

  return 1;
}

void
CrosshairsInteractionMode
::UpdateCrosshairs(const Vector3f &xClick)
{
  // Compute the new cross-hairs position in image space
  Vector3f xCross = m_Parent->MapSliceToImage(xClick);

  // Round the cross-hairs position down to integer
  Vector3i xCrossInteger = to_int(xCross);
  
  // Make sure that the cross-hairs position is within bounds by clamping
  // it to image dimensions
  Vector3i xSize = to_int(m_Driver->GetCurrentImageData()->GetVolumeExtents());
  Vector3ui xCrossClamped = to_unsigned_int(
    xCrossInteger.clamp(Vector3i(0),xSize - Vector3i(1)));

  // Update the crosshairs position in the current image data
  m_Parent->m_ImageData->SetCrosshairs(xCrossClamped);
  
  // Update the crosshairs position in the global state
  m_GlobalState->SetCrosshairsPosition(xCrossClamped);

  // Cause a repaint
  m_NeedToRepaintControls = true;
  m_ParentUI->RedrawWindows();  
}

void
CrosshairsInteractionMode
::UpdateCrosshairs(const FLTKEvent &event)
{
  // Compute the position in slice coordinates
  Vector3f xClick = m_Parent->MapWindowToSlice(event.XSpace.extract(2));
  UpdateCrosshairs(xClick);
}

int 
CrosshairsInteractionMode
::OnKeyDown(const FLTKEvent &event)
{
  // Must have the cursor inside the window to process key events
  if(!m_Parent->m_Focus) return 0;

  // Vector encoding the movement in pixels
  Vector3f xMotion(0.0f);

  // Check the shift state
  float xStep = (event.State & FL_SHIFT) ? 5.0f : 1.0f;

  // Handle up, down, left, right, pg-up and pg-down events
  switch(Fl::event_key())
    {
    case FL_Right:     xMotion[0] += xStep; break;
    case FL_Left:      xMotion[0] -= xStep; break;
    case FL_Down:      xMotion[1] -= xStep; break;
    case FL_Up:        xMotion[1] += xStep; break;
    case FL_Page_Up:   xMotion[2] -= xStep; break;
    case FL_Page_Down: xMotion[2] += xStep; break;
    default: return 0;
    };

  // Get the crosshair coordinates, in slice space
  Vector3f xCross = 
    m_Parent->MapImageToSlice(
      to_float(m_GlobalState->GetCrosshairsPosition()));

  // Add the motion vector
  UpdateCrosshairs(xCross + xMotion);

  // Handled!
  return 1;
}

void 
CrosshairsInteractionMode::
OnDraw()
{
  // Do the painting only if necessary
  if(m_NeedToRepaintControls)
    {
    // Update the image probe and scrollbar controls
    m_ParentUI->OnCrosshairPositionUpdate();
    
    // No need to call this until another update
    m_NeedToRepaintControls = false;
    }

  // Get the current cursor position
  Vector3ui xCursorInteger = m_GlobalState->GetCrosshairsPosition();

  // Shift the cursor position by by 0.5 in order to have it appear
  // between voxels
  Vector3f xCursorImage = to_float(xCursorInteger) + Vector3f(0.5f);
  
  // Get the cursor position on the slice
  Vector3f xCursorSlice = m_Parent->MapImageToSlice(xCursorImage);

  // Upper and lober bounds to which the crosshairs are drawn
  Vector2i lower(0);
  Vector2i upper = m_Parent->m_SliceSize.extract(2);

  // Get the line color, thickness and dash spacing
  SNAPAppearanceSettings::Element elt = 
    m_ParentUI->GetAppearanceSettings()->GetUIElement(
      SNAPAppearanceSettings::CROSSHAIRS);

  // If in thumbnail mode, replace the line properties with those of the thumbnail
  if(m_Parent->m_ThumbnailIsDrawing)
    {
    SNAPAppearanceSettings::Element eltThumb = 
      m_ParentUI->GetAppearanceSettings()->GetUIElement(
        SNAPAppearanceSettings::ZOOM_THUMBNAIL);
    elt.LineThickness = eltThumb.LineThickness;
    }

  // Set line properties
  glPushAttrib(GL_LINE_BIT | GL_COLOR_BUFFER_BIT);

  // Apply the line properties; thick line is only applied in zoom thumbnail (?)
  SNAPAppearanceSettings::ApplyUIElementLineSettings(elt);

  // Apply the color
  glColor3dv(elt.NormalColor.data_block());
  
  // Refit matrix so that the lines are centered on the current pixel
  glPushMatrix();
  glTranslated( xCursorSlice(0), xCursorSlice(1), 0.0 );

  // Paint the cross-hairs
  glBegin(GL_LINE_STRIP);  
  glVertex2f(lower(0) - xCursorSlice(0),0); 
  glVertex2f(0, 0);
  glVertex2f(upper(0) - xCursorSlice(0), 0);
  glEnd();

  glBegin(GL_LINE_STRIP);  
  glVertex2f(0, lower(1) - xCursorSlice(1)); 
  glVertex2f(0, 0);
  glVertex2f(0, upper(1) - xCursorSlice(1));
  glEnd();

  glPopMatrix();

  glPopAttrib();
}

